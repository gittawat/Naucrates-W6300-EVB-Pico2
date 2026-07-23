add_library(naucrates_wiznet STATIC)

target_sources(naucrates_wiznet PRIVATE
    w6300_driver.cpp
)

target_link_libraries(naucrates_wiznet PUBLIC
    wiz6300
    naucrates_platform
)
