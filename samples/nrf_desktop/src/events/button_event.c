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

static void log_event(const struct event_header *eh, uint16_t event_type_id)
{
	struct button_event *event = cast_button_event(eh);
	SEGGER_SYSVIEW_RecordU32x3(event_type_id, get_event_id(eh), event->key_id, (event->pressed)?(1):(0));
}

static const char description[] = "button_event event_id=%u button_id=%u status=%u";


EVENT_TYPE_DEFINE(button_event, print_event, log_event, description);
