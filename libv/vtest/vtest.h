// BSD 3-Clause License
//
// Copyright (c) 2026, vincer2040
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

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

static tests ts = {0};
static int failed = 0;

static inline void vtest_add_test_(const test* t) {
    if (ts.length == ts.capacity) {
        ts.capacity += 1;
        ts.capacity <<= 1;
        ts.tests = realloc(ts.tests, ts.capacity * sizeof(test));
        if (!ts.tests) {
            fprintf(stderr, "error allocating memory for tests\n");
            abort();
        }
    }
    size_t name_length = strlen(t->suite_name) + strlen(t->test_name);
    if (name_length > ts.longest_name) {
        ts.longest_name = name_length;
    }
    ts.tests[ts.length++] = *t;
}

#define fail_test(fmt, ...)                                                    \
    do {                                                                       \
        fprintf(stderr, fmt, __VA_ARGS__);                                     \
        failed = 1;                                                            \
    } while (0)

static inline void _assert_int_eq(int64_t a, int64_t b, const char* vara, const char* varb,
                    const char* file, int line) {
    if (a == b) {
        return;
    }
    fail_test("ASSERTION FAILED(%s:%d) %s != %s, %s = %ld, %s = %ld\n", file,
              line, vara, varb, vara, a, varb, b);
}

static inline void _assert_int_ne(int64_t a, int64_t b, const char* vara, const char* varb,
                    const char* file, int line) {
    if (a != b) {
        return;
    }
    fail_test("ASSERTION FAILED(%s:%d) %s == %s, %s = %ld, %s = %ld\n", file,
              line, vara, varb, vara, a, varb, b);
}

static inline void _assert_uint_eq(uint64_t a, uint64_t b, const char* vara, const char* varb,
                     const char* file, int line) {
    if (a == b) {
        return;
    }
    fail_test("ASSERTION FAILED(%s:%d) %s != %s, %s = %lu, %s = %lu\n", file,
              line, vara, varb, vara, a, varb, b);
}

static inline void _assert_uint_ne(uint64_t a, uint64_t b, const char* vara, const char* varb,
                     const char* file, int line) {
    if (a != b) {
        return;
    }
    fail_test("ASSERTION FAILED(%s:%d) %s == %s, %s = %lu, %s = %lu\n", file,
              line, vara, varb, vara, a, varb, b);
}

static inline void _assert_double_eq(double a, double b, const char* vara, const char* varb,
                       const char* file, int line) {
    if (fabs(a - b) < (DBL_EPSILON * fabs(a + b))) {
        return;
    }
    fail_test("ASSERTION FAILED(%s:%d) %s != %s, %s = %f, %s = %f\n", file,
              line, vara, varb, vara, a, varb, b);
}

static inline void _assert_str_eq(const char* a, const char* b, const char* vara,
                    const char* varb, const char* file, int line) {
    size_t alen = strlen(a);
    size_t blen = strlen(b);
    if (alen != blen) {
        fail_test("ASSERTION FAILED(%s:%d) %s != %s, %s = %s, %s = %s\n", file,
                  line, vara, varb, vara, a, varb, b);
        return;
    }
    if (strncmp(a, b, alen) != 0) {
        fail_test("ASSERTION FAILED(%s:%d) %s != %s, %s = %s, %s = %s\n", file,
                  line, vara, varb, vara, a, varb, b);
        return;
    }
}

static inline void _assert_mem_eq(const void* a, const void* b, size_t size, const char* vara,
                    const char* varb, const char* file, int line) {
    if (memcmp(a, b, size) == 0) {
        return;
    }
    fail_test("ASSERTION FAILED(%s:%d) %s != %s, %s = %p, %s = %p\n", file,
              line, vara, varb, vara, a, varb, b);
}

static inline void _assert_ptr_nonnull(const void* a, const char* vara, const char* file,
                         int line) {
    if (a != NULL) {
        return;
    }
    fail_test("ASSERTION FAILED(%s:%d) %s == NULL\n", file, line, vara);
}

static inline void _assert_ptr_null(const void* a, const char* vara, const char* file,
                      int line) {
    if (a == NULL) {
        return;
    }
    fail_test("ASSERTION FAILED(%s:%d) %s != NULL\n", file, line, vara);
}

static inline void _assert_true(bool cond, const char* cond_, const char* file, int line) {
    if (cond) {
        return;
    }
    fail_test("ASSERTION FAILED(%s:%d) %s != true\n", file, line, cond_);
}

static inline void _assert_false(bool cond, const char* cond_, const char* file, int line) {
    if (!cond) {
        return;
    }
    fail_test("ASSERTION FAILED(%s:%d) %s != false\n", file, line, cond_);
}

static inline void _assert(bool cond, const char* cond_str, const char* file, int line) {
    if (cond) {
        return;
    }
    fail_test("ASSERTION FAILED(%s:%d) %s\n", file, line, cond_str);
}

static inline void vtest_destroy(void) { free(ts.tests); }

static inline void vtest_list_tests(void) {
    for (size_t i = 0; i < ts.length; ++i) {
        printf("%s.%s\n", ts.tests[i].suite_name, ts.tests[i].test_name);
    }
    vtest_destroy();
}

static inline int vtest_run_test(const char* suite_name, const char* test_name) {
    size_t suite_name_length = strlen(suite_name);
    size_t test_name_length = strlen(suite_name);
    //bool found = false;
    for (size_t i = 0; i < ts.length; ++i) {
        test* t = &ts.tests[i];
        if (strncmp(suite_name, t->suite_name, suite_name_length) == 0 &&
            strncmp(test_name, t->test_name, test_name_length) == 0) {
            t->test_fn();
        }
        vtest_destroy();
        return failed;
    }
    vtest_destroy();
    return 1;
}

static inline int vtest_run_tests(void) {
    size_t amount_to_print = ts.longest_name + 3;
    size_t num_passed = 0;
    // size_t num_failed = 0;
    for (size_t i = 0; i < ts.length; ++i) {
        test* t = &ts.tests[i];
        printf("%s.%s", t->suite_name, t->test_name);
        size_t current_printed_length =
            strlen(t->suite_name) + strlen(t->test_name) + 1;
        while (current_printed_length < amount_to_print) {
            printf(".");
            current_printed_length++;
        }
        fflush(stdout);
        t->test_fn();
        if (failed) {
            printf("FAILED\n");
            goto failed;
        } else {
            printf("PASSED\n");
            num_passed++;
        }
        failed = 0;
    }
    printf("%zu/%zu tests passed\n", num_passed, ts.length);
    vtest_destroy();
    return 0;
failed:
    vtest_destroy();
    return 1;
}

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

#define assert_true(cond_) _assert_true(cond_, #cond_, __FILE__, __LINE__)
#define assert_false(cond_) _assert_false(cond_, #cond_, __FILE__, __LINE__)

#define vassert(cond_) _assert(cond_ #cond_, __FILE__, __LINE__)

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
