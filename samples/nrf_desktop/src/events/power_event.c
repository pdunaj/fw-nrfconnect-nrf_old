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
	ARG_UNUSED(event);
	printk("requested by %s", event->module_name);
}

static int log_args(struct log_event_buf *buf, const struct event_header *eh)
{
	return 0;
}

EVENT_INFO_DEFINE(keep_active_event, ENCODE(), ENCODE(), log_args);
EVENT_TYPE_DEFINE(keep_active_event, print_event, &ev_info);
