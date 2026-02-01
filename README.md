# libv

A collection of useful C libraries.

## vtest
### vtest — A Minimal, Zero‑Dependency C Unit Testing Library

vtest is a lightweight, self‑registering unit testing framework for C. It is designed to be:

* **Simple**: no build system integration, no external tools
* **Fast**: minimal runtime overhead
* **Portable**: pure C with optional compiler extensions
* **Ergonomic**: readable tests and expressive assertions

vtest is particularly well‑suited for low‑level systems code, libraries, and data‑structure testing, where bringing in large test frameworks is undesirable.

---

### Table of Contents

1. Design Overview
2. Getting Started
3. Writing Tests
4. Test Suites
5. Assertions
6. Running Tests
7. Command‑Line Interface
8. API Reference
9. Portability Notes
10. Usage Example

---

### 1. Design Overview

vtest follows a **static self‑registration** model:

* Each test registers itself at program startup using a constructor attribute
* No manual test lists or registration code is required
* Tests are grouped into *suites* for organization

A test is simply a function:

```c
void test_fn(void);
```

Tests are identified by:

* **Suite name** (string)
* **Test name** (string)

Failures abort the current test and are reported immediately with file and line information.

---

### 2. Getting Started

#### Compiling Your Test Program

vtest consists of a single public header (`vtest.h`) and a single implementation file (`vtest.c`).

You **must compile and link `vtest.c` exactly once** into your test executable.

##### Example (Clang / GCC)

```sh
cc -std=c11 -Wall -Wextra -O2 \
   tests.c libv/vtest/vtest.c \
   -o tests
```

Where:

* `tests.c` contains your `TEST(...)` definitions and `VTEST_MAIN()`
* `vtest.c` provides the implementation of the test runner and assertions

#### Multiple Test Files

You may split tests across multiple translation units:

```sh
cc -std=c11 -Wall -Wextra -O2 \
   test_vmap.c test_vec.c test_alloc.c \
   libv/vtest/vtest.c \
   -o tests
```

Rules:

* **Exactly one** source file must contain `VTEST_MAIN()`
* All other test files may freely use `TEST(...)`

##### Static Libraries

If you prefer, `vtest.c` may be compiled into a static library:

```sh
cc -c libv/vtest/vtest.c -o vtest.o
ar rcs libvtest.a vtest.o

cc tests.c libvtest.a -o tests
```

##### Required Compiler Support

vtest relies on:

* `__attribute__((constructor))` for test self-registration

This is supported by:

* Clang
* GCC

If your compiler does not support constructor attributes, tests must be registered manually (advanced usage).

---

#### Include the Header

```c
#include "libv/vtest/vtest.h"
```

#### Provide a Test Entry Point

Exactly **one** translation unit must define the test runner entry point:

```c
VTEST_MAIN();
```

This expands to a `main()` function that handles test execution and CLI options.

---

### 3. Writing Tests

Tests are defined using the `TEST` macro:

```c
TEST(suite_name, test_name) {
    // test code
}
```

Example:

```c
TEST(math, addition) {
    assert_int_eq(2 + 2, 4);
}
```

#### Naming Rules

* `suite_name` and `test_name` must be valid C identifiers
* Names are converted to strings automatically
* The pair `(suite_name, test_name)` uniquely identifies a test

---

### 4. Test Suites

A **suite** is a logical grouping of tests.

```c
TEST(vmap, insert) { ... }
TEST(vmap, erase)  { ... }
TEST(vec, push)    { ... }
```

Suites are used for:

* Organization
* Selective test execution
* Clear output grouping

There is no explicit suite declaration—suites exist implicitly.

---

### 5. Assertions

Assertions validate conditions inside a test. If an assertion fails:

* The failure is reported
* The test is aborted
* Execution continues with the next test

All assertion macros capture:

* Expression text
* Source file
* Line number

#### Integer Assertions

```c
assert_int_eq(a, b);
assert_int_ne(a, b);

assert_uint_eq(a, b);
assert_uint_ne(a, b);
```

* Signed assertions use `int64_t`
* Unsigned assertions use `uint64_t`

#### Floating‑Point Assertions

```c
assert_double_eq(a, b);
```

* Uses a tolerance suitable for `double`
* Intended for equality, not ordering

#### Boolean Assertions

```c
assert_true(cond);
assert_false(cond);
```

#### Pointer Assertions

```c
assert_ptr_nonnull(ptr);
assert_ptr_null(ptr);
```

#### String Assertions

```c
assert_str_eq(a, b);
```

* Compares C strings using `strcmp`
* `NULL` is treated as unequal to any non‑`NULL` string

#### Memory Assertions

```c
assert_mem_eq(a, b, size);
```

* Performs a byte‑wise comparison of `size` bytes
* Useful for structs and raw buffers

#### Generic Assertion

```c
vassert(cond);
```

Fails if `cond` evaluates to false.

---

### 6. Running Tests

#### Run All Tests

```sh
./tests
```

Runs every registered test and returns:

* `0` on success
* non‑zero if any test fails

#### List Tests

```sh
./tests --list-tests
```

Outputs all available tests grouped by suite.

#### Run a Single Test

```sh
./tests --run <suite> <test>
```

Example:

```sh
./tests --run vmap insert1
```

---

### 7. Command‑Line Interface

The `VTEST_MAIN()` macro provides the following behavior:

| Command                    | Description       |
| -------------------------- | ----------------- |
| `./tests`                  | Run all tests     |
| `./tests --list-tests`     | List all tests    |
| `./tests --run suite test` | Run a single test |

Any other argument combination exits with failure.

---

### 8. API Reference

#### Data Structures

```c
typedef struct {
    const char* suite_name;
    const char* test_name;
    void (*test_fn)(void);
} test;
```

Represents a single test case.

```c
typedef struct {
    size_t length;
    size_t capacity;
    size_t longest_name;
    test* tests;
} tests;
```

Internal dynamic registry of tests.

#### Registration

```c
void vtest_add_test_(const test* test);
```

Registers a test with the global test registry.

Normally invoked automatically by `TEST()`.

#### Execution

```c
int vtest_run_tests(void);
int vtest_run_test(const char* suite, const char* test);
void vtest_list_tests(void);
```

---

### 9. Portability Notes

* Uses `__attribute__((constructor))`

  * Supported by **Clang** and **GCC**
  * Not supported by MSVC without alternatives

If constructor attributes are unavailable, tests must be registered manually.

The library otherwise depends only on the C standard library.

---

### 10. Example

```c
#include "libv/vtest/vtest.h"

TEST(math, multiply) {
    assert_int_eq(6 * 7, 42);
}

TEST(ptr, allocation) {
    void* p = malloc(16);
    assert_ptr_nonnull(p);
    free(p);
}

VTEST_MAIN();
```

---

### Philosophy

vtest intentionally avoids:

* Fixtures
* Mocking frameworks
* Reflection
* Global setup/teardown magic

Instead, it favors:

* Explicit code
* Fast iteration
* Clear failures
* Zero friction

If you want *just enough testing* for serious C code, vtest is built for that job.

