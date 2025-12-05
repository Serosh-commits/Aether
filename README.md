# Aether — Memory Safety for C

A single-header, zero-overhead, drop-in memory allocator that silently eliminates:

- Memory leaks  
- Double frees  
- Use-after-free bugs  

No runtime cost in release.  
No code changes required.  
Just include and forget.

```c
#include "aether.h"

void any_function() {
    char *p = aether_alloc(1000);
    strcpy(p, "hello");
    // no free() needed — ever
}
