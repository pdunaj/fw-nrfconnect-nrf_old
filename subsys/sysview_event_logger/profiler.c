/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <profiler.h>

void event_log_start(struct log_event_buf* b)
{
	b->pPayload = b->pPayloadStart + 4; //protocol demands this
}

void event_log_add_32(u32_t data, struct log_event_buf* b)
{
	b->pPayload = SEGGER_SYSVIEW_EncodeU32(b->pPayload, data);
}

void event_log_add_event_id(const void *eh, struct log_event_buf* b)
{
	b->pPayload = SEGGER_SYSVIEW_EncodeU32(b->pPayload, get_event_id(eh));
}

void event_log_send(u16_t event_type_id, struct log_event_buf* b)
{
	SEGGER_SYSVIEW_SendPacket(b->pPayloadStart, b->pPayload, event_type_id);
}


