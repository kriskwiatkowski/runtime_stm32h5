add_executable(
    hello
    hello.c)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(
        hello PRIVATE
        -Wl,--whole-archive ${RUNNER_TARGET} -Wl,--no-whole-archive)
endif()

target_link_options(hello PRIVATE -Wl,-z,noexecstack)
target_postbuild_executable(hello)


# ---- Install sample app ----
# Install the ELF as hello.bin: QEMU identifies ELF by magic bytes (not extension),
# so this lets `qemu-system-arm ... -kernel out/an521/hello.bin` load correctly.
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/hello.bin
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    RENAME hello.bin)
