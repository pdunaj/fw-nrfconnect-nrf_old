/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include "power_event.h"
#define LOG_ARG_CNT 0

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

static const enum profiler_arg log_args_types[LOG_ARG_CNT] = {};
static const char *log_args_labels[LOG_ARG_CNT] = {};

static struct profiler_info prof_info = {
	.log_args = log_args,
	.log_args_cnt = LOG_ARG_CNT,
	.log_args_labels = log_args_labels,
	.log_args_types = log_args_types,
};


EVENT_TYPE_DEFINE(keep_active_event, print_event, &prof_info);
