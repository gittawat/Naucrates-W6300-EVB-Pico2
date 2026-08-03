add_executable(naucrates
    main.cpp
)
target_link_libraries(naucrates PRIVATE
    naucrates_features
    #    naucrates_wiznet
    naucrates_common
    pico_multicore
)
