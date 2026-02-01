add_executable(
    vmap_test
    vmap_test.c
)

target_compile_options(vmap_test
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
    vmap_test
    vtest
)

target_include_directories(vmap_test PRIVATE
    ${CMAKE_SOURCE_DIR}
)

add_test(NAME vmap COMMAND vmap_test)

