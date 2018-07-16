#include <event_logger.h>
#include <event_manager.h>

/* Services provided to SYSVIEW by Zephyr */
extern const SEGGER_SYSVIEW_OS_API SYSVIEW_X_OS_TraceAPI;

static void send_task_list_cb(void)
{
//For now commented out - it is dedicated to work with new API
/*
//#ifdef CONFIG_SYSTEMVIEW_TASK_MONITOR
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
//#endif
*/
}


static U64 get_time_cb(void)
{
	return (U64)k_cycle_get_32();
}


const SEGGER_SYSVIEW_OS_API SYSVIEW_X_OS_TraceAPI = {
	get_time_cb,
	send_task_list_cb,
};


u32_t sysview_get_timestamp(void)
{
	return k_cycle_get_32();
}

//#ifndef CONFIG_TRACING
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
	/* Setting RAMBase is just an optimization: this value is subtracted
	 * from all pointers in order to save bandwidth.  It's not an error
	 * if a platform does not set this value.
	 */
#endif
}
//#endif

static void event_module_description(void);
struct SEGGER_SYSVIEW_MODULE_STRUCT events = {
                .sModule = "M=EventManager",
                .NumEvents = 7,
                .EventOffset = 0,
                .pfSendModuleDesc = event_module_description,
                .pNext = NULL
        };

static void event_module_description(void) {
        SEGGER_SYSVIEW_RecordModuleDescription(&events, "0 event_execution event_id=%u");
        SEGGER_SYSVIEW_RecordModuleDescription(&events, "1 event_end event_id=%u");
        SEGGER_SYSVIEW_RecordModuleDescription(&events, "2 button_event event_id=%u button_id=%u status=%u");
        SEGGER_SYSVIEW_RecordModuleDescription(&events, "3 motion_event event_id=%u dx=%d dy=%d");
        SEGGER_SYSVIEW_RecordModuleDescription(&events, "4 battery_event event_id=%u level=%u");
        SEGGER_SYSVIEW_RecordModuleDescription(&events, "5 hid_axis_event event_id=%u x=%d u=%d");
        SEGGER_SYSVIEW_RecordModuleDescription(&events, "6 keep_active_event event_id=%u");
}

void log_event_exec(const struct event_header *eh)
{
        SEGGER_SYSVIEW_RecordU32(events.EventOffset, get_event_id(eh));
}

void log_event_end(const struct event_header *eh)
{
        SEGGER_SYSVIEW_RecordU32(events.EventOffset+1, get_event_id(eh));
}

