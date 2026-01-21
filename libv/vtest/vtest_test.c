#include "vtest.h"

TEST(mytest, compiles) {
    assert_int_eq(1, 1);
}

int main(void) {
    return vtest_run_tests();
}
