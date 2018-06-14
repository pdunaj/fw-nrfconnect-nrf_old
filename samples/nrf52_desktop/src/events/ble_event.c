#include "ble_event.h"

static void print_event(const struct event_header *eh)
{
}

EVENT_TYPE_DEFINE(ble_peer_event, print_event);
