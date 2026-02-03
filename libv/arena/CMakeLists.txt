add_executable(
    arena_test
    arena_test.c
)

target_compile_options(arena_test
    PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wno-gnu-zero-variadic-macro-arguments
    -pedantic
    -fstack-clash-protection
    -fstack-protector-all
    -fstack-protector-strong
    -Werror=format-security
    -Werror=implicit-function-declaration
    -g
)

target_link_libraries(
    arena_test
    vtest
)

target_include_directories(arena_test PRIVATE
    ${CMAKE_SOURCE_DIR}
)

add_test(NAME arena COMMAND arena_test)

