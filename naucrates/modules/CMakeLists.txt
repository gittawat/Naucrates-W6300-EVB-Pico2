add_library(naucrates_modules INTERFACE)
target_include_directories(naucrates_modules INTERFACE
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(naucrates_modules INTERFACE
    naucrates_platform
)
