
add_executable(
    vstr_test
    vstr_test.c
)

target_link_libraries(
    vstr_test
    vtest
)

target_include_directories(vstr_test PRIVATE
    ${CMAKE_SOURCE_DIR}   # libv root
)

add_test(NAME vstr COMMAND vstr_test)

