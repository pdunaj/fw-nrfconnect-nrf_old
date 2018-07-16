#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: BSD-5-Clause-Nordic
#

zephyr_sources(event_manager.c)
zephyr_sources_ifdef(CONFIG_SYSVIEW_LOG_CUSTOM_EVENTS event_logger.c)
