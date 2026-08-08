add_executable(triple_buffer_mcu_test
    triple_buffer_test.cpp
)
target_link_libraries(triple_buffer_mcu_test PRIVATE
    naucrates_common
    naucrates_utilities
    pico_multicore
)

add_executable(triple_buffer_test_2khz_read_200khz_write
    triple_buffer_test_2khz_read_200khz_write.cpp
)
target_link_libraries(triple_buffer_test_2khz_read_200khz_write PRIVATE
    naucrates_common
    naucrates_utilities
    pico_multicore
    pico_time
    hardware_gpio
)
