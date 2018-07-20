/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#ifndef _SYSVIEW_CUSTOM_EVENT_LOGGER_H_
#define _SYSVIEW_CUSTOM_EVENT_LOGGER_H_

#include <zephyr.h>
#include <systemview/SEGGER_SYSVIEW.h>
#include <rtt/SEGGER_RTT.h>
#include <stdio.h>
#include <string.h>
#define NUMBER_OF_PREDEFINED_EVENTS 2

#ifdef CONFIG_SYSVIEW_LOG_CUSTOM_EVENTS

extern struct SEGGER_SYSVIEW_MODULE_STRUCT events;
struct event_header;
struct event_type;

void log_event_exec_start(const struct event_header *eh);
void log_event_exec_end(const struct event_header *eh);
void send_event_description(const struct event_type* et, uint16_t event_id);
void SEGGER_SYSVIEW_Conf(void);
#endif

#ifndef CONFIG_SYSVIEW_INITIALIZATION
	#define SEGGER_SYSVIEW_RecordVoid(a)
	#define SEGGER_SYSVIEW_RecordU32(a, b)
	#define SEGGER_SYSVIEW_RecordU32x2(a, b, c)
	#define SEGGER_SYSVIEW_RecordU32x3(a, b, c, d)
	#define SEGGER_SYSVIEW_RecordU32x4(a, b, c, d, e)
	#define SEGGER_SYSVIEW_RecordU32x5(a, b, c, d, e, f)
	#define SEGGER_SYSVIEW_RecordU32x6(a, b, c, d, e, f, g)
	#define SEGGER_SYSVIEW_RecordU32x7(a, b, c, d, e, f, g, h)
	#define SEGGER_SYSVIEW_RecordU32x8(a, b, c, d, e, f, g, h, i)
	#define SEGGER_SYSVIEW_RecordU32x9(a, b, c, d, e, f, g, h, i, j)
	#define SEGGER_SYSVIEW_RecordU32x10(a, b, c, d, e, f, g, h, i, j, k)
	#define SEGGR_SYSVIEW_RecordString(a, b)
#endif

#endif /* _SYSVIEW_CUSTOM_EVENT_LOGGER_H_ */
