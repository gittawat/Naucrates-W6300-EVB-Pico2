add_library(naucrates_system INTERFACE)
target_include_directories(naucrates_system INTERFACE
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(naucrates_system INTERFACE
    etl
    pico_time
    hardware_timer
    hardware_irq
    naucrates_common
)
