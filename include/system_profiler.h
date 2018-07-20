/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#ifndef _SYSTEM_PROFILER_H_
#define _SYSTEM_PROFILER_H_

#ifdef CONFIG_SYSTEM_PROFILER

#include <zephyr.h>
#include <systemview/SEGGER_SYSVIEW.h>
#include <rtt/SEGGER_RTT.h>

#include <sysview_custom_event_logger.h>
//#include <sysview_kernel_event_logger.h>

struct log_event_buf
{
	U8* pPayload;
	U8 pPayloadStart[CONFIG_CUSTOM_EVENT_BUF_LEN];
};

u32_t inline get_event_id(const void *event_mem_address)
{
       #ifdef CONFIG_SRAM_BASE_ADDRESS
                return (u32_t)(event_mem_address - CONFIG_SRAM_BASE_ADDRESS);
        #else
                return (u32_t)event_mem_address;
        #endif
}

void system_profiler_init(uint16_t num_events);
void system_profiler_event_exec_start(const void *event_mem_address);
void system_profiler_event_exec_end(const void *event_mem_address);
void system_profiler_event_submit(const struct event_header* eh, u16_t event_type_id, void (*log_event)(const struct event_header* eh, u16_t event_type_id) );

void event_log_start(struct log_event_buf* b);
void event_log_add_32(u32_t data, struct log_event_buf* b);
void event_log_add_event_mem_address(const void *event_mem_address, struct log_event_buf* b);
void event_log_send(u16_t event_type_id, struct log_event_buf* b);

#endif

#endif
