#include <zephyr/types.h>

#include <misc/printk.h>

#include <device.h>
#include <gpio.h>
#include <i2c.h>

#include "event_manager.h"
#include "module_event.h"
#include "button_event.h"
#include "power_event.h"

#define MODULE		kdb_driver
#define MODULE_NAME	STRINGIFY(MODULE)

#define SYS_LOG_DOMAIN	MODULE_NAME
#define SYS_LOG_LEVEL	CONFIG_DESKTOP_SYS_LOG_KBD_DRIVER_LEVEL
#include <logging/sys_log.h>


#define SX1509_ADDRESS	0x3E

#define RegInputDisableB    0x00
#define RegInputDisableA    0x01
#define RegLongSlewB        0x02
#define RegLongSlewA        0x03
#define RegLowDriveB        0x04
#define RegLowDriveA        0x05
#define RegPullUpB          0x06
#define RegPullUpA          0x07
#define RegPullDownB        0x08
#define RegPullDownA        0x09
#define RegOpenDrainB       0x0A
#define RegOpenDrainA       0x0B
#define RegPolarityB        0x0C
#define RegPolarityA        0x0D
#define RegDirB             0x0E
#define RegDirA             0x0F
#define RegDataB            0x10
#define RegDataA            0x11
#define RegInterruptMaskB   0x12
#define RegInterruptMaskA   0x13
#define RegSenseHighB       0x14
#define RegSenseLowB        0x15
#define RegSenseHighA       0x16
#define RegSenseLowA        0x17
#define RegInterruptSourceB 0x18
#define RegInterruptSourceA 0x19
#define RegEventStatusB     0x1A
#define RegEventStatusA     0x1B
#define RegLevelShifter1    0x1C
#define RegLevelShifter2    0x1D
#define RegClock            0x1E
#define RegMisc             0x1F
#define RegLEDDriverEnableB 0x20
#define RegLEDDriverEnableA 0x21

#define RegDebounceConfig   0x22
#define RegDebounceEnableB  0x23
#define RegDebounceEnableA  0x24
#define RegKeyConfig1       0x25
#define RegKeyConfig2       0x26
#define RegKeyData1         0x27
#define RegKeyData2         0x28

#define RegHighInputB       0x69
#define RegHighInputA       0x6A
#define RegReset            0x7D

static const u8_t register_config[][2] = {
	{ RegReset,         0x12 }, /* Software Reset - Part 1. */
	{ RegReset,         0x34 }, /* Software Reset - Part 2. */
	{ RegPullUpA,       0xFF }, /* Enable pull-ups on Port A (rows). */
	{ RegPolarityA,     0xFF }, /* Enable polarity inversion on Port A. */
	{ RegPolarityB,     0xFF }, /* Enable polarity inversion on Port B. */
	{ RegDataB,         0x00 }, /* Do not drive Port B (columns). */
	{ RegOpenDrainB,    0xFF }, /* Set Port B (cols) pins as open-drain. */
	{ RegDirB,          0x00 }, /* Set Port B (cols) pins as outputs. */
	{ RegInputDisableB, 0xFF }, /* Disable inputs on Port B (cols). */
};


static struct device *i2c_dev;
static struct k_delayed_work kbd_driver_scan;

static bool active;


static int read_bytes(struct device *i2c_dev, u8_t addr, u8_t *data,
		u32_t num_bytes)
{
	struct i2c_msg msgs[2];

	u8_t wr_addr = addr;

	/* Send the address to read from */
	msgs[0].buf = &wr_addr;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	/* Read from device. STOP after this. */
	msgs[1].buf = data;
	msgs[1].len = num_bytes;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, &msgs[0], ARRAY_SIZE(msgs),
			SX1509_ADDRESS);
}

static int write_bytes(struct device *i2c_dev, u8_t addr, const u8_t *data,
		u32_t num_bytes)
{
	struct i2c_msg msgs[2];

	u8_t wr_addr = addr;

	/* Send the address to read from */
	msgs[0].buf = &wr_addr;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	/* Read from device. STOP after this. */
	msgs[1].buf = (u8_t *)data;
	msgs[1].len = num_bytes;
	msgs[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, &msgs[0], ARRAY_SIZE(msgs),
			SX1509_ADDRESS);
}

static int set_registers(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(register_config); i++) {
		int err = write_bytes(i2c_dev, register_config[i][0],
				&register_config[i][1],
				sizeof(register_config[i][1]));

		if (err) {
			return err;
		}
	}

	return 0;
}

static void scan_fn(struct k_work *work)
{
	u8_t row_state[8];
	u8_t mask = 0x01;

	int err;

	/* TODO this implementation MUST be fixed - push SDK to implemnt I2C
	 * async callback!
	 * We also need multiple transaction sequence.
	 */

	for (size_t i = 0; i < ARRAY_SIZE(row_state); i++) {
		err = write_bytes(i2c_dev, RegDataB, &mask, sizeof(mask));
		if (err) {
			break;
		}
		err = read_bytes(i2c_dev, RegDataA, &row_state[i],
				sizeof(mask));
		if (err) {
			break;
		}
		mask = mask << 1;
	}
	if (!err) {
		err = write_bytes(i2c_dev, RegDataB, &mask, sizeof(mask));
	}

	if (err) {
		SYS_LOG_ERR("i2c read error (%d) from %s:%d",
				err, __func__, __LINE__);
	} else {
		static bool matrix[8][8] = {0};

		for (size_t i = 0; i < 8; i++) {
			for (size_t j = 0; j < 8; j++) {
				bool is_pressed = (row_state[i] & (1 << j));

				if (is_pressed && (!matrix[i][j])) {
					struct button_event *event =
						new_button_event();

					event->key_id = (i << 4) | (j & 0x0F);
					event->pressed = true;
					EVENT_SUBMIT(event);
				} else if (!is_pressed && matrix[i][j]) {
					struct button_event *event =
						new_button_event();

					event->key_id = (i << 4) | (j & 0x0F);
					event->pressed = false;
					EVENT_SUBMIT(event);
				}
				matrix[i][j] = is_pressed;
			}
		}

		/* TODO note that we cannot go below kernel tick
		 * actually if tick is 10ms (default) interval cannot go below
		 * 20ms. I set kernel tick to 1ms.
		 */
		if (active) {
			k_delayed_work_submit(&kbd_driver_scan, 210);
		}
	}
}

static void async_init_fn(struct k_work *work)
{
	/* Check if TP is connected */
	i2c_dev = device_get_binding("I2C_0");
	if (!i2c_dev) {
		SYS_LOG_ERR("cannot get I2C device");
		return;
	}

	if (set_registers()) {
		SYS_LOG_ERR("device cannot be initialized");
		return;
	}

	module_set_state("ready");

	k_delayed_work_submit(&kbd_driver_scan, 25);
}
K_WORK_DEFINE(kdb_driver_async_init, async_init_fn);

static bool event_handler(const struct event_header *eh)
{
	if (is_module_event(eh)) {
		struct module_event *event = cast_module_event(eh);

		if (check_state(event, "main", "ready")) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			k_delayed_work_init(&kbd_driver_scan, scan_fn);
			return false;
		}

		if (check_state(event, "board_driver", "ready")) {
			if (!active) {
				active = true;
				k_work_submit(&kdb_driver_async_init);
			}
			return false;
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		if (active) {
			active = false;
			k_delayed_work_cancel(&kbd_driver_scan);

			module_set_state("off");
		}

		return active;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_event);
EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);