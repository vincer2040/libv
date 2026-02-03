#include "arena.h"
#include "libv/vtest/vtest.h"
#include <stdint.h>
#include <string.h>

TEST(arena, basic_alloc) {
    arena a = arena_new();

    void* ptr = arena_alloc(&a, 100);
    assert_ptr_nonnull(ptr);

    memset(ptr, 0xAB, 100);

    arena_destroy(&a);
}

TEST(arena, alloc_type) {
    arena a = arena_new();

    int* ptr = arena_alloc_type(&a, int);
    assert_ptr_nonnull(ptr);

    *ptr = 42;
    assert_int_eq(*ptr, 42);

    arena_destroy(&a);
}

TEST(arena, alloc_array) {
    arena a = arena_new();

    int* arr = arena_alloc_array(&a, int, 10);
    assert_ptr_nonnull(arr);

    for (int i = 0; i < 10; i++) {
        arr[i] = i * 2;
    }

    for (int i = 0; i < 10; i++) {
        assert_int_eq(arr[i], i * 2);
    }

    arena_destroy(&a);
}

TEST(arena, alignment) {
    arena a = arena_new();

    void* ptr1 = arena_alloc_aligned(&a, 1, 1);
    void* ptr2 = arena_alloc_aligned(&a, 1, 2);
    void* ptr4 = arena_alloc_aligned(&a, 1, 4);
    void* ptr8 = arena_alloc_aligned(&a, 1, 8);
    void* ptr16 = arena_alloc_aligned(&a, 1, 16);

    assert_ptr_nonnull(ptr1);
    assert_ptr_nonnull(ptr2);
    assert_ptr_nonnull(ptr4);
    assert_ptr_nonnull(ptr8);
    assert_ptr_nonnull(ptr16);

    assert_uint_eq((uintptr_t)ptr2 % 2, 0);
    assert_uint_eq((uintptr_t)ptr4 % 4, 0);
    assert_uint_eq((uintptr_t)ptr8 % 8, 0);
    assert_uint_eq((uintptr_t)ptr16 % 16, 0);

    arena_destroy(&a);
}

TEST(arena, struct_alignment) {
    typedef struct {
        char c;
        double d;
        int i;
    } test_struct;

    arena a = arena_new();

    test_struct* ptr = arena_alloc_type(&a, test_struct);
    assert_ptr_nonnull(ptr);
    assert_uint_eq((uintptr_t)ptr % _Alignof(test_struct), 0);

    ptr->c = 'A';
    ptr->d = 3.14159;
    ptr->i = 42;

    assert_int_eq(ptr->c, 'A');
    assert_double_eq(ptr->d, 3.14159);
    assert_int_eq(ptr->i, 42);

    arena_destroy(&a);
}

TEST(arena, multiple_allocs) {
    arena a = arena_new();

    void* ptrs[100];
    for (int i = 0; i < 100; i++) {
        ptrs[i] = arena_alloc(&a, 64);
        assert_ptr_nonnull(ptrs[i]);
        memset(ptrs[i], i & 0xFF, 64);
    }

    for (int i = 0; i < 100; i++) {
        for (int j = i + 1; j < 100; j++) {
            assert_true(ptrs[i] != ptrs[j]);
        }
    }

    arena_destroy(&a);
}

TEST(arena, large_alloc) {
    arena a = arena_new();

    size_t large_size = 1024 * 1024; // 1MB
    void* ptr = arena_alloc(&a, large_size);
    assert_ptr_nonnull(ptr);

    memset(ptr, 0xFF, large_size);

    arena_destroy(&a);
}

TEST(arena, zero_size_alloc) {
    arena a = arena_new();

    void* ptr = arena_alloc(&a, 0);
    assert_ptr_null(ptr);

    arena_destroy(&a);
}

TEST(arena, reset) {
    arena a = arena_new();

    void* ptr1 = arena_alloc(&a, 100);
    assert_ptr_nonnull(ptr1);

    size_t used_before = a.stats.alloc_used;
    assert_true(used_before > 0);

    arena_reset(&a);

    assert_uint_eq(a.stats.alloc_used, 0);
    assert_ptr_nonnull(a.head);

    void* ptr2 = arena_alloc(&a, 100);
    assert_ptr_nonnull(ptr2);

    arena_destroy(&a);
}

