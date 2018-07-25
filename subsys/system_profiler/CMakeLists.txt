#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: BSD-5-Clause-Nordic
#

zephyr_sources_ifdef(CONFIG_PROFILER_SYSVIEW sysview_profiler_init.c)
zephyr_sources_ifdef(CONFIG_PROFILER_SYSVIEW system_profiler_sysview.c)
zephyr_sources_ifdef(CONFIG_PROFILER_NORDIC system_profiler_nordic.c)
