
add_library(
    vtest
    vtest.c
)

add_executable(
    vtest_test
    vtest_test.c
)

target_link_libraries(
    vtest_test
    vtest
)

add_test(NAME vtest COMMAND vtest_test)
