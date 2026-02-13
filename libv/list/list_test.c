#include "libv/base/base.h"
#include "libv/vtest/vtest.h"
#include "list.h"

LIST_DECLARE_DEFAULT(int_list, int);

TEST(list, push_pop_back) {
    int_list l = int_list_new();

    for (int i = 0; i < 10; ++i) {
        int_list_push_back(&l, &i);
    }

    assert_uint_eq(int_list_size(&l), 10);

    for (int i = 9; i >= 0; --i) {
        int x;
        assert_int_eq(int_list_pop_back(&l, &x), LIBV_OK);
        assert_int_eq(x, i);
    }

    assert_uint_eq(int_list_size(&l), 0);

    int_list_destroy(&l);
}

TEST(list, push_pop_font) {
    int_list l = int_list_new();

    for (int i = 0; i < 10; ++i) {
        int_list_push_front(&l, &i);
    }

    assert_uint_eq(int_list_size(&l), 10);

    for (int i = 9; i >= 0; --i) {
        int x;
        assert_int_eq(int_list_pop_front(&l, &x), LIBV_OK);
        assert_int_eq(x, i);
    }

    assert_uint_eq(int_list_size(&l), 0);

    int_list_destroy(&l);
}

TEST(list, insert_at) {
    int_list l = int_list_new();

    for (int i = 0; i < 10; ++i) {
        int_list_push_back(&l, &i);
    }

    assert_uint_eq(int_list_size(&l), 10);

    int x = 99999;
    int_list_insert_at_index(&l, &x, 5);
    assert_uint_eq(int_list_size(&l), 11);

    int exp[] = {0, 1, 2, 3, 4, 99999, 5, 6, 7, 8, 9};
    for (size_t i = 0; i < array_size(exp); ++i) {
        assert_int_eq(*int_list_get_at_index(&l, i), exp[i]);
    }

    int_list_destroy(&l);
}

VTEST_MAIN()
