function(target_postbuild_executable tgt)
    set(ELF_FILE $<TARGET_FILE:${tgt}>)
    set(ELF_BASE $<TARGET_FILE_DIR:${tgt}>/$<TARGET_FILE_BASE_NAME:${tgt}>)

    set(HEX_FILE ${ELF_BASE}.hex)
    set(SECTIONS_FILE ${ELF_BASE}.sections)
    set(LST_FILE ${ELF_BASE}.lst)
    set(BIN_FILE ${ELF_BASE}.bin)

    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        add_custom_command(
            TARGET ${tgt} POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -Obinary ${ELF_FILE} ${BIN_FILE}
            COMMAND ${CMAKE_OBJCOPY} -Oihex   ${ELF_FILE} ${HEX_FILE}
            COMMENT "Invoking: objcopy")

        add_custom_command(
            TARGET ${tgt} POST_BUILD
            COMMAND ${CMAKE_OBJDUMP} -h -D ${ELF_FILE} > ${SECTIONS_FILE}
            COMMAND ${CMAKE_OBJDUMP} -S --disassemble ${ELF_FILE} > ${LST_FILE}
            COMMENT "Invoking: objdump")
    endif()

    set_property(
        TARGET ${tgt} APPEND
        PROPERTY ADDITIONAL_CLEAN_FILES
        ${HEX_FILE} ${BIN_FILE} ${SECTIONS_FILE} ${LST_FILE})
endfunction()
