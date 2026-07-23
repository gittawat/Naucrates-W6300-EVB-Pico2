add_library(naucrates_modules STATIC)

target_sources(naucrates_modules PRIVATE
    module.cpp
    module_runner.cpp
)

target_include_directories(naucrates_modules PUBLIC
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(naucrates_modules PUBLIC
    naucrates_platform
    etl
)

target_compile_options(naucrates_modules PRIVATE
    -Wall -Wextra -Wno-unused-parameter
)
