add_executable(
    vtest_test
    vtest_test.c
)
target_include_directories(vtest_test PRIVATE
    ${CMAKE_SOURCE_DIR}
)

add_test(NAME vtest COMMAND vtest_test)
