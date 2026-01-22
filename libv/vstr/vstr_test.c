#include "libv/vtest/vtest.h"
#include "vstr.h"

VSTR_DECLARE_DEFAULT(u8test);

TEST(u8_vstr, from) {
    u8test t = u8test_from("foo");
    assert_int_eq(u8test_is_small(&t), true);
    assert_uint_eq(u8test_length(&t), 3);
    assert_str_eq(u8test_data(&t), "foo");
    u8test_free(&t);

    t = u8test_from("foobarbazfoobarbazfooba");
    assert_int_eq(u8test_is_small(&t), true);
    assert_uint_eq(u8test_length(&t), 23);
    assert_str_eq(u8test_data(&t), "foobarbazfoobarbazfooba");
    u8test_free(&t);

    t = u8test_from("foobarbazfoobarbazfoobar");
    assert_int_eq(u8test_is_small(&t), false);
    assert_int_eq(u8test_is_large(&t), true);
    assert_uint_eq(u8test_length(&t), 24);
    assert_str_eq(u8test_data(&t), "foobarbazfoobarbazfoobar");
    u8test_free(&t);
}

int main(void) {
    return vtest_run_tests();
}
