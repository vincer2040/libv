#ifndef __VEC_H__

#define __VEC_H__

#include "libv/base/base.h"
#include <memory.h>
#include <stdbool.h>
#include <stddef.h>

LIBV_BEGIN

typedef struct {
    char* data;
    size_t size;
    size_t capacity;
} vec_raw;

typedef struct {
    void* (*alloc)(size_t size);
    void* (*realloc)(void* ptr, size_t size);
    void (*free)(void* ptr);
} vec_alloc_policy;

typedef struct {
    size_t size;
    size_t align;
    void (*copy)(void* dst, const void* src);
    bool (*eq)(const void* needle, const void* candidate);
    void (*dtor)(void* value);
} vec_object_policy;

typedef struct {
    const vec_alloc_policy* alloc;
    const vec_object_policy* obj;
} vec_policy;

static inline vec_raw vec_raw_new(void) {
    vec_raw self = {0};
    return self;
}

static inline void vec_raw_free(const vec_policy* policy, vec_raw* self) {
    if (policy->obj->dtor) {
        for (size_t i = 0; i < self->size; ++i) {
            policy->obj->dtor(self->data + (policy->obj->size * i));
        }
    }
    policy->alloc->free(self->data);
    self->size = 0;
    self->capacity = 0;
}

static inline size_t vec_raw_size(const vec_raw* self) { return self->size; }

static inline size_t vec_raw_capacity(const vec_raw* self) {
    return self->capacity;
}

static inline int vec_raw_maybe_resize(const vec_policy* policy,
                                       vec_raw* self) {
    if (self->size < self->capacity) {
        return LIBV_OK;
    }
    size_t new_capacity = self->capacity == 0 ? 4 : self->capacity << 1;
    void* tmp =
        policy->alloc->realloc(self->data, new_capacity * policy->obj->size);
    if (!tmp) {
        return LIBV_ERR;
    }
    self->data = tmp;
    self->capacity = new_capacity;
    return LIBV_OK;
}

static inline int vec_raw_push_back(const vec_policy* policy, vec_raw* self,
                                    const void* value) {
    if (vec_raw_maybe_resize(policy, self) == LIBV_ERR) {
        return LIBV_ERR;
    }
    memcpy(self->data + (self->size * policy->obj->size), value,
           policy->obj->size);
    self->size++;
    return LIBV_OK;
}

static inline const void* vec_raw_get_at_unchecked(const vec_policy* policy,
                                                   const vec_raw* self,
                                                   size_t index) {
    return self->data + (index * policy->obj->size);
}

static inline const void* vec_raw_get_at(const vec_policy* policy,
                                         const vec_raw* self, size_t index) {
    if (index >= self->size) {
        return NULL;
    }
    return vec_raw_get_at_unchecked(policy, self, index);
}

static inline int vec_raw_pop_back(const vec_policy* policy, vec_raw* self,
                                   void* out) {
    if (self->size == 0) {
        return LIBV_ERR;
    }
    self->size--;
    if (out) {
        policy->obj->copy(out, self->data + (self->size * policy->obj->size));
    } else {
        if (policy->obj->dtor) {
            policy->obj->dtor(self->data + (self->size * policy->obj->size));
        }
    }
    return LIBV_OK;
}

static inline void vec_raw_remove_at_unchecked(const vec_policy* policy,
                                               vec_raw* self, size_t index,
                                               void* out) {
    if (out) {
        policy->obj->copy(out, self->data + (index * policy->obj->size));
    } else {
        if (policy->obj->dtor) {
            policy->obj->dtor(self->data + (index * policy->obj->size));
        }
    }
    self->size--;
    if (index == self->size) {
        return;
    }
    memmove(self->data + (index * policy->obj->size),
            self->data + ((index + 1) * policy->obj->size),
            (self->size - index) * policy->obj->size);
}

static inline int vec_raw_remove_at(const vec_policy* policy, vec_raw* self,
                                    size_t index, void* out) {
    if (index >= self->size) {
        return LIBV_ERR;
    }
    vec_raw_remove_at_unchecked(policy, self, index, out);
    return LIBV_OK;
}

typedef struct {
    vec_raw* vec;
    size_t position;
} vec_raw_iter;

static inline vec_raw_iter vec_raw_iter_new(vec_raw* self) {
    return (vec_raw_iter){self, 0};
}

