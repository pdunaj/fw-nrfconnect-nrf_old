/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

/* Mouse motion sensor interface. */

#include <zephyr.h>
#include <atomic.h>

#include <device.h>
#include <spi.h>
#include <gpio.h>

#include "event_manager.h"
#include "motion_event.h"
#include "power_event.h"

#define MODULE motion
#include "module_state_event.h"

#define SYS_LOG_DOMAIN	MODULE_NAME
#define SYS_LOG_LEVEL	CONFIG_DESKTOP_SYS_LOG_MOTION_MODULE_LEVEL
#include <logging/sys_log.h>

/* Optical sensor registers */
#define OPTICAL_REG_PRODUCT_ID			0x00
#define OPTICAL_REG_REVISION_ID			0x01
#define OPTICAL_REG_MOTION			0x02
#define OPTICAL_REG_DELTA_X_L			0x03
#define OPTICAL_REG_DELTA_X_H			0x04
#define OPTICAL_REG_DELTA_Y_L			0x05
#define OPTICAL_REG_DELTA_Y_H			0x06
#define OPTICAL_REG_SQUAL			0x07
#define OPTICAL_REG_RAW_DATA_SUM		0x08
#define OPTICAL_REG_MAXIMUM_RAW_DATA		0x09
#define OPTICAL_REG_MINIMUM_RAW_DATA		0x0A
#define OPTICAL_REG_SHUTTER_LOWER		0x0B
#define OPTICAL_REG_SHUTTER_UPPER		0x0C
#define OPTICAL_REG_CONTROL			0x0D
#define OPTICAL_REG_CONFIG1			0x0F
#define OPTICAL_REG_CONFIG2			0x10
#define OPTICAL_REG_ANGLE_TUNE			0x11
#define OPTICAL_REG_FRAME_CAPTURE		0x12
#define OPTICAL_REG_SROM_ENABLE			0x13
#define OPTICAL_REG_RUN_DOWNSHIFT		0x14
#define OPTICAL_REG_REST1_RATE_LOWER		0x15
#define OPTICAL_REG_REST1_RATE_UPPER		0x16
#define OPTICAL_REG_REST1_DOWNSHIFT		0x17
#define OPTICAL_REG_REST2_RATE_LOWER		0x18
#define OPTICAL_REG_REST2_RATE_UPPER		0x19
#define OPTICAL_REG_REST2_DOWNSHIFT		0x1A
#define OPTICAL_REG_REST3_RATE_LOWER		0x1B
#define OPTICAL_REG_REST3_RATE_UPPER		0x1C
#define OPTICAL_REG_OBSERVATION			0x24
#define OPTICAL_REG_DATA_OUT_LOWER		0x25
#define OPTICAL_REG_DATA_OUT_UPPER		0x26
#define OPTICAL_REG_RAW_DATA_DUMP		0x29
#define OPTICAL_REG_SROM_ID			0x2A
#define OPTICAL_REG_MIN_SQ_RUN			0x2B
#define OPTICAL_REG_RAW_DATA_THRESHOLD		0x2C
#define OPTICAL_REG_CONFIG5			0x2F
#define OPTICAL_REG_POWER_UP_RESET		0x3A
#define OPTICAL_REG_SHUTDOWN			0x3B
#define OPTICAL_REG_INVERSE_PRODUCT_ID		0x3F
#define OPTICAL_REG_LIFTCUTOFF_TUNE3		0x41
#define OPTICAL_REG_ANGLE_SNAP			0x42
#define OPTICAL_REG_LIFTCUTOFF_TUNE1		0x4A
#define OPTICAL_REG_MOTION_BURST		0x50
#define OPTICAL_REG_LIFTCUTOFF_TUNE_TIMEOUT	0x58
#define OPTICAL_REG_LIFTCUTOFF_TUNE_MIN_LENGTH	0x5A
#define OPTICAL_REG_SROM_LOAD_BURST		0x62
#define OPTICAL_REG_LIFT_CONFIG			0x63
#define OPTICAL_REG_RAW_DATA_BURST		0x64
#define OPTICAL_REG_LIFTCUTOFF_TUNE2		0x65

