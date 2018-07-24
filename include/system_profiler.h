/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#ifndef _SYSTEM_PROFILER_H_
#define _SYSTEM_PROFILER_H_
#include <zephyr.h>
#include <stdio.h>
#include <systemview/SEGGER_SYSVIEW.h>
#include <rtt/SEGGER_RTT.h>

/** @brief Data types for logging in system profiler.
 */
enum data_type {
	u32,
	s32,
	timestamp
};

/** @brief Buffer for event's data.
 *
 * Buffer required for data, which is send with event. 
 *
 */
struct log_event_buf
{
	/* Pointer to end of payload */
	u8_t* pPayload;
	/* Array where payload is located before it is send */
	u8_t pPayloadStart[CONFIG_PROFILER_CUSTOM_EVENT_BUF_LEN];
};

#ifdef CONFIG_PROFILER
/** @brief Funciton to initialize system profiler module.
 *
 * @return Zero if successful
 */
int profiler_init();

/** @brief Funciton to register type of event in system profiler.
 * 
 * @param name Name of event type.
 * @param args Names of data values send with event.
 * @param arg_types Types of data values send with event.
 * @param arg_cnt Number of data values send with event.
 * 
 * @return ID given to event type in system profiler.
 */
u16_t profiler_register_event_type(const char *name, const char **args, const enum data_type* arg_types, const u8_t arg_cnt);


/* @brief Function to initialize buffer for events' data.
 * 
 * @param buf Pointer to data buffer.
*/
void event_log_start(struct log_event_buf* buf);

/* @brief Function to encode and add data to buffer.
 * 
 * @warning Buffer has to be initialized with event_log_start function first.
 * @param data Data to add to buffer.
 * @param buf Pointer to data buffer.
*/
void event_log_add_32(u32_t data, struct log_event_buf* buf);

/* @brief Function to encode and add event's address in device's memory to buffer.
 * 
 * Used for event identification
 * @warning Buffer has to be initialized with event_log_start function first.
 *
 * @param data Memory address to encode.
 * @param buf Pointer to data buffer.
*/
void event_log_add_mem_address(const void *mem_address, struct log_event_buf* buf);

/* @brief Function to send data added to buffer to host.
 *
 * @note This funciton only sends data which is already stored in buffer. Data is added to buffer using event_log_add_32
 * and event_log_add_mem_address functions.
 * @param event_type_id ID of event in system profiler. It is given to event type while it is registered.
 * @param buf Pointer to data buffer.
*/
void event_log_send(u16_t event_type_id, struct log_event_buf* buf);

#else
#define event_log_start(b)
#define event_log_add_32(data, b)
#define event_log_add_mem_address(mem_address, b)
#define event_log_send(event_type_id, b)


#endif
#endif /* _SYSTEM_PROFILER_H_ */
