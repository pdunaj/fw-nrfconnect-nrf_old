/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <gps.h>
#include <sensor.h>
#include <console.h>
#include <nrf_cloud.h>
#include <dk_buttons_and_leds.h>
#include <lte_lc.h>
#include <misc/reboot.h>
#include <bsd.h>

#include "aggregator.h"
#include "ble.h"

#include "alarm.h"

/* Interval in milliseconds between each time status LEDs are updated. */
#define LEDS_UPDATE_INTERVAL	        500

/* Interval in microseconds between each time LEDs are updated when indicating
 * that an error has occurred.
 */
#define LEDS_ERROR_UPDATE_INTERVAL      250000

#define BUTTON_1			BIT(0)
#define BUTTON_2			BIT(1)
#define SWITCH_1			BIT(2)
#define SWITCH_2			BIT(3)

#define LED_ON(x)			(x)
#define LED_BLINK(x)			((x) << 8)
#define LED_GET_ON(x)			((x) & 0xFF)
#define LED_GET_BLINK(x)		(((x) >> 8) & 0xFF)

enum {
	LEDS_INITIALIZING     = LED_ON(0),
	LEDS_CONNECTING       = LED_BLINK(DK_LED3_MSK),
	LEDS_PATTERN_WAIT     = LED_BLINK(DK_LED3_MSK | DK_LED4_MSK),
	LEDS_PATTERN_ENTRY    = LED_ON(DK_LED3_MSK) | LED_BLINK(DK_LED4_MSK),
	LEDS_PATTERN_DONE     = LED_BLINK(DK_LED4_MSK),
	LEDS_PAIRED           = LED_ON(DK_LED4_MSK),
	LEDS_ERROR            = LED_ON(DK_ALL_LEDS_MSK)
} display_state;

 /* Variables to keep track of nRF cloud user association. */
static u8_t ua_pattern[10];
static int buttons_to_capture;
static int buttons_captured;
static bool pattern_recording;
static struct k_sem user_assoc_sem;

/* Sensor data */
static struct gps_data nmea_data;
static struct nrf_cloud_sensor_data gps_cloud_data = {
	.type = NRF_CLOUD_SENSOR_GPS,
	.tag = 0x1,
	.data.ptr = nmea_data.str,
	.data.len = GPS_NMEA_SENTENCE_MAX_LENGTH,
};
static atomic_val_t send_data_enable;

/* Structures for work */
static struct k_delayed_work leds_update_work;
static struct k_work connect_work;

enum error_type {
	ERROR_NRF_CLOUD,
	ERROR_BSD_RECOVERABLE,
	ERROR_BSD_IRRECOVERABLE
};

/* Forward declaration of functions */
static void cloud_connect(struct k_work *work);
static void sensors_init(void);
static void work_init(void);

void sensor_data_send(struct nrf_cloud_sensor_data *data);

