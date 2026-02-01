#include "vtest.h"

TEST(mytest, compiles) { assert_int_eq(1, 1); }

VTEST_MAIN()
