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

static int log_args(struct log_event_buf *buf, const struct event_header *eh)
{
	struct motion_event *event = cast_motion_event(eh);
	ARG_UNUSED(event);
	profiler_log_encode_u32(buf, event->dx);
	profiler_log_encode_u32(buf, event->dy);
	return 0;
}

//static const enum profiler_arg log_arg_types[] = {PROFILER_ARG_S32, PROFILER_ARG_S32};
//static const char *log_arg_labels[] = {"dx", "dy"};

EVENT_INFO_DEFINE(motion_event, WRAP(PROFILER_ARG_S32, PROFILER_ARG_S32), WRAP("dx", "dy") , log_args);
EVENT_TYPE_DEFINE(motion_event, print_event, &ev_info);
