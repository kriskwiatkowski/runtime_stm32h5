cmake_minimum_required(VERSION 3.10)
include(FetchContent)

project(stm32h5_runtime LANGUAGES C ASM)
set(MAIN_TARGET stm32h5_runtime)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_STANDARD 99)
set(OUTPUT ${CMAKE_BINARY_DIR}/pack)
set(CMAKE_INSTALL_BINDIR ${OUTPUT}/bin)
set(CMAKE_INSTALL_LIBDIR ${OUTPUT}/lib)
set(CMAKE_INSTALL_SRCDIR ${OUTPUT}/src)
set(CMAKE_INSTALL_INCLUDEDIR ${OUTPUT}/include/platform)
set(LINKER_SCRIPT ${PROJECT_SOURCE_DIR}/src/stm32h573.ld)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")

include(postbuild)

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    string(APPEND CMAKE_C_FLAGS " -O0")
endif()

add_library(
    ${MAIN_TARGET} STATIC
    src/stm32hal/Src/stm32h5xx_hal.c
    src/stm32hal/Src/stm32h5xx_hal_cortex.c
    src/stm32hal/Src/stm32h5xx_hal_crc.c
    src/stm32hal/Src/stm32h5xx_hal_crc_ex.c
    src/stm32hal/Src/stm32h5xx_hal_dma.c
    src/stm32hal/Src/stm32h5xx_hal_dma_ex.c
    src/stm32hal/Src/stm32h5xx_hal_exti.c
    src/stm32hal/Src/stm32h5xx_hal_flash.c
    src/stm32hal/Src/stm32h5xx_hal_flash_ex.c
    src/stm32hal/Src/stm32h5xx_hal_gpio.c
    src/stm32hal/Src/stm32h5xx_hal_icache.c
    src/stm32hal/Src/stm32h5xx_hal_pwr.c
    src/stm32hal/Src/stm32h5xx_hal_pwr_ex.c
    src/stm32hal/Src/stm32h5xx_hal_rcc.c
    src/stm32hal/Src/stm32h5xx_hal_rcc_ex.c
    src/stm32hal/Src/stm32h5xx_hal_rtc.c
    src/stm32hal/Src/stm32h5xx_hal_rtc_ex.c
    src/stm32hal/Src/stm32h5xx_hal_uart.c
    src/stm32hal/Src/stm32h5xx_hal_uart_ex.c    
    src/main.c
    src/printf.c
    src/startup.s
    src/stm32h5xx_hal_msp.c
    src/stm32h5xx_it.c
    src/syscalls.c
    src/sysmem.c
    src/system_stm32h5xx_ns.c
    src/trace.c
    src/detection.c)

target_sources(
    ${MAIN_TARGET}
    PUBLIC
    FILE_SET HEADERS
    FILES
        ${CMAKE_SOURCE_DIR}/inc/platform/printf.h
        ${CMAKE_SOURCE_DIR}/inc/platform/platform.h
    BASE_DIRS
        ${CMAKE_SOURCE_DIR}/inc/platform)

add_compile_definitions(STM32H573xx)
        
target_compile_definitions(
    ${MAIN_TARGET} PRIVATE
    ${EXTRA_C_FLAGS})

target_include_directories(
    ${MAIN_TARGET}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/CMSIS/Device
        ${CMAKE_SOURCE_DIR}/src/CMSIS/Include
        ${CMAKE_SOURCE_DIR}/src/stm32hal/Inc
        ${CMAKE_SOURCE_DIR}/src/stm32hal/Inc/Legacy
        ${CMAKE_SOURCE_DIR}/app
    PUBLIC
        ${CMAKE_SOURCE_DIR}/inc)

add_executable(
    hello
    app/secure_manager_api.c
    src/syscalls.c
    src/sysmem.c)
target_include_directories(
        hello PRIVATE ${CMAKE_SOURCE_DIR}/app/)
target_link_libraries(
    hello
    ${MAIN_TARGET})
    
target_link_options(
    hello PRIVATE
    "-T${LINKER_SCRIPT}")

target_postbuild_executable(
    hello 
    ${CMAKE_INSTALL_BINDIR})

install(
    TARGETS ${MAIN_TARGET}
    ARCHIVE
    PUBLIC_HEADER
    FILE_SET HEADERS)
