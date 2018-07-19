/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#ifndef _PROFILER_H_
#define _PROFILER_H_


#include <zephyr.h>
#include <systemview/SEGGER_SYSVIEW.h>
#include <rtt/SEGGER_RTT.h>

struct log_event_buf
{
	U8* pPayload;
	U8 pPayloadStart[CONFIG_CUSTOM_EVENT_BUF_LEN];
};

u32_t inline get_event_id(const void *eh)
{
       #ifdef CONFIG_SRAM_BASE_ADDRESS
                return (u32_t)(eh - CONFIG_SRAM_BASE_ADDRESS);
        #else
                return (u32_t)eh;
        #endif
}

void event_log_start(struct log_event_buf* b);
void event_log_add_32(u32_t data, struct log_event_buf* b);
void event_log_add_event_id(const void *eh, struct log_event_buf* b);
void event_log_send(u16_t event_type_id, struct log_event_buf* b);

#endif
