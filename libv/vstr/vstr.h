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

#ifndef __LIBV_VSTR_H__

#define __LIBV_VSTR_H__

#include "libv/base/base.h"
#include <memory.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

LIBV_BEGIN

typedef struct {
    const libv_alloc_policy* alloc;
} vstr_policy;

typedef struct __attribute__((packed)) {
    char* data;
    uint64_t capacity;
    uint64_t length : 56;
} vstr_large;

#define VSTR_SMALL_MAX_SIZE 23

typedef char vstr_small[VSTR_SMALL_MAX_SIZE];

typedef struct {
    union {
        vstr_large large;
        vstr_small small;
    } string;
    uint8_t small_available : 7;
    uint8_t is_large : 1;
} vstr;

static inline uint64_t vstr_strlen_u8(const char* s) {
    const char* s_ = s;
    for (; *s_; ++s_) {
    }
    return s_ - s;
}

static inline vstr vstr_new(void) {
    vstr self = {0};
    self.small_available = VSTR_SMALL_MAX_SIZE;
    return self;
}

static inline vstr_large vstr_large_from_length(const vstr_policy* policy,
                                                const char* buf,
                                                uint64_t buf_size) {
    vstr_large self = {0};
    self.data = policy->alloc->alloc(buf_size + 1, _Alignof(char));
    memcpy(self.data, buf, buf_size);
    self.data[buf_size] = '\0';
    self.capacity = buf_size + 1;
    self.length = buf_size;
    return self;
}

static inline vstr vstr_from_length(const vstr_policy* policy, const char* buf,
                                    uint64_t buf_size) {
    vstr self = vstr_new();
    if (buf_size > VSTR_SMALL_MAX_SIZE) {
        self.string.large = vstr_large_from_length(policy, buf, buf_size);
        self.is_large = 1;
        return self;
    }
    memcpy(self.string.small, buf, buf_size);
    self.small_available -= buf_size;
    return self;
}

static inline vstr vstr_from(const vstr_policy* policy, const char* buf) {
    return vstr_from_length(policy, buf, vstr_strlen_u8(buf));
}

static inline void vstr_free(const vstr_policy* policy, vstr* self) {
    if (self->is_large) {
        policy->alloc->free(self->string.large.data,
                            self->string.large.capacity, _Alignof(char));
    }
    memset(self, 0, sizeof *self);
    self->small_available = VSTR_SMALL_MAX_SIZE;
}

static inline const char* vstr_data(const vstr* self) {
    if (self->is_large) {
        return self->string.large.data;
    }
    return self->string.small;
}

static inline char* vstr_mut_data(vstr* self) {
    if (self->is_large) {
        return self->string.large.data;
    }
    return self->string.small;
}

static inline uint64_t vstr_length(const vstr* self) {
    if (self->is_large) {
        return self->string.large.length;
    }
    return VSTR_SMALL_MAX_SIZE - self->small_available;
}

static inline uint64_t vstr_capacity(const vstr_policy* policy,
                                     const vstr* self) {
    if (self->is_large) {
        return self->string.large.capacity;
    }
    return VSTR_SMALL_MAX_SIZE;
}

static inline int vstr_large_set_length(vstr_large* self, uint64_t length) {
    if (length >= self->capacity - 1) {
        return LIBV_ERR;
    }
    self->length = length;
    self->data[length] = '\0';
    return LIBV_OK;
}

static inline int vstr_set_length(vstr* self, uint64_t length) {
    if (self->is_large) {
        return vstr_large_set_length(&self->string.large, length);
    }
    if (length > VSTR_SMALL_MAX_SIZE) {
        // the string is in an invalid state.
        libv_panic("vstr is marked as small, but attempting to set a length "
                   "(%lu) greater than VSTR_SMALL_MAX_SIZE",
                   length);
    }
    self->small_available = VSTR_SMALL_MAX_SIZE - (uint8_t)length;
    return LIBV_OK;
}

static inline bool vstr_is_small(const vstr* self) { return !self->is_large; }

static inline bool vstr_is_large(const vstr* self) { return self->is_large; }

