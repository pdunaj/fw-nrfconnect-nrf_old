#include "motion_event.h"

static void print_event(const struct event_header *eh)
{
	struct motion_event *event = cast_motion_event(eh);

	printk("dx=%d, dy=%d", event->dx, event->dy);
}

EVENT_TYPE_DEFINE(motion_event, print_event);
