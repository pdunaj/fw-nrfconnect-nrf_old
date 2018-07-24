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

extern struct SEGGER_SYSVIEW_MODULE_STRUCT events;
struct event_header;
struct event_type;

void system_profiler_event_exec_start(const void *event_mem_address);
void system_profiler_event_exec_end(const void *event_mem_address);
void system_profiler_event_submit(const struct event_header* eh, u16_t event_type_id, void (*log_event)(const struct event_header* eh, u16_t event_type_id) );

void send_event_description(const struct event_type* et, uint16_t event_id);
void SEGGER_SYSVIEW_Conf(void);

#endif /* _SYSVIEW_CUSTOM_EVENT_LOGGER_H_ */
