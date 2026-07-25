# Firmware executable — add your source files here
#
# Example:
#   add_executable(naucrates
#       main.cpp
#       ../modules/<name>/<name>.cpp
#   )
#   target_link_libraries(naucrates
#       naucrates_wiznet
#       naucrates_modules
#       naucrates_platform
#       pico_multicore
#   )
#
# Flash target (via probe-rs SWD):
#   add_custom_target(flash
#       COMMAND probe-rs download --chip RP235x --protocol swd $<TARGET_FILE:naucrates>
#       COMMAND probe-rs reset --chip RP235x --protocol swd
#       DEPENDS naucrates
#   )
