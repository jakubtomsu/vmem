#include <stdint.h>
#include <stdio.h>
#define VMEM_IMPLEMENTATION
#include "vmem.h"

#define SIZE (1024 * 1024 * 2)

int main() {
    const size_t page_size = vmem_get_page_size();
    printf("Page size: %i\n", page_size);

    uint8_t* mem = (uint8_t*)vmem_reserve(SIZE);
    vmem_commit(mem, page_size * 2);
    for(int i = 0; i < page_size * 2; i++) {
        mem[i] = 0xfa;
    }

    vmem_release(mem);
    return 0;
}