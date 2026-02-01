#ifndef __VMAP_H__

#define __VMAP_H__

#include "libv/base/base.h"
#include "libv/vmap/rapidhash.h"
#include "libv/vstr/vstr.h"
#include <assert.h>
#include <memory.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

LIBV_BEGIN

static inline uint32_t vmap_leading_zeros64(uint64_t x) {
#if LIBV_HAVE_CLANG_BUILTIN(__builtin_clzll) || LIBV_IS_GCC
    static_assert(sizeof(unsigned long long) == sizeof x,
                  "__builtin_clzll does not take 64 but arg");
    return x == 0 ? 64 : __builtin_clzll(x);
#elif LIBV_IS_MSVC
    unsigned long result = 0;
#if defined(_M_X64) || defined(_M_ARM64)
    if (_BitScanReverse64(&result, x)) {
        return 63 - result;
    }
#else
    unsigned long result = 0;
    if ((x >> 32) && _BitScanReverse(&result, (unsigned long)(x >> 32))) {
        return 31 - result;
    }
    if (_BitScanReverse(&result, static_cast<unsigned long>(x))) {
        return 63 - result;
    }
#endif
    return 64;
#else
    uint32_t zeroes = 60;
    if (x >> 32) {
        zeroes -= 32;
        x >>= 32;
    }
    if (x >> 16) {
        zeroes -= 16;
        x >>= 16;
    }
    if (x >> 8) {
        zeroes -= 8;
        x >>= 8;
    }
    if (x >> 4) {
        zeroes -= 4;
        x >>= 4;
    }
    return "\4\3\2\2\1\1\1\1\0\0\0\0\0\0\0"[x] + zeroes;
#endif
}

#define vmap_leading_zeros(x_)                                                 \
    (vmap_leading_zeros64(x_) -                                                \
     (uint32_t)((sizeof(unsigned long long) - sizeof(x_)) * 8))

typedef uint8_t vmap_control_byte;

// clang-format off
#define vmap_empty 0
#define vmap_full (1 << 0)
#define vmap_deleted (1 << 1)
#define vmap_sentinel (1 << 2) // this is only used by _iter to tell us to stop iterating
// clang-format on

static inline vmap_control_byte vmap_control_byte_from_slot(const void* slot) {
    return *((const vmap_control_byte*)slot);
}

static inline bool vmap_is_empty(vmap_control_byte ctrl) {
    return ctrl == vmap_empty;
}

static inline bool vmap_is_full(vmap_control_byte ctrl) {
    return ctrl == vmap_full;
}

static inline bool vmap_is_deleted(vmap_control_byte ctrl) {
    return ctrl == vmap_deleted;
}

static inline bool vmap_is_sentinel(vmap_control_byte ctrl) {
    return ctrl == vmap_sentinel;
}

typedef struct {
    void* (*alloc)(size_t size);
    void (*free)(void* ptr);
} vmap_alloc_policy;

typedef struct {
    size_t size;
    size_t align;
    void (*transfer)(void* dst, void* src);
    void* (*get)(const void* slot);
} vmap_slot_policy;

typedef struct {
    size_t size;
    size_t align;
    void (*copy)(void* dst, const void* src);
    void (*dtor)(void* value);
} vmap_object_policy;

typedef struct {
    size_t (*hash)(const void* key);
    bool (*eq)(const void* needle, const void* candidate);
} vmap_key_policy;

typedef struct {
    const vmap_alloc_policy* alloc;
    const vmap_slot_policy* slot;
    const vmap_object_policy* object;
    const vmap_key_policy* key;
} vmap_policy;

#define VMAP_DECLARE_DEFAULT_ALLOC_POLICY(name_)                               \
    static inline void* name_##_default_alloc(size_t size) {                   \
        return libv_default_alloc(size);                                       \
    }                                                                          \
    static inline void name_##_default_free(void* ptr) {                       \
        libv_default_free(ptr);                                                \
    }                                                                          \
    static const vmap_alloc_policy name_##_alloc_policy = {                    \
        .alloc = name_##_default_alloc,                                        \
        .free = name_##_default_free,                                          \
    }

