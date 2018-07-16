#ifndef _EVENT_LOGGER_H_
#define _EVENT_LOGGER_H_

#include <zephyr.h>
#include <event_manager.h>
#include <systemview/SEGGER_SYSVIEW.h>
#include <rtt/SEGGER_RTT.h>

#ifdef CONFIG_SYSVIEW_LOG_CUSTOM_EVENTS

struct SEGGER_SYSVIEW_MODULE_STRUCT events;

u32_t inline get_event_id(const struct event_header *eh)
{
       #ifdef CONFIG_SRAM_BASE_ADDRESS
                return (u32_t)(eh - CONFIG_SRAM_BASE_ADDRESS);
        #else
                return (u32_t)eh;
        #endif
}

void log_event_exec(const struct event_header *eh);
void log_event_end(const struct event_header *eh);
void SEGGER_SYSVIEW_Conf(void);
#endif


#endif /* _EVENT_LOGGER_H_ */
