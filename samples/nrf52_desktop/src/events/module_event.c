#include "module_event.h"

static void print_event(const struct event_header *eh)
{
	struct module_event *event = cast_module_event(eh);

	printk("module:%s state:%s", event->name, event->state);
}

EVENT_TYPE_DEFINE(module_event, print_event);
