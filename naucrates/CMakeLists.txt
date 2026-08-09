add_library(naucrates_common INTERFACE)
target_compile_options(naucrates_common INTERFACE
    -Wall -Wextra -Wno-unused-parameter
    $<$<COMPILE_LANGUAGE:CXX>:
        # Tier 1: warnings promoted to errors
        -Werror=return-type
        -Werror=uninitialized
        -Werror=maybe-uninitialized
        -Werror=sign-compare
        -Werror=overflow
        -Werror=format
        -Werror=format-security
        # Tier 2: additional warnings
        -Wformat=2
        -Wpedantic
        -Wshadow=local
        -Wdouble-promotion
        -Wold-style-cast
        -Wzero-as-null-pointer-constant
        -Woverloaded-virtual
        -Wuseless-cast
        -Wself-move
        -Wfloat-equal
        -Wcast-align
        -Wswitch-enum
    >
)

add_subdirectory(utilities)
add_subdirectory(system)
add_subdirectory(drivers/wiznet)
add_subdirectory(features)
add_subdirectory(main)
add_subdirectory(tests)
