add_library(cmsis OBJECT)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(LINKER_SCRIPT_NAME STM32H573xx_FLASH.ld)
    target_sources(cmsis PRIVATE
        startup_gnu/startup_stm32h573xx.s)
else()
    message(ERROR "Unknown compiler")
endif()

set(LINKER_SCRIPT
    ${CMAKE_CURRENT_SOURCE_DIR}/startup_gnu/${LINKER_SCRIPT_NAME}
    CACHE INTERNAL "STM32 linker script")
target_link_options(cmsis PRIVATE -T${LINKER_SCRIPT})

target_include_directories(cmsis_if INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/device)

install(
    FILES ${LINKER_SCRIPT}
    DESTINATION ${CMAKE_INSTALL_PREFIX}
    RENAME ${LINKER_SCRIPT_NAME})