/**@brief nRF Cloud error handler. */
void error_handler(enum error_type err_type, int err)
{
	if (err_type == ERROR_NRF_CLOUD) {
		int err;

		/* Turn off and shutdown modem */
		k_sched_lock();
		err = lte_lc_power_off();
		__ASSERT(err == 0, "lte_lc_power_off failed: %d", err);
		bsd_shutdown();
	}

#if !defined(CONFIG_DEBUG)
	sys_reboot(SYS_REBOOT_COLD);
#else
	u8_t led_pattern;

	switch (err_type) {
	case ERROR_NRF_CLOUD:
		/* Blinking all LEDs ON/OFF in pairs (1 and 4, 2 and 3)
		 * if there is an application error.
		 */
		led_pattern = DK_LED1_MSK | DK_LED4_MSK;
		printk("Error of type ERROR_NRF_CLOUD: %d\n", err);
		break;
	case ERROR_BSD_RECOVERABLE:
		/* Blinking all LEDs ON/OFF in pairs (1 and 3, 2 and 4)
		 * if there is a recoverable error.
		 */
		led_pattern = DK_LED1_MSK | DK_LED3_MSK;
		printk("Error of type ERROR_BSD_RECOVERABLE: %d\n", err);
		break;
	case ERROR_BSD_IRRECOVERABLE:
		/* Blinking all LEDs ON/OFF if there is an
		 * irrecoverable error.
		 */
		led_pattern = DK_ALL_LEDS_MSK;
		printk("Error of type ERROR_BSD_IRRECOVERABLE: %d\n", err);
		break;
	default:
		/* Blinking all LEDs ON/OFF in pairs (1 and 2, 3 and 4)
		 * undefined error.
		 */
		led_pattern = DK_LED1_MSK | DK_LED2_MSK;
		break;
	}

	while (true) {
		dk_set_leds(led_pattern & 0x0f);
		k_busy_wait(LEDS_ERROR_UPDATE_INTERVAL);
		dk_set_leds(~led_pattern & 0x0f);
		k_busy_wait(LEDS_ERROR_UPDATE_INTERVAL);
	}
#endif /* CONFIG_DEBUG */
}

void nrf_cloud_error_handler(int err)
{
	error_handler(ERROR_NRF_CLOUD, err);
}

/**@brief Recoverable BSD library error. */
void bsd_recoverable_error_handler(uint32_t err)
{
	error_handler(ERROR_BSD_RECOVERABLE, (int)err);
}

/**@brief Irrecoverable BSD library error. */
void bsd_irrecoverable_error_handler(uint32_t err)
{
	error_handler(ERROR_BSD_IRRECOVERABLE, (int)err);
}

/**@brief Callback for GPS trigger events */
static void gps_trigger_handler(struct device *dev, struct gps_trigger *trigger)
{
	ARG_UNUSED(trigger);

	u32_t button_state, has_changed;

	dk_read_buttons(&button_state, &has_changed);

	if (!(button_state & SWITCH_2) && atomic_get(&send_data_enable)) {
		gps_sample_fetch(dev);
		gps_channel_get(dev, GPS_CHAN_NMEA, &nmea_data);

		gps_cloud_data.tag++;

		if (gps_cloud_data.tag == 0) {
			gps_cloud_data.tag = 0x1;
		}

		struct sensor_data in_data = {
			.type = GPS_POSITION,
			.length = gps_cloud_data.data.len,
		};

		memcpy(&in_data.data[0], &gps_cloud_data.tag,
		       sizeof(gps_cloud_data.tag));

		memcpy(&in_data.data[sizeof(gps_cloud_data.tag)],
		gps_cloud_data.data.ptr,
		       gps_cloud_data.data.len);

		if (aggregator_put(in_data) != 0) {
			printk("Failed to store GPS data.\n");
		}
	}
}

/**@brief Update LEDs state. */
static void leds_update(struct k_work *work)
{
	static bool led_on;
	static u8_t current_led_on_mask;
	u8_t led_on_mask = current_led_on_mask;

	ARG_UNUSED(work);

	/* Reset LED3 and LED4. */
	led_on_mask &= ~(DK_LED3_MSK | DK_LED4_MSK);

	/* Set LED3 and LED4 to match current state. */
	led_on_mask |= LED_GET_ON(display_state);

	led_on = !led_on;
	if (led_on) {
		led_on_mask |= LED_GET_BLINK(display_state);
	} else {
		led_on_mask &= ~LED_GET_BLINK(display_state);
	}

	if (led_on_mask != current_led_on_mask) {
		dk_set_leds(led_on_mask);
		current_led_on_mask = led_on_mask;
	}

	k_delayed_work_submit(&leds_update_work, LEDS_UPDATE_INTERVAL);
}

