add_library(cmsis_if INTERFACE)
add_library(drivers_if INTERFACE)
add_library(drivers_config_if INTERFACE)
add_library(system_if INTERFACE)

add_subdirectory(CMSIS)
add_subdirectory(drivers)
add_subdirectory(drivers_config)
add_subdirectory(system)

target_link_libraries(drivers PUBLIC cmsis_if drivers_config_if)
