find_program(iwyu_path NAMES include-what-you-use iwyu OPTIONAL)

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
  set(CLANG 1)
endif()

# Global configuration
string(APPEND C_CXX_FLAGS " ")
string(APPEND C_CXX_FLAGS "-Wignored-qualifiers ")
string(APPEND C_CXX_FLAGS "-Wall ")
string(APPEND C_CXX_FLAGS "-Werror ")
string(APPEND C_CXX_FLAGS "-Wextra ")
#string(APPEND C_CXX_FLAGS "-Wpedantic ")
string(APPEND C_CXX_FLAGS "-Wshadow ")
string(APPEND C_CXX_FLAGS "-Wno-variadic-macros ")
string(APPEND C_CXX_FLAGS "-Wundef ")
#string(APPEND C_CXX_FLAGS "-Wunused ")
string(APPEND C_CXX_FLAGS "-Wunused-result ")
string(APPEND C_CXX_FLAGS "-Wno-vla ")
string(APPEND C_CXX_FLAGS "-Wredundant-decls ")
string(APPEND C_CXX_FLAGS "-Wno-unused-parameter ")
if(CLANG)
  string(APPEND C_CXX_FLAGS "-Wconditional-uninitialized ")
  string(APPEND C_CXX_FLAGS " -Wnewline-eof -fcolor-diagnostics ")
  if(CMAKE_BUILD_TYPE_LOWER STREQUAL "release")
    # Int conversion checks - only on clang and release build
    string(APPEND C_FLAGS "-Wconversion ")
    string(APPEND C_FLAGS "-Wno-sign-conversion ")
    string(APPEND C_FLAGS "-Wno-implicit-int-conversion ") # Should be enabled in the future
  endif()
endif()

if(STACK_USAGE)
  string(APPEND C_CXX_FLAGS "-fstack-usage")
endif()

# Cortex-M33 specific flags
set(TARGET_FLAGS "-mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard ")

set(DEBUG_FLAGS "-Wno-pedantic -Wno-unused -Wno-unused-result -Wno-undef -Wno-unused-parameter")

string(APPEND CMAKE_C_FLAGS " ${C_FLAGS} ${C_CXX_FLAGS} ${EXTRA_C_FLAGS}")
string(APPEND CMAKE_C_FLAGS_DEBUG " ${DEBUG_FLAGS}")

string(APPEND CMAKE_CXX_FLAGS " ${C_CXX_FLAGS}")
string(APPEND CMAKE_CXX_FLAGS_DEBUG " ${DEBUG_FLAGS}")

string(APPEND CMAKE_ASM_FLAGS " ${EXTRA_ASM_FLAGS} -Wa,--noexecstack")
string(APPEND CMAKE_ASM_FLAGS_DEBUG " ${DEBUG_FLAGS}")
