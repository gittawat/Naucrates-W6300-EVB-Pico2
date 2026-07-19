cmake_minimum_required(VERSION 3.13)

set(PICO_PLATFORM rp2350)
set(PICO_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/sdk/pico-sdk")

set(PICO_NO_PICOTOOL TRUE)
#set(PICOTOOL_FETCH_FROM_GIT_PATH "/home/gttwt/Projects/shared-picotool")


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Pull in SDK configuration macros
include(pico_sdk_import.cmake)

project(ETL_RTT_testing C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Add ETL library subdirectory
add_subdirectory(library/etl)

# Add W6300 Ethernet library
add_subdirectory(library/wiz6300-lib)

# Define SEGGER RTT target
add_library(rtt STATIC
    library/RTT/RTT/SEGGER_RTT.c
    library/RTT/RTT/SEGGER_RTT_printf.c
)
target_include_directories(rtt PUBLIC
    library/RTT/RTT
    library/RTT/Config
)

add_executable(${PROJECT_NAME}
    main.cpp
)

# Pull in standard library dependencies, ETL, and RTT
target_link_libraries(${PROJECT_NAME} pico_stdlib pico_multicore etl rtt wiz6300)

# Enable USB standard output and disable UART output (optional, useful for printing)
#pico_enable_stdio_usb(${PROJECT_NAME} 1)
#pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Generate standard output files including the .uf2 image
#pico_add_extra_outputs(${PROJECT_NAME})


# cmake --build build/debug --target flash
add_custom_target(flash
    COMMAND probe-rs download --chip RP235x --protocol swd $<TARGET_FILE:${PROJECT_NAME}>
    COMMAND probe-rs reset --chip RP235x --protocol swd
    DEPENDS ${PROJECT_NAME}
    COMMENT "Flashing target MCU using probe-rs..."
)

# Force a single compile_commands.json in the main build/ folder
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    # Ensure the parent build/ directory exists
    file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

    add_custom_target(shared_compile_commands ALL
        COMMAND ${CMAKE_COMMAND} -E copy
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/build/compile_commands.json"
        COMMENT "Updating the single shared compile_commands.json in build/"
    )
endif()
