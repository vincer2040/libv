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

// base file
// this file is for determining platform information and some debug helping
// functions. this file should never include anything other than items from the
// standard library.

#ifndef __LIBV_BASE_H__

#define __LIBV_BASE_H__

#include <stdio.h>
#include <stdlib.h>

#define LIBV_OK 0
#define LIBV_ERR -1

#define LIBV_UNUSED(x_) ((void)x_)

#ifdef __clang__
#define LIBV_IS_CLANG 1
#else
#define LIBV_IS_CLANG 0
#endif // __clang__

#if defined(__GNUC__)
#define LIBV_IS_GCCISH 1
#else
#define LIBV_IS_GCCISH 0
#endif // __GNUC__

#if defined(_MSC_VER)
#define LIBV_IS_MSCVISH 1
#else
#define LIBV_IS_MSCVISH 0
#endif // _MSC_VER

#define LIBV_PRAGMA(pragma_) _Pragma(#pragma_)

#if LIBV_IS_GCCISH
#define LIBV_GCC_PUSH LIBV_PRAGMA(GCC diagnostic push)
#define LIBV_GCC_ALLOW(w_) LIBV_PRAGMA(GCC diagnostic ignored w_)
#define LIBV_GCC_POP LIBV_PRAGMA(GCC diagnostic pop)
#else
#define LIBV_GCC_PUSH
#define LIBV_GCC_ALLOW(w_)
#define LIBV_GCC_POP
#endif

#ifdef __has_attribute
#define LIBV_HAVE_GCC_ATTRIBUTE(x_) __has_attribute(x_)
#else
#define LIBV_HAVE_GCC_ATTRIBUTE(x_) 0
#endif // __has_attribute

#ifdef __has_builtin
#define LIBV_HAVE_CLANG_BUILTIN(x_) __has_builtin(x_)
#else
#define LIBV_HAVE_CLANG_BUILTIN(x_) 0
#endif // __has_builtin

#if LIBV_HAVE_GCC_ATTRIBUTE(always_inline)
#define LIBV_INLINE_ALWAYS __attribute__((always_inline))
#else
#define LIBV_INLINE_ALWAYS
#endif // always_inline

#if LIBV_HAVE_GCC_ATTRIBUTE(noinline)
#define LIBV_INLINE_NEVER __attribute__((noinline))
#else
#define LIBV_INLINE_NEVER
#endif // noinline

#if LIBV_HAVE_CLANG_BUILTIN(__builtin_expect) || LIBV_IS_GCC
#define LIBV_LIKELY(cond_) (__builtin_expect(false || (cond_), true))
#define LIBV_UNLIKELY(cond_) (__builtin_expect(false || (cond_), false))
#else
#define LIBV_LIKELY(cond_) (cond_)
#define LIBV_UNLIKELY(cond_) (cond_)
#endif

#define LIBV_IS_GCC (LIBV_IS_GCCISH && !LIBV_IS_CLANG)
#define LIBV_IS_MSVC (LIBV_IS_MSCVISH && !LIBV_IS_CLANG)

#define libv_panic(fmt, ...)                                                   \
    do {                                                                       \
        fprintf(stderr, "LIBV PANIC (%s:%d) ", __FILE__, __LINE__);            \
        fprintf(stderr, fmt, __VA_ARGS__);                                     \
        fflush(stdout);                                                        \
        abort();                                                               \
    } while (0)

#define LIBV_BEGIN                                                             \
    LIBV_GCC_PUSH                                                              \
    LIBV_GCC_ALLOW("-Wunused-function")                                        \
    LIBV_GCC_ALLOW("-Wunused-parameter")                                       \
    LIBV_GCC_ALLOW("-Wcast-qual")                                              \
    LIBV_GCC_ALLOW("-Wmissing-field-initializers")

#define LIBV_END LIBV_GCC_POP

LIBV_BEGIN

static int libv_debug_mode = 0;

static inline void libv_set_debug_mode(int mode) { libv_debug_mode = mode; }

#define libv_debug(...)                                                        \
    do {                                                                       \
        if (libv_debug_mode) {                                                 \
            fprintf(stderr, "DEBUG: (%s:%d) ", __FILE__, __LINE__);            \
            fprintf(stderr, __VA_ARGS__);                                      \
            fflush(stderr);                                                    \
        }                                                                      \
    } while (0)

#define libv_assert_(cond_, ...)                                               \
    do {                                                                       \
        if (cond_) {                                                           \
            break;                                                             \
        }                                                                      \
        fprintf(stderr, "libv_assert FAILED (%s:%d) ", __FILE__, __LINE__);    \
        fprintf(stderr, __VA_ARGS__);                                          \
        fprintf(stderr, "\n");                                                 \
        fflush(stderr);                                                        \
        abort();                                                               \
    } while (0)

#ifdef NDEBUG
#define libv_assert ((void)0)
#else
#define libv_assert libv_assert_
#endif

#define array_size(arr) sizeof arr / sizeof arr[0]

typedef struct {
    void* (*alloc)(size_t size, size_t align);
    void* (*calloc)(size_t nmem, size_t size);
    void* (*realloc)(void* ptr, size_t old_size, size_t new_size, size_t align);
    void (*free)(void* ptr, size_t size, size_t align);
} libv_alloc_policy;

typedef struct {
    void* (*alloc)(size_t size, size_t align);
    void (*free)(void* ptr, size_t size, size_t align);
} libv_basic_alloc_policy;

static inline void* libv_default_alloc(size_t size, size_t align) {
    LIBV_UNUSED(align);
    void* ptr = malloc(size);
    if (!ptr) {
        libv_panic("failed to allocate %zu bytes\n", size);
    }
    return ptr;
}

static inline void* libv_default_realloc(void* ptr, size_t old_size,
                                         size_t new_size, size_t align) {
    LIBV_UNUSED(old_size);
    LIBV_UNUSED(align);
    void* new_ptr = realloc(ptr, new_size);
    if (!new_ptr) {
        libv_panic("failed to re-allocate %p %zu bytes\n", ptr, new_size);
    }
    return new_ptr;
}

static inline void libv_default_free(void* ptr, size_t size, size_t align) {
    LIBV_UNUSED(size);
    LIBV_UNUSED(align);
    free(ptr);
    ptr = NULL;
}

LIBV_END

#endif // __LIBV_BASE_H__