static inline const void* vec_raw_iter_get(const vec_policy* policy,
                                           const vec_raw_iter* self) {
    if (self->position >= self->vec->size) {
        return NULL;
    }
    return self->vec->data + (self->position * policy->obj->size);
}

static inline void vec_raw_iter_next(vec_raw_iter* self) { self->position++; }

#define VEC_DECLARE_DEFAULT_POLICY_(policy_, type_)                            \
    LIBV_BEGIN                                                                 \
    static inline void policy_##_default_copy(void* dst, const void* src) {    \
        memcpy(dst, src, sizeof(type_));                                       \
    }                                                                          \
    static inline bool policy_##_default_eq(const void* a, const void* b) {    \
        return memcmp(a, b, sizeof(type_)) == 0;                               \
    }                                                                          \
    static const vec_object_policy policy_##_object_policy = {                 \
        sizeof(type_),                                                         \
        _Alignof(type_),                                                       \
        policy_##_default_copy,                                                \
        policy_##_default_eq,                                                  \
        NULL,                                                                  \
    };                                                                         \
    static const vec_alloc_policy policy_##_alloc_policy = {                   \
        libv_default_alloc,                                                    \
        libv_default_realloc,                                                  \
        libv_default_free,                                                     \
    };                                                                         \
    LIBV_END                                                                   \
    static const vec_policy policy_ = {                                        \
        &policy_##_alloc_policy,                                               \
        &policy_##_object_policy,                                              \
    };

#define VEC_DECLARE(name_, policy_, type_)                                     \
    LIBV_BEGIN                                                                 \
    typedef struct {                                                           \
        vec_raw vec;                                                           \
    } name_;                                                                   \
    static inline name_ name_##_new(void) { return (name_){vec_raw_new()}; }   \
    static inline void name_##_free(name_* self) {                             \
        vec_raw_free(&policy_, &self->vec);                                    \
    }                                                                          \
    static inline size_t name_##_size(const name_* self) {                     \
        return vec_raw_size(&self->vec);                                       \
    }                                                                          \
    static inline size_t name_##_capacity(const name_* self) {                 \
        return vec_raw_capacity(&self->vec);                                   \
    }                                                                          \
    static inline int name_##_push_back(name_* self, type_* value) {           \
        return vec_raw_push_back(&policy_, &self->vec, value);                 \
    }                                                                          \
    static inline int name_##_pop_back(name_* self, type_* out) {              \
        return vec_raw_pop_back(&policy_, &self->vec, out);                    \
    }                                                                          \
    static inline const type_* name_##_get_at(const name_* self,               \
                                              size_t index) {                  \
        return vec_raw_get_at(&policy_, &self->vec, index);                    \
    }                                                                          \
    static inline const type_* name_##_get_at_unchecked(const name_* self,     \
                                                        size_t index) {        \
        return vec_raw_get_at_unchecked(&policy_, &self->vec, index);          \
    }                                                                          \
    static inline int name_##_remove_at(name_* self, size_t index,             \
                                        void* out) {                           \
        return vec_raw_remove_at(&policy_, &self->vec, index, out);            \
    }                                                                          \
    static inline void name_##_remove_at_unchecked(name_* self, size_t index,  \
                                                   void* out) {                \
        vec_raw_remove_at_unchecked(&policy_, &self->vec, index, out);         \
    }                                                                          \
    typedef struct {                                                           \
        vec_raw_iter it;                                                       \
    } name_##_iter;                                                            \
    static inline name_##_iter name_##_iter_new(name_* self) {                 \
        return (name_##_iter){vec_raw_iter_new(&self->vec)};                   \
    }                                                                          \
    static inline const type_* name_##_iter_get(const name_##_iter* self) {    \
        return (type_*)vec_raw_iter_get(&policy_, &self->it);                  \
    }                                                                          \
    static inline void name_##_iter_next(name_##_iter* self) {                 \
        vec_raw_iter_next(&self->it);                                          \
    }                                                                          \
    LIBV_END                                                                   \
    /* Force a semicolon. */ struct name_##_needstrailingsemicolon_ { int x; }

#define VEC_DECLARE_DEFAULT(name_, type_)                                      \
    VEC_DECLARE_DEFAULT_POLICY_(name_##_policy, type_)                         \
    VEC_DECLARE(name_, name_##_policy, type_)

LIBV_END

#endif // __VEC_H__
