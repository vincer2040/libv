#ifndef __VMAP_H__

#define __VMAP_H__

#include "libv/base/base.h"
#include "libv/vmap/rapidhash.h"
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

#define VMAP_DECLARE_SLOT_POLICY(name_, key_)                                  \
    typedef struct {                                                           \
        vmap_ctrl_byte ctrl;                                                   \
        key_ key;                                                              \
    } name_##_slot;                                                            \
    static inline void name_##_slot_transfer(void* dst, void* src) {           \
        memcpy(dst, src, sizeof(name_##_slot));                                \
    }                                                                          \
    static inline void* name_##_slot_get(const void* slot) {                   \
        return &((name_##_slot*)slot)->key;                                    \
    }                                                                          \
    const vmap_slot_policy name_##_slot_policy = {                             \
        .size = sizeof(name_##_slot),                                          \
        .align = _Alignof(name_##_slot),                                       \
        .transfer = name_##_slot_transfer,                                     \
        .get = name_##_slot_get,                                               \
    }

#define VMAP_DECLARE_DEFAULT_POLICY_(name_, key_, slot_)                       \
    static inline void* name_##_default_alloc(size_t size) {                   \
        return libv_default_alloc(size);                                       \
    }                                                                          \
    static inline void name_##_default_free(void* ptr) {                       \
        libv_default_free(ptr);                                                \
    }                                                                          \
    static inline void name_##_default_slot_transfer(void* dst, void* src) {   \
        memcpy(dst, src, sizeof(slot_));                                       \
    }                                                                          \
    static inline void* name_##_default_slot_get(const void* slot) {           \
        return &((slot_*)slot)->key;                                           \
    }                                                                          \
    static inline void name_##_default_object_copy(void* dst,                  \
                                                   const void* src) {          \
        memcpy(dst, src, sizeof(key_));                                        \
    }                                                                          \
    static inline size_t name_##_default_key_hash(const void* key) {           \
        return rapidhash(key, sizeof(key_));                                   \
    }                                                                          \
    static inline bool name_##_default_key_eq(const void* needle,              \
                                              const void* candidate) {         \
        return memcmp(needle, candidate, sizeof(key_)) == 0;                   \
    }                                                                          \
    const vmap_alloc_policy name_##_alloc_policy = {                           \
        .alloc = name_##_default_alloc,                                        \
        .free = name_##_default_free,                                          \
    };                                                                         \
    const vmap_slot_policy name_##_slot_policy = {                             \
        .size = sizeof(slot_),                                                 \
        .align = _Alignof(slot_),                                              \
        .transfer = name_##_default_slot_transfer,                             \
        .get = name_##_default_slot_get,                                       \
    };                                                                         \
    const vmap_object_policy name_##_object_policy = {                         \
        .size = sizeof(key_),                                                  \
        .align = _Alignof(key_),                                               \
        .copy = name_##_default_object_copy,                                   \
        .dtor = NULL,                                                          \
    };                                                                         \
    const vmap_key_policy name_##_key_policy = {                               \
        .hash = name_##_default_key_hash,                                      \
        .eq = name_##_default_key_eq,                                          \
    };                                                                         \
    const vmap_policy name_##_policy = {                                       \
        .alloc = &name_##_alloc_policy,                                        \
        .slot = &name_##_slot_policy,                                          \
        .object = &name_##_object_policy,                                      \
        .key = &name_##_key_policy,                                            \
    };

