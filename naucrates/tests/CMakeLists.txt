add_executable(triple_buffer_mcu_test
    main.cpp
)
target_link_libraries(triple_buffer_mcu_test PRIVATE
    naucrates_common
    pico_multicore
)
