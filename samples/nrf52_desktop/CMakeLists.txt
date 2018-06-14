set(KCONFIG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig)
set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

FILE(GLOB main     src/*.c)
FILE(GLOB events   src/events/*.c)
FILE(GLOB drivers  src/drivers/*.c)
FILE(GLOB modules  src/modules/*.c)
FILE(GLOB services src/services/*.c)

target_sources(app PRIVATE ${main} ${events} ${drivers} ${modules} ${services})

zephyr_library_include_directories(src/drivers)
zephyr_library_include_directories(src/events)
zephyr_library_include_directories(src/modules)
zephyr_library_include_directories(src/services)