#define VMAP_DECLARE_DEFAULT_SET_POLICY_(name_, key_)                          \
    typedef struct {                                                           \
        vmap_ctrl_byte ctrl;                                                   \
        key_ key;                                                              \
    } name_##_slot;                                                            \
    VMAP_DECLARE_DEFAULT_POLICY_(name_, key_, name_##_slot)

typedef uint8_t vmap_ctrl_byte;

#define vmap_emtpy 0
#define vmap_deleted (1 << 0)
#define vmap_full (1 << 1)
#define vmap_sentinel (1 << 2)

typedef struct {
    char* slots;
    size_t size;
    size_t capacity;
    size_t growth_left;
} vmap_raw;

static inline size_t vmap_capacity_to_growth(size_t capacity) {
    return capacity - capacity / 8;
}

static inline void vmap_reset_growth_left(vmap_raw* self) {
    self->growth_left = vmap_capacity_to_growth(self->capacity);
}

static inline size_t vmap_normalize_capacity(size_t capacity) {
    if (capacity <= 16) {
        return 16;
    }
    return 1 << vmap_leading_zeros(capacity - 1);
}

static inline bool vmap_is_empty(const vmap_ctrl_byte* ctrl) {
    return *ctrl == vmap_emtpy;
}

static inline bool vmap_is_deleted(const vmap_ctrl_byte* ctrl) {
    return *ctrl == vmap_deleted;
}

static inline bool vmap_is_full(const vmap_ctrl_byte* ctrl) {
    return *ctrl == vmap_full;
}

static inline bool vmap_is_sentinel(vmap_ctrl_byte* ctrl) {
    return *ctrl == vmap_sentinel;
}

LIBV_INLINE_ALWAYS static size_t
vmap_hash_key(const vmap_key_policy* key_policy, const vmap_raw* set,
              const void* key) {
    return key_policy->hash(key) & (set->capacity - 1);
}

static inline void vmap_raw_initialize_slots(const vmap_policy* policy,
                                             vmap_raw* self) {
    self->slots = policy->alloc->alloc(policy->slot->size * self->capacity);
    memset(self->slots, 0, policy->slot->size * self->capacity);
    *(self->slots + (self->capacity - 1) * policy->slot->size) = vmap_sentinel;
    vmap_reset_growth_left(self);
}

static inline vmap_raw vmap_raw_new(const vmap_policy* policy,
                                    size_t capacity) {
    vmap_raw self = {0};
    self.capacity = vmap_normalize_capacity(capacity);
    vmap_raw_initialize_slots(policy, &self);
    return self;
}

static inline void vmap_raw_destroy(const vmap_policy* policy, vmap_raw* self) {
    if (!self->capacity) {
        return;
    }

    if (policy->object->dtor) {
        for (size_t i = 0; i < self->capacity; ++i) {
            if (!vmap_is_full(
                    (vmap_ctrl_byte*)(self->slots + i * policy->slot->size))) {
                continue;
            }
            policy->object->dtor(
                policy->slot->get(self->slots + i * policy->slot->size));
        }
    }

    policy->alloc->free(self->slots);
    self->capacity = self->size = self->growth_left = 0;
}

static inline void vmap_raw_dump(const vmap_policy* policy,
                                 const vmap_raw* self) {
    fprintf(stderr, "ptr: %p, length: %zu, capacity: %zu, growth left: %zu\n",
            (void*)self->slots, self->size, self->capacity, self->growth_left);
    if (self->capacity == 0) {
        return;
    }

    for (size_t i = 0; i < self->capacity; ++i) {
        fprintf(stderr, "[%4zu] %p / ", i,
                (void*)(self->slots + i * policy->slot->size));
        switch (*(vmap_ctrl_byte*)(self->slots + i * policy->slot->size)) {
        case vmap_emtpy:
            fprintf(stderr, "empty\n");
            continue;
        case vmap_deleted:
            fprintf(stderr, "deleted\n");
            continue;
        case vmap_sentinel:
            fprintf(stderr, "sentinel\n");
            continue;
        default:
            break;
        }

        char* elem = policy->slot->get(self->slots + i * policy->slot->size);
        fprintf(stderr, " -> ");
        for (size_t j = 0; j < policy->object->size; ++j) {
            fprintf(stderr, " %02x", (unsigned char)elem[j]);
        }
        fprintf(stderr, "\n");
    }
}

static inline size_t vmap_raw_size(const vmap_raw* self) { return self->size; }

static inline bool vmap_raw_empty(const vmap_raw* self) { return !self->size; }

static inline size_t vmap_raw_capacity(const vmap_raw* self) {
    return self->capacity;
}

static inline size_t
vmap_raw_find_first_empty_or_deleted(const vmap_policy* policy,
                                     const vmap_raw* self, size_t hash) {
    size_t index = hash;
    while (true) {
        char* slot = self->slots + index * policy->slot->size;
        if (vmap_is_empty((vmap_ctrl_byte*)slot) ||
            vmap_is_deleted((vmap_ctrl_byte*)slot)) {
            return index;
        }
        index = (index + 1) & (self->capacity - 1);
    }
}

static inline void vmap_raw_rehash_and_grow(const vmap_policy* policy,
                                            vmap_raw* self) {
    const size_t old_capacity = self->capacity;
    char* old_slots = self->slots;

    self->capacity = old_capacity << 1;
    vmap_raw_initialize_slots(policy, self);
    vmap_reset_growth_left(self);

    for (size_t i = 0; i < old_capacity; ++i) {
        char* slot = old_slots + i * policy->slot->size;
        if (!vmap_is_full((vmap_ctrl_byte*)slot)) {
            continue;
        }
        size_t hash = vmap_hash_key(policy->key, self, policy->slot->get(slot));
        size_t target =
            vmap_raw_find_first_empty_or_deleted(policy, self, hash);
        policy->slot->transfer(self->slots + target * policy->slot->size, slot);
        self->growth_left -= 1;
    }

    policy->alloc->free(old_slots);
}

LIBV_INLINE_NEVER static size_t
vmap_raw_prepare_insert(const vmap_policy* policy, vmap_raw* self,
                        const void* value, size_t hash) {
    size_t target = vmap_raw_find_first_empty_or_deleted(policy, self, hash);
    if (LIBV_UNLIKELY(
            self->growth_left == 0 &&
            !vmap_is_deleted((vmap_ctrl_byte*)(self->slots +
                                               target * policy->slot->size)))) {
        vmap_raw_rehash_and_grow(policy, self);
        target = vmap_raw_find_first_empty_or_deleted(policy, self, hash);
    }

    vmap_ctrl_byte* ctrl =
        (vmap_ctrl_byte*)(self->slots + target * policy->slot->size);

    ++self->size;
    self->growth_left -= vmap_is_empty(
        (vmap_ctrl_byte*)(self->slots + target * policy->slot->size));

    *ctrl = vmap_full;
    return target;
}

typedef struct {
    size_t index;
    bool inserted;
} vmap_prepare_insert;

static inline vmap_prepare_insert
vmap_raw_find_or_prepare_insert(const vmap_policy* policy, vmap_raw* self,
                                const void* key) {
    size_t hash = vmap_hash_key(policy->key, self, key);
    size_t index = hash;
    while (true) {
        void* slot = self->slots + index * policy->slot->size;
        switch (*(vmap_ctrl_byte*)(slot)) {
        case vmap_emtpy:
            return (vmap_prepare_insert){
                vmap_raw_prepare_insert(policy, self, key, hash), true};
        case vmap_full:
            // TODO: do some testing to see if we can
            // wrap this in LIBV_LIKELY
            if (policy->key->eq(key, policy->slot->get(slot))) {
                return (vmap_prepare_insert){index, false};
            }
            break;
        default:
            break;
        }
        index = (index + 1) & (self->capacity - 1);
    }
    return (vmap_prepare_insert){0};
}

typedef struct {
    vmap_raw* set;
    char* slot;
} vmap_raw_iter;

static inline void* vmap_raw_iter_get(const vmap_policy* policy,
                                      const vmap_raw_iter* self) {
    if (!self->slot) {
        return NULL;
    }
    if (!vmap_is_full(((const vmap_ctrl_byte*)self->slot))) {
        return NULL;
    }
    return policy->slot->get(self->slot);
}

typedef struct {
    const vmap_raw* self;
    const char* slot;
} vmap_raw_citer;

static inline const void* vmap_raw_citer_get(const vmap_policy* policy,
                                             const vmap_raw_citer* self) {
    if (!self->slot) {
        return NULL;
    }
    if (!vmap_is_full(((const vmap_ctrl_byte*)self->slot))) {
        return NULL;
    }
    return policy->slot->get(self->slot);
}

static inline void
vmap_raw_citer_skip_empty_or_deleted(const vmap_policy* policy,
                                     vmap_raw_citer* self) {
    const char* slot = self->slot;
    while (true) {
        switch (*(const vmap_ctrl_byte*)slot) {
        case vmap_sentinel:
            slot = NULL;
            goto done;
        case vmap_full:
            goto done;
        default:
            break;
        }
        slot += policy->slot->size;
    }
done:
    self->slot = slot;
}

static inline void vmap_raw_citer_next(const vmap_policy* policy,
                                       vmap_raw_citer* self) {
    if (self->slot == NULL) {
        return;
    }
    vmap_raw_citer_skip_empty_or_deleted(policy, self);
}

static inline vmap_raw_citer vmap_raw_citer_at(const vmap_policy* policy,
                                               const vmap_raw* self,
                                               size_t index) {
    return (vmap_raw_citer){self, self->slots + index * policy->slot->size};
}

static inline vmap_raw_iter vmap_raw_iter_at(const vmap_policy* policy,
                                             vmap_raw* self, size_t index) {
    return (vmap_raw_iter){self, self->slots + index * policy->slot->size};
}

static inline void vmap_raw_iter_next(const vmap_policy* policy,
                                      vmap_raw_iter* self) {
    vmap_raw_citer_next(policy, (vmap_raw_citer*)self);
}

typedef struct {
    vmap_raw_iter it;
    bool inserted;
} vmap_insert;

static inline vmap_insert vmap_raw_insert(const vmap_policy* policy,
                                          vmap_raw* self, const void* value) {
    vmap_prepare_insert res =
        vmap_raw_find_or_prepare_insert(policy, self, value);
    if (res.inserted) {
        policy->object->copy(
            policy->slot->get(self->slots + res.index * policy->slot->size),
            value);
    }
    return (vmap_insert){vmap_raw_iter_at(policy, self, res.index),
                         res.inserted};
}

static inline vmap_raw_citer vmap_raw_find_hinted(const vmap_policy* policy,
                                                  const vmap_raw* self,
                                                  const void* key,
                                                  size_t hash) {
    while (true) {
        char* slot = self->slots + hash * policy->slot->size;
        switch (*((vmap_ctrl_byte*)slot)) {
        case vmap_emtpy:
            return (vmap_raw_citer){0};
        case vmap_full:
            if (policy->key->eq(key, policy->slot->get(slot))) {
                return vmap_raw_citer_at(policy, self, hash);
            }
        default:
            break;
        }
        hash = (hash + 1) & (self->capacity - 1);
    }
}

static inline vmap_raw_iter vmap_raw_find_mut_hinted(const vmap_policy* policy,
                                                     vmap_raw* self,
                                                     const void* key,
                                                     size_t hash) {
    while (true) {
        char* slot = self->slots + hash * policy->slot->size;
        switch (*((vmap_ctrl_byte*)slot)) {
        case vmap_emtpy:
            return (vmap_raw_iter){0};
        case vmap_full:
            if (policy->key->eq(key, policy->slot->get(slot))) {
                return vmap_raw_iter_at(policy, self, hash);
            }
        default:
            break;
        }
        hash = (hash + 1) & (self->capacity - 1);
    }
}

static inline vmap_raw_citer vmap_raw_find(const vmap_policy* policy,
                                           const vmap_raw* self,
                                           const void* key) {
    size_t hash = vmap_hash_key(policy->key, self, key);
    return vmap_raw_find_hinted(policy, self, key, hash);
}

static inline vmap_raw_iter vmap_raw_find_mut(const vmap_policy* policy,
                                              vmap_raw* self, const void* key) {
    size_t hash = vmap_hash_key(policy->key, self, key);
    return vmap_raw_find_mut_hinted(policy, self, key, hash);
}

static inline void vmap_raw_erase_at(const vmap_policy* policy,
                                     vmap_raw_iter* it) {
    if (policy->object->dtor) {
        policy->object->dtor(policy->slot->get(it->slot));
    }
    *it->slot = vmap_deleted;
    --it->set->size;
}

static inline bool vmap_raw_erase(const vmap_policy* policy, vmap_raw* self,
                                  const void* key) {
    vmap_raw_iter it = vmap_raw_find_mut(policy, self, key);
    if (it.slot == NULL) {
        return false;
    }
    vmap_raw_erase_at(policy, &it);
    return true;
}

static inline bool vmap_raw_contains(const vmap_policy* policy,
                                     const vmap_raw* self, const void* key) {
    vmap_raw_citer it = vmap_raw_find(policy, self, key);
    return it.slot == NULL;
}

#define VMAP_DECLARE_(name_, policy_, key_, type_)                             \
    LIBV_BEGIN                                                                 \
    typedef struct {                                                           \
        vmap_raw set;                                                          \
    } name_;                                                                   \
    static inline name_ name_##_new(size_t capacity) {                         \
        return (name_){vmap_raw_new(&policy_, capacity)};                      \
    }                                                                          \
    static inline void name_##_destroy(name_* self) {                          \
        vmap_raw_destroy(&policy_, &self->set);                                \
    }                                                                          \
    static inline void name_##_dump(const name_* self) {                       \
        vmap_raw_dump(&policy_, &self->set);                                   \
    }                                                                          \
    static inline size_t name_##_size(const name_* self) {                     \
        return vmap_raw_size(&self->set);                                      \
    }                                                                          \
    static inline bool name_##_empty(const name_* self) {                      \
        return vmap_raw_empty(&self->set);                                     \
    }                                                                          \
    typedef struct {                                                           \
        vmap_raw_iter it;                                                      \
    } name_##_iter;                                                            \
    static inline type_* name_##_iter_get(const name_##_iter* self) {          \
        return vmap_raw_iter_get(&policy_, &self->it);                         \
    }                                                                          \
    typedef struct {                                                           \
        vmap_raw_citer it;                                                     \
    } name_##_citer;                                                           \
    static inline const type_* name_##_citer_get(const name_##_citer* self) {  \
        return vmap_raw_citer_get(&policy_, &self->it);                        \
    }                                                                          \
    typedef struct {                                                           \
        name_##_iter it;                                                       \
        bool inserted;                                                         \
    } name_##_insert_result;                                                   \
    static inline name_##_insert_result name_##_insert(name_* self,            \
                                                       const type_* value) {   \
        vmap_insert res = vmap_raw_insert(&policy_, &self->set, value);        \
        return (name_##_insert_result){(name_##_iter){res.it}, res.inserted};  \
    }                                                                          \
    static inline name_##_citer name_##_find(const name_* self,                \
                                             const key_* key) {                \
        return (name_##_citer){vmap_raw_find(&policy_, &self->set, key)};      \
    }                                                                          \
    static inline name_##_iter name_##_find_mut(name_* self,                   \
                                                const key_* key) {             \
        return (name_##_iter){vmap_raw_find_mut(&policy_, &self->set, key)};   \
    }                                                                          \
    static inline bool name_##_erase(name_* self, const key_* key) {           \
        return vmap_raw_erase(&policy_, &self->set, key);                      \
    }                                                                          \
    LIBV_END                                                                   \
    /* Force a semicolon. */ struct name_##_needstrailingsemicolon_ { int x; }

#define VMAP_DECLARE_DEFAULT_SET(name_, key_)                                  \
    VMAP_DECLARE_DEFAULT_SET_POLICY_(name_, key_)                              \
    VMAP_DECLARE_(name_, name_##_policy, key_, key_)

#define VMAP_DECLARE_SET(name_, policy_, key_)                                 \
    VMAP_DECLARE_(name_, policy_, key_, key_)

LIBV_END

#endif // __VMAP_H__
