add_library(drivers OBJECT)
target_sources(drivers
    PRIVATE
        src/stm32h5xx_hal_cortex.c
        src/stm32h5xx_hal_icache.c
        src/stm32h5xx_hal_rcc.c
        src/stm32h5xx_hal_rcc_ex.c
        src/stm32h5xx_hal_flash.c
        src/stm32h5xx_hal_flash_ex.c
        src/stm32h5xx_hal_gpio.c
        src/stm32h5xx_hal_pwr.c
        src/stm32h5xx_hal_pwr_ex.c
        src/stm32h5xx_hal.c
        src/stm32h5xx_hal_exti.c
        src/stm32h5xx_hal_rng.c
        src/stm32h5xx_hal_rng_ex.c
        src/stm32h5xx_ll_rng.c
        src/stm32h5xx_hal_uart.c
        src/stm32h5xx_hal_uart_ex.c)

target_include_directories(drivers_if INTERFACE
    inc/
    inc/Legacy)

target_link_libraries(drivers PUBLIC drivers_if cmsis_if drivers_config_if)

target_compile_definitions(drivers_if INTERFACE
    -DSTM32H573xx -DSTM32H5xx -DSTM32 -DUSE_HAL_DRIVER $<$<CONFIG:Debug>:DEBUG>)