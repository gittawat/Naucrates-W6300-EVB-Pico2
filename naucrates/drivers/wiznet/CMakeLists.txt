add_library(naucrates_wiznet INTERFACE)
target_include_directories(naucrates_wiznet INTERFACE
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(naucrates_wiznet INTERFACE
    wiz6300
    naucrates_common
)