/**@brief Send sensor data to nRF Cloud. **/
void sensor_data_send(struct nrf_cloud_sensor_data *data)
{
	int err;

	if (pattern_recording || !atomic_get(&send_data_enable)) {
		return;
	}

	if (data->type == NRF_CLOUD_SENSOR_FLIP) {
		err = nrf_cloud_sensor_data_stream(data);
	} else {
		err = nrf_cloud_sensor_data_send(data);
	}

	if (err) {
		printk("Failed to send data to cloud: %d\n", err);
		nrf_cloud_error_handler(err);
	}
}

/**@brief Callback for user association event received from nRF Cloud. */
static void on_user_association_req(const struct nrf_cloud_evt *evt)
{
	if (!pattern_recording) {
		k_sem_init(&user_assoc_sem, 0, 1);
		display_state = LEDS_PATTERN_WAIT;
		pattern_recording = true;
		buttons_captured = 0;
		buttons_to_capture = evt->param.ua_req.sequence.len;

		printk("Please enter the user association pattern");

		if (IS_ENABLED(CONFIG_CLOUD_UA_BUTTONS)) {
			printk(" using the buttons and switches\n");
		} else if (IS_ENABLED(CONFIG_CLOUD_UA_CONSOLE)) {
			printk(" using the console\n");
		} else {
			printk(". No valid assosiation pattern input found.\n");
		}
	}
}

/**@brief Callback for nRF Cloud events. */
static void cloud_event_handler(const struct nrf_cloud_evt *evt)
{
	int err;

	switch (evt->type) {
	case NRF_CLOUD_EVT_TRANSPORT_CONNECTED:
		printk("NRF_CLOUD_EVT_TRANSPORT_CONNECTED\n");
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST:
		printk("NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST\n");
		on_user_association_req(evt);
		break;
	case NRF_CLOUD_EVT_USER_ASSOCIATED:
		printk("NRF_CLOUD_EVT_USER_ASSOCIATED\n");
		break;
	case NRF_CLOUD_EVT_READY:
		printk("NRF_CLOUD_EVT_READY\n");
		display_state = LEDS_PAIRED;
		struct nrf_cloud_sa_param param = {
			.sensor_type = NRF_CLOUD_SENSOR_FLIP,
		};

		err = nrf_cloud_sensor_attach(&param);

		if (err) {
			printk("nrf_cloud_sensor_attach failed: %d\n", err);
			nrf_cloud_error_handler(err);
		}

		param.sensor_type = NRF_CLOUD_SENSOR_GPS;
		err = nrf_cloud_sensor_attach(&param);

		if (err) {
			printk("nrf_cloud_sensor_attach failed: %d\n", err);
			nrf_cloud_error_handler(err);
		}

		sensors_init();
		atomic_set(&send_data_enable, 1);
		break;
	case NRF_CLOUD_EVT_SENSOR_ATTACHED:
		printk("NRF_CLOUD_EVT_SENSOR_ATTACHED\n");
		break;
	case NRF_CLOUD_EVT_SENSOR_DATA_ACK:
		printk("NRF_CLOUD_EVT_SENSOR_DATA_ACK\n");
		break;
	case NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED:
		printk("NRF_CLOUD_EVT_TRANSPORT_DISCONNECTED\n");
		atomic_set(&send_data_enable, 0);
		display_state = LEDS_INITIALIZING;

		/* Reconnect to nRF Cloud. */
		k_work_submit(&connect_work);
		break;
	case NRF_CLOUD_EVT_ERROR:
		printk("NRF_CLOUD_EVT_ERROR, status: %d\n", evt->status);
		atomic_set(&send_data_enable, 0);
		display_state = LEDS_ERROR;
		nrf_cloud_error_handler(evt->status);
		break;
	default:
		printk("Received unknown event %d\n", evt->type);
		break;
	}
}

/**@brief Initialize nRF CLoud library. */
static void cloud_init(void)
{
	const struct nrf_cloud_init_param param = {
		.event_handler = cloud_event_handler
	};

	int err = nrf_cloud_init(&param);

	__ASSERT(err == 0, "nRF Cloud library could not be initialized.");
}

