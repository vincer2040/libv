#include "libv/vtest/vtest.h"
#include "vmap.h"

VMAP_DECLARE_DEFAULT_SET(int_set, int);

TEST(vmap, empty) {
    int_set t = int_set_new(0);
    assert_uint_eq(int_set_size(&t), 0);
    assert_true(int_set_empty(&t));
    int_set_destroy(&t);
}

TEST(vmap, find_empty) {
    int_set t = int_set_new(0);
    int x = 0;
    int_set_citer f = int_set_find(&t, &x);
    assert_ptr_null(int_set_citer_get(&f));
    int_set_destroy(&t);
}

TEST(vmap, insert1) {
    int_set t = int_set_new(0);

    int x = 0;
    int_set_citer f = int_set_find(&t, &x);
    assert_ptr_null(int_set_citer_get(&f));

    assert_true(int_set_insert(&t, &x).inserted);

    f = int_set_find(&t, &x);
    assert_int_eq(*int_set_citer_get(&f), x);

    assert_uint_eq(int_set_size(&t), 1);
    int_set_destroy(&t);
}

TEST(vmap, insert2) {
    int_set t = int_set_new(0);

    int x = 0;
    int y = 1;

    int_set_citer f = int_set_find(&t, &x);
    assert_ptr_null(int_set_citer_get(&f));
    assert_true(int_set_insert(&t, &x).inserted);
    f = int_set_find(&t, &x);
    assert_int_eq(*int_set_citer_get(&f), x);
    assert_uint_eq(int_set_size(&t), 1);

    f = int_set_find(&t, &y);
    assert_ptr_null(int_set_citer_get(&f));
    assert_true(int_set_insert(&t, &y).inserted);
    f = int_set_find(&t, &y);
    assert_int_eq(*int_set_citer_get(&f), y);
    assert_uint_eq(int_set_size(&t), 2);

    f = int_set_find(&t, &x);
    assert_int_eq(*int_set_citer_get(&f), x);

    int_set_destroy(&t);
}

static inline void bad_set_object_copy(void* dst, const void* src) {
    memcpy(dst, src, sizeof(int));
}

static inline size_t bad_set_key_hash(const void* key) {
    LIBV_UNUSED(key);
    return 0;
}

static inline bool bad_set_key_eq(const void* needle, const void* candidate) {
    return *((int*)needle) == *((int*)candidate);
}

const vmap_alloc_policy bad_set_alloc = {
    .alloc = libv_default_alloc,
    .free = libv_default_free,
};

VMAP_DECLARE_SLOT_POLICY(bad_set, int);

const vmap_object_policy bad_set_object = {
    .size = sizeof(int),
    .align = _Alignof(int),
    .copy = bad_set_object_copy,
    .dtor = NULL,
};

const vmap_key_policy bad_set_key = {
    .hash = bad_set_key_hash,
    .eq = bad_set_key_eq,
};

const vmap_policy bad_set_policy = {
    .alloc = &bad_set_alloc,
    .slot = &bad_set_slot_policy,
    .object = &bad_set_object,
    .key = &bad_set_key,
};

VMAP_DECLARE_SET(bad_set, bad_set_policy, int);

TEST(vmap, collision) {
    bad_set t = bad_set_new(0);

    int x = 0;
    int y = 1;

    bad_set_citer f = bad_set_find(&t, &x);
    assert_ptr_null(bad_set_citer_get(&f));
    assert_true(bad_set_insert(&t, &x).inserted);
    f = bad_set_find(&t, &x);
    assert_int_eq(*bad_set_citer_get(&f), x);
    assert_uint_eq(bad_set_size(&t), 1);

    f = bad_set_find(&t, &y);
    assert_ptr_null(bad_set_citer_get(&f));
    assert_true(bad_set_insert(&t, &y).inserted);
    f = bad_set_find(&t, &y);
    assert_int_eq(*bad_set_citer_get(&f), y);
    assert_uint_eq(bad_set_size(&t), 2);

    f = bad_set_find(&t, &x);
    assert_int_eq(*bad_set_citer_get(&f), x);

    bad_set_destroy(&t);
}

TEST(vmap, insert_collision_and_find_after_delete) {
    bad_set t = bad_set_new(0);
    const int num_inserts = 37;
    for (int i = 0; i < num_inserts; ++i) {
        assert_true(bad_set_insert(&t, &i).inserted);
        assert_uint_eq(bad_set_size(&t), i + 1);
    }

    bad_set_dump(&t);

    for (int i = 0; i < num_inserts; ++i) {
        assert_true(bad_set_erase(&t, &i));
        for (int j = i + 1; j < num_inserts; ++j) {
            bad_set_citer f = bad_set_find(&t, &j);
            assert_int_eq(*bad_set_citer_get(&f), j);

            bad_set_insert_result fj = bad_set_insert(&t, &j);
            assert_int_eq(*bad_set_iter_get(&fj.it), j);
            assert_uint_eq(bad_set_size(&t), num_inserts - i - 1);
        }
    }

    assert_true(bad_set_empty(&t));

    bad_set_destroy(&t);
}

VTEST_MAIN()
