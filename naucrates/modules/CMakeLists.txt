add_library(naucrates_modules STATIC)
target_include_directories(naucrates_modules PUBLIC
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(naucrates_modules PUBLIC
    naucrates_common
)
target_sources(naucrates_modules PRIVATE
    blinky/blinky_module.cpp
)
