add_library(naucrates_common INTERFACE)
target_link_libraries(naucrates_common INTERFACE naucrates_platform)
target_compile_options(naucrates_common INTERFACE
    -Wall -Wextra -Wno-unused-parameter
)

add_subdirectory(platform)
add_subdirectory(drivers/wiznet)
add_subdirectory(modules)
add_subdirectory(main)
