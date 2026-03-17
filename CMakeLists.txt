cmake_minimum_required(VERSION 3.22)
set(RUNNER_TARGET rtm_stm32h5)
project(
    ${RUNNER_TARGET}_runtime
    VERSION 0.0.3
    LANGUAGES C ASM)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")
set(CMAKE_INSTALL_INCLUDEDIR include/platform)

include(GNUInstallDirs)
include(cmake/postbuild)

include(cmake/comp_gcc_flags.cmake)
string(APPEND CMAKE_C_FLAGS " ${C_CXX_FLAGS} ${EXTRA_C_FLAGS}")
string(APPEND CMAKE_ASM_FLAGS " ${EXTRA_ASM_FLAGS}")

# Removes quotes
separate_arguments(LOCAL_COMPILE_OPTIONS NATIVE_COMMAND "${EXTRA_COMPILE_OPTIONS}")
add_compile_options(${LOCAL_COMPILE_OPTIONS})

# Components
add_subdirectory(src)
add_subdirectory(app)

# ---- Declare target library ----
add_library(${RUNNER_TARGET} STATIC
    src/printf.c
    src/platform.c
    src/rng.c
    src/uart.c)

target_link_libraries(${RUNNER_TARGET} PRIVATE
    $<BUILD_INTERFACE:cmsis>
    $<BUILD_INTERFACE:drivers>
    $<BUILD_INTERFACE:drivers_config_if>
    $<BUILD_INTERFACE:system>)

target_include_directories(
    ${RUNNER_TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/..>)

target_sources(
    ${RUNNER_TARGET}
    PUBLIC
    FILE_SET HEADERS
    FILES
        ${CMAKE_SOURCE_DIR}/inc/platform/printf.h
        ${CMAKE_SOURCE_DIR}/inc/platform/platform.h
        ${CMAKE_SOURCE_DIR}/inc/platform/utils.h
        ${CMAKE_SOURCE_DIR}/inc/platform/stack_usage.h
    BASE_DIRS
        ${CMAKE_SOURCE_DIR}/inc/platform)

include(GenerateExportHeader)
generate_export_header(
    ${RUNNER_TARGET}
    BASE_NAME ${RUNNER_TARGET}
    EXPORT_FILE_NAME export/${RUNNER_TARGET}/${RUNNER_TARGET}_export.h)

include(cmake/install-rules.cmake)

# ---- CPack ----
string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOW)
string(TOLOWER "${CMAKE_C_COMPILER_ID}" CMAKE_C_COMPILER_ID_LOWER)
set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "${RUNNER_TARGET}_${PROJECT_VERSION}_${CMAKE_C_COMPILER_ID_LOWER}_${CMAKE_BUILD_TYPE_LOW}")
include(CPack)
