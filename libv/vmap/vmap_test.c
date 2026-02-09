#include "libv/vec/vec.h"
#include "libv/vtest/vtest.h"
#include "vmap.h"

TEST(capacity, normalize_capacity) {
    assert_uint_eq(vmap_normalize_capacity(0), 16);
    assert_uint_eq(vmap_normalize_capacity(250), 256);
    assert_uint_eq(vmap_normalize_capacity(500), 512);
    assert_uint_eq(vmap_normalize_capacity(900), 1024);
    assert_uint_eq(vmap_normalize_capacity(1500), 2048);
    assert_uint_eq(vmap_normalize_capacity(3353), 4096);
    assert_uint_eq(vmap_normalize_capacity(7432), 8192);
}

VMAP_DECLARE_DEFAULT_SET(int_set, int);

TEST(vmap, insert1) {
    int_set t = int_set_new(0);

    int x = 0;
    int_set_insert_result res = int_set_insert(&t, &x);
    assert_true(res.inserted);
    assert_uint_eq(int_set_size(&t), 1);

    int_set_iter it = int_set_find(&t, &x);
    assert_ptr_nonnull(int_set_iter_get(&it));
    assert_int_eq(*int_set_iter_get(&it), x);

    int_set_destroy(&t);
}

TEST(vmap, insert2) {
    int_set t = int_set_new(0);

    int x = 0;
    int_set_insert_result res = int_set_insert(&t, &x);
    assert_true(res.inserted);
    assert_uint_eq(int_set_size(&t), 1);

    int_set_iter it = int_set_find(&t, &x);
    assert_ptr_nonnull(int_set_iter_get(&it));
    assert_int_eq(*int_set_iter_get(&it), x);

    x = 1;
    res = int_set_insert(&t, &x);
    assert_true(res.inserted);
    assert_uint_eq(int_set_size(&t), 2);

    it = int_set_find(&t, &x);
    assert_ptr_nonnull(int_set_iter_get(&it));
    assert_int_eq(*int_set_iter_get(&it), x);

    int_set_destroy(&t);
}

static inline size_t bad_set_hash(const void* key) {
    LIBV_UNUSED(key);
    return 0;
}

static inline bool bad_set_key_eq(const void* needle, const void* candidate) {
    return *((int*)needle) == *((int*)candidate);
}

VMAP_DECLARE_SET_SLOT(bad_set, int);
VMAP_DECLARE_DEFAULT_ALLOC_POLICY(bad_set);
VMAP_DECLARE_SLOT_POLICY(bad_set, bad_set_slot);
VMAP_DECLARE_DEFAULT_OBJECT_POLICY(bad_set, int);

static const vmap_key_policy bad_set_key = {
    .hash = bad_set_hash,
    .eq = bad_set_key_eq,
};

static const vmap_policy bad_set_policy = {
    .alloc = &bad_set_alloc_policy,
    .slot = &bad_set_slot_policy,
    .object = &bad_set_object_policy,
    .key = &bad_set_key,
};

VMAP_DECLARE_SET(bad_set, bad_set_policy, int);

TEST(vmap, collisions) {
    bad_set t = bad_set_new(0);

    int x = 0;
    bad_set_insert_result res = bad_set_insert(&t, &x);
    assert_true(res.inserted);
    assert_uint_eq(bad_set_size(&t), 1);

    bad_set_iter it = bad_set_find(&t, &x);
    assert_ptr_nonnull(bad_set_iter_get(&it));
    assert_int_eq(*bad_set_iter_get(&it), x);

    x = 1;
    res = bad_set_insert(&t, &x);
    assert_true(res.inserted);
    assert_uint_eq(bad_set_size(&t), 2);

    it = bad_set_find(&t, &x);
    assert_ptr_nonnull(bad_set_iter_get(&it));
    assert_int_eq(*bad_set_iter_get(&it), x);

    bad_set_destroy(&t);
}

