add_executable(naucrates
    main.cpp
    ../modules/udp_echo/udp_echo_module.cpp
)

target_link_libraries(naucrates
    naucrates_wiznet
    naucrates_modules
    naucrates_platform
    pico_multicore
)

add_custom_target(flash
    COMMAND probe-rs download --chip RP235x --protocol swd $<TARGET_FILE:naucrates>
    COMMAND probe-rs reset --chip RP235x --protocol swd
    DEPENDS naucrates
    COMMENT "Flashing target MCU using probe-rs..."
)
