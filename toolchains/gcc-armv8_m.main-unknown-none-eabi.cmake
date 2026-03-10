set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_CPU_ARCHITECTURE armv8-m.main)
set(CMAKE_C_COMPILER_ID GNU)

# Some default GCC settings
# arm-none-eabi- must be part of path environment
set(TOOLCHAIN_PREFIX                arm-none-eabi-)
set(CMAKE_C_COMPILER                ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_ASM_COMPILER              ${CMAKE_C_COMPILER})
set(CMAKE_CXX_COMPILER              ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_LINKER                    ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_OBJCOPY                   ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_OBJDUMP                   ${TOOLCHAIN_PREFIX}objdump)
set(CMAKE_SIZE                      ${TOOLCHAIN_PREFIX}size)

# Set arch for source compilation
set(C_FLAGS "-march=${CMAKE_CPU_ARCHITECTURE}")
# Use Thumb2 also and possibility to switch between Thumb2 and ARM (maybe not needed here)
set(C_FLAGS "${C_FLAGS} -mthumb -mthumb-interwork")
# Generate a separate ELF section for each function in the source file
set(C_FLAGS "${C_FLAGS} -ffunction-sections -fdata-sections")

set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE "-Os -g0")

set(CMAKE_C_FLAGS_INIT ${C_FLAGS} CACHE INTERNAL "" FORCE)

# Set arch for linker
set(LINKER_FLAGS "-march=${CMAKE_CPU_ARCHITECTURE}")
# Don't use sys.specs
#Use nano-specs, only. It defines system calls that should be implemented as
# stubs that return errors when called (-lnosys)
set(LINKER_FLAGS "-specs=nosys.specs -specs=nano.specs -lnosys ")
# Generate a separate ELF section for each function in the source file
set(LINKER_FLAGS "${LINKER_FLAGS} -Wl,-gc-sections -ffunction-sections -fdata-sections ")
# Provide nice table with memory usage at the end of the build process
set(LINKER_FLAGS "${LINKER_FLAGS} -Wl,--print-memory-usage")
set(CMAKE_EXE_LINKER_FLAGS_INIT ${LINKER_FLAGS} CACHE INTERNAL "" FORCE)

set(CMAKE_TOOLCHAIN_CMD_STRIP ${TOOLCHAIN_PREFIX}-strip)
set(OBJCOPY ${CMAKE_OBJCOPY} CACHE INTERNAL "")
set(OBJDUMP ${CMAKE_OBJDUMP} CACHE INTERNAL "")
set(NEEDS_OBJCOPY 1)
set(NEEDS_OBJDUMP 1)


set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_C_COMPILER_WORKS 1)

# Try-compile should not try to run on host
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
