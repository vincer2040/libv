#ifndef __LIBV_ARENA_H__

#define __LIBV_ARENA_H__

#include "libv/vmap/vmap.h"
#include <stddef.h>

LIBV_BEGIN

typedef struct arena_block arena_block;

struct arena_block {
    arena_block* next;
    size_t used;
    size_t size;
    unsigned char data[];
};

typedef struct {
    size_t num_blocks;   // number of blocks
    size_t alloc_size;   // total amount allocated
    size_t alloc_used;   // total amount used
    size_t alloc_wasted; // size lost to alignment and fragmentation
} arena_stats;

typedef struct {
    arena_block* head;
    arena_block* tail;
    arena_stats stats;
} arena;

#ifndef LIBV_ARENA_BLOCK_SIZE
#define LIBV_ARENA_BLOCK_SIZE 4096
#endif // LIBV_ARENA_BLOCK_SIZE

static inline bool is_power_of_two(size_t x) {
    return x != 0 && (x & (x - 1)) == 0;
}

static inline uintptr_t align_up(uintptr_t p, size_t align) {
    return (p + align - 1) & ~(uintptr_t)(align - 1);
}

static inline arena_block* arena_block_new(arena* arena, size_t size) {
    arena_block* self = malloc(sizeof *self + size);
    if (!self) {
        return NULL;
    }
    arena->stats.alloc_size += size;
    arena->stats.alloc_wasted += size;
    ++arena->stats.num_blocks;
    self->size = size;
    self->next = NULL;
    self->used = 0;
    return self;
}

static inline void arena_block_destroy(arena_block* self) { free(self); }

static inline arena arena_new(void) { return (arena){0}; }

static inline void arena_destroy(arena* self) {
    arena_block* current = self->head;
    while (true) {
        if (!current) {
            break;
        }

        arena_block* next = current->next;

        arena_block_destroy(current);

        current = next;
    }

    self->head = self->tail = NULL;
    self->stats.num_blocks = 0;
}

static inline int arena_maybe_initialize(arena* self, size_t size) {
    if (self->tail != NULL) {
        return LIBV_OK;
    }
    libv_assert(self->head == NULL,
                "arena in invalid state, tail == NULL but head != NULL");

    size = size > LIBV_ARENA_BLOCK_SIZE ? size : LIBV_ARENA_BLOCK_SIZE;

    arena_block* block = arena_block_new(self, size);
    if (block == NULL) {
        return LIBV_ERR;
    }

    self->head = self->tail = block;

    return LIBV_OK;
}

typedef struct {
    arena_block* block;
    uintptr_t aligned;
    size_t size_needed;
} arena_valid_block;

static inline arena_valid_block arena_get_valid_block(arena* self, size_t size,
                                                      size_t align) {
    arena_block* current = self->tail;
    uintptr_t ptr, aligned;
    size_t size_needed;
    while (true) {
        if (!current) {
            size_t alloc_size = size + align > LIBV_ARENA_BLOCK_SIZE
                                    ? size + align
                                    : LIBV_ARENA_BLOCK_SIZE;
            arena_block* block = arena_block_new(self, alloc_size);
            if (block == NULL) {
                return (arena_valid_block){0};
            }

            ptr = (uintptr_t)block->data;
            aligned = align_up(ptr, align);
            size_needed = aligned - ptr + size;

            libv_assert(
                size_needed <= block->size,
                "block created for size %zu but align made block too small",
                size);

            self->tail->next = block;
            self->tail = block;
            current = block;
            break;
        }

        ptr = (uintptr_t)(current->data + current->used);
        aligned = align_up(ptr, align);
        size_needed = aligned - ptr + size;

        if (size_needed <= current->size - current->used) {
            break;
        }

        current = current->next;
    }

    return (arena_valid_block){current, aligned, size_needed};
}

static inline void* arena_alloc_aligned(arena* self, size_t size,
                                        size_t align) {

    if (size == 0) {
        return NULL;
    }

    if (align < sizeof(void*)) {
        align = sizeof(void*);
    }

    if (!is_power_of_two(align)) {
        return NULL;
    }

    if (arena_maybe_initialize(self, size + align) == LIBV_ERR) {
        return NULL;
    }

    arena_valid_block res = arena_get_valid_block(self, size, align);

    if (res.block == NULL) {
        return NULL;
    }

    self->stats.alloc_wasted -= size;
    self->stats.alloc_used += size;
    res.block->used += res.size_needed;
    return (void*)res.aligned;
}

#define arena_alloc_type(arena_, type_)                                        \
    arena_alloc_aligned(arena_, sizeof(type_), _Alignof(type_))

#define arena_alloc_array(arena_, type_, count_)                               \
    arena_alloc_aligned(arena_, sizeof(type_) * count_, _Alignof(type_))

static inline void* arena_alloc(arena* self, size_t size) {
    return arena_alloc_aligned(self, size, _Alignof(max_align_t));
}

static inline void* arena_memcpy(void* dst, const void* src, size_t size) {
    unsigned char* d = dst;
    const unsigned char* s = src;
    for (; size; --size) {
        *d++ = *s++;
    }
    return dst;
}

static inline void* arena_realloc_aligned(arena* self, void* ptr,
                                          size_t old_size, size_t size,
                                          size_t align) {
    void* new_ptr = arena_alloc_aligned(self, size, align);
    if (new_ptr == NULL) {
        return NULL;
    }
    return arena_memcpy(new_ptr, ptr, old_size);
}

static inline void* arena_realloc(arena* self, void* ptr, size_t old_size,
                                  size_t size) {
    self->stats.alloc_used -= old_size;
    self->stats.alloc_wasted += old_size;
    return arena_realloc_aligned(self, ptr, old_size, size,
                                 _Alignof(max_align_t));
}

static inline void arena_reset(arena* self) {
    libv_assert(self->head, "passed uninitialized arena to arena_reset");
    for (arena_block* block = self->head; block != NULL; block = block->next) {
        block->used = 0;
    }
    self->stats.alloc_wasted = self->stats.alloc_size;
    self->stats.alloc_used = 0;
    self->tail = self->head;
}

LIBV_END

#endif // __LIBV_ARENA_H__