#define VMAP_DECLARE_SET_SLOT(name_, key_)                                     \
    typedef struct {                                                           \
        vmap_control_byte ctrl;                                                \
        key_ key;                                                              \
    } name_##_slot

#define VMAP_DECLARE_SLOT_POLICY(name_, slot_)                                 \
    static inline void name_##_slot_transfer(void* dst, void* src) {           \
        memcpy(dst, src, sizeof(slot_));                                       \
    }                                                                          \
    static inline void* name_##_slot_get(const void* slot) {                   \
        return &((slot_*)slot)->key;                                           \
    }                                                                          \
    static const vmap_slot_policy name_##_slot_policy = {                      \
        .size = sizeof(slot_),                                                 \
        .align = _Alignof(slot_),                                              \
        .transfer = name_##_slot_transfer,                                     \
        .get = name_##_slot_get,                                               \
    }

#define VMAP_DECLARE_DEFAULT_OBJECT_POLICY(name_, type_)                       \
    static inline void name_##_default_object_copy(void* dst,                  \
                                                   const void* src) {          \
        memcpy(dst, src, sizeof(type_));                                       \
    }                                                                          \
    static const vmap_object_policy name_##_object_policy = {                  \
        .size = sizeof(type_),                                                 \
        .align = _Alignof(type_),                                              \
        .copy = name_##_default_object_copy,                                   \
        .dtor = NULL,                                                          \
    }

#define VMAP_DECLARE_DEFAULT_KEY_POLICY(name_, key_)                           \
    static inline size_t name_##_default_key_hash(const void* key) {           \
        return rapidhash(key, sizeof(key_));                                   \
    }                                                                          \
    static inline bool name_##_default_key_eq(const void* needle,              \
                                              const void* candidate) {         \
        return memcmp(needle, candidate, sizeof(key_)) == 0;                   \
    }                                                                          \
    static const vmap_key_policy name_##_key_policy = {                        \
        .hash = name_##_default_key_hash,                                      \
        .eq = name_##_default_key_eq,                                          \
    }

#define VMAP_DECLARE_DEFAULT_POLICY_(name_, key_, type_, slot_)                \
    LIBV_BEGIN                                                                 \
    VMAP_DECLARE_DEFAULT_ALLOC_POLICY(name_);                                  \
    VMAP_DECLARE_SLOT_POLICY(name_, slot_);                                    \
    VMAP_DECLARE_DEFAULT_OBJECT_POLICY(name_, type_);                          \
    VMAP_DECLARE_DEFAULT_KEY_POLICY(name_, key_);                              \
    LIBV_END                                                                   \
    static const vmap_policy name_##_policy = {                                \
        .alloc = &name_##_alloc_policy,                                        \
        .slot = &name_##_slot_policy,                                          \
        .object = &name_##_object_policy,                                      \
        .key = &name_##_key_policy,                                            \
    }

