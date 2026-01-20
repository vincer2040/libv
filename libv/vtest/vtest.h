#ifndef __VTEST_H__

#define __VTEST_H__

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char* suite_name;
    const char* test_name;
    void (*test_fn)(void);
} test;

typedef struct {
    size_t length;
    size_t capacity;
    size_t longest_name;
    test* tests;
} tests;

void vtest_add_test_(const test* test);
void _assert_int_eq(int64_t a, int64_t b, const char* vara, const char* varb,
                    const char* file, int line);
void _assert_uint_eq(uint64_t a, uint64_t b, const char* vara, const char* varb,
                     const char* file, int line);
void _assert_double_eq(double a, double b, const char* vara, const char* varb,
                       const char* file, int line);
void _assert_str_eq(const char* a, const char* b, const char* vara,
                    const char* varb, const char* file, int line);
void _assert_mem_eq(const void* a, const void* b, size_t size, const char* vara,
                    const char* varb, const char* file, int line);
void _assert_ptr_nonnull(const void* a, const char* vara, const char* file,
                         int line);
void _assert_ptr_null(const void* a, const char* vara, const char* file,
                      int line);
void _assert(bool cond, const char* cond_str, const char* file, int line);
void vtest_run_tests_(void);

#define TEST(suite_name_, test_name_)                                          \
    static void vtest_##suite_name_##_##test_name_(void)

#define VTEST_ADD_TEST(suite_name_, test_name_)                                \
    do {                                                                       \
        test t = {                                                             \
            .suite_name = #suite_name_,                                        \
            .test_name = #test_name_,                                          \
            .test_fn = vtest_##suite_name_##_##test_name_,                     \
        };                                                                     \
        vtest_add_test_(&t);                                                   \
    } while (0)

#define VTEST_RUN_ALL()                                                        \
    do {                                                                       \
        vtest_run_tests();                                                     \
    } while (0)

#define assert_int_eq(a, b) _assert_int_eq(a, b, #a, #b, __FILE__, __LINE__)
#define assert_uint_eq(a, b) _assert_uint_eq(a, b, #a, #b, __FILE__, __LINE__)
#define assert_double_eq(a, b)                                                 \
    _assert_double_eq(a, b, #a, #b, __FILE__, __LINE__)
#define assert_str_eq(a, b) _assert_str_eq(a, b, #a, #b, __FILE__, __LINE__)
#define assert_mem_eq(a, b, size)                                              \
    _assert_mem_eq(a, b, size, #a, #b, __FILE__, __LINE__)
#define assert_ptr_nonnull(a) _assert_ptr_nonnull(a, #a, __FILE__, __LINE__)
#define assert_ptr_null(a) _assert_ptr_null(a, #a, __FILE__, __LINE__)

#endif // __VTEST_H__
