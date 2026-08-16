add_executable(triple_buffer_mcu_test
    triple_buffer/triple_buffer_test.cpp
)
target_link_libraries(triple_buffer_mcu_test PRIVATE
    naucrates_common
    naucrates_utilities
    pico_multicore
)

add_executable(triple_buffer_test_2khz_read_200khz_write
    triple_buffer/triple_buffer_test_2khz_read_200khz_write.cpp
)
target_link_libraries(triple_buffer_test_2khz_read_200khz_write PRIVATE
    naucrates_common
    naucrates_utilities
    pico_multicore
    pico_time
    hardware_gpio
)

add_executable(triple_buffer_test_2khz_isr_read_200khz_write
    triple_buffer/triple_buffer_test_2khz_isr_read_200khz_write.cpp
)
target_link_libraries(triple_buffer_test_2khz_isr_read_200khz_write PRIVATE
    naucrates_common
    naucrates_utilities
    pico_multicore
    pico_time
    hardware_gpio
)




add_executable(timer_isr_setup_test
    hardware_timer/timer_isr_setup_test.cpp
)
target_link_libraries(timer_isr_setup_test PRIVATE
    naucrates_common
    naucrates_system
    pico_stdlib
    pico_multicore
    hardware_gpio
)




add_executable(timer_isr_runtime_core_test
    hardware_timer/timer_isr_runtime_core_test.cpp
)
target_link_libraries(timer_isr_runtime_core_test PRIVATE
    naucrates_common
    naucrates_system
    pico_stdlib
    pico_multicore
    hardware_gpio
)


add_executable(timer_isr_static_test
    hardware_timer/timer_isr_static_test.cpp
)
target_link_libraries(timer_isr_static_test PRIVATE
    naucrates_common
    naucrates_system
    pico_stdlib
    pico_multicore
    hardware_gpio
)


add_executable(timer_isr_comptime_test
    hardware_timer/timer_isr_comptime_test.cpp
)
target_link_libraries(timer_isr_comptime_test PRIVATE
    naucrates_common
    naucrates_system
    pico_stdlib
    pico_multicore
    hardware_gpio
)


add_executable(timerisr_warper_test
    hardware_timer/timerisr_warper_test.cpp
)
target_link_libraries(timerisr_warper_test PRIVATE
    naucrates_common
    naucrates_system
    pico_stdlib
    pico_multicore
    hardware_gpio
)

add_executable(typed_timer_owner_test
    hardware_timer/typed_timer_owner_test.cpp
)
target_link_libraries(typed_timer_owner_test PRIVATE
    naucrates_common
    naucrates_system
    pico_stdlib
    pico_multicore
    hardware_gpio
)
