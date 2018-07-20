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

static void log_args(const struct event_header* eh, struct log_event_buf *buf)
{
	struct motion_event *event = cast_motion_event(eh);
	event_log_add_32(event->dx, buf);
	event_log_add_32(event->dy, buf);
}

static const char description[] = "motion_event event_id=%u dx=%d dy=%d";

EVENT_TYPE_DEFINE(motion_event, print_event, log_args, description);
