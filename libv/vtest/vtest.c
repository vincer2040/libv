#include "vtest.h"

static tests ts = {0};
static int failed = 0;

void vtest_add_test_(const test* t) {
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

void _assert_int_eq(int64_t a, int64_t b, const char* vara, const char* varb,
                    const char* file, int line) {
    if (a == b) {
        return;
    }
    fail_test("ASSERTION FAILED(%s:%d) %s != %s, %s = %ld, %s = %ld\n", file,
              line, vara, varb, vara, a, varb, b);
}

void _assert_int_ne(int64_t a, int64_t b, const char* vara, const char* varb,
                    const char* file, int line) {
    if (a != b) {
        return;
    }
    fail_test("ASSERTION FAILED(%s:%d) %s == %s, %s = %ld, %s = %ld\n", file,
              line, vara, varb, vara, a, varb, b);
}

void _assert_uint_eq(uint64_t a, uint64_t b, const char* vara, const char* varb,
                     const char* file, int line) {
    if (a == b) {
        return;
    }
    fail_test("ASSERTION FAILED(%s:%d) %s != %s, %s = %lu, %s = %lu\n", file,
              line, vara, varb, vara, a, varb, b);
}

void _assert_uint_ne(uint64_t a, uint64_t b, const char* vara, const char* varb,
                     const char* file, int line) {
    if (a != b) {
        return;
    }
    fail_test("ASSERTION FAILED(%s:%d) %s == %s, %s = %lu, %s = %lu\n", file,
              line, vara, varb, vara, a, varb, b);
}

void _assert_double_eq(double a, double b, const char* vara, const char* varb,
                       const char* file, int line) {
    if (fabs(a - b) < (DBL_EPSILON * fabs(a + b))) {
        return;
    }
    fail_test("ASSERTION FAILED(%s:%d) %s != %s, %s = %f, %s = %f\n", file,
              line, vara, varb, vara, a, varb, b);
}

void _assert_str_eq(const char* a, const char* b, const char* vara,
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

void _assert_mem_eq(const void* a, const void* b, size_t size, const char* vara,
                    const char* varb, const char* file, int line) {
    if (memcmp(a, b, size) == 0) {
        return;
    }
    fail_test("ASSERTION FAILED(%s:%d) %s != %s, %s = %p, %s = %p\n", file,
              line, vara, varb, vara, a, varb, b);
}

void _assert_ptr_nonnull(const void* a, const char* vara, const char* file,
                         int line) {
    if (a != NULL) {
        return;
    }
    fail_test("ASSERTION FAILED(%s:%d) %s == NULL", file, line, vara);
}

void _assert_ptr_null(const void* a, const char* vara, const char* file,
                      int line) {
    if (a == NULL) {
        return;
    }
    fail_test("ASSERTION FAILED(%s:%d) %s != NULL", file, line, vara);
}

void _assert(bool cond, const char* cond_str, const char* file, int line) {
    if (cond) {
        return;
    }
    fail_test("ASSERTION FAILED(%s:%d) %s", file, line, cond_str);
}

static void vtest_destroy(void) { free(ts.tests); }

int vtest_run_tests(void) {
    size_t amount_to_print = ts.longest_name + 3;
    size_t num_passed = 0;
    size_t num_failed = 0;
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
