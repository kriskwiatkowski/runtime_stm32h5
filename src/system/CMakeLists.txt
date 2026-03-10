add_library(system OBJECT)
target_sources(system
    PRIVATE
        stm32h5xx_hal_msp.c
        stm32h5xx_it.c
        system_stm32h5xx.c
        syscalls.c
        sysmem.c)

target_link_libraries(system PUBLIC cmsis drivers drivers_config_if)
