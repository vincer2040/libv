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

#define libv_panic(fmt, ...) do {\
    fprintf(stderr, "LIBV PANIC (%s:%d) ", __FILE__, __LINE__);\
    fprintf(stderr, fmt, __VA_ARGS__);\
    fflush(stdout);\
    abort();\
} while (0)

#define LIBV_BEGIN                                                             \
    LIBV_GCC_PUSH                                                            \
    LIBV_GCC_ALLOW("-Wunused-function")                                      \
    LIBV_GCC_ALLOW("-Wunused-parameter")                                     \
    LIBV_GCC_ALLOW("-Wcast-qual")                                            \
    LIBV_GCC_ALLOW("-Wmissing-field-initializers")

#define LIBV_END LIBV_GCC_POP

LIBV_BEGIN

static inline void* libv_default_alloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        libv_panic("failed to allocate %zu bytes\n", size);
    }
    return ptr;
}

static inline void* libv_default_realloc(void* ptr, size_t size) {
    void* new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        libv_panic("failed to re-allocate %p %zu bytes\n", ptr, size);
    }
    return new_ptr;
}

static inline void libv_default_free(void* ptr) {
    free(ptr);
    ptr = NULL;
}

LIBV_END

#endif // __LIBV_BASE_H__
