/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <system_profiler.h>

void system_profiler_init(uint16_t num_events)
{
	if (IS_ENABLED(CONFIG_SYSVIEW_INITIALIZATION)) {
		SEGGER_SYSVIEW_Conf();
		if (IS_ENABLED(CONFIG_SYSVIEW_START_LOGGING_ON_SYSTEM_START)) {		
			SEGGER_SYSVIEW_Start();
		}
	}

/* Currently not supported
	if (IS_ENABLED(CONFIG_SYSVIEW_LOG_KERNEL_EVENTS)) {
		kernel_event_logger_init();
	}
*/

	if (IS_ENABLED(CONFIG_SYSVIEW_LOG_CUSTOM_EVENTS)) {
		events.NumEvents = num_events;
		SEGGER_SYSVIEW_RegisterModule(&events);
	}
}

void system_profiler_event_exec_start(const void *event_mem_addres)
{
        SEGGER_SYSVIEW_RecordU32(events.EventOffset, get_event_id(event_mem_addres));
}

void system_profiler_event_exec_end(const void *event_mem_addres)
{
        SEGGER_SYSVIEW_RecordU32(events.EventOffset+1, get_event_id(event_mem_addres));
}

void system_profiler_event_submit(const void *event_mem_addres, u16_t event_type_id, void (*log_event)(const struct event_header * eh, u16_t event_type_id) )
{
	if (log_event){
		log_event(event_mem_addres, events.EventOffset + NUMBER_OF_PREDEFINED_EVENTS + event_type_id);
	}
}

void event_log_start(struct log_event_buf* b)
{
	b->pPayload = b->pPayloadStart + 4; //protocol implementation in SysView demands this
}

void event_log_add_32(u32_t data, struct log_event_buf* b)
{
	b->pPayload = SEGGER_SYSVIEW_EncodeU32(b->pPayload, data);
}

void event_log_add_event_mem_address(const void *event_mem_addres, struct log_event_buf* b)
{
	b->pPayload = SEGGER_SYSVIEW_EncodeU32(b->pPayload, get_event_id(event_mem_addres));
}

void event_log_send(u16_t event_type_id, struct log_event_buf* b)
{
	SEGGER_SYSVIEW_SendPacket(b->pPayloadStart, b->pPayload, event_type_id);
}


