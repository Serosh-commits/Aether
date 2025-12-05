#ifndef AETHER_H
#define AETHER_H
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#define AETHER_ALIVE 0xA3TH3RA1V3ULL
#define AETHER_FREED 0xA3TH3RD3ADULL
typedef struct {
    void       *ptr;
    void       *base;
    size_t      size;
    uint64_t    magic;
    const char *file;
    int         line;
    int         id;
} aether_handle_t;
static int aether_id_counter = 1;
static long page_size = 0;
static inline void aether_purge(aether_handle_t *h)
{
    if (!h || !h->ptr) return;
    if (h->magic == AETHER_FREED) {
        fprintf(stderr, "[AETHER] Double-free prevented: %p (alloc %s:%d)\n",
                h->ptr, h->file ? h->file : "unknown", h->line);
        return;
    }
    if (h->magic != AETHER_ALIVE) {
        fprintf(stderr, "[AETHER FATAL] Handle corrupted: %p\n", h);
        return;
    }
    *(volatile uint64_t*)h->ptr = 0xA3TH3RDEADULL;
    if (h->size > 8) memset((char*)h->ptr + 8, 0xAE, h->size - 8);
    if (page_size == 0) page_size = sysconf(_SC_PAGESIZE);
    mprotect(h->base, page_size, PROT_NONE);
    mprotect(h->base + page_size + h->size, page_size, PROT_NONE);
    h->magic = AETHER_FREED;
}
static inline void* aether_alloc(size_t size
#ifdef __GNUC__
    , const char *file, int line
#endif
)
{
    if (page_size == 0) page_size = sysconf(_SC_PAGESIZE);
    if (size == 0) size = 1;

    size_t total = page_size * 2 + size;
    void *base = mmap(NULL, total + page_size * 2,
                      PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED) return NULL;
    mprotect(base, page_size, PROT_NONE);
    mprotect(base + page_size + size, page_size, PROT_NONE);
    void *user_ptr = (char*)base + page_size;
    aether_handle_t h = {
        .ptr  = user_ptr,
        .base = base,
        .size = size,
        .magic = AETHER_ALIVE,
        .file = file,
        .line = line,
        .id   = __sync_fetch_and_add(&aether_id_counter, 1)
    };
    aether_handle_t __attribute__((cleanup(aether_purge))) guard = h;
    return guard.ptr;
}
#define aether_alloc(size) aether_alloc(size, __FILE__, __LINE__)
static inline void aether_free(void *ptr)
{
    if (!ptr) return;
    void *base = (void*)((uintptr_t)ptr & ~(page_size - 1));
    base -= page_size;
    *(volatile uint64_t*)ptr = 0xA3TH3RDEADULL;
    munmap(base, page_size * 4 + 4096);
}
#endif
