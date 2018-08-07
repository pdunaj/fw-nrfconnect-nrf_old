/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <system_profiler.h>

static char descr[CONFIG_MAX_NUMBER_OF_CUSTOM_EVENTS][CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS];
static char * arg_types_encodings[] = {"%u", "%d", "%u", "%d", "%u", "%d", "%s" "%D" };


static void event_module_description(void);
struct SEGGER_SYSVIEW_MODULE_STRUCT events = {
                .sModule = "M=EventManager",
                .NumEvents = 0,
                .EventOffset = 0,
                .pfSendModuleDesc = event_module_description,
                .pNext = NULL
        };

static void event_module_description(void) 
{
	for (size_t i = 0; i < events.NumEvents; i++) {
		SEGGER_SYSVIEW_RecordModuleDescription(&events, descr[i]);
	}
}

u32_t static shorten_mem_address(const void *event_mem_address)
{
#ifdef CONFIG_SRAM_BASE_ADDRESS
	return (u32_t)(event_mem_address - CONFIG_SRAM_BASE_ADDRESS);
#else
	return (u32_t)event_mem_address;
#endif
}


int profiler_init(void)
{
	if (IS_ENABLED(CONFIG_SYSVIEW_INITIALIZATION)) {
		SEGGER_SYSVIEW_Conf();
		if (IS_ENABLED(CONFIG_SYSVIEW_START_LOGGING_ON_SYSTEM_START)) {		
			SEGGER_SYSVIEW_Start();
		}
	}

	SEGGER_SYSVIEW_RegisterModule(&events);
	return 0;
}

void profiler_sleep(void)
{

}

u16_t profiler_register_event_type(const char *name, const char **args, const enum profiler_arg *arg_types, const u8_t arg_cnt)
{
	u8_t event_number = events.NumEvents;
	u8_t pos = 0;
	pos += sprintf(descr[event_number], "%d %s", event_number, name);

	if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_TRACE_EVENT_EXECUTION)) {
		pos += sprintf(descr[event_number] + pos, " %s=%s", "mem_address", "%u");
	}	

	u8_t t;
	for(t = 0; t < arg_cnt; t++) {
		pos += sprintf(descr[event_number] + pos, " %s=%s", args[t], arg_types_encodings[arg_types[t]]);
	}
	events.NumEvents ++;
	return events.EventOffset + event_number;
}

void event_log_start(struct log_event_buf *buf)
{
	/* protocol implementation in SysView demands incrementing pointer by 4 on start */
	buf->pPayload = buf->pPayloadStart + 4; 
}

void event_log_add_32(struct log_event_buf *buf, u32_t data)
{
	buf->pPayload = SEGGER_SYSVIEW_EncodeU32(buf->pPayload, data);
}

void event_log_add_mem_address(struct log_event_buf *buf, const void *event_mem_addres)
{
	buf->pPayload = SEGGER_SYSVIEW_EncodeU32(buf->pPayload, shorten_mem_address(event_mem_addres));
}

void event_log_send(struct log_event_buf *buf, u16_t event_type_id)
{
	SEGGER_SYSVIEW_SendPacket(buf->pPayloadStart, buf->pPayload, event_type_id);
}
