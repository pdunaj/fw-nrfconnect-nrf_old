/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <profiler.h>


static U8 buffer[128];
static U8* pPayload;
static U8* pPayloadStart = buffer;



void event_log_start()
{
	pPayload = buffer + 4;
}

void event_log_add_u32(u32_t data)
{
	pPayload = SEGGER_SYSVIEW_EncodeU32(pPayload, data);
}

void event_log_add_s32(s32_t data)
{
	pPayload = SEGGER_SYSVIEW_EncodeU32(pPayload, data);
}


void event_log_add_event_id(const void *eh)
{
	pPayload = SEGGER_SYSVIEW_EncodeU32(pPayload, get_event_id(eh));
}


void event_log_send(u16_t event_type_id)
{
	SEGGER_SYSVIEW_SendPacket(pPayloadStart, pPayload, event_type_id);
}


