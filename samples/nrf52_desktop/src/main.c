#include "event_manager.h"
#include "module_event.h"

#define MODULE		main
#define MODULE_NAME	STRINGIFY(MODULE)

#define SYS_LOG_DOMAIN	MODULE_NAME
#include <logging/sys_log.h>

#if !defined(CONFIG_BOARD_NRF52_DESKTOP_MOUSE)    && \
    !defined(CONFIG_BOARD_NRF52_DESKTOP_KEYBOARD) && \
    !defined(CONFIG_BOARD_NRF52_DESKTOP_DONGLE)
#error "Please select nRF52 Desktop board"
#endif

void main(void)
{
	if (event_manager_init()) {
		SYS_LOG_ERR("Event manager not initialized");
		return;
	}

	module_set_state("ready");
}