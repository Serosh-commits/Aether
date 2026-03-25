#ifndef AETHER_H
#define AETHER_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdbool.h>

#define AETHER_ALIVE 0xAE74E8A117EULL
#define AETHER_FREED 0xAE74E8D3ADULL
#define AETHER_STASH_SIZE 64

typedef struct {
    void       *base;
    size_t      total_size;
    size_t      user_size;
    uint64_t    magic;
    const char *file;
    int         line;
    int         id;
    void       *ptr;
} aether_handle_t;

static int aether_id_counter = 1;
static long aether_page_size = 0;
static void *aether_stash[AETHER_STASH_SIZE];
static int aether_stash_idx = 0;

#define aether_ptr __attribute__((cleanup(aether_cleanup_ptr)))

static inline void aether_free(void *ptr);

static inline void aether_cleanup_ptr(void *ptr_ref) {
    void **p = (void**)ptr_ref;
    if (p && *p) {
        aether_free(*p);
        *p = NULL;
    }
}

static inline void* aether_alloc_internal(size_t size, const char *file, int line) {
    if (aether_page_size == 0) aether_page_size = sysconf(_SC_PAGESIZE);
    if (size == 0) size = 1;

    size_t meta_size = (sizeof(aether_handle_t) + 15) & ~15;
    size_t data_pages_size = (size + meta_size + aether_page_size - 1) & ~(aether_page_size - 1);
    size_t total_size = aether_page_size * 2 + data_pages_size;

    void *base = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED) return NULL;

    mprotect(base, aether_page_size, PROT_NONE);
    mprotect((char*)base + total_size - aether_page_size, aether_page_size, PROT_NONE);

    aether_handle_t *h = (aether_handle_t*)((char*)base + aether_page_size);
    h->base       = base;
    h->total_size = total_size;
    h->user_size  = size;
    h->magic      = AETHER_ALIVE;
    h->file       = file;
    h->line       = line;
    h->id         = __sync_fetch_and_add(&aether_id_counter, 1);
    h->ptr        = (char*)h + meta_size;

    return h->ptr;
}

#define aether_alloc(size) aether_alloc_internal(size, __FILE__, __LINE__)

static inline void aether_free(void *ptr) {
    if (!ptr) return;

    for (int i = 0; i < AETHER_STASH_SIZE; i++) {
        if (aether_stash[i] == ptr) {
            fprintf(stderr, "[AETHER] Prevented Double-free (ZOMBIED): %p\n", ptr);
            return;
        }
    }

    if (aether_page_size == 0) aether_page_size = sysconf(_SC_PAGESIZE);

    size_t meta_size = (sizeof(aether_handle_t) + 15) & ~15;
    aether_handle_t *h = (aether_handle_t*)((char*)ptr - meta_size);

    if (h->magic != AETHER_ALIVE) {
        return;
    }

    memset(ptr, 0xAE, h->user_size);
    if (h->user_size >= 8) {
        *(volatile uint64_t*)ptr = AETHER_FREED;
    }

    void *base = h->base;
    size_t total_size = h->total_size;
    h->magic = AETHER_FREED;

    aether_stash[aether_stash_idx] = ptr;
    aether_stash_idx = (aether_stash_idx + 1) % AETHER_STASH_SIZE;

    munmap(base, total_size);
}

#endif
