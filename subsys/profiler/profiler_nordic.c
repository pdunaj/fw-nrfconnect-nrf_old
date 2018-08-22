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


static K_SEM_DEFINE(profiler_sem, 0, 1);

static K_SEM_DEFINE(protocol_running_sem, 0, 1);
static K_SEM_DEFINE(sending_events_sem, 0, 1);

enum nordic_command
{
	NORDIC_COMMAND_START	= 1,
	NORDIC_COMMAND_STOP	= 2,
	NORDIC_COMMAND_INFO	= 3
};

static char descr[CONFIG_MAX_NUMBER_OF_CUSTOM_EVENTS][CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS];
static char *arg_types_encodings[] = {	"u8",  /* u8_t */
					"s8",  /* s8_t */
					"u16", /* u16_t */
					"s16", /* s16_t */
					"u32", /* u32_t */
					"s32", /* s32_t */
					"s",   /* string */
					"t"    /* time */
					};

static u8_t num_events;

static u8_t buffer_data[CONFIG_PROFILER_NORDIC_DATA_BUFFER_SIZE];
static u8_t buffer_info[CONFIG_PROFILER_NORDIC_INFO_BUFFER_SIZE];
static u8_t buffer_commands[CONFIG_PROFILER_NORDIC_COMMAND_BUFFER_SIZE];

static k_tid_t protocol_thread_id;

static K_THREAD_STACK_DEFINE(profiler_nordic_stack, 128);
static struct k_thread profiler_nordic_thread;

static void send_system_description(void)
{
	u16_t num_bytes_send;
	for (size_t t = 0; t < num_events; t++) {
		num_bytes_send = SEGGER_RTT_WriteString(CONFIG_PROFILER_NORDIC_RTT_CHANNEL_INFO, descr[t]);
		__ASSERT_NO_MSG(num_bytes_send > 0);
		num_bytes_send = SEGGER_RTT_PutChar(CONFIG_PROFILER_NORDIC_RTT_CHANNEL_INFO, '\n');
		__ASSERT_NO_MSG(num_bytes_send > 0);
	}
	num_bytes_send = SEGGER_RTT_PutChar(CONFIG_PROFILER_NORDIC_RTT_CHANNEL_INFO, '\n');
	__ASSERT_NO_MSG(num_bytes_send > 0);
}

static void profiler_nordic_thread_fn(void)
{
	while (k_sem_count_get(&protocol_running_sem)) {
		u8_t read_data;
		enum nordic_command command;
		if (SEGGER_RTT_Read(CONFIG_PROFILER_NORDIC_RTT_CHANNEL_COMMANDS, &read_data, sizeof(read_data))) {
			command = (enum nordic_command)read_data;
			int ret;			
			switch (command) {
			case NORDIC_COMMAND_START:
				if (!k_sem_count_get(&sending_events_sem)) {
					k_sem_give(&sending_events_sem);
				}
				break;			
			case NORDIC_COMMAND_STOP:
				if (k_sem_count_get(&sending_events_sem)) {
					ret = k_sem_take(&sending_events_sem, K_NO_WAIT);
					__ASSERT_NO_MSG(ret == 0);				
				}				
				break;
			case NORDIC_COMMAND_INFO:
				send_system_description();
				break;			
			}

		}
		k_sleep(500);
	}
	k_sem_give(&profiler_sem);
}

int profiler_init(void)
{

	k_sem_give(&protocol_running_sem);
	if (IS_ENABLED(CONFIG_PROFILER_NORDIC_START_LOGGING_ON_SYSTEM_START)) {
		k_sem_give(&sending_events_sem);
	}

	int ret;

	ret = SEGGER_RTT_ConfigUpBuffer(CONFIG_PROFILER_NORDIC_RTT_CHANNEL_DATA, 
		"Nordic profiler data", buffer_data, CONFIG_PROFILER_NORDIC_DATA_BUFFER_SIZE,
	       	SEGGER_RTT_MODE_NO_BLOCK_SKIP);
	__ASSERT_NO_MSG(ret >= 0);

	ret = SEGGER_RTT_ConfigUpBuffer(CONFIG_PROFILER_NORDIC_RTT_CHANNEL_INFO, 
		"Nordic profiler info", buffer_info, CONFIG_PROFILER_NORDIC_INFO_BUFFER_SIZE,
		SEGGER_RTT_MODE_NO_BLOCK_SKIP);
	 __ASSERT_NO_MSG(ret >= 0);  

	ret = SEGGER_RTT_ConfigDownBuffer(CONFIG_PROFILER_NORDIC_RTT_CHANNEL_COMMANDS,
		"Nordic profiler command", buffer_commands, CONFIG_PROFILER_NORDIC_COMMAND_BUFFER_SIZE,
	       	SEGGER_RTT_MODE_NO_BLOCK_SKIP);
	 __ASSERT_NO_MSG(ret >= 0);

	protocol_thread_id =  k_thread_create(&profiler_nordic_thread, profiler_nordic_stack,
			K_THREAD_STACK_SIZEOF(profiler_nordic_stack),
			(k_thread_entry_t) profiler_nordic_thread_fn,
			NULL, NULL, NULL, K_PRIO_COOP(1), 0, 0);
	return 0;
}

void profiler_term(void)
{
	k_sem_reset(&sending_events_sem);
	k_sem_reset(&protocol_running_sem);
	k_wakeup(protocol_thread_id);
	k_sem_take(&profiler_sem, K_FOREVER);
}

u16_t profiler_register_event_type(const char *name, const char **args, const enum profiler_arg *arg_types, u8_t arg_cnt)
{
	u8_t temp, pos = 0;
	temp = snprintf(descr[num_events], CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS, "%s,%d", name, num_events);
	pos += temp;
	__ASSERT_NO_MSG(pos < CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS && temp > 0);
	
	u8_t t;
	for (t = 0; t < arg_cnt; t++) {
		temp = snprintf(descr[num_events] + pos, CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS - pos,
				 ",%s", arg_types_encodings[arg_types[t]]);
		pos += temp;
		__ASSERT_NO_MSG(pos < CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS && temp > 0);
	}

	for (t = 0; t < arg_cnt; t++) {
		temp = snprintf(descr[num_events] + pos,
				 CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS - pos, ",%s", args[t]);
		pos += temp;
		__ASSERT_NO_MSG(pos < CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS && temp > 0);	
	}
	
	__DMB();	
	num_events++;
	return num_events - 1;
}

void profiler_log_start(struct log_event_buf *buf)
{
	/* Adding one to pointer to make space for event type ID */
	buf->payload = buf->payload_start + 1;
	profiler_log_encode_u32(buf, k_cycle_get_32());
}

void profiler_log_encode_u32(struct log_event_buf *buf, u32_t data)
{
	sys_put_le32(data, buf->payload);
	buf->payload += sizeof(data);
}

void profiler_log_add_mem_address(struct log_event_buf *buf, const void *mem_address)
{
	profiler_log_encode_u32(buf, (u32_t)mem_address);
}

void profiler_log_send(struct log_event_buf *buf, u16_t event_type_id)
{
	__ASSERT_NO_MSG(event_type_id <= UCHAR_MAX);
	if (k_sem_count_get(&sending_events_sem)) {
		u8_t type_id = event_type_id & UCHAR_MAX;
		buf->payload_start[0] = type_id;
		u8_t num_bytes_send = SEGGER_RTT_Write(CONFIG_PROFILER_NORDIC_RTT_CHANNEL_DATA, 
				buf->payload_start, buf->payload - buf->payload_start);
		__ASSERT_NO_MSG(num_bytes_send > 0);
	}
}
