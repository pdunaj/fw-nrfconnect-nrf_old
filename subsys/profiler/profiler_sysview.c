/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <profiler.h>
#include <kernel_structs.h>

static char descr[CONFIG_MAX_NUMBER_OF_CUSTOM_EVENTS][CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS];
static char *arg_types_encodings[] = {	"%u",  /* u8_t */
				      	"%d",  /* s8_t */
					"%u",  /* u16_t */
					"%d",  /* s16_t */
					"%u",  /* u32_t */
					"%d",  /* s32_t */
					"%s",  /* string */
					 "%D"  /* time */
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
	/* Memory barrier to make sure that data is visible before being accessed */
	u32_t ne = events.NumEvents;
	__DMB();

	for (size_t i = 0; i < ne; i++) {
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
	/* Lock to make sure that this function can be called from multiple threads */
	k_sched_lock();
	u32_t ne = events.NumEvents;

	size_t temp = snprintf(descr[ne], CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS, "%u %s", ne, name);
	size_t pos = temp;
	__ASSERT_NO_MSG((pos < CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS) && (temp > 0));
	
	for (size_t i = 0; i < arg_cnt; i++) {
		temp = snprintf(descr[ne] + pos, CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS - pos,
				" %s=%s", args[i], arg_types_encodings[arg_types[i]]);
		pos += temp;
		__ASSERT_NO_MSG((pos < CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS) && (temp > 0));
	}
	
	/* Memory barrier to make sure that data is visible before being accessed */
	__DMB();	
	events.NumEvents++;
	k_sched_unlock();
	return events.EventOffset + ne;
}

void profiler_log_start(struct log_event_buf *buf)
{
	/* protocol implementation in SysView demands incrementing pointer by sizeof(u32_t) on start */
	buf->payload = buf->payload_start + sizeof(u32_t); 
}

void profiler_log_encode_u32(struct log_event_buf *buf, u32_t data)
{
	buf->payload = SEGGER_SYSVIEW_EncodeU32(buf->payload, data);
}

void profiler_log_add_mem_address(struct log_event_buf *buf, const void *event_mem_addres)
{
	buf->payload = SEGGER_SYSVIEW_EncodeU32(buf->payload, shorten_mem_address(event_mem_addres));
}

void profiler_log_send(struct log_event_buf *buf, u16_t event_type_id)
{
	SEGGER_SYSVIEW_SendPacket(buf->payload_start, buf->payload, event_type_id);
}