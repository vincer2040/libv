// BSD 3-Clause License
//
// Copyright (c) 2026, vincer2040
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

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

// init / destroy

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

// capacity

static inline bool vec_raw_empty(const vec_raw* self) {
    return self->size == 0;
}

static inline size_t vec_raw_size(const vec_raw* self) { return self->size; }

static inline size_t vec_raw_capacity(const vec_raw* self) {
    return self->capacity;
}

static inline int vec_raw_realloc_self(const vec_policy* policy, vec_raw* self,
                                       size_t new_capacity) {
    void* tmp =
        policy->alloc->realloc(self->data, new_capacity * policy->obj->size);
    if (!tmp) {
        return LIBV_ERR;
    }
    self->data = tmp;
    self->capacity = new_capacity;
    return LIBV_OK;
}

static inline int vec_raw_maybe_resize(const vec_policy* policy,
                                       vec_raw* self) {
    if (self->size < self->capacity) {
        return LIBV_OK;
    }
    size_t new_capacity;
    if (self->capacity == 0) {
        new_capacity = 4;
    } else if (self->capacity >= 1024) {
        new_capacity = self->capacity + (self->capacity / 2);
    } else {
        new_capacity = self->capacity << 1;
    }
    return vec_raw_realloc_self(policy, self, new_capacity);
}

static inline int vec_raw_reserve(const vec_policy* policy, vec_raw* self,
                                  size_t capacity) {
    if (self->capacity >= capacity) {
        return LIBV_OK;
    }
    return vec_raw_realloc_self(policy, self, capacity);
}

static inline int vec_raw_shrink_to_size(const vec_policy* policy,
                                         vec_raw* self) {
    if (self->size == self->capacity) {
        return LIBV_OK;
    }
    return vec_raw_realloc_self(policy, self, self->size);
}

static inline void vec_raw_clear(const vec_policy* policy, vec_raw* self) {
    if (policy->obj->dtor) {
        for (size_t i = 0; i < self->size; ++i) {
            policy->obj->dtor(self->data + (i * policy->obj->size));
        }
    }
    self->size = 0;
}

// access

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

static inline const void* vec_raw_data(const vec_raw* self) {
    return self->data;
}

static inline const void* vec_raw_front(const vec_raw* self) {
    if (self->size == 0) {
        return NULL;
    }
    return self->data;
}

static inline const void* vec_raw_back(const vec_policy* policy,
                                       const vec_raw* self) {
    if (self->size == 0) {
        return NULL;
    }
    return self->data + ((self->size - 1) * policy->obj->size);
}

// modification

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

static inline int vec_raw_pop_front(const vec_policy* policy, vec_raw* self,
                                    void* out) {
    return vec_raw_remove_at(policy, self, 0, out);
}

static inline int vec_raw_push_front(const vec_policy* policy, vec_raw* self,
                                     const void* value) {
    if (vec_raw_maybe_resize(policy, self) == LIBV_ERR) {
        return LIBV_ERR;
    }
    if (self->size == 0) {
        policy->obj->copy(self->data, value);
        self->size++;
        return LIBV_OK;
    }
    memmove(self->data + policy->obj->size, self->data,
            self->size * policy->obj->size);
    policy->obj->copy(self->data, value);
    self->size++;
    return LIBV_OK;
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

static inline int vec_raw_push_back(const vec_policy* policy, vec_raw* self,
                                    const void* value) {
    if (vec_raw_maybe_resize(policy, self) == LIBV_ERR) {
        return LIBV_ERR;
    }
    policy->obj->copy(self->data + (self->size * policy->obj->size), value);
    self->size++;
    return LIBV_OK;
}

static inline int vec_raw_append(const vec_policy* policy, vec_raw* self,
                                 const vec_raw* other) {
    if (vec_raw_reserve(policy, self, self->size + other->size) == LIBV_ERR) {
        return LIBV_ERR;
    }
    for (size_t i = 0; i < other->size; ++i) {
        if (vec_raw_push_back(policy, self,
                              other->data + (i * policy->obj->size)) ==
            LIBV_ERR) {
            return LIBV_ERR;
        }
    }
    return LIBV_OK;
}

// iter

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
    static inline bool name_##_empty(const name_* self) {                      \
        return vec_raw_empty(&self->vec);                                      \
    }                                                                          \
    static inline size_t name_##_size(const name_* self) {                     \
        return vec_raw_size(&self->vec);                                       \
    }                                                                          \
    static inline size_t name_##_capacity(const name_* self) {                 \
        return vec_raw_capacity(&self->vec);                                   \
    }                                                                          \
    static inline int name_##_reserve(name_* self, size_t new_capacity) {      \
        return vec_raw_reserve(&policy_, &self->vec, new_capacity);            \
    }                                                                          \
    static inline int name_##_shrink_to_size(name_* self) {                    \
        return vec_raw_shrink_to_size(&policy_, &self->vec);                   \
    }                                                                          \
    static inline void name_##_clear(name_* self) {                            \
        vec_raw_clear(&policy_, &self->vec);                                   \
    }                                                                          \
    static inline const type_* name_##_get_at_unchecked(const name_* self,     \
                                                        size_t index) {        \
        return vec_raw_get_at_unchecked(&policy_, &self->vec, index);          \
    }                                                                          \
    static inline const type_* name_##_get_at(const name_* self,               \
                                              size_t index) {                  \
        return vec_raw_get_at(&policy_, &self->vec, index);                    \
    }                                                                          \
    static inline const type_* name_##_data(const name_* self) {               \
        return vec_raw_data(&self->vec);                                       \
    }                                                                          \
    static inline const type_* name_##_front(const name_* self) {              \
        return vec_raw_front(&self->vec);                                      \
    }                                                                          \
    static inline const type_* name_##_back(const name_* self) {               \
        return vec_raw_back(&policy_, &self->vec);                             \
    }                                                                          \
    static inline void name_##_remove_at_unchecked(name_* self, size_t index,  \
                                                   void* out) {                \
        vec_raw_remove_at_unchecked(&policy_, &self->vec, index, out);         \
    }                                                                          \
    static inline int name_##_remove_at(name_* self, size_t index,             \
                                        void* out) {                           \
        return vec_raw_remove_at(&policy_, &self->vec, index, out);            \
    }                                                                          \
    static inline int name_##_pop_front(name_* self, type_* out) {             \
        return vec_raw_pop_front(&policy_, &self->vec, out);                   \
    }                                                                          \
    static inline int name_##_push_front(name_* self, type_* value) {          \
        return vec_raw_push_front(&policy_, &self->vec, value);                \
    }                                                                          \
    static inline int name_##_pop_back(name_* self, type_* out) {              \
        return vec_raw_pop_back(&policy_, &self->vec, out);                    \
    }                                                                          \
    static inline int name_##_push_back(name_* self, type_* value) {           \
        return vec_raw_push_back(&policy_, &self->vec, value);                 \
    }                                                                          \
    static inline int name_##_append(name_* self, const name_* other) {        \
        return vec_raw_append(&policy_, &self->vec, &other->vec);              \
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
