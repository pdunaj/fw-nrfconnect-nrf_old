/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include "power_event.h"

EVENT_TYPE_DEFINE(power_down_event, NULL, NULL, NULL);

static void print_event(const struct event_header *eh)
{
	struct keep_active_event *event = cast_keep_active_event(eh);

	printk("requested by %s", event->module_name);
}

static void log_event(const struct event_header *eh, uint16_t event_type_id)
{
	//struct keep_active_event *event = cast_keep_active_event(eh);
	
	event_log_start(1);
	event_log_add_event_id(eh);
	event_log_send(event_type_id);
}

static const char description[] = "keep_active_event event_id=%u";

EVENT_TYPE_DEFINE(keep_active_event, print_event, log_event, description);
