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
	SEGGER_SYSVIEW_RecordU32x3(event_type_id, get_event_id(eh), event->dx, event->dy);
}

static const char description[] = "motion_event event_id=%u dx=%d dy=%d";

EVENT_TYPE_DEFINE(motion_event, print_event, log_event, description);
