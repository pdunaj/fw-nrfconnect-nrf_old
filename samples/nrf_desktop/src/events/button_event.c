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
	event_log_start();
	event_log_add_event_id(eh);
	event_log_add_u32(event->key_id);
	event_log_add_u32((event->pressed)?(1):(0));
	event_log_send(event_type_id);
}

static const char description[] = "button_event event_id=%u button_id=%u status=%u";


EVENT_TYPE_DEFINE(button_event, print_event, log_event, description);