TEST(arena, reset_multiple_blocks) {
    arena a = arena_new();

    for (int i = 0; i < 100; i++) {
        void* ptr = arena_alloc(&a, 10000);
        assert_ptr_nonnull(ptr);
    }

    size_t num_blocks = a.stats.num_blocks;
    assert_true(num_blocks > 1);

    arena_reset(&a);

    assert_uint_eq(a.stats.alloc_used, 0);
    assert_uint_eq(a.stats.num_blocks, num_blocks); // Blocks preserved

    arena_destroy(&a);
}

TEST(arena, realloc_grow) {
    arena a = arena_new();

    void* ptr = arena_alloc(&a, 100);
    assert_ptr_nonnull(ptr);
    memset(ptr, 0xAB, 100);

    void* new_ptr = arena_realloc(&a, ptr, 100, 200);
    assert_ptr_nonnull(new_ptr);

    unsigned char* bytes = (unsigned char*)new_ptr;
    for (int i = 0; i < 100; i++) {
        assert_uint_eq(bytes[i], 0xAB);
    }

    arena_destroy(&a);
}

TEST(arena, realloc_shrink) {
    arena a = arena_new();

    void* ptr = arena_alloc(&a, 200);
    assert_ptr_nonnull(ptr);
    memset(ptr, 0xCD, 200);

    void* new_ptr = arena_realloc(&a, ptr, 200, 100);
    assert_ptr_nonnull(new_ptr);

    unsigned char* bytes = (unsigned char*)new_ptr;
    for (int i = 0; i < 100; i++) {
        assert_uint_eq(bytes[i], 0xCD);
    }

    arena_destroy(&a);
}

TEST(arena, realloc_last_alloc) {
    arena a = arena_new();

    void* ptr1 = arena_alloc(&a, 50);
    void* ptr2 = arena_alloc(&a, 100);
    assert_ptr_nonnull(ptr1);
    assert_ptr_nonnull(ptr2);

    memset(ptr2, 0xEF, 100);

    void* new_ptr = arena_realloc(&a, ptr2, 100, 200);
    assert_ptr_nonnull(new_ptr);

    unsigned char* bytes = (unsigned char*)new_ptr;
    for (int i = 0; i < 100; i++) {
        assert_uint_eq(bytes[i], 0xEF);
    }

    arena_destroy(&a);
}

TEST(arena, realloc_null) {
    arena a = arena_new();

    void* ptr = arena_realloc(&a, NULL, 0, 100);
    assert_ptr_nonnull(ptr);

    arena_destroy(&a);
}

TEST(arena, stats_tracking) {
    arena a = arena_new();

    assert_uint_eq(a.stats.alloc_used, 0);

    arena_alloc(&a, 100);
    assert_true(a.stats.alloc_used >= 100);
    assert_true(a.stats.alloc_size > 0);
    assert_true(a.stats.num_blocks > 0);

    size_t used_after_first = a.stats.alloc_used;

    arena_alloc(&a, 200);
    assert_true(a.stats.alloc_used >= used_after_first + 200);

    arena_destroy(&a);
}

TEST(arena, stats_after_reset) {
    arena a = arena_new();

    arena_alloc(&a, 100);
    size_t alloc_size = a.stats.alloc_size;
    size_t num_blocks = a.stats.num_blocks;

    arena_reset(&a);

    assert_uint_eq(a.stats.alloc_used, 0);
    assert_uint_eq(a.stats.alloc_size, alloc_size);
    assert_uint_eq(a.stats.num_blocks, num_blocks);

    arena_destroy(&a);
}

TEST(arena, many_small_allocs) {
    arena a = arena_new();

    void* ptrs[1000];
    for (int i = 0; i < 1000; i++) {
        ptrs[i] = arena_alloc(&a, 16);
        assert_ptr_nonnull(ptrs[i]);
        *(int*)ptrs[i] = i;
    }

    for (int i = 0; i < 1000; i++) {
        assert_int_eq(*(int*)ptrs[i], i);
    }

    arena_destroy(&a);
}

TEST(arena, alternating_sizes) {
    arena a = arena_new();

    for (int i = 0; i < 100; i++) {
        size_t size = (i % 2 == 0) ? 16 : 1024;
        void* ptr = arena_alloc(&a, size);
        assert_ptr_nonnull(ptr);
        memset(ptr, i & 0xFF, size);
    }

    arena_destroy(&a);
}

