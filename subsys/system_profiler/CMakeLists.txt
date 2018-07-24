#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: BSD-5-Clause-Nordic
#

zephyr_sources_ifdef((CONFIG_PROFILER_SYSVIEW && CONFIG_PROFILER_KERNEL_EVENTS) sysview_kernel_event_logger.c)
zephyr_sources_ifdef(CONFIG_PROFILER_SYSVIEW sysview_profiler_init.c)
zephyr_sources_ifdef(CONFIG_PROFILER_SYSVIEW system_profiler_sysview.c)
