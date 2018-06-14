#ifndef _BATTERY_EVENT_H_
#define _BATTERY_EVENT_H_

/**
 * @brief Battery Event
 * @defgroup battery_event Battery Event
 * @{
 */

#include <zephyr.h>
#include <zephyr/types.h>

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct battery_event {
	struct event_header header;

	u8_t level;
};

EVENT_TYPE_DECLARE(battery_event);

#ifdef __cplusplus
}
#endif

#endif /* _BATTERY_EVENT_H_ */
