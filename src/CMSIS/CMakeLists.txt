add_library(cmsis OBJECT)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    # Runs from FLASH (default)
    set(LINKER_SCRIPT_FLASH_NAME STM32H573xx_FLASH.ld)
    set(LINKER_SCRIPT_FLASH
        ${CMAKE_CURRENT_SOURCE_DIR}/startup_gnu/${LINKER_SCRIPT_FLASH_NAME}
        CACHE INTERNAL "STM32 linker script for running from FLASH")
    # Runs from RAM (debugging)
    set(LINKER_SCRIPT_RAM_NAME STM32H573xx_RAM.ld)
    set(LINKER_SCRIPT_RAM
        ${CMAKE_CURRENT_SOURCE_DIR}/startup_gnu/${LINKER_SCRIPT_RAM_NAME}
        CACHE INTERNAL "STM32 linker script for running from RAM")
    # Default linker script (FLASH)
    set(LINKER_SCRIPT ${LINKER_SCRIPT_FLASH}
        CACHE INTERNAL "STM32 linker script")

    # Startup script
    target_sources(cmsis PRIVATE startup_gnu/startup_stm32h573xx.s)
else()
    message(ERROR "Unknown compiler")
endif()

target_include_directories(cmsis_if INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/device)

target_link_options(cmsis PUBLIC "-T${LINKER_SCRIPT}")

