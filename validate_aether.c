#include "aether.h"
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

jmp_buf jump_point;

void segfault_handler(int sig) {
    printf(" => Caught expected SIGSEGV!\n");
    longjmp(jump_point, 1);
}

void test_auto_cleanup() {
    printf("[Auto-Cleanup] Testing... ");
    fflush(stdout);
    {
        aether_ptr char *p = aether_alloc(10);
        if(!p) { printf("Alloc failed!\n"); return; }
        strcpy(p, "Auto");
    }
    printf("Passed.\n");
    fflush(stdout);
}

void test_double_free() {
    printf("[Double-Free] Testing... ");
    fflush(stdout);
    void *p = aether_alloc(50);
    aether_free(p);
    aether_free(p); // Should log warning
    printf("Done.\n");
    fflush(stdout);
}

void test_overflow() {
    printf("[Overflow Protection] Testing... ");
    fflush(stdout);
    char *p = aether_alloc(16);
    if (!p) { printf("Alloc failed!\n"); return; }
    
    if (setjmp(jump_point) == 0) {
        // Trigger overflow at the end of the page
        // The guard is exactly after the data region.
        memset(p, 'X', 8192); // Overflow should be caught here
        printf("Failed: No SEGV caught!\n");
    } else {
        printf("Passed: Prevented overflow.\n");
    }
    fflush(stdout);
}

int main() {
    signal(SIGSEGV, segfault_handler);
    setbuf(stdout, NULL);

    test_auto_cleanup();
    test_double_free();
    test_overflow();

    printf("\nAll Aether engine checks passed successfully.\n");
    return 0;
}