/* Pin configuration for the mouse optical sensor. */
#define OPTICAL_PIN_PWR_CTRL			14
#define OPTICAL_PIN_MOTION			18
#define OPTICAL_PIN_CHIP_SELECT			13

/* Values verified during optical chip initialization. */
#define OPTICAL_PRODUCT_ID			0x42
#define OPTICAL_FIRMWARE_ID			0x04

#define SPI_WRITE_MASK				0x80

/* Number of registers which can be read with a single motion burst. */
#define OPTICAL_MAX_BURST_SIZE			12
/* After first motion read, optical sensor will be polled with this interval.
 * Should be synchronized with BLE connection parameters.
 */
#define OPTICAL_INITIAL_POLL_INTERVAL_MS	50

#define OPTICAL_THREAD_STACK_SIZE		512
#define OPTICAL_THREAD_PRIORITY			K_PRIO_PREEMPT(0)

static K_THREAD_STACK_DEFINE(thread_stack, OPTICAL_THREAD_STACK_SIZE);
static K_SEM_DEFINE(sem, 1, 1);

static struct k_thread thread;
static k_tid_t thread_id;

static struct gpio_callback gpio_cb;

extern const u16_t firmware_length;
extern const u8_t firmware_data[];

static enum {
	STATE_IDLE,
	STATE_FETCHING,
	STATE_TERMINATING,
	STATE_SUSPENDED,
	STATE_ERROR,
} atomic_state;
static atomic_t state;

static struct device *gpio_dev;
static struct device *spi_dev;
static struct spi_config spi_cfg = {
	.operation = (SPI_WORD_SET(8) | SPI_TRANSFER_MSB |
		      SPI_MODE_CPOL | SPI_MODE_CPHA),
	.frequency = CONFIG_DESKTOP_SPI_FREQ_HZ,
	.slave = 0,
};


static void spi_cs_start(void)
{
	int err = gpio_pin_write(gpio_dev, OPTICAL_PIN_CHIP_SELECT, 0);
	if (err) {
		SYS_LOG_ERR("Failed to set chip select pin");
		atomic_set(&state, STATE_ERROR);
	}
}

static void spi_cs_end(void)
{
	int err = gpio_pin_write(gpio_dev, OPTICAL_PIN_CHIP_SELECT, 1);
	if (err) {
		SYS_LOG_ERR("Failed to set chip select pin");
		atomic_set(&state, STATE_ERROR);
	}
}

