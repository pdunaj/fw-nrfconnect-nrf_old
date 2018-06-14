#include "power_event.h"

EVENT_TYPE_DEFINE(power_down_event, NULL);

static void print_event(const struct event_header *eh)
{
	struct keep_active_event *event = cast_keep_active_event(eh);

	printk("requested by %s", event->module_name);
}

EVENT_TYPE_DEFINE(keep_active_event, print_event);
