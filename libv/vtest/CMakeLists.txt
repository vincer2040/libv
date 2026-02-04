add_executable(
    vtest_test
    vtest_test.c
)

add_test(NAME vtest COMMAND vtest_test)
