# vtest

A basic unit testing framework. Designed to be minimal but efficient.

### Example

```C
#include "libv/vtest/vtest.h"

int add(int x, int y) { return x + y; }

TEST(vtest, example) {
    assert_int_eq(add(1, 2), 3);
}

VTEST_MAIN()

```

### Purpose

vtest is a minimalistic testing framework that works perfectly for most
C projects.

### assertions

`assert_int_eq`  - ints are equal

`assert_uint_eq` - unsigned ints are equal

`assert_int_ne` - ints are not equal

`assert_uint_ne` - unsigned ints are not equal

`assert_double_eq` - doubles are equal (within a certain precision)

`assert_str_eq` - string equality

`assert_mem_eq` - raw memory equality

`assert_ptr_nonnull` - pointer is not null

`assert_ptr_null` - pointer is null

`assert_true` - condition is true

`assert_false` - condition is false

`vassert` - expression is truthy