TEST(arena, mixed_alignments) {
    arena a = arena_new();

    char* c = arena_alloc_type(&a, char);
    double* d = arena_alloc_type(&a, double);
    int* i = arena_alloc_type(&a, int);
    long long* ll = arena_alloc_type(&a, long long);

    assert_ptr_nonnull(c);
    assert_ptr_nonnull(d);
    assert_ptr_nonnull(i);
    assert_ptr_nonnull(ll);

    assert_uint_eq((uintptr_t)d % _Alignof(double), 0);
    assert_uint_eq((uintptr_t)i % _Alignof(int), 0);
    assert_uint_eq((uintptr_t)ll % _Alignof(long long), 0);

    *c = 'X';
    *d = 2.718;
    *i = 123;
    *ll = 9876543210LL;

    assert_int_eq(*c, 'X');
    assert_double_eq(*d, 2.718);
    assert_int_eq(*i, 123);
    assert_int_eq(*ll, 9876543210LL);

    arena_destroy(&a);
}

TEST(arena, destroy_and_recreate) {
    arena a = arena_new();

    arena_alloc(&a, 1000);
    assert_true(a.stats.alloc_used > 0);

    arena_destroy(&a);

    a = arena_new();
    assert_uint_eq(a.stats.alloc_used, 0);
    assert_uint_eq(a.stats.num_blocks, 0);

    void* ptr = arena_alloc(&a, 100);
    assert_ptr_nonnull(ptr);

    arena_destroy(&a);
}

TEST(arena, power_of_2_alignments) {
    arena a = arena_new();

    size_t alignments[] = {1, 2, 4, 8, 16, 32, 64, 128, 256};

    for (size_t i = 0; i < sizeof(alignments) / sizeof(alignments[0]); i++) {
        void* ptr = arena_alloc_aligned(&a, 1, alignments[i]);
        assert_ptr_nonnull(ptr);
        assert_uint_eq((uintptr_t)ptr % alignments[i], 0);
    }

    arena_destroy(&a);
}

TEST(arena, string_storage) {
    arena a = arena_new();

    const char* original = "Hello, Arena!";
    size_t len = strlen(original) + 1;

    char* stored = arena_alloc_array(&a, char, len);
    assert_ptr_nonnull(stored);

    strcpy(stored, original);
    assert_str_eq(stored, original);

    arena_destroy(&a);
}

TEST(arena, multiple_strings) {
    arena a = arena_new();

    const char* strings[] = {"First string", "Second string", "Third string",
                             "Fourth string"};

    char* stored[4];
    for (int i = 0; i < 4; i++) {
        size_t len = strlen(strings[i]) + 1;
        stored[i] = arena_alloc_array(&a, char, len);
        strcpy(stored[i], strings[i]);
    }

    for (int i = 0; i < 4; i++) {
        assert_str_eq(stored[i], strings[i]);
    }

    arena_destroy(&a);
}

TEST(arena, nested_structures) {
    typedef struct {
        int id;
        char name[32];
    } person;

    typedef struct {
        person* people;
        size_t count;
    } group;

    arena a = arena_new();

    group* g = arena_alloc_type(&a, group);
    assert_ptr_nonnull(g);

    g->count = 5;
    g->people = arena_alloc_array(&a, person, g->count);
    assert_ptr_nonnull(g->people);

    for (size_t i = 0; i < g->count; i++) {
        g->people[i].id = (int)i;
        snprintf(g->people[i].name, sizeof(g->people[i].name), "Person %zu", i);
    }

    for (size_t i = 0; i < g->count; i++) {
        assert_int_eq(g->people[i].id, (int)i);
    }

    arena_destroy(&a);
}

TEST(arena, stress_alloc_reset) {
    arena a = arena_new();

    for (int round = 0; round < 10; round++) {
        for (int i = 0; i < 100; i++) {
            void* ptr = arena_alloc(&a, 128);
            assert_ptr_nonnull(ptr);
            memset(ptr, i & 0xFF, 128);
        }
        arena_reset(&a);
    }

    arena_destroy(&a);
}

VTEST_MAIN()
