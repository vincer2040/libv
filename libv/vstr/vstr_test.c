#include "vstr.h"
#include "libv/vtest/vtest.h"

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

TEST(u8_vstr, push_char) {
    char buf[] = "foo bar baz foo bar baz foo bar baz foo bar baz";
    size_t buf_length = sizeof buf / sizeof buf[0];
    u8test t = u8test_new();
    size_t i;
    for (i = 0; i < 23; ++i) {
        u8test_push_char(&t, buf[i]);
        assert_uint_eq(u8test_length(&t), i + 1);
        assert_int_eq(u8test_is_small(&t), true);
        assert_mem_eq(u8test_data(&t), buf, u8test_length(&t));
    }
    for (; i < buf_length; ++i) {
        u8test_push_char(&t, buf[i]);
        assert_uint_eq(u8test_length(&t), i + 1);
        assert_int_eq(u8test_is_large(&t), true);
        assert_mem_eq(u8test_data(&t), buf, u8test_length(&t));
    }
    u8test_free(&t);
}

TEST(u8_vstr, cat_string) {
    char buf[] = "foobarbaz";
    u8test t = u8test_new();

    u8test_cat_string(&t, buf);
    assert_int_eq(u8test_is_small(&t), true);
    assert_str_eq(u8test_data(&t), buf);

    u8test_cat_string(&t, buf);
    assert_int_eq(u8test_is_small(&t), true);
    assert_str_eq(u8test_data(&t), "foobarbazfoobarbaz");

    u8test_cat_string(&t, buf);
    assert_int_eq(u8test_is_large(&t), true);
    assert_str_eq(u8test_data(&t), "foobarbazfoobarbazfoobarbaz");

    u8test_free(&t);
}

int main(void) {
    return vtest_run_tests();
}
