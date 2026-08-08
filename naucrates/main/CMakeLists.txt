add_executable(naucrates
    main.cpp
)
target_link_libraries(naucrates PRIVATE
    naucrates_features
    #    naucrates_wiznet
    naucrates_common
    naucrates_utilities
    pico_multicore
)
