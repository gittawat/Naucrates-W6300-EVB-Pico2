add_library(naucrates_modules STATIC)

target_sources(naucrates_modules PRIVATE
    module.cpp
    module_runner.cpp
)

target_link_libraries(naucrates_modules PUBLIC
    naucrates_platform
)
