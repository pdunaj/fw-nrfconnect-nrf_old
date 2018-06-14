#ifndef _HID_EVENT_H_
#define _HID_EVENT_H_

/**
 * @brief HID Event
 * @defgroup hid_event HID Event
 * @{
 */

#include <zephyr.h>
#include <zephyr/types.h>

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct hid_keys_event {
	struct event_header header;

	u8_t keys[3];
};

EVENT_TYPE_DECLARE(hid_keys_event);

struct hid_axis_event {
	struct event_header header;

	s16_t x;
	s16_t y;
};

EVENT_TYPE_DECLARE(hid_axis_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _HID_EVENT_H_ */