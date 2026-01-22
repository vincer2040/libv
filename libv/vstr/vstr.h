#ifndef __VSTR_H__

#define __VSTR_H__

#include "libv/base/base.h"
#include <memory.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    void* (*alloc)(size_t size);
    void* (*realloc)(void* ptr, size_t size);
    void (*free)(void* ptr);
} vstr_alloc_policy;

typedef enum {
    VSTR_ENCODING_UTF8 = 1,
    VSTR_ENCODING_UTF16 = 2,
    VSTR_ENCODING_UTF32 = 4,
} vstr_encoding_policy;

typedef struct {
    const vstr_encoding_policy encoding;
    const vstr_alloc_policy* alloc;
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

static inline vstr vstr_new(void) {
    vstr self = {0};
    self.small_available = VSTR_SMALL_MAX_SIZE;
    return self;
}

static inline vstr_large vstr_large_from_length(const vstr_policy* policy,
                                                const char* buf,
                                                uint64_t buf_size) {
    vstr_large self = {0};
    self.data = policy->alloc->alloc(buf_size + 1);
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
    return vstr_from_length(policy, buf, strlen(buf));
}

static inline void vstr_free(const vstr_policy* policy, vstr* self) {
    if (self->is_large) {
        policy->alloc->free(self->string.large.data);
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

static inline uint64_t vstr_length(const vstr_policy* policy,
                                   const vstr* self) {
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

static inline int vstr_large_set_length(const vstr_policy* policy,
                                        vstr_large* self, uint64_t length) {
    LIBV_UNUSED(policy);
    if (length >= self->capacity - 1) {
        return LIBV_ERR;
    }
    self->length = length;
    self->data[length] = '\0';
    return LIBV_OK;
}

static inline int vstr_set_length(const vstr_policy* policy, vstr* self,
                                  uint64_t length) {
    if (self->is_large) {
        return vstr_large_set_length(policy, &self->string.large, length);
    }
    if (length > VSTR_SMALL_MAX_SIZE) {
        // the string is in an invalid state.
        libv_panic("vstr is marked as small, but attempting to set a length "
                   "(%lu) greater than VSTR_SMALL_MAX_SIZE",
                   length);
    }
    self->small_available = VSTR_SMALL_MAX_SIZE - length;
    return LIBV_OK;
}

static inline bool vstr_is_small(const vstr* self) { return !self->is_large; }

static inline bool vstr_is_large(const vstr* self) { return self->is_large; }

#define VSTR_DECLARE(name_, policy_)                                           \
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
        return vstr_length(&policy_, &self->string);                           \
    }                                                                          \
    static inline uint64_t name_##_capacity(const name_* self) {               \
        return vstr_capacity(&policy_, &self->string);                         \
    }                                                                          \
    static inline const char* name_##_data(const name_* self) {                \
        return vstr_data(&self->string);                                       \
    }                                                                          \
    static inline int name_##_set_length(name_* self, uint64_t length) {       \
        return vstr_set_length(&policy_, &self->string, length);               \
    }                                                                          \
    static inline bool name_##_is_small(const name_* self) {                   \
        return vstr_is_small(&self->string);                                   \
    }                                                                          \
    static inline bool name_##_is_large(const name_* self) {                   \
        return vstr_is_large(&self->string);                                   \
    }                                                                          \
    /* Force a semicolon. */ struct name_##_needstrailingsemicolon_ { int x; }

#define VSTR_DECLARE_DEFAULT(name_)                                            \
    const vstr_alloc_policy name_##_alloc_policy = {                           \
        .alloc = libv_default_alloc,                                           \
        .realloc = libv_default_realloc,                                       \
        .free = libv_default_free,                                             \
    };                                                                         \
    const vstr_encoding_policy name_##_encoding = VSTR_ENCODING_UTF8;          \
    const vstr_policy name_##_policy = {                                       \
        .alloc = &name_##_alloc_policy,                                        \
        .encoding = name_##_encoding,                                          \
    };                                                                         \
    VSTR_DECLARE(name_, name_##_policy)

#endif // __VSTR_H__
