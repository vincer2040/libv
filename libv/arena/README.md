# arena

a basic arena allocator

## example

```C
#include "libv/arena/arena.h"

int main(void) {
    arena a = arena_new();

    // allocate
    int* allocated_int = arena_alloc(&a, sizeof *allocated_int);

    // allocate based on type
    int* allocated_type = arena_alloc_type(&a, int);

    // allocate an array of type
    int* allocated_array = arena_alloc_array(&a, int, 5);

    // realloc
    int* realloced_array = arena_realloc(&a, &allocated_array, sizeof(int) * 5, sizeof(int) * 10));

    arena_destroy(&a);

    return 0;
}
```
