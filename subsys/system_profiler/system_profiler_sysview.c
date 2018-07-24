/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <system_profiler.h>

static char descr[CONFIG_MAX_NUMBER_OF_CUSTOM_EVENTS][128];
static char * arg_types_encodings[] = {"%u", "%d", "%D" };


static void event_module_description(void);
struct SEGGER_SYSVIEW_MODULE_STRUCT events = {
                .sModule = "M=EventManager",
                .NumEvents = 0,
                .EventOffset = 0,
                .pfSendModuleDesc = event_module_description,
                .pNext = NULL
        };

static void event_module_description(void) {
	u8_t i;	
	for (i = 0; i < events.NumEvents; i++)
	{
		SEGGER_SYSVIEW_RecordModuleDescription(&events, descr[i]);
	}
}

u32_t inline subtract_sram_base_from_mem_address(const void *event_mem_address)
{
       #ifdef CONFIG_SRAM_BASE_ADDRESS
                return (u32_t)(event_mem_address - CONFIG_SRAM_BASE_ADDRESS);
        #else
                return (u32_t)event_mem_address;
        #endif
}


int profiler_init()
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

	SEGGER_SYSVIEW_RegisterModule(&events);
	return 0;
}

u16_t profiler_register_event_type(const char *name, const char **args, const enum data_type *arg_types, const u8_t arg_cnt)
{
	u8_t event_number = events.NumEvents;
	u8_t pos = 0;
	pos += sprintf(descr[event_number], "%d %s", event_number, name);

	if (IS_ENABLED(CONFIG_PROFILER_CUSTOM_EVENTS_LOG_MEM_ADDR))
	{
		pos += sprintf(descr[event_number] + pos, " %s=%s", "mem_address", "%u");
	}	

	u8_t t;
	for(t = 0; t < arg_cnt; t++)
	{
		pos += sprintf(descr[event_number] + pos, " %s=%s", args[t], arg_types_encodings[arg_types[t]]);
	}
	events.NumEvents ++;
	return events.EventOffset + event_number;
}

void event_log_start(struct log_event_buf* b)
{
	/* protocol implementation in SysView demands incrementing pointer by 4 on start */
	b->pPayload = b->pPayloadStart + 4; 
}

void event_log_add_32(u32_t data, struct log_event_buf* b)
{
	b->pPayload = SEGGER_SYSVIEW_EncodeU32(b->pPayload, data);
}

void event_log_add_mem_address(const void *event_mem_addres, struct log_event_buf* b)
{
	b->pPayload = SEGGER_SYSVIEW_EncodeU32(b->pPayload, subtract_sram_base_from_mem_address(event_mem_addres));
}

void event_log_send(u16_t event_type_id, struct log_event_buf* b)
{
	SEGGER_SYSVIEW_SendPacket(b->pPayloadStart, b->pPayload, event_type_id);
}
