#include <zephyr/types.h>

#include <misc/reboot.h>

#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

#include "module_event.h"
#include "ble_event.h"


#define MODULE		ble_adv
#define MODULE_NAME	STRINGIFY(MODULE)

#define SYS_LOG_DOMAIN	MODULE_NAME
#define SYS_LOG_LEVEL	CONFIG_DESKTOP_SYS_LOG_BLE_ADV_LEVEL
#include <logging/sys_log.h>


#define DEVICE_NAME		CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)


static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
			  0x12, 0x18,	/* HID Service */
			  0x0f, 0x18),	/* Battery Service */
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void ble_adv_start(void)
{
	/* TODO: use bond manager to check if it possible to pair with another
	 * device
	 * Currently bt_keys and bt_settings APIs are private
	 */
	int err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
				  sd, ARRAY_SIZE(sd));

	if (err) {
		SYS_LOG_ERR("Advertising failed to start (err %d)", err);
		sys_reboot(SYS_REBOOT_WARM);
	}

	SYS_LOG_INF("Advertising started");
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_event(eh)) {
		struct module_event *event = cast_module_event(eh);
		static unsigned int srv_ready_cnt;
		/* The services that must be initialized before starting advertising. */
		const char* services[] = {"hog", "bas", "dis"};

		SYS_LOG_DBG("event from %s", event->name);

		for (int i=0; i<ARRAY_SIZE(services); i++) {
			if (check_state(event, services[i], "ready")) {
				srv_ready_cnt++;
				SYS_LOG_WRN("received %s ready! cnt: %u", services[i], srv_ready_cnt);
			}
		}

		if (srv_ready_cnt >= ARRAY_SIZE(services)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			ble_adv_start();
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_event);
EVENT_SUBSCRIBE(MODULE, ble_peer_event);