static inline int vstr_large_push_char(const vstr_policy* policy,
                                       vstr_large* self, char ch) {
    if (self->length >= self->capacity - 1) {
        uint64_t new_capacity = self->capacity << 1;
        void* tmp = policy->alloc->realloc(self->data, self->capacity,
                                           new_capacity, _Alignof(char));
        if (!tmp) {
            return LIBV_ERR;
        }
        self->capacity = new_capacity;
        self->data = tmp;
    }
    self->data[self->length++] = ch;
    self->data[self->length] = '\0';
    return LIBV_OK;
}

static inline int vstr_make_large(const vstr_policy* policy, vstr* self,
                                  uint64_t length_to_add) {
    vstr_large large = {0};
    large.capacity =
        (VSTR_SMALL_MAX_SIZE - self->small_available) + length_to_add + 1;
    large.data = policy->alloc->alloc(large.capacity, _Alignof(char));
    if (!large.data) {
        return LIBV_ERR;
    }
    large.length = VSTR_SMALL_MAX_SIZE - self->small_available;
    memcpy(large.data, self->string.small, large.length);
    large.data[large.length] = '\0';
    self->string.large = large;
    self->small_available = 0;
    self->is_large = 1;
    return LIBV_OK;
}

static inline int vstr_push_char(const vstr_policy* policy, vstr* self,
                                 char ch) {
    if (self->is_large) {
        return vstr_large_push_char(policy, &self->string.large, ch);
    }
    if (self->small_available == 0) {
        if (vstr_make_large(policy, self, 1) == LIBV_ERR) {
            return LIBV_ERR;
        }
        return vstr_large_push_char(policy, &self->string.large, ch);
    }
    self->string.small[VSTR_SMALL_MAX_SIZE - self->small_available] = ch;
    self->small_available--;
    return LIBV_OK;
}

static inline int vstr_large_make_available(const vstr_policy* policy,
                                            vstr_large* self, uint64_t size) {
    if (self->capacity - 1 > self->length + size) {
        return LIBV_OK;
    }
    uint64_t new_capacity = self->length + size + 1;
    void* tmp = policy->alloc->realloc(self->data, self->capacity, new_capacity,
                                       _Alignof(char));
    if (tmp == NULL) {
        return LIBV_OK;
    }
    self->data = tmp;
    self->data[self->length] = '\0';
    self->capacity = new_capacity;
    return LIBV_OK;
}

static inline int vstr_large_cat_string_length(const vstr_policy* policy,
                                               vstr_large* self,
                                               const char* string,
                                               uint64_t string_size) {
    if (vstr_large_make_available(policy, self, string_size) == LIBV_ERR) {
        return LIBV_ERR;
    }
    memcpy(self->data + self->length, string, string_size);
    self->length += string_size;
    self->data[self->length] = '\0';
    return LIBV_OK;
}

static inline int vstr_cat_string_length(const vstr_policy* policy, vstr* self,
                                         const char* string,
                                         uint64_t string_size) {
    if (self->is_large) {
        return vstr_large_cat_string_length(policy, &self->string.large, string,
                                            string_size);
    }
    if (self->small_available < string_size) {
        vstr_make_large(policy, self, string_size);
        return vstr_large_cat_string_length(policy, &self->string.large, string,
                                            string_size);
    }
    memcpy(self->string.small + (VSTR_SMALL_MAX_SIZE - self->small_available),
           string, string_size);
    self->small_available -= string_size;
    return LIBV_OK;
}

static inline int vstr_cat_string(const vstr_policy* policy, vstr* self,
                                  const char* string) {
    return vstr_cat_string_length(policy, self, string, vstr_strlen_u8(string));
}

static inline int vstr_cat_vstr(const vstr_policy* policy, vstr* self,
                                const vstr* other) {
    if (other->is_large) {
        return vstr_cat_string_length(policy, self, other->string.large.data,
                                      other->string.large.length);
    }
    return vstr_cat_string_length(policy, self, other->string.small,
                                  VSTR_SMALL_MAX_SIZE - other->small_available);
}