TEST(vmap, collision_and_find_after_delete) {
    bad_set t = bad_set_new(0);

    const int num_inserts = 37;
    for (int i = 0; i < num_inserts; ++i) {
        bad_set_insert_result res = bad_set_insert(&t, &i);
        assert_true(res.inserted);
        assert_int_eq(*bad_set_iter_get(&res.it), i);
        assert_uint_eq(bad_set_size(&t), i + 1);
    }

    for (int i = 0; i < num_inserts; ++i) {
        assert_true(bad_set_erase(&t, &i));
        for (int j = i + 1; j < num_inserts; ++j) {
            bad_set_iter it = bad_set_find(&t, &j);
            assert_ptr_nonnull(bad_set_iter_get(&it));
            assert_int_eq(*bad_set_iter_get(&it), j);

            bad_set_insert_result ins = bad_set_insert(&t, &j);
            assert_false(ins.inserted);
            assert_int_eq(*bad_set_iter_get(&ins.it), j);

            assert_uint_eq(bad_set_size(&t), num_inserts - i - 1);
        }
    }

    assert_true(bad_set_is_empty(&t));

    bad_set_destroy(&t);
}

TEST(vmap, contains) {
    int_set t = int_set_new(0);

    int x = 0, y = 1;
    assert_false(int_set_contains(&t, &x));

    int_set_insert_result res = int_set_insert(&t, &x);
    assert_true(res.inserted);
    assert_true(int_set_contains(&t, &x));
    assert_false(int_set_contains(&t, &y));

    res = int_set_insert(&t, &y);
    assert_true(res.inserted);
    assert_true(int_set_contains(&t, &x));
    assert_true(int_set_contains(&t, &y));

    int_set_destroy(&t);
}

size_t max_density_size(size_t n) {
    int_set t = int_set_new(n);

    int x;
    for (size_t i = 0; i < n; ++i) {
        x = i;
        int_set_insert(&t, &x);
    }

    const size_t c = int_set_capacity(&t);

    int y;
    while (c == int_set_capacity(&t)) {
        y = n++;
        int_set_insert(&t, &y);
    }

    size_t res = int_set_size(&t) - 1;

    int_set_destroy(&t);

    return res;
}

VEC_DECLARE_DEFAULT(int_vec, int);

TEST(vmap, insert_erase_stress_test) {
    int_set t = int_set_new(0);

    const size_t min_element_count = 250;

    int_vec keys = int_vec_new();

    size_t i = 0;
    for (; i < max_density_size(min_element_count); ++i) {
        int x = i;
        int_set_insert(&t, &x);
        int_vec_push_back(&keys, &x);
    }

    const size_t max_iterations = 1000000;
    for (; i < max_iterations; ++i) {
        assert_true(int_set_erase(&t, int_vec_front(&keys)));
        int_vec_pop_front(&keys, NULL);
        int x = i;
        int_set_insert(&t, &x);
        int_vec_push_back(&keys, &x);
    }

    int_vec_free(&keys);
    int_set_destroy(&t);
}

TEST(vmap, large_table) {
    int_set t = int_set_new(0);

    for (int i = 0; i < 100000; i++) {
        int_set_insert(&t, &i);
    }

    for (int i = 0; i < 100000; ++i) {
        int_set_iter it = int_set_find(&t, &i);
        assert_ptr_nonnull(int_set_iter_get(&it));
        assert_int_eq(*int_set_iter_get(&it), i);
    }

    int_set_destroy(&t);
}

VMAP_DECLARE_DEFAULT_MAP(int_map, int, int);

TEST(vmap, insert_or_assign) {
    int_map t = int_map_new(0);

    int_map_entry e = { 1, 1 };

    int_map_insert_result r = int_map_insert_or_assign(&t, &e);
    assert_true(r.inserted);
    assert_mem_eq(&e, int_map_iter_get(&r.it), sizeof e);

    int x = 1;

    int_map_iter it = int_map_find(&t, &x);
    assert_ptr_nonnull(int_map_iter_get(&it));
    assert_mem_eq(&e, int_map_iter_get(&it), sizeof e);

    e.value = 2;
    r = int_map_insert_or_assign(&t, &e);
    assert_false(r.inserted);
    assert_mem_eq(&e, int_map_iter_get(&r.it), sizeof e);

    it = int_map_find(&t, &x);
    assert_ptr_nonnull(int_map_iter_get(&it));
    assert_mem_eq(&e, int_map_iter_get(&it), sizeof e);

    int_map_destroy(&t);
}

VTEST_MAIN()
