#include <event_logger.h>
#include <event_manager.h>


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

/* Services provided to SYSVIEW by Zephyr */
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
                .NumEvents = 0,
                .EventOffset = 0,
                .pfSendModuleDesc = event_module_description,
                .pNext = NULL
        };

static void event_module_description(void) {
	SEGGER_SYSVIEW_RecordModuleDescription(&events, "0 event_execution event_id=%u");
        SEGGER_SYSVIEW_RecordModuleDescription(&events, "1 event_end event_id=%u");	
	for (const struct event_type *et = __start_event_types;
	    (et != NULL) && (et != __stop_event_types);
	    et++) 
	{
		if (et->description)
		{
			send_event_description(et, NUMBER_OF_PREDEFINED_EVENTS + et - __start_event_types);	
		}
	}
}

void log_event_exec(const struct event_header *eh)
{
        SEGGER_SYSVIEW_RecordU32(events.EventOffset, get_event_id(eh));
}

void log_event_end(const struct event_header *eh)
{
        SEGGER_SYSVIEW_RecordU32(events.EventOffset+1, get_event_id(eh));
}

void send_event_description(const struct event_type* et, uint16_t event_id)
{
	uint8_t description_length = strlen(et->description);
	if (NULL == et->description)
	{
		return;
	}
	char res[description_length+6];
	sprintf(res, "%u", event_id);
	uint8_t i = 0;
	while(res[i] != '\0')
	{
		i++;
	}
	res[i++] = ' '; 
	memcpy(res + i, et->description, description_length);
	res[i+description_length] = '\0';
	SEGGER_SYSVIEW_RecordModuleDescription(&events, res);
}