static inline void vstr_clear(const vstr_policy* policy, vstr* self) {
    if (self->is_large) {
        policy->alloc->free(self->string.large.data,
                            self->string.large.capacity, _Alignof(char));
    }
    self->small_available = VSTR_SMALL_MAX_SIZE;
    self->is_large = 0;
    self->string.small[0] = '\0';
}

static inline int vstr_fast_cmp(const vstr* self, const vstr* other) {
    uint64_t self_length = vstr_length(self);
    uint64_t other_length = vstr_length(other);
    if (self_length != other_length) {
        return -1;
    }
    const char* self_data = vstr_data(self);
    const char* other_data = vstr_data(other);
    return memcmp(self_data, other_data, self_length);
}

static inline int vstr_cmp(const vstr* self, const vstr* other) {
    uint64_t self_length = vstr_length(self);
    uint64_t other_length = vstr_length(other);
    const char* self_data = vstr_data(self);
    const char* other_data = vstr_data(other);

    int rv;

    if (self_length > other_length) {
        rv = memcmp(self_data, other_data, other_length);
        if (rv == 0) {
            return 1;
        } else {
            return -1;
        }
    } else if (self_length < other_length) {
        rv = memcmp(self_data, other_data, self_length);
        if (rv == 0) {
            return -1;
        } else {
            return 1;
        }
    } else {
        return memcmp(self_data, other_data, self_length);
    }
}

typedef struct {
    size_t size;
    size_t capacity;
    vstr* data;
} vstr_array;

static inline int vstr_array_realloc(const vstr_policy* policy,
                                     vstr_array* self) {
    size_t new_capacity;
    if (self->capacity == 0) {
        new_capacity = 8;
    } else {
        new_capacity = self->capacity << 1;
    }
    void* tmp =
        policy->alloc->realloc(self->data, self->capacity * sizeof(vstr),
                               new_capacity * sizeof(vstr), _Alignof(vstr));
    if (!tmp) {
        return LIBV_ERR;
    }
    self->data = tmp;
    self->capacity = new_capacity;
    return LIBV_OK;
}

static inline int vstr_array_push(const vstr_policy* policy, vstr_array* self,
                                  const vstr* item) {
    if (self->size == self->capacity) {
        if (vstr_array_realloc(policy, self) == LIBV_ERR) {
            return LIBV_ERR;
        }
    }
    self->data[self->size++] = *item;
    return LIBV_OK;
}

static inline vstr_array vstr_split_char(const vstr_policy* policy,
                                         const vstr* self, char ch) {
    vstr_array res = {0};
    size_t data_length = vstr_length(self);
    const char* buf = vstr_data(self);
    const char* start = NULL;
    const char* end;
    size_t length;
    vstr s;
    for (size_t i = 0; i < data_length; ++i) {
        if (buf[i] == ch) {
            if (start == NULL) {
                s = vstr_new();
                if (vstr_array_push(policy, &res, &s) == LIBV_ERR) {
                    goto err;
                }
                continue;
            }
            end = buf + i - 1;
            length = end - start + 1;
            s = vstr_from_length(policy, start, length);
            if (vstr_array_push(policy, &res, &s) == LIBV_ERR) {
                goto err;
            }
            start = NULL;
            continue;
        }
        if (start == NULL) {
            start = buf + i;
        }
    }
    if (start == NULL) {
        s = vstr_new();
        if (vstr_array_push(policy, &res, &s) == LIBV_ERR) {
            goto err;
        }
    } else {
        end = buf + data_length - 1;
        length = end - start + 1;
        s = vstr_from_length(policy, start, length);
        if (vstr_array_push(policy, &res, &s) == LIBV_ERR) {
            goto err;
        }
    }
    return res;
err:
    policy->alloc->free(res.data, res.capacity * sizeof(vstr), _Alignof(vstr));
    memset(&res, 0, sizeof res);
    return res;
}

static inline void vstr_array_free(const vstr_policy* policy,
                                   vstr_array* self) {
    for (size_t i = 0; i < self->size; ++i) {
        vstr_free(policy, &self->data[i]);
    }
    policy->alloc->free(self->data, self->capacity * sizeof(vstr),
                        _Alignof(vstr));
}

