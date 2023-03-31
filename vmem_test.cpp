// This is a simple program to demonstrate the API and test it's features for correctness.

#define VMEM_IMPLEMENTATION
#include "vmem.h"

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#define SIZE (1024 * 1024 * 2)

int main() {
    assert(vmem_get_page_size() == vmem_query_page_size());

    // Basic stuff
    printf("Test basic...\n");
    {
        const size_t page_size = vmem_get_page_size();
        printf("Page size: %i\n", page_size);

        uint8_t* mem = (uint8_t*)vmem_alloc(SIZE);
        vmem_commit(mem, page_size * 2);
        for(int i = 0; i < page_size * 2; i++) {
            mem[i] = 0xfa;
        }

        vmem_decommit(mem, page_size);

        for(int i = page_size; i < page_size * 2; i++) {
            mem[i] = 0xff;
        }

        vmem_free(mem, SIZE);
    }

    // Test align functions
    printf("Test align functions...\n");
    {
        assert(vmem_align_forward(0, 8) == 0);
        assert(vmem_align_forward(16, 8) == 16);
        assert(vmem_align_forward(1, 8) == 8);
        assert(vmem_align_forward(14, 8) == 16);
        assert(vmem_align_forward(1, 1024) == 1024);

        assert(vmem_align_backward(0, 8) == 0);
        assert(vmem_align_backward(1, 8) == 0);
        assert(vmem_align_backward(14, 8) == 8);
        assert(vmem_align_backward(1, 1024) == 0);
    }

    return 0;
}