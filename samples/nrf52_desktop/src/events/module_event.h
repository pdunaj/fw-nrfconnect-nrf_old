#ifndef _MODULE_EVENT_H_
#define _MODULE_EVENT_H_

/**
 * @brief Module Event
 * @defgroup module_event Module Event
 * @{
 */

#include <string.h>

#include <zephyr.h>
#include <zephyr/types.h>

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct module_event {
	struct event_header header;

	const char *name;
	const char *state;
};

EVENT_TYPE_DECLARE(module_event);


static inline void _module_set_state(const char *name, const char *state)
{
	struct module_event *event = new_module_event();

	if (event) {
		event->name = name;
		event->state = state;
		EVENT_SUBMIT(event);
	}
}

#define module_set_state(st) _module_set_state(MODULE_NAME, st)

#define check_state(event, mod, st) \
	(!strcmp(event->name, mod) && !strcmp(event->state, st))

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _MODULE_EVENT_H_ */