#define VSTR_DECLARE_DEFAULT(name_)                                            \
    const libv_alloc_policy name_##_alloc_policy = {                           \
        .alloc = libv_default_alloc,                                           \
        .calloc = NULL,                                                        \
        .realloc = libv_default_realloc,                                       \
        .free = libv_default_free,                                             \
    };                                                                         \
    const vstr_policy name_##_policy = {                                       \
        .alloc = &name_##_alloc_policy,                                        \
    };                                                                         \
    VSTR_DECLARE(name_, name_##_policy)

#define VSTR_DECLARE(name_, policy_)                                           \
    LIBV_BEGIN                                                                 \
    typedef struct {                                                           \
        vstr string;                                                           \
    } name_;                                                                   \
    static inline name_ name_##_new(void) { return (name_){vstr_new()}; }      \
    static inline name_ name_##_from(const char* buf) {                        \
        return (name_){vstr_from(&policy_, buf)};                              \
    }                                                                          \
    static inline name_ name_##_from_length(const char* buf,                   \
                                            uint64_t buf_size) {               \
        return (name_){vstr_from_length(&policy_, buf, buf_size)};             \
    }                                                                          \
    static inline void name_##_free(name_* self) {                             \
        vstr_free(&policy_, &self->string);                                    \
    }                                                                          \
    static inline uint64_t name_##_length(const name_* self) {                 \
        return vstr_length(&self->string);                                     \
    }                                                                          \
    static inline uint64_t name_##_capacity(const name_* self) {               \
        return vstr_capacity(&policy_, &self->string);                         \
    }                                                                          \
    static inline const char* name_##_data(const name_* self) {                \
        return vstr_data(&self->string);                                       \
    }                                                                          \
    static inline char* name_##_mut_data(name_* self) {                        \
        return vstr_mut_data(&self->string);                                   \
    }                                                                          \
    static inline int name_##_set_length(name_* self, uint64_t length) {       \
        return vstr_set_length(&self->string, length);                         \
    }                                                                          \
    static inline bool name_##_is_small(const name_* self) {                   \
        return vstr_is_small(&self->string);                                   \
    }                                                                          \
    static inline bool name_##_is_large(const name_* self) {                   \
        return vstr_is_large(&self->string);                                   \
    }                                                                          \
    static inline int name_##_push_char(name_* self, char ch) {                \
        return vstr_push_char(&policy_, &self->string, ch);                    \
    }                                                                          \
    static inline int name_##_cat_string(name_* self, const char* string) {    \
        return vstr_cat_string(&policy_, &self->string, string);               \
    }                                                                          \
    static inline int name_##_cat_string_length(                               \
        name_* self, const char* string, uint64_t string_size) {               \
        return vstr_cat_string_length(&policy_, &self->string, string,         \
                                      string_size);                            \
    }                                                                          \
    static inline int name_##_cat_##name_(name_* self, const name_* other) {   \
        return vstr_cat_vstr(&policy_, &self->string, &other->string);         \
    }                                                                          \
    static inline void name_##_clear(name_* self) {                            \
        vstr_clear(&policy_, &self->string);                                   \
    }                                                                          \
    static inline int name_##_fast_cmp(const name_* self,                      \
                                       const name_* other) {                   \
        return vstr_fast_cmp(&self->string, &other->string);                   \
    }                                                                          \
    static inline int name_##_cmp(const name_* self, const name_* other) {     \
        return vstr_cmp(&self->string, &other->string);                        \
    }                                                                          \
    typedef struct {                                                           \
        size_t size;                                                           \
        size_t capacity;                                                       \
        name_* data;                                                           \
    } name_##_array;                                                           \
    static inline name_##_array name_##_split_char(const name_* self,          \
                                                   char ch) {                  \
        vstr_array res = vstr_split_char(&policy_, &self->string, ch);         \
        return (name_##_array){res.size, res.capacity, (name_*)res.data};      \
    }                                                                          \
    static inline void name_##_array_free(name_##_array* self) {               \
        vstr_array_free(&policy_, (vstr_array*)self);                          \
    }                                                                          \
    LIBV_END                                                                   \
    /* Force a semicolon. */ struct name_##_needstrailingsemicolon_ { int x; }

LIBV_END
#endif // __LIBV_VSTR_H__
