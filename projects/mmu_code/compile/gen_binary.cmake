
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <CMAKE_C_LINK_FLAGS> <OBJECTS> -o <TARGET>.elf <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <CMAKE_CXX_LINK_FLAGS> <OBJECTS> -o <TARGET>.elf <LINK_LIBRARIES>")


# Config toolchain
if(CONFIG_TOOLCHAIN_PATH)
    set(CMAKE_SIZE   "${CONFIG_TOOLCHAIN_PATH}/${CONFIG_TOOLCHAIN_PREFIX}size${EXT}")
    set(CMAKE_OBJDUMP "${CONFIG_TOOLCHAIN_PATH}/${CONFIG_TOOLCHAIN_PREFIX}objdump${EXT}")
else()
    set(CMAKE_SIZE   "size${EXT}")
    set(CMAKE_SIZE   "objdump${EXT}")
endif()

# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#         COMMAND ${CMAKE_OBJCOPY} --output-format=binary ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
#         COMMENT "-- Generating .bin firmware at ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin"
#         )

# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#         COMMAND ${CMAKE_SIZE} ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf
#         COMMENT "============= firmware ============="
#         )

# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#         COMMAND ${CMAKE_OBJDUMP} -S ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf > ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.txt
#         )


add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_SIZE} ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf
        COMMENT "============= firmware ============="
        )

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJDUMP} -S ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf > ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.txt
        )

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} --output-format=binary ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf --remove-section .text_flash ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin
        COMMENT "-- Generating .bin firmware at ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin"
        )

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} --output-format=binary ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf --only-section .text_flash ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_flash.bin
        COMMENT "-- Generating .bin firmware at ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_flash.bin"
        )

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${python} ${PROJECT_PATH}/compile/gen_kfpkg.py
        COMMENT "-- Generating .kfpkg firmware at ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.kfpkg"
        )





