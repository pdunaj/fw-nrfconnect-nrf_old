#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: BSD-5-Clause-Nordic
#

zephyr_sources_ifdef(CONFIG_SYSVIEW_LOG_CUSTOM_EVENTS sysview_custom_event_logger.c)
zephyr_sources_ifdef(CONFIG_SYSVIEW_LOG_KERNEL_EVENTS sysview_kernel_event_logger.c)
zephyr_sources_ifdef(CONFIG_SYSTEM_PROFILER_SYSVIEW sysview_event_logger_init.c)
zephyr_sources_ifdef(CONFIG_SYSTEM_PROFILER_SYSVIEW system_profiler_sysview.c)
