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

static void log_args(const struct event_header* eh, struct log_event_buf *buf)
{
}

static const char description[] = "keep_active_event event_id=%u";

EVENT_TYPE_DEFINE(keep_active_event, print_event, log_args, description);