/**@brief Connect to nRF Cloud, */
static void cloud_connect(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);

	const enum nrf_cloud_ua supported_uas[] = {
		NRF_CLOUD_UA_BUTTON
	};

	const enum nrf_cloud_sensor supported_sensors[] = {
		NRF_CLOUD_SENSOR_GPS,
		NRF_CLOUD_SENSOR_FLIP
	};

	const struct nrf_cloud_ua_list ua_list = {
		.size = ARRAY_SIZE(supported_uas),
		.ptr = supported_uas
	};

	const struct nrf_cloud_sensor_list sensor_list = {
		.size = ARRAY_SIZE(supported_sensors),
		.ptr = supported_sensors
	};

	const struct nrf_cloud_connect_param param = {
		.ua = &ua_list,
		.sensor = &sensor_list,
	};

	display_state = LEDS_CONNECTING;
	err = nrf_cloud_connect(&param);

	if (err) {
		printk("nrf_cloud_connect failed: %d\n", err);
		nrf_cloud_error_handler(err);
	}
}

/**@brief Send user association information to nRF Cloud. */
static void cloud_user_associate(void)
{
	int err;
	const struct nrf_cloud_ua_param ua = {
		.type = NRF_CLOUD_UA_BUTTON,
		.sequence = {
			.len = buttons_to_capture,
			.ptr = ua_pattern
		}
	};

	err = nrf_cloud_user_associate(&ua);

	if (err) {
		printk("nrf_cloud_user_associate failed: %d\n", err);
		nrf_cloud_error_handler(err);
	}
}

/**@brief Function to keep track of user association input when using
 * buttons and switches to register the association pattern.
 */
static void pairing_button_register(u32_t button_state, u32_t has_changed)
{
	if (buttons_captured < buttons_to_capture) {
		if (display_state == LEDS_PATTERN_WAIT)	{
			display_state = LEDS_PATTERN_ENTRY;
		}

		if (button_state & has_changed & BUTTON_1) {
			ua_pattern[buttons_captured++] =
				NRF_CLOUD_UA_BUTTON_INPUT_3;
			printk("Button 1\n");
		} else if (button_state & has_changed & BUTTON_2) {
			ua_pattern[buttons_captured++] =
				NRF_CLOUD_UA_BUTTON_INPUT_4;
			printk("Button 2\n");
		} else if (has_changed & SWITCH_1) {
			ua_pattern[buttons_captured++] =
				NRF_CLOUD_UA_BUTTON_INPUT_1;
			printk("Switch 1\n");
		} else if (has_changed & SWITCH_2) {
			ua_pattern[buttons_captured++] =
				NRF_CLOUD_UA_BUTTON_INPUT_2;
			printk("Switch 2\n");
		}
	}

	if (buttons_captured == buttons_to_capture) {
		display_state = LEDS_PATTERN_DONE;
		k_sem_give(&user_assoc_sem);
	}
}

/**@brief Callback for button events from the DK buttons and LEDs library. */
static void button_handler(u32_t buttons, u32_t has_changed)
{
	if (pattern_recording && IS_ENABLED(CONFIG_CLOUD_UA_BUTTONS)) {
		pairing_button_register(buttons, has_changed);
		return;
	}
}

