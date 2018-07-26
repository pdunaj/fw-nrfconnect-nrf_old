/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */
#include <kernel_structs.h>
#include <misc/printk.h>
#include <misc/util.h>
#include <zephyr.h>
#include <rtt/SEGGER_RTT.h>
#include <system_profiler.h>
#define RTT_CHANNEL_DATA 1
#define RTT_CHANNEL_INFO 2

static char descr[CONFIG_MAX_NUMBER_OF_CUSTOM_EVENTS][128];
static char * arg_types_encodings[] = {"u8", "s8", "u16", "s16", "u32", "s32", "s" "t" };

u8_t *buffer[128];

/* First 10 IDs are for predefined events */
static u8_t num_events = 1; 

bool sending_events = true;

K_THREAD_STACK_DEFINE(profiler_nordic_stack, 1024);

static struct k_thread profiler_nordic_stack_data;

static void send_system_description()
{
	u8_t t;
	for (t = 1; t < num_events; t++)
	{
		SEGGER_RTT_WriteString(RTT_CHANNEL_INFO, descr[t]);
		SEGGER_RTT_PutChar(RTT_CHANNEL_INFO, '\n');
	}
	SEGGER_RTT_PutChar(RTT_CHANNEL_INFO, '\n');
}

static void profiler_nordic_thread()
{
	k_sleep(5000);
	send_system_description();
}

int profiler_init()
{
	SEGGER_RTT_ConfigUpBuffer(1, "Nordic profiler data", buffer, 512, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
	SEGGER_RTT_ConfigUpBuffer(2, "Nordic profiler info", buffer, 512, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);	
	k_thread_create(&profiler_nordic_stack_data, profiler_nordic_stack,
			K_THREAD_STACK_SIZEOF(profiler_nordic_stack),
			(k_thread_entry_t) profiler_nordic_thread,
			NULL, NULL, NULL, K_PRIO_COOP(1), 0, 0);
	return 0;
}


u16_t profiler_register_event_type(const char *name, const char **args, const enum data_type* arg_types, const u8_t arg_cnt)
{
	u8_t pos = 0;
	pos += sprintf(descr[num_events], "%s,%d", name, num_events);

	if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_TRACE_EVENT_EXECUTION))
	{
		pos += sprintf(descr[num_events] + pos, ",%s", "u32");
	}	

	u8_t t;
	for(t = 0; t < arg_cnt; t++)
	{
		pos += sprintf(descr[num_events] + pos, ",%s", arg_types_encodings[arg_types[t]]);
	}

	if (IS_ENABLED(CONFIG_DESKTOP_EVENT_MANAGER_TRACE_EVENT_EXECUTION))
	{
		pos += sprintf(descr[num_events] + pos, ",%s", "mem_address");
	}
	
	for(t = 0; t < arg_cnt; t++)
	{
		pos += sprintf(descr[num_events] + pos, ",%s", args[t]);
	}
	descr[num_events][pos] = '\0';
	num_events++;
	return num_events - 1;
}

void event_log_start(struct log_event_buf* buf)
{
	buf->pPayload = buf->pPayloadStart;
	event_log_add_32(k_cycle_get_32(), buf);
}

void event_log_add_8(u8_t data, struct log_event_buf* buf)
{
	*(buf->pPayload) = data;
	buf->pPayload++;
}

void event_log_add_16(u16_t data, struct log_event_buf* buf)
{
	*(buf->pPayload) = (data>>8) & 255;
	buf->pPayload++;
	*(buf->pPayload) = data & 255;
	buf->pPayload++;
}

void event_log_add_32(u32_t data, struct log_event_buf* buf)
{
	u8_t i;
	for (i = 0; i < 4; i++){
		*(buf->pPayload) = ((data>>(8*i)) & 255);
		buf->pPayload++;	
	}	
}

void event_log_add_mem_address(const void *mem_address, struct log_event_buf* buf)
{
	event_log_add_32((u32_t)mem_address, buf);
}

void event_log_send(u16_t event_type_id, struct log_event_buf* buf)
{
	if (sending_events)
	{
		u8_t type_id = event_type_id & 255;
		SEGGER_RTT_Write(RTT_CHANNEL_DATA, &type_id, 1);
		SEGGER_RTT_Write(RTT_CHANNEL_DATA, buf->pPayloadStart, buf->pPayload - buf->pPayloadStart);
	}
}
