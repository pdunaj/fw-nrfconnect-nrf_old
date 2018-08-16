/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <profiler.h>

static char descr[CONFIG_MAX_NUMBER_OF_CUSTOM_EVENTS][CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS];
static char *arg_types_encodings[] = {	"%u",  //u8_t
				      	"%d",  //s8_t
					"%u",  //u16_t
					"%d",  //s16_t
					"%u",  //u32_t
					"%d",  //s32_t
					"%s",  //string
					 "%D"  //time
					};


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

void profiler_term(void)
{
}

u16_t profiler_register_event_type(const char *name, const char **args, const enum profiler_arg *arg_types, u8_t arg_cnt)
{
	u8_t event_number = events.NumEvents;
	u8_t pos = 0;
	pos += snprintf(descr[event_number], CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS, "%d %s", event_number, name);
	__ASSERT_NO_MSG(pos < CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS && pos >0);
	
	for(size_t i = 0; i < arg_cnt; i++) {
		pos += snprintf(descr[event_number] + pos, CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS - pos,
				" %s=%s", args[i], arg_types_encodings[arg_types[i]]);
		__ASSERT_NO_MSG(pos < CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS && pos >0);
	}

	events.NumEvents ++;
	return events.EventOffset + event_number;
}

void profiler_log_start(struct log_event_buf *buf)
{
	/* protocol implementation in SysView demands incrementing pointer by 4 on start */
	buf->pPayload = buf->pPayloadStart + 4; 
}

void profiler_log_encode_u32(struct log_event_buf *buf, u32_t data)
{
	buf->pPayload = SEGGER_SYSVIEW_EncodeU32(buf->pPayload, data);
}

void profiler_log_add_mem_address(struct log_event_buf *buf, const void *event_mem_addres)
{
	buf->pPayload = SEGGER_SYSVIEW_EncodeU32(buf->pPayload, shorten_mem_address(event_mem_addres));
}

void profiler_log_send(struct log_event_buf *buf, u16_t event_type_id)
{
	SEGGER_SYSVIEW_SendPacket(buf->pPayloadStart, buf->pPayload, event_type_id);
}
