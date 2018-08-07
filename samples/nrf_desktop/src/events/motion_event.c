/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include "motion_event.h"
#define LOG_ARG_CNT 2


static void print_event(const struct event_header *eh)
{
	struct motion_event *event = cast_motion_event(eh);

	printk("dx=%d, dy=%d", event->dx, event->dy);
}

static int log_args(const struct event_header *eh, struct log_event_buf *buf)
{
	struct motion_event *event = cast_motion_event(eh);
	ARG_UNUSED(event);
	event_log_add_32(event->dx, buf);
	event_log_add_32(event->dy, buf);
	return 0;
}

static const enum data_type log_args_types[LOG_ARG_CNT] = {s32, s32};
static const char *log_args_labels[LOG_ARG_CNT] = {"dx", "dy"};

static struct profiler_info prof_info = {
	.log_args = log_args,
	.log_args_cnt = LOG_ARG_CNT,
	.log_args_labels = log_args_labels,
	.log_args_types = log_args_types,
};


EVENT_TYPE_DEFINE(motion_event, print_event, &prof_info);
