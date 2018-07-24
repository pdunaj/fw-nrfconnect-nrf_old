/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include "power_event.h"
#define LOG_ARG_CNT 0

EVENT_TYPE_DEFINE(power_down_event, NULL, NULL, NULL);

static void print_event(const struct event_header *eh)
{
	struct keep_active_event *event = cast_keep_active_event(eh);

	printk("requested by %s", event->module_name);
}

static void log_args(const struct event_header* eh, struct log_event_buf *buf)
{
}

static const enum data_type log_args_types[LOG_ARG_CNT] = {};
static const char * log_args_labels[LOG_ARG_CNT] = {};

static struct event_log_info log_info = {
	.log_args = log_args,
	.log_args_cnt = LOG_ARG_CNT,
	.log_args_labels = log_args_labels,
	.log_args_types = log_args_types,
};


EVENT_TYPE_DEFINE(keep_active_event, print_event, log_args, &log_info);
