/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include "motion_event.h"

static void print_event(const struct event_header *eh)
{
	struct motion_event *event = cast_motion_event(eh);

	printk("dx=%d, dy=%d", event->dx, event->dy);
}

static void log_event(const struct event_header *eh, uint16_t event_type_id)
{
	struct motion_event *event = cast_motion_event(eh);
	
	struct log_event_buf b;
	event_log_start(&b);
	event_log_add_event_id(eh, &b);
	event_log_add_32(event->dx, &b);
	event_log_add_32(event->dy, &b);
	event_log_send(event_type_id, &b);
}

static const char description[] = "motion_event event_id=%u dx=%d dy=%d";

EVENT_TYPE_DEFINE(motion_event, print_event, log_event, description);
