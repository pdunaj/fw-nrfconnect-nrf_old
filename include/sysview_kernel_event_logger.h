/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */


#ifndef _SYSVIEW_KERNEL_EVENT_LOGGER_H_
#define _SYSVIEW_KERNEL_EVENT_LOGGER_H_

#include <logging/kernel_event_logger.h>
#include <misc/printk.h>
#include <misc/util.h>
#include <zephyr.h>

#include <systemview/SEGGER_SYSVIEW.h>
#include <rtt/SEGGER_RTT.h>

void kernel_event_logger_init();

#endif /* _SYSVIEW_KERNEL_EVENT_LOGGER_H_ */
