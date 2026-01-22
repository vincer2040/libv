#ifndef __LIBV_BASE_H__

#define __LIBV_BASE_H__

#include <stdio.h>
#include <stdlib.h>

#define LIBV_OK 0
#define LIBV_ERR -1

#define LIBV_UNUSED(x_) ((void)x_)

#define libv_panic(fmt, ...) do {\
    fprintf(stderr, "LIBV PANIC (%s:%d) ", __FILE__, __LINE__);\
    fprintf(stderr, fmt, __VA_ARGS__);\
    fflush(stdout);\
    abort();\
} while (0)

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

#endif // __LIBV_BASE_H__
