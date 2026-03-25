#include "aether.h"
#include <stdio.h>
#include <string.h>

void test_auto_cleanup() {
    printf("--- Testing Auto Cleanup ---\n");
    aether_ptr char *p = aether_alloc(100);
    strcpy(p, "Scoped memory");
    printf("Allocated: %s at %p\n", p, (void*)p);
    // p will be freed automatically at the end of this block
}

void test_double_free_protection() {
    printf("\n--- Testing Double Free Protection ---\n");
    void *p = aether_alloc(100);
    printf("First free...\n");
    aether_free(p);
    printf("Second free (should be caught)...\n");
    aether_free(p);
}

int main() {
    test_auto_cleanup();
    test_double_free_protection();
    printf("\nTests complete.\n");
    return 0;
}
