/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */



#include <sysview_kernel_event_logger.h>

K_THREAD_STACK_DEFINE(sysview_stack, 1024);

static struct k_thread sysview_thread_data;
static uint32_t interrupt;


uint32_t sysview_get_interrupt()
{
	return interrupt;
}


static void publish_context_switch(u32_t *event_data)
{
#ifdef CONFIG_SYSVIEW_LOG_KERNEL_EVENTS_CONTEXT_SWITCH
	SEGGER_SYSVIEW_OnTaskStartExec(event_data[1]);
#endif
}

static void publish_interrupt(u32_t *event_data)
{
#ifdef CONFIG_SYSVIEW_LOG_KERNEL_EVENTS_INTERRUPT
	interrupt = event_data[1];

	/* FIXME: RecordEnter and RecordExit seem to be required to keep
	 * SystemView happy; however, there's currently no way to measure
	 * the time it takes for an ISR to execute.
	 */
	SEGGER_SYSVIEW_RecordEnterISR();
	SEGGER_SYSVIEW_RecordExitISR();
#endif
}

static void publish_sleep(u32_t *event_data)
{
#ifdef CONFIG_SYSVIEW_LOG_KERNEL_EVENTS_SLEEP
	SEGGER_SYSVIEW_OnIdle();
#endif
}

static void publish_task(u32_t *event_data)
{
#ifdef CONFIG_SYSVIEW_LOG_KERNEL_EVENTS_THREAD
	u32_t thread_id;

	thread_id = event_data[1];

	switch ((enum sys_k_event_logger_thread_event)event_data[2]) {
	case KERNEL_LOG_THREAD_EVENT_READYQ:
		SEGGER_SYSVIEW_OnTaskStartReady(thread_id);
		break;
	case KERNEL_LOG_THREAD_EVENT_PEND:
		SEGGER_SYSVIEW_OnTaskStopReady(thread_id, 3<<3);
		break;
	case KERNEL_LOG_THREAD_EVENT_EXIT:
		SEGGER_SYSVIEW_OnTaskTerminate(thread_id);
		break;
	}
	/* FIXME: Maybe we need a KERNEL_LOG_THREAD_EVENT_CREATE? */
#endif
}


static void sysview_thread(void)
{

	while (1)
	{
		u32_t event_data[4];
		u16_t event_id;
		u8_t dropped;
		u8_t event_data_size = (u8_t)ARRAY_SIZE(event_data);
		int ret;

		ret = sys_k_event_logger_get_wait(&event_id,
						  &dropped,
						  event_data,
						  &event_data_size);
		if (ret < 0) {
			continue;
		}

		switch (event_id) {
		case KERNEL_EVENT_LOGGER_CONTEXT_SWITCH_EVENT_ID:
			publish_context_switch(event_data);
			break;
		case KERNEL_EVENT_LOGGER_INTERRUPT_EVENT_ID:
			publish_interrupt(event_data);
			break;
		case KERNEL_EVENT_LOGGER_SLEEP_EVENT_ID:
			publish_sleep(event_data);
			break;
		case KERNEL_EVENT_LOGGER_THREAD_EVENT_ID:
			publish_task(event_data);
			break;
		}
	}
}

void kernel_event_logger_init(void)
{
	k_thread_create(&sysview_thread_data, sysview_stack,
			K_THREAD_STACK_SIZEOF(sysview_stack),
			(k_thread_entry_t)sysview_thread,
			NULL, NULL, NULL, K_PRIO_COOP(1), 0, 0);
}
