cmake_minimum_required(VERSION 3.13)

set(PICO_PLATFORM rp2350)
set(PICO_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/sdk/pico-sdk")

set(PICO_NO_PICOTOOL TRUE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(pico_sdk_import.cmake)

project(naucrates C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)

pico_sdk_init()

add_subdirectory(library/etl)
add_subdirectory(library/wiz6300-lib)

add_library(rtt STATIC
    library/RTT/RTT/SEGGER_RTT.c
    library/RTT/RTT/SEGGER_RTT_printf.c
)
target_include_directories(rtt PUBLIC
    library/RTT/RTT
    library/RTT/Config
)

add_subdirectory(naucrates)

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
    add_custom_target(shared_compile_commands ALL
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/build/compile_commands.json"
        COMMENT "Updating the single shared compile_commands.json in build/"
    )
endif()
