#ifndef _SYSVIEW_CUSTOM_EVENT_LOGGER_H_
#define _SYSVIEW_CUSTOM_EVENT_LOGGER_H_

#include <zephyr.h>
#include <systemview/SEGGER_SYSVIEW.h>
#include <rtt/SEGGER_RTT.h>
#include <stdio.h>
#include <string.h>
#define NUMBER_OF_PREDEFINED_EVENTS 2

#ifdef CONFIG_SYSVIEW_LOG_CUSTOM_EVENTS

extern struct SEGGER_SYSVIEW_MODULE_STRUCT events;
struct event_header;
struct event_type;

void log_event_exec(const struct event_header *eh);
void log_event_end(const struct event_header *eh);
void send_event_description(const struct event_type* et, uint16_t event_id);
void SEGGER_SYSVIEW_Conf(void);
#endif


#endif /* _SYSVIEW_CUSTOM_EVENT_LOGGER_H_ */
