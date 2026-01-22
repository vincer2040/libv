
add_executable(
    vstr_test
    vstr_test.c
)

target_compile_options(vstr_test
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
    # -O2
    -g
)

target_link_libraries(
    vstr_test
    vtest
)

target_include_directories(vstr_test PRIVATE
    ${CMAKE_SOURCE_DIR}   # libv root
)

add_test(NAME vstr COMMAND vstr_test)

