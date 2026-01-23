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
void _assert_int_ne(int64_t a, int64_t b, const char* vara, const char* varb,
                    const char* file, int line);
void _assert_uint_ne(uint64_t a, uint64_t b, const char* vara, const char* varb,
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
int vtest_run_tests(void);
void vtest_list_tests(void);
int vtest_run_test(const char* suite_name, const char* test_name);

#define TEST(suite_name_, test_name_)                                          \
    static void vtest_##suite_name_##_##test_name_(void);                      \
    __attribute__((constructor)) static void                                   \
        vtest_register_##suite_name_##_##test_name_(void) {                    \
        test t = {                                                             \
            .suite_name = #suite_name_,                                        \
            .test_name = #test_name_,                                          \
            .test_fn = vtest_##suite_name_##_##test_name_,                     \
        };                                                                     \
        vtest_add_test_(&t);                                                   \
    }                                                                          \
    static void vtest_##suite_name_##_##test_name_(void)

#define assert_int_eq(a_, b_)                                                  \
    _assert_int_eq(a_, b_, #a_, #b_, __FILE__, __LINE__)

#define assert_uint_eq(a_, b_)                                                 \
    _assert_uint_eq(a_, b_, #a_, #b_, __FILE__, __LINE__)

#define assert_int_ne(a_, b_)                                                  \
    _assert_int_ne(a_, b_, #a_, #b_, __FILE__, __LINE__)

#define assert_uint_ne(a_, b_)                                                 \
    _assert_uint_ne(a_, b_, #a_, #b_, __FILE__, __LINE__)

#define assert_double_eq(a_, b_)                                               \
    _assert_double_eq(a_, b_, #a_, #b_, __FILE__, __LINE__)

#define assert_str_eq(a_, b_)                                                  \
    _assert_str_eq(a_, b_, #a_, #b_, __FILE__, __LINE__)

#define assert_mem_eq(a_, b_, size_)                                           \
    _assert_mem_eq(a_, b_, size_, #a_, #b_, __FILE__, __LINE__)

#define assert_ptr_nonnull(a_) _assert_ptr_nonnull(a_, #a_, __FILE__, __LINE__)

#define assert_ptr_null(a_) _assert_ptr_null(a_, #a_, __FILE__, __LINE__)

#define vassert(cond_) _assert(cond_ #cond_, __FiLE__, __LINE__)

#define VTEST_MAIN()                                                           \
    int main(int argc, char* argv[]) {                                         \
        if (argc == 1) {                                                       \
            return vtest_run_tests();                                          \
        }                                                                      \
        if (argc == 2 && strcmp(argv[1], "--list-tests") == 0) {               \
            vtest_list_tests();                                                \
            return 0;                                                          \
        }                                                                      \
        if (argc == 4 && strcmp(argv[1], "--run") == 0) {                      \
            const char* suite_name = argv[2];                                  \
            const char* test_name = argv[3];                                   \
            return vtest_run_test(suite_name, test_name);                      \
        }                                                                      \
        return 1;                                                              \
    }

#endif // __VTEST_H__