/**@brief Processing of user inputs to the application. */
static void input_process(void)
{
	if (!pattern_recording) {
		return;
	}

	if (k_sem_take(&user_assoc_sem, K_NO_WAIT) == 0) {
		cloud_user_associate();
		pattern_recording = false;
		return;
	}

	if (!IS_ENABLED(CONFIG_CLOUD_UA_CONSOLE)) {
		return;
	}

	u8_t c[2];

	c[0] = console_getchar();
	c[1] = console_getchar();

	if (c[0] == 'b' && c[1] == '1') {
		printk("Button 1\n");
		ua_pattern[buttons_captured++] = NRF_CLOUD_UA_BUTTON_INPUT_3;
	} else if (c[0] == 'b' && c[1] == '2') {
		printk("Button 2\n");
		ua_pattern[buttons_captured++] = NRF_CLOUD_UA_BUTTON_INPUT_4;
	} else if (c[0] == 's' && c[1] == '1') {
		printk("Switch 1\n");
		ua_pattern[buttons_captured++] = NRF_CLOUD_UA_BUTTON_INPUT_1;
	} else if (c[0] == 's' && c[1] == '2') {
		printk("Switch 2\n");
		ua_pattern[buttons_captured++] = NRF_CLOUD_UA_BUTTON_INPUT_2;
	}

	if (buttons_captured == buttons_to_capture) {
		k_sem_give(&user_assoc_sem);
	}
}

/**@brief Initializes and submits delayed work. */
static void work_init(void)
{
	k_delayed_work_init(&leds_update_work, leds_update);
	k_work_init(&connect_work, cloud_connect);
	k_delayed_work_submit(&leds_update_work, LEDS_UPDATE_INTERVAL);
}

/**@brief Configures modem to provide LTE link. Blocks until link is
 * successfully established.
 */
static void modem_configure(void)
{
	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already turned on and connected. */
	} else {
		int err;

		printk("LTE LC config ...\n");
		err = lte_lc_init_and_connect();
		__ASSERT(err == 0, "LTE link could not be established.");
	}
}

/**@brief Initializes GPS device and configures trigger if set.
 * Gets initial sample from GPS device.
 */
static void gps_init(void)
{
	int err;
	struct device *gps_dev = device_get_binding(CONFIG_GPS_DEV_NAME);
	struct gps_trigger gps_trig = {
		.type = GPS_TRIG_DATA_READY,
	};

	if (gps_dev == NULL) {
		printk("Could not get %s device\n", CONFIG_GPS_DEV_NAME);
		return;
	}
	printk("GPS device found\n");

	if (IS_ENABLED(CONFIG_GPS_TRIGGER)) {
		err = gps_trigger_set(gps_dev, &gps_trig, gps_trigger_handler);
		if (err) {
			printk("Could not set trigger, error code: %d\n", err);
			return;
		}
	}

	err = gps_sample_fetch(gps_dev);
	__ASSERT(err == 0, "GPS sample could not be fetched.");

	err = gps_channel_get(gps_dev, GPS_CHAN_NMEA, &nmea_data);
	__ASSERT(err == 0, "GPS sample could not be retrieved.");
}

/**@brief Initializes the sensors that are used by the application. */
static void sensors_init(void)
{
	gps_init();
}

/**@brief Initializes buttons and LEDs, using the DK buttons and LEDs
 * library.
 */
static void buttons_leds_init(void)
{
	int err;

	err = dk_buttons_init(button_handler);
	if (err) {
		printk("Could not initialize buttons, err code: %d\n", err);
	}

	err = dk_leds_init();
	if (err) {
		printk("Could not initialize leds, err code: %d\n", err);
	}

	err = dk_set_leds_state(0x00, DK_ALL_LEDS_MSK);
	if (err) {
		printk("Could not set leds state, err code: %d\n", err);
	}
}

void main(void)
{
	printk("Application started\n");

	buttons_leds_init();
	ble_init();

	work_init();
	cloud_init();
	modem_configure();
	cloud_connect(NULL);

	if (IS_ENABLED(CONFIG_CLOUD_UA_CONSOLE)) {
		console_init();
	}

	while (true) {
		nrf_cloud_process();
		input_process();
		send_aggregated_data();
		if (IS_ENABLED(CONFIG_LOG)) {
			/* if logging is enabled, sleep */
			k_sleep(K_MSEC(10));
		} else {
			/* otherwise, put CPU to idle to save power */
			k_cpu_idle();
		}
	}
}
