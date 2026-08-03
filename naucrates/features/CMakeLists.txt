add_library(naucrates_features STATIC)
target_include_directories(naucrates_features PUBLIC
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(naucrates_features PUBLIC
    naucrates_common
)
target_sources(naucrates_features PRIVATE
    blinky/blinky.cpp
)
