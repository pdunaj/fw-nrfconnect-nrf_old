/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include "button_event.h"
#define LOG_ARG_CNT 2

static void print_event(const struct event_header *eh)
{
	struct button_event *event = cast_button_event(eh);

	printk("key_id=0x%x %s",
			event->key_id,
			(event->pressed)?("pressed"):("released"));
}

static void log_args(const struct event_header *eh, struct log_event_buf *buf)
{
	struct button_event *event = cast_button_event(eh);
	ARG_UNUSED(event);
	event_log_add_32(event->key_id, buf);
	event_log_add_32((event->pressed)?(1):(0), buf);
}

static const enum data_type log_args_types[LOG_ARG_CNT] = {u32, u32};
static const char * log_args_labels[LOG_ARG_CNT] = {"button_id", "status"};

static struct event_log_info log_info = {
	.log_args = log_args,
	.log_args_cnt = LOG_ARG_CNT,
	.log_args_labels = log_args_labels,
	.log_args_types = log_args_types,
};


EVENT_TYPE_DEFINE(button_event, print_event, log_args, &log_info);
