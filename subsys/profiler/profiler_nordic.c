/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */
#include <kernel_structs.h>
#include <misc/printk.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <zephyr.h>
#include <rtt/SEGGER_RTT.h>
#include <profiler.h>


enum nordic_command
{
	NORDIC_COMMAND_START = 1,
	NORDIC_COMMAND_STOP = 2,
	NORDIC_COMMAND_INFO = 3
};

static char descr[CONFIG_MAX_NUMBER_OF_CUSTOM_EVENTS][CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS];
static char *arg_types_encodings[] = {	"u8",  //u8_t
					"s8",  //s8_t
					"u16", //u16_t
					"s16", //s16_t
					"u32", //u32_t
					"s32", //s32_t
					"s",   //string
					"t"    //time
					};

static u8_t buffer_data[CONFIG_PROFILER_NORDIC_DATA_BUFFER_SIZE];
static u8_t buffer_info[CONFIG_PROFILER_NORDIC_INFO_BUFFER_SIZE];
static u8_t buffer_commands[CONFIG_PROFILER_NORDIC_COMMAND_BUFFER_SIZE];

static k_tid_t protocol_thread_id;

static u8_t num_events = 0;

volatile bool protocol_running = false;
volatile bool sending_events = CONFIG_PROFILER_NORDIC_START_LOGGING_ON_SYSTEM_START;

K_THREAD_STACK_DEFINE(profiler_nordic_stack, 1024);

static struct k_thread profiler_nordic_stack_data;

static void send_system_description(void)
{
	for (size_t t = 0; t < num_events; t++) {
		SEGGER_RTT_WriteString(CONFIG_PROFILER_NORDIC_RTT_CHANNEL_INFO, descr[t]);
		SEGGER_RTT_PutChar(CONFIG_PROFILER_NORDIC_RTT_CHANNEL_INFO, '\n');
	}
	SEGGER_RTT_PutChar(CONFIG_PROFILER_NORDIC_RTT_CHANNEL_INFO, '\n');
}

static void profiler_nordic_thread(void)
{
	enum nordic_command command;
	while (protocol_running) {
		if (SEGGER_RTT_Read(CONFIG_PROFILER_NORDIC_RTT_CHANNEL_COMMANDS, &command, 1)) {
			switch (command) {
			case NORDIC_COMMAND_START:
				sending_events = true;
				break;			
			case NORDIC_COMMAND_STOP:
				sending_events = false;
				break;
			case NORDIC_COMMAND_INFO:
				send_system_description();
				break;			
			}

		}
		k_sleep(500);
	}
}

int profiler_init(void)
{
	protocol_running = true;
	int ret;
	ret = SEGGER_RTT_ConfigUpBuffer(CONFIG_PROFILER_NORDIC_RTT_CHANNEL_DATA, 
		"Nordic profiler data", buffer_data, CONFIG_PROFILER_NORDIC_DATA_BUFFER_SIZE,
	       	SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
	__ASSERT_NO_MSG(ret>=0);
	ret = SEGGER_RTT_ConfigUpBuffer(CONFIG_PROFILER_NORDIC_RTT_CHANNEL_INFO, 
		"Nordic profiler info", buffer_info, CONFIG_PROFILER_NORDIC_INFO_BUFFER_SIZE,
		SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
	 __ASSERT_NO_MSG(ret>=0);  
	ret = SEGGER_RTT_ConfigDownBuffer(CONFIG_PROFILER_NORDIC_RTT_CHANNEL_COMMANDS,
		"Nordic profiler command", buffer_commands, CONFIG_PROFILER_NORDIC_COMMAND_BUFFER_SIZE,
	       	SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
	 __ASSERT_NO_MSG(ret>=0);
	protocol_thread_id =  k_thread_create(&profiler_nordic_stack_data, profiler_nordic_stack,
			K_THREAD_STACK_SIZEOF(profiler_nordic_stack),
			(k_thread_entry_t) profiler_nordic_thread,
			NULL, NULL, NULL, K_PRIO_COOP(1), 0, 0);
	return 0;
}

void profiler_term(void)
{
	sending_events = false;
	protocol_running = false;
	k_wakeup(protocol_thread_id);
	k_yield();
}

u16_t profiler_register_event_type(const char *name, const char **args, const enum profiler_arg *arg_types, u8_t arg_cnt)
{
	u8_t pos = 0;
	pos += snprintf(descr[num_events], CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS, "%s,%d", name, num_events);

	u8_t t;
	for(t = 0; t < arg_cnt; t++) {
		pos += snprintf(descr[num_events] + pos, CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS - pos, ",%s", arg_types_encodings[arg_types[t]]);
	}
	
	for(t = 0; t < arg_cnt; t++) {
		pos += snprintf(descr[num_events] + pos, CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS - pos, ",%s", args[t]);
	}
	__ASSERT_NO_MSG(pos < CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS);
	__DMB();	
	num_events++;
	return num_events - 1;
}

void profiler_log_start(struct log_event_buf *buf)
{
	buf->pPayload = buf->pPayloadStart;
	profiler_log_encode_u32(buf, k_cycle_get_32());
}

void profiler_log_encode_u32(struct log_event_buf *buf, u32_t data)
{
	sys_put_le32(data, buf->pPayload);
	buf->pPayload += 4;
}

void profiler_log_add_mem_address(struct log_event_buf *buf, const void *mem_address)
{
	profiler_log_encode_u32(buf, (u32_t)mem_address);
}

void profiler_log_send(struct log_event_buf *buf, u16_t event_type_id)
{
	__ASSERT_NO_MSG(event_type_id < 256);
	if (sending_events) {
		u8_t type_id = event_type_id & 255;
		SEGGER_RTT_Write(CONFIG_PROFILER_NORDIC_RTT_CHANNEL_DATA, &type_id, 1);
		SEGGER_RTT_Write(CONFIG_PROFILER_NORDIC_RTT_CHANNEL_DATA, buf->pPayloadStart, buf->pPayload - buf->pPayloadStart);
	}
}