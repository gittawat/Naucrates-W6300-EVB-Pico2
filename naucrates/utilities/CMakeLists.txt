add_library(naucrates_utilities INTERFACE)
target_include_directories(naucrates_utilities INTERFACE
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(naucrates_utilities INTERFACE
    etl
    rtt
    pico_stdlib
    hardware_irq
    hardware_gpio
)
