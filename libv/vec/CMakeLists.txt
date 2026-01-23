add_executable(
    vec_test
    vec_test.c
)

target_compile_options(vec_test
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
    vec_test
    vtest
)

target_include_directories(vec_test PRIVATE
    ${CMAKE_SOURCE_DIR}
)

add_test(NAME vec COMMAND vec_test)