#define VMAP_DECLARE_DEFAULT_SET_POLICY(name_, key_)                           \
    VMAP_DECLARE_SET_SLOT(name_, key_);                                        \
    VMAP_DECLARE_DEFAULT_POLICY_(name_, key_, key_, name_##_slot)

static inline size_t vmap_normalize_capacity(size_t capacity) {
    if (capacity <= 16) {
        return 16;
    }
    return 1ULL << ((sizeof(capacity) * 8) - vmap_leading_zeros64(capacity));
}

static inline size_t vmap_growth_to_capacity(size_t capacity) {
    return capacity - capacity / 8;
}

typedef struct {
    char* slots;
    size_t capacity;
    size_t size;
    size_t growth_left;
} vmap_raw;

static size_t vmap_raw_find_first_non_full(const vmap_policy* policy,
                                           const vmap_raw* self, size_t hash) {
    size_t index = hash;
    while (true) {
        const void* slot = self->slots + index * policy->slot->size;
        vmap_control_byte ctrl = vmap_control_byte_from_slot(slot);
        if (vmap_is_empty(ctrl) || vmap_is_deleted(ctrl)) {
            return index;
        }
        index = (index + 1) & (self->capacity - 1);
    }
}

static inline void vmap_reset_growth_left(vmap_raw* self) {
    self->growth_left = vmap_growth_to_capacity(self->capacity) - self->size;
}

static inline void vmap_initialize_slots(const vmap_policy* policy,
                                         vmap_raw* self) {
    char* mem = policy->alloc->alloc(policy->slot->size * self->capacity);
    memset(mem, vmap_empty, policy->slot->size * self->capacity);
    self->slots = mem;
    vmap_reset_growth_left(self);
}

static inline size_t vmap_hash_key(const vmap_policy* policy,
                                   const vmap_raw* self, const void* key) {
    return policy->key->hash(key) & (self->capacity - 1);
}

static inline void vmap_raw_dump(const vmap_policy* policy,
                                 const vmap_raw* self) {
    fprintf(stderr, "ptr: %p, len: %zu, cap: %zu, growth: %zu\n",
            (void*)self->slots, self->size, self->capacity, self->growth_left);
    if (self->capacity == 0) {
        return;
    }
    for (size_t i = 0; i < self->capacity; ++i) {
        const void* slot = self->slots + i * policy->slot->size;
        fprintf(stderr, "[%4zu] %p / ", i, slot);
        switch (vmap_control_byte_from_slot(slot)) {
        case vmap_sentinel:
            fprintf(stderr, "sentinel: //\n");
            continue;
        case vmap_empty:
            fprintf(stderr, "   empty\n");
            continue;
        case vmap_deleted:
            fprintf(stderr, " deleted\n");
            continue;
        default:
            fprintf(stderr, "    full");
            break;
        }

        const char* elem = (char*)policy->slot->get(slot);
        fprintf(stderr, ": %p /", (const void*)elem);
        fprintf(stderr, " ->");
        for (size_t j = 0; j < policy->object->size; ++j) {
            fprintf(stderr, " %02x", (unsigned char)elem[j]);
        }
        fprintf(stderr, "\n");
    }
}

static inline vmap_raw vmap_raw_new(const vmap_policy* policy,
                                    size_t capacity) {
    vmap_raw self = {0};
    self.capacity = vmap_normalize_capacity(capacity);
    vmap_initialize_slots(policy, &self);
    return self;
}

static inline size_t vmap_raw_size(const vmap_raw* self) { return self->size; }

static inline size_t vmap_raw_capacity(const vmap_raw* self) {
    return self->capacity;
}

static inline bool vmap_raw_is_empty(const vmap_raw* self) {
    return self->size == 0;
}

static inline void vmap_raw_destroy_slots(const vmap_policy* policy,
                                          vmap_raw* self) {
    if (policy->object->dtor) {
        for (size_t i = 0; i < self->capacity; ++i) {
            policy->object->dtor(
                policy->slot->get(self->slots + i * policy->slot->size));
        }
    }
    memset(self->slots, vmap_empty, self->capacity * policy->slot->size);
    self->size = 0;
    vmap_reset_growth_left(self);
}

static inline void vmap_raw_destroy(const vmap_policy* policy, vmap_raw* self) {
    vmap_raw_destroy_slots(policy, self);
    policy->alloc->free(self->slots);
    self->size = self->capacity = self->growth_left = 0;
}

static inline void vmap_raw_rehash_and_grow(const vmap_policy* policy,
                                            vmap_raw* self,
                                            size_t new_capacity) {
    char* old_slots = self->slots;
    const size_t old_capacity = self->capacity;

    self->capacity = new_capacity;
    vmap_initialize_slots(policy, self);

    for (size_t i = 0; i < old_capacity; ++i) {
        char* slot = old_slots + i * policy->slot->size;
        if (!vmap_is_full(vmap_control_byte_from_slot(slot))) {
            continue;
        }

        size_t hash = vmap_hash_key(policy, self, policy->slot->get(slot));

        size_t target = vmap_raw_find_first_non_full(policy, self, hash);

        policy->slot->transfer(self->slots + target * policy->slot->size, slot);
    }

    policy->alloc->free(old_slots);
}

typedef struct {
    const vmap_raw* self;
    const char* slot;
} vmap_raw_iter;

static inline vmap_raw_iter vmap_raw_iter_at(const vmap_policy* policy,
                                             const vmap_raw* self,
                                             size_t index) {
    return (vmap_raw_iter){self, self->slots + index * policy->slot->size};
}

static inline const void* vmap_raw_iter_get(const vmap_policy* policy,
                                            const vmap_raw_iter* it) {
    if (it->slot == NULL) {
        return NULL;
    }
    return policy->slot->get(it->slot);
}

typedef struct {
    vmap_raw* self;
    char* slot;
} vmap_raw_iter_mut;

LIBV_INLINE_NEVER static size_t
vmap_raw_prepare_insert(const vmap_policy* policy, vmap_raw* self,
                        const void* key, size_t hash) {
    size_t target = vmap_raw_find_first_non_full(policy, self, hash);
    char* slot = self->slots + target * policy->slot->size;
    if (LIBV_UNLIKELY(self->growth_left == 0 &&
                      !vmap_is_deleted(vmap_control_byte_from_slot(slot)))) {
        vmap_raw_rehash_and_grow(policy, self, self->capacity * 2);
        hash = vmap_hash_key(policy, self, key);
        target = vmap_raw_find_first_non_full(policy, self, hash);
        slot = self->slots + target * policy->slot->size;
    }
    ++self->size;
    self->growth_left -= vmap_is_empty(vmap_control_byte_from_slot(slot));
    *((vmap_control_byte*)slot) = vmap_full;
    return target;
}

typedef struct {
    size_t index;
    bool inserted;
} vmap_prepare_insert;

static inline vmap_prepare_insert
vmap_raw_find_or_prepare_insert(const vmap_policy* policy, vmap_raw* self,
                                const void* value, size_t hash) {
    size_t index = hash;
    while (true) {
        char* slot = self->slots + index * policy->slot->size;
        vmap_control_byte ctrl = vmap_control_byte_from_slot(slot);
        if (vmap_is_empty(ctrl)) {
            return (vmap_prepare_insert){
                vmap_raw_prepare_insert(policy, self, value, index), true};
        }
        if (vmap_is_full(ctrl)) {
            if (policy->key->eq(value, policy->slot->get(slot))) {
                return (vmap_prepare_insert){index, false};
            }
        }
        index = (index + 1) & (self->capacity - 1);
    }
}

typedef struct {
    vmap_raw_iter it;
    bool inserted;
} vmap_raw_insert_result;

static inline vmap_raw_insert_result
vmap_raw_insert(const vmap_policy* policy, vmap_raw* self, const void* value) {
    size_t hash = vmap_hash_key(policy, self, value);
    vmap_prepare_insert res =
        vmap_raw_find_or_prepare_insert(policy, self, value, hash);
    if (res.inserted) {
        policy->object->copy(
            policy->slot->get(self->slots + res.index * policy->slot->size),
            value);
    }
    return (vmap_raw_insert_result){vmap_raw_iter_at(policy, self, res.index),
                                    res.inserted};
}

static inline vmap_raw_iter vmap_raw_find_hinted(const vmap_policy* policy,
                                                 const vmap_raw* self,
                                                 const void* key, size_t hash) {
    size_t index = hash;
    while (true) {
        char* slot = self->slots + index * policy->slot->size;
        vmap_control_byte ctrl = vmap_control_byte_from_slot(slot);
        if (vmap_is_empty(ctrl)) {
            return (vmap_raw_iter){0};
        }
        if (vmap_is_full(ctrl)) {
            if (policy->key->eq(key, policy->slot->get(slot))) {
                return vmap_raw_iter_at(policy, self, index);
            }
        }
        index = (index + 1) & (self->capacity - 1);
    }
}

static inline vmap_raw_iter vmap_raw_find(const vmap_policy* policy,
                                          const vmap_raw* self,
                                          const void* key) {
    size_t hash = vmap_hash_key(policy, self, key);
    return vmap_raw_find_hinted(policy, self, key, hash);
}

static inline void vmap_raw_erase_at(const vmap_policy* policy, vmap_raw* self,
                                     const vmap_raw_iter_mut* it) {
    if (policy->object->dtor) {
        policy->object->dtor(
            (void*)vmap_raw_iter_get(policy, (vmap_raw_iter*)it));
    }
    --self->size;
    *it->slot = vmap_deleted;
}

static inline bool vmap_raw_erase(const vmap_policy* policy, vmap_raw* self,
                                  const void* key) {
    vmap_raw_iter it = vmap_raw_find(policy, self, key);
    if (it.slot == NULL) {
        return false;
    }
    vmap_raw_erase_at(policy, self, (vmap_raw_iter_mut*)&it);
    return true;
}

static inline bool vmap_raw_contains(const vmap_policy* policy,
                                     const vmap_raw* self, const void* key) {
    return vmap_raw_find(policy, self, key).slot != NULL;
}

#define VMAP_DECLARE_(name_, policy_, key_, type_)                             \
    LIBV_BEGIN                                                                 \
    typedef struct {                                                           \
        vmap_raw set;                                                          \
    } name_;                                                                   \
    static inline name_ name_##_new(size_t capacity) {                         \
        return (name_){vmap_raw_new(&policy_, capacity)};                      \
    }                                                                          \
    static inline void name_##_dump(name_* self) {                             \
        vmap_raw_dump(&policy_, &self->set);                                   \
    }                                                                          \
    static inline void name_##_destroy(name_* self) {                          \
        vmap_raw_destroy(&policy_, &self->set);                                \
    }                                                                          \
    static inline size_t name_##_size(const name_* self) {                     \
        return vmap_raw_size(&self->set);                                      \
    }                                                                          \
    static inline size_t name_##_capacity(const name_* self) {                 \
        return vmap_raw_capacity(&self->set);                                  \
    }                                                                          \
    static inline bool name_##_is_empty(const name_* self) {                   \
        return vmap_raw_is_empty(&self->set);                                  \
    }                                                                          \
    typedef struct {                                                           \
        vmap_raw_iter it;                                                      \
    } name_##_iter;                                                            \
    static inline const type_* name_##_iter_get(const name_##_iter* it) {      \
        return (const type_*)vmap_raw_iter_get(&policy_, &it->it);             \
    }                                                                          \
    typedef struct {                                                           \
        name_##_iter it;                                                       \
        bool inserted;                                                         \
    } name_##_insert_result;                                                   \
    static inline name_##_insert_result name_##_insert(name_* self,            \
                                                       const type_* value) {   \
        vmap_raw_insert_result res =                                           \
            vmap_raw_insert(&policy_, &self->set, value);                      \
        return (name_##_insert_result){(name_##_iter){res.it}, res.inserted};  \
    }                                                                          \
    static inline name_##_iter name_##_find(const name_* self,                 \
                                            const key_* key) {                 \
        return (name_##_iter){vmap_raw_find(&policy_, &self->set, key)};       \
    }                                                                          \
    static inline bool name_##_erase(name_* self, const key_* key) {           \
        return vmap_raw_erase(&policy_, &self->set, key);                      \
    }                                                                          \
    static inline bool name_##_contains(name_* self, const key_* key) {        \
        return vmap_raw_contains(&policy_, &self->set, key);                   \
    }                                                                          \
    LIBV_END                                                                   \
    /* Force a semicolon. */ struct name_##_needstrailingsemicolon_ { int x; }

#define VMAP_DECLARE_DEFAULT_SET(name_, key_)                                  \
    VMAP_DECLARE_DEFAULT_SET_POLICY(name_, key_);                              \
    VMAP_DECLARE_(name_, name_##_policy, key_, key_)

#define VMAP_DECLARE_SET(name_, policy_, key_)                                 \
    VMAP_DECLARE_(name_, policy_, key_, key_)

LIBV_END

#endif // __VMAP_H__
