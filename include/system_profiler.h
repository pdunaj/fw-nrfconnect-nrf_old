/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#ifndef _SYSTEM_PROFILER_H_
#define _SYSTEM_PROFILER_H_
#include <zephyr.h>

/** @brief Data types for system profiler.
 */
enum data_type {
	u32,
	s32,
	timestamp
};

#ifndef CONFIG_PROFILER_CUSTOM_EVENT_BUF_LEN
#define CONFIG_PROFILER_CUSTOM_EVENT_BUF_LEN 1
#endif
struct log_event_buf
{
	u8_t* pPayload;
	u8_t pPayloadStart[CONFIG_PROFILER_CUSTOM_EVENT_BUF_LEN];
};

#include <stdio.h>
#include <systemview/SEGGER_SYSVIEW.h>
#include <rtt/SEGGER_RTT.h>



u32_t inline get_mem_address(const void *event_mem_address)
{
       #ifdef CONFIG_SRAM_BASE_ADDRESS
                return (u32_t)(event_mem_address - CONFIG_SRAM_BASE_ADDRESS);
        #else
                return (u32_t)event_mem_address;
        #endif
}

#ifdef CONFIG_PROFILER

void profiler_init();

u16_t profiler_register_event_type(const char *name, const char **args, const enum data_type* arg_types, const u8_t arg_cnt);

void event_log_start(struct log_event_buf* b);
void event_log_add_32(u32_t data, struct log_event_buf* b);
void event_log_add_mem_address(const void *mem_address, struct log_event_buf* b);
void event_log_send(u16_t event_type_id, struct log_event_buf* b);

#else

#define profiler_init() 
#define profiler_register_event_type(name, args, arg_types, arg_cnt)
#define event_log_start(b)
#define event_log_add_32(data, b)
#define event_log_add_mem_address(mem_address, b)
#define event_log_send(event_type_id, b)

#endif /* CONFIG_PROFILER */

#endif /* _SYSTEM_PROFILER_H_ */
