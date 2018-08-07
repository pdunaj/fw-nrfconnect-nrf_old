/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */


#include <sysview_profiler_init.h>

#ifndef CONFIG_SMP
extern k_tid_t const _idle_thread;
#endif

static int is_idle_thread(struct k_thread *thread)
{
#ifdef CONFIG_SMP
	return thread->base.is_idle;
#else
	return thread == _idle_thread;
#endif
}

static U64 get_time_cb(void)
{
	return (U64)k_cycle_get_32();
}

static void send_task_list_cb(void)
{
#ifdef CONFIG_SYSVIEW_LOG_KERNEL_EVENTS_THREAD
	struct k_thread *thread;

	for (thread = _kernel.threads; thread; thread = thread->next_thread) {
		char name[20];

		if (is_idle_thread(thread)) {
			continue;
		}

		snprintk(name, sizeof(name), "T%xE%x", (uintptr_t)thread,
			 (uintptr_t)&thread->entry);
		SEGGER_SYSVIEW_SendTaskInfo(&(SEGGER_SYSVIEW_TASKINFO) {
			.TaskID = (u32_t)(uintptr_t)thread,
			.sName = name,
			.StackSize = thread->stack_info.size,
			.StackBase = thread->stack_info.start,
			.Prio = thread->base.prio,
		});
	}
#endif
}

/* Services provided to SYSVIEW by Zephyr */
const SEGGER_SYSVIEW_OS_API SYSVIEW_X_OS_TraceAPI = {
	get_time_cb,
	send_task_list_cb,
};


u32_t sysview_get_timestamp(void)
{
	return k_cycle_get_32();
}


static void _cbSendSystemDesc(void)
{
	SEGGER_SYSVIEW_SendSysDesc("N=ZephyrSysView");
	SEGGER_SYSVIEW_SendSysDesc("D=" CONFIG_BOARD " "
				   CONFIG_SOC_SERIES " " CONFIG_ARCH);
	SEGGER_SYSVIEW_SendSysDesc("O=Zephyr");
}

void SEGGER_SYSVIEW_Conf(void)
{
	SEGGER_SYSVIEW_Init(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
			    CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
			    &SYSVIEW_X_OS_TraceAPI, _cbSendSystemDesc);

#if defined(CONFIG_PHYS_RAM_ADDR)       /* x86 */
	SEGGER_SYSVIEW_SetRAMBase(CONFIG_PHYS_RAM_ADDR);
#elif defined(CONFIG_SRAM_BASE_ADDRESS) /* arm, default */
	SEGGER_SYSVIEW_SetRAMBase(CONFIG_SRAM_BASE_ADDRESS);
#else

#endif
}
