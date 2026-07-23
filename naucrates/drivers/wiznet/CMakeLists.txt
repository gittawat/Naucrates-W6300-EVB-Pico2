add_library(naucrates_wiznet STATIC)

target_sources(naucrates_wiznet PRIVATE
    w6300_driver.cpp
)

target_include_directories(naucrates_wiznet PUBLIC
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(naucrates_wiznet PUBLIC
    wiz6300
    naucrates_platform
)

target_compile_options(naucrates_wiznet PRIVATE
    -Wall -Wextra -Wno-unused-parameter
)
