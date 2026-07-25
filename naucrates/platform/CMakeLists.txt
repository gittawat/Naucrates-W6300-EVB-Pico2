add_library(naucrates_platform INTERFACE)
target_include_directories(naucrates_platform INTERFACE
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(naucrates_platform INTERFACE
    etl
    rtt
    pico_stdlib
    hardware_irq
    hardware_gpio
)
