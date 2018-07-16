/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include "power_event.h"

EVENT_TYPE_DEFINE(power_down_event, NULL, NULL);

static void print_event(const struct event_header *eh)
{
	struct keep_active_event *event = cast_keep_active_event(eh);

	printk("requested by %s", event->module_name);
}

static void log_event(const struct event_header *eh)
{
	//struct keep_active_event *event = cast_keep_active_event(eh);
	SEGGER_SYSVIEW_RecordU32(events.EventOffset+6, get_event_id(eh));
}

EVENT_TYPE_DEFINE(keep_active_event, print_event, log_event);