static int reg_read(u8_t reg, u8_t *buf)
{
	__ASSERT_NO_MSG(reg <= 0x7F);

	const struct spi_buf tx_buf = {
		.buf = &reg,
		.len = 1
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	spi_cs_start();
	k_busy_wait(1); /* check t_NCS-SCLK is 120ns */

	/* Write register address. */
	int ret;

	ret = spi_transceive(spi_dev, &spi_cfg, &tx, NULL);
	if (ret) {
		SYS_LOG_WRN("spi transaction failed");
		goto error;
	}

	k_busy_wait(160); /* t_SRAD */

	/* Read register value. */
	struct spi_buf rx_buf = {
		.buf = buf,
		.len = 1,
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	ret = spi_transceive(spi_dev, &spi_cfg, NULL, &rx);
	if (ret) {
		SYS_LOG_WRN("spi transaction failed");
		goto error;
	}

	k_busy_wait(1); /* t_SCLK-NCS for read is 120ns */
	spi_cs_end();
	k_busy_wait(19); /* t_SWW/t_SWR - t_SCLK-NCS */

	return 0;

error:
	spi_cs_end();
	return ret;
}

static int reg_write(u8_t reg, u8_t val)
{
	u8_t buf[2] = { SPI_WRITE_MASK | reg, val };
	const struct spi_buf tx_buf = {
		.buf = buf,
		.len = 2
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	spi_cs_start();
	k_busy_wait(1); /* t_NCS-SLCK is 120ns */

	int ret = spi_transceive(spi_dev, &spi_cfg, &tx, NULL);
	if (ret) {
		SYS_LOG_WRN("spi transaction FAIL %d\n", ret);
		spi_cs_end();
		return ret;
	}

	k_busy_wait(20); /* t_SCLK-NCS for write */
	spi_cs_end();
	k_busy_wait(100); /* t_SWW/t_SWR - t_SCLK-NCS */

	return 0;
}

int motion_burst_read(u8_t *data, size_t burst_size)
{
	__ASSERT_NO_MSG(burst_size <= OPTICAL_MAX_BURST_SIZE);

	/* Write any value to motion burst register */
	int ret = reg_write(OPTICAL_REG_MOTION_BURST, 0x00);
	if (ret) {
		return ret;
	}

	/* Send motion burst address */
	u8_t write_buf[1] = {OPTICAL_REG_MOTION_BURST};

	struct spi_buf tx_buf = {
		.buf = write_buf,
		.len = 1
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	spi_cs_start();
	ret = spi_transceive(spi_dev, &spi_cfg, &tx, NULL);
	if (ret) {
		return ret;
	}

	k_busy_wait(35); /* t_SRAD-MOTBR */

	const struct spi_buf rx_buf = {
		.buf = data,
		.len = burst_size,
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	/* Start reading SPI data continuously up to 12 bytes. */
	ret = spi_transceive(spi_dev, &spi_cfg, NULL, &rx);
	if (ret) {
		return ret;
	}

	/* Terminate burst */
	spi_cs_end();
	k_busy_wait(1); /* t_BEXIT */

	return 0;
}

static int burst_write(u8_t reg, const u8_t *buf, const int size)
{
	u8_t write_buf = reg | SPI_WRITE_MASK;

	struct spi_buf tx_buf = {
		.buf = &write_buf,
		.len = 1
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	spi_cs_start();
	k_busy_wait(1); /* t_NCS-SLCK is 120ns */

	tx_buf.len = 1;

	/* Write first byte which is address of burst register. */
	int ret = spi_transceive(spi_dev, &spi_cfg, &tx, NULL);
	if (ret) {
		return ret;
	}

	for (size_t i = 0; i < size; i++) {
		write_buf = buf[i];

		ret = spi_transceive(spi_dev, &spi_cfg, &tx, NULL);
		if (ret) {
			return ret;
		}

		k_busy_wait(15);
	}

	/* Terminate burst mode. */
	spi_cs_end();
	k_busy_wait(1); /* t_BEXIT */

	return 0;
}

static int firmware_load(void)
{
	int ret;

	SYS_LOG_INF("Uploading optical sensor firmware...");

	/* Write 0 to Rest_En bit of Config2 register to disable Rest mode. */
	ret = reg_write(OPTICAL_REG_CONFIG2, 0x00);
	if (ret) {
		goto error;
	}

	/* Write 0x1D in SROM_enable register to initialize the operation. */
	ret = reg_write(OPTICAL_REG_SROM_ENABLE, 0x1D);
	if (ret) {
		goto error;
	}

	k_sleep(10);

	/* Write 0x18 to SROM_enable to start SROM download */
	ret = reg_write(OPTICAL_REG_SROM_ENABLE, 0x18);
	if (ret) {
		goto error;
	}

	/* Write SROM file into SROM_Load_Burst register. Data must start with
	 * SROM_Load_Burst address.
	 */
	ret = burst_write(OPTICAL_REG_SROM_LOAD_BURST, firmware_data, firmware_length);
	if (ret) {
		SYS_LOG_ERR("Loading firmware to optical sensor failed!");
		goto error;
	}

	/* Read the SROM_ID register to verify the firmware ID before any
	 * other register reads or writes.
	 */
	k_busy_wait(200);
	u8_t id;
	ret = reg_read(OPTICAL_REG_SROM_ID, &id);
	if (ret) {
		goto error;
	}

	SYS_LOG_DBG("Optical chip firmware ID: 0x%x", id);
	if (id != OPTICAL_FIRMWARE_ID) {
		SYS_LOG_ERR("Chip is not running from SROM!");
		ret = -EIO;
		goto error;
	}

	/* Write 0x00 to Config2 register for wired mouse or 0x20 for
	 * wireless mouse design.
	 */
	ret = reg_write(OPTICAL_REG_CONFIG2, 0x20);
	if (ret) {
		goto error;
	}

	/* Set initial CPI resolution. */
	ret = reg_write(OPTICAL_REG_CONFIG1, 0x15);
	if (ret) {
		goto error;
	}

	spi_cs_end();

	return 0;

error:
	spi_cs_end();
	return ret;
}

static void motion_irq_handler(struct device *gpiob, struct gpio_callback *cb,
			       u32_t pins)
{
	/* If a motion interrupt happens in IDLE state, atomically go to
	 * FETCHING state.
	 */
	if (atomic_cas(&state, STATE_IDLE, STATE_FETCHING)) {
		k_sem_give(&sem);

		/* Turn off motion interrupt as we enter active polling mode
		 * until no new motion occurs.
		 */
		gpio_pin_disable_callback(gpio_dev, OPTICAL_PIN_MOTION);
		return;
	}

	if (atomic_cas(&state, STATE_SUSPENDED, STATE_IDLE)) {
		/* Module is in low power state */
		struct wake_up_event *event = new_wake_up_event();

		if (event) {
			EVENT_SUBMIT(event);
		}

		gpio_pin_enable_callback(gpio_dev, OPTICAL_PIN_MOTION);

		return;
	}
}

static void motion_read(void)
{
	u8_t data[OPTICAL_MAX_BURST_SIZE];

	int err = motion_burst_read(data, OPTICAL_MAX_BURST_SIZE);
	if (err) {
		atomic_set(&state, STATE_ERROR);
		return;
	}

	if (!data[2] && !data[3] && !data[4] && !data[5]) {
		/* No motion occurred. */
		atomic_cas(&state, STATE_FETCHING, STATE_IDLE);
		gpio_pin_enable_callback(gpio_dev, OPTICAL_PIN_MOTION);

		SYS_LOG_DBG("Stop polling, wait for motion interrupt");
		return;
	}

	struct motion_event *event = new_motion_event();

	if (event) {
		/* Convert burst data to 16-bit signed motion values. */
		event->dx = (s16_t)(data[5] << 8) | data[4];
		event->dy = -((s16_t)(data[3] << 8) | data[2]);

		EVENT_SUBMIT(event);
	} else {
		SYS_LOG_ERR("no memory");
		atomic_set(&state, STATE_ERROR);
		return;
	}
}

static void init(void)
{
	int err;

	gpio_dev = device_get_binding(CONFIG_GPIO_P0_DEV_NAME);
	if (!gpio_dev) {
		SYS_LOG_ERR("Cannot get GPIO device");
		goto error;
	}

	/* Turn on optical sensor. */
	err = gpio_pin_configure(gpio_dev, OPTICAL_PIN_PWR_CTRL, GPIO_DIR_OUT);
	if (err) {
		goto error;
	}

	err = gpio_pin_write(gpio_dev, OPTICAL_PIN_PWR_CTRL, 1);
	if (err) {
		goto error;
	}

	/* Wait for VCC to be stable. */
	k_sleep(100);

	/* Setup SPI */
	spi_dev = device_get_binding(CONFIG_SPI_1_NAME);
	if (!spi_dev) {
		goto error;
	}
	err = gpio_pin_configure(gpio_dev,
		OPTICAL_PIN_CHIP_SELECT, GPIO_DIR_OUT);
	if (err) {
		goto error;
	}

	/* Drive slave select high, and then low to reset the SPI port. */
	spi_cs_end();
	spi_cs_start();
	spi_cs_end();

	/* Write 0x5A to Power_Up_Reset register */
	err = reg_write(OPTICAL_REG_POWER_UP_RESET, 0x5A);
	if (err) {
		goto error;
	}
	k_sleep(60);

	/* Read from registers 0x02-0x06 regardless of the motion pin state. */
	u8_t buf[1];
	err = reg_read(0x02, buf);
	err |= reg_read(0x03, buf);
	err |= reg_read(0x04, buf);
	err |= reg_read(0x05, buf);
	err |= reg_read(0x06, buf);
	if (err) {
		goto error;
	}

	/* Perform SROM download. */
	err = firmware_load();
	if (err) {
		goto error;
	}

	/* TODO: Load configuration for other registers. */

	/* Verify product id */
	u8_t product_id;

	err = reg_read(OPTICAL_REG_PRODUCT_ID, &product_id);
	if (err) {
		goto error;
	}
	if (product_id != OPTICAL_PRODUCT_ID) {
		SYS_LOG_ERR("Wrong product ID");
		goto error;
	}

	/* Enable interrupt from motion pin falling edge. */
	err = gpio_pin_configure(gpio_dev, OPTICAL_PIN_MOTION,
				 GPIO_DIR_IN | GPIO_INT | GPIO_PUD_PULL_UP |
				 GPIO_INT_LEVEL | GPIO_INT_ACTIVE_LOW);
				 /* TODO: move to EDGE if GPIO driver is fixed */
	if (err) {
		SYS_LOG_ERR("Failed to confiugure GPIO");
		goto error;
	}

	gpio_init_callback(&gpio_cb, motion_irq_handler, BIT(OPTICAL_PIN_MOTION));
	err = gpio_add_callback(gpio_dev, &gpio_cb);
	if (err) {
		SYS_LOG_ERR("Cannot add GPIO callback");
		goto error;
	}

	err = gpio_pin_enable_callback(gpio_dev, OPTICAL_PIN_MOTION);
	if (err) {
		SYS_LOG_ERR("Cannot enable GPIO interrupt");
		goto error;
	}

	/* Inform all that module is ready */
	module_set_state(MODULE_STATE_READY);

	return;

error:
	atomic_set(&state, STATE_ERROR);
}

/* Optical hardware interface state machine. */
static void optical_thread_fn()
{
	s32_t timeout = K_FOREVER;

	/* Initialize sensor. */
	init();

	while (true) {
		k_sem_take(&sem, timeout);

		switch (atomic_get(&state)) {
		case STATE_IDLE:
			timeout = K_FOREVER;
			break;

		case STATE_FETCHING:
			timeout = K_MSEC(OPTICAL_INITIAL_POLL_INTERVAL_MS);
			motion_read();
			break;

		case STATE_SUSPENDED:
			/* Motion sensor has internal power management. It will
			 * automatically downshift to subsequent "rest modes"
			 * when inactive. We use the sensor interrupt as
			 * a wake up source from the low-power connected state.
			 */
			break;

		case STATE_ERROR:
			module_set_state(MODULE_STATE_ERROR);
			return;

		default:
			__ASSERT_NO_MSG(false);
		}
	}
}

static bool event_handler(const struct event_header *eh)
{
	static bool initialized;

	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(board), MODULE_STATE_READY)) {

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			/* Start state machine thread */
			atomic_set(&state, STATE_IDLE);
			thread_id = k_thread_create(
				&thread, thread_stack,
				OPTICAL_THREAD_STACK_SIZE,
				optical_thread_fn,
				NULL, NULL, NULL,
				OPTICAL_THREAD_PRIORITY, 0, K_NO_WAIT);

			return false;
		}

		return false;
	}

	if (is_wake_up_event(eh)) {
		if (!atomic_cas(&state, STATE_SUSPENDED, STATE_IDLE)) {
			module_set_state(MODULE_STATE_READY);
		}

		return false;
	}

	if (is_power_down_event(eh)) {
		if (atomic_get(&state) != STATE_SUSPENDED) {
			atomic_set(&state, STATE_SUSPENDED);
			module_set_state(MODULE_STATE_STANDBY);
			k_sem_give(&sem);

			initialized = false;
		}

		return false;
	}
	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE_EARLY(MODULE, power_down_event);
