/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <event_manager.h>


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

