#include "hid_event.h"

static void print_event(const struct event_header *eh)
{
}

EVENT_TYPE_DEFINE(hid_keys_event, print_event);
EVENT_TYPE_DEFINE(hid_axis_event, print_event);
