#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: BSD-5-Clause-Nordic
#

zephyr_sources_ifdef(CONFIG_SYSVIEW_LOG_CUSTOM_EVENTS custom_event_logger.c)
zephyr_sources_ifdef(CONFIG_SYSVIEW_LOG_KERNEL_EVENTS kernel_event_logger.c)
