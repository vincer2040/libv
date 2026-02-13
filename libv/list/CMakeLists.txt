add_executable(
    list_test
    list_test.c
)

target_compile_options(list_test
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

target_include_directories(list_test PRIVATE
    ${CMAKE_SOURCE_DIR}
)

add_test(NAME list COMMAND list_test)

