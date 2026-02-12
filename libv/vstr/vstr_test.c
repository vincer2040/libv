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

TEST(u8_vstr, fast_cmp) {
    u8test t1 = u8test_from("foo");
    u8test t2 = u8test_from("bar");

    assert_int_ne(u8test_fast_cmp(&t1, &t2), 0);

    t1 = u8test_from("foo");
    t2 = u8test_from("foo");

    assert_int_eq(u8test_fast_cmp(&t1, &t2), 0);

    t1 = u8test_from("foobar");
    t2 = u8test_from("foo");

    assert_int_ne(u8test_fast_cmp(&t1, &t2), 0);
}

TEST(u8_vstr, cmp) {
    u8test t1 = u8test_from("foo");
    u8test t2 = u8test_from("bar");

    assert_int_ne(u8test_cmp(&t1, &t2), 0);

    t1 = u8test_from("foo");
    t2 = u8test_from("foo");

    assert_int_eq(u8test_cmp(&t1, &t2), 0);

    t1 = u8test_from("foobar");
    t2 = u8test_from("foo");

    assert_int_eq(u8test_cmp(&t1, &t2), 1);

    t1 = u8test_from("foo");
    t2 = u8test_from("foobar");

    assert_int_eq(u8test_cmp(&t1, &t2), -1);
}

TEST(u8_vstr, split_char) {
    typedef struct {
        const char* input;
        char ch;
        const char* split[10];
        size_t num_split;
    } split_test;

    split_test tests[] = {
        {
            "foo\nbar",
            '\n',
            {"foo", "bar"},
            2,
        },
        {
            "foo\n",
            '\n',
            {"foo", ""},
            2,
        },
        {
            "\nfoo",
            '\n',
            {"", "foo"},
            2,
        },
        {
            "foo\n\nbar",
            '\n',
            {"foo", "", "bar"},
            3,
        },
        {
            "foo\nbar\nbaz",
            '\n',
            {"foo", "bar", "baz"},
            3,
        },
    };

    for (size_t i = 0; i < array_size(tests); ++i) {
        split_test t = tests[i];
        u8test s = u8test_from(t.input);
        u8test_array a = u8test_split_char(&s, t.ch);
        assert_uint_eq(a.size, t.num_split);
        for (size_t j = 0; j < t.num_split; ++j) {
            assert_str_eq(u8test_data(&a.data[j]), t.split[j]);
        }
        u8test_array_free(&a);
    }
}

VTEST_MAIN()
