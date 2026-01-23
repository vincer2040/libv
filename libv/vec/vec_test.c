#include "libv/vtest/vtest.h"
#include "vec.h"

VEC_DECLARE_DEFAULT(int_vec, int);

TEST(vec, push_back) {
    int_vec v = int_vec_new();

    for (int i = 0; i < 10; ++i) {
        assert_int_eq(int_vec_push_back(&v, &i), LIBV_OK);
    }

    for (int i = 0; i < 10; ++i) {
        assert_int_eq(*int_vec_get_at(&v, i), i);
    }

    int_vec_free(&v);
}

TEST(vec, pop_back) {
    int_vec v = int_vec_new();

    for (int i = 0; i < 10; ++i) {
        assert_int_eq(int_vec_push_back(&v, &i), LIBV_OK);
    }

    for (int i = 9; i >= 0; --i) {
        int x;
        assert_int_eq(int_vec_pop_back(&v, &x), LIBV_OK);
        assert_int_eq(x, i);
    }

    assert_int_eq(int_vec_pop_back(&v, NULL), LIBV_ERR);

    int_vec_free(&v);
}

int main(void) {
    return vtest_run_tests();
}
