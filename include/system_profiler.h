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

/** @brief Data type for system profiler.
 */

enum data_type {
	u32,
	s32,
	timestamp
};

void profiler_event_exec_start(const void *event_mem_addres);
void profiler_event_exec_end(const void *event_mem_addres);

struct log_event_buf
{
	U8* pPayload;
	U8 pPayloadStart[CONFIG_PROFILER_CUSTOM_EVENT_BUF_LEN];
};

u32_t inline get_mem_address(const void *event_mem_address)
{
       #ifdef CONFIG_SRAM_BASE_ADDRESS
                return (u32_t)(event_mem_address - CONFIG_SRAM_BASE_ADDRESS);
        #else
                return (u32_t)event_mem_address;
        #endif
}


void profiler_init();

u16_t profiler_register_event_type(const char *name, const char **args, const enum data_type* arg_types, const u8_t arg_cnt);
void event_log_start(struct log_event_buf* b);
void event_log_add_32(u32_t data, struct log_event_buf* b);
void event_log_add_mem_address(const void *mem_address, struct log_event_buf* b);
void event_log_send(u16_t event_type_id, struct log_event_buf* b);

#endif /* CONFIG_SYSTEM_PROFILER */

#endif /* _SYSTEM_PROFILER_H_ */
