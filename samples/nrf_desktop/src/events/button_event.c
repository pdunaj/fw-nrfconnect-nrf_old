/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include "button_event.h"

static void print_event(const struct event_header *eh)
{
	struct button_event *event = cast_button_event(eh);

	printk("key_id=0x%x %s",
			event->key_id,
			(event->pressed)?("pressed"):("released"));
}

static void log_event(const struct event_header *eh)
{
	struct button_event *event = cast_button_event(eh);
	SEGGER_SYSVIEW_RecordU32x3(events.EventOffset+2, get_event_id(eh), event->key_id, (event->pressed)?(1):(0));
}


EVENT_TYPE_DEFINE(button_event, print_event, log_event);
