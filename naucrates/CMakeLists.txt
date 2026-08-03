add_library(naucrates_common INTERFACE)
target_link_libraries(naucrates_common INTERFACE naucrates_utilities)
target_compile_options(naucrates_common INTERFACE
    -Wall -Wextra -Wno-unused-parameter
)

add_subdirectory(utilities)
add_subdirectory(drivers/wiznet)
add_subdirectory(features)
add_subdirectory(main)
add_subdirectory(tests)
