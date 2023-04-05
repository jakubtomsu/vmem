#define VMEM_ON_ERROR(opt_string) // Ignore for tests
#define VMEM_IMPLEMENTATION
#include "../vmem.h"

#include "utest.h"

#define VPOOL_IMPLEMENTATION
#include "../vpool.h"
#include <stdio.h>

UTEST(vpool, common) {
    VPool<int, int> p = {};
    p.init_alloc(1024);
    ASSERT_TRUE(p.is_valid());

    ASSERT_TRUE(p.is_in_bounds(p.put(123)));
    const int i2 = p.put(234);
    ASSERT_EQ(p.head_slot, 2);
    ASSERT_TRUE(i2);
    p.remove(i2);
    ASSERT_EQ(p.head_slot, 2);
    ASSERT_EQ(p.first_free_slot, i2);
    ASSERT_EQ(i2, p.put(345));
    ASSERT_EQ(p.first_free_slot, p.INVALID_INDEX);
    ASSERT_EQ(p.head_slot, 2);

    p.deinit_dealloc();
}

UTEST(vpool, init_arena) {
    VMemArena arena = vmem_arena_init_alloc(1024 * 1024);
    VPool<int, int> p = {};
    p.init(arena.mem, arena.size_bytes);
    ASSERT_TRUE(p.is_valid());
    ASSERT_TRUE(vmem_arena_deinit_dealloc(&arena));
}

UTEST(vpool, init_vmem) {
    const int size = 1024 * 1024;
    void* ptr = vmem_alloc(size);
    VPool<int, int> p = {};
    p.init(ptr, size);
    ASSERT_TRUE(p.is_valid());
    vmem_dealloc(ptr, size);
}

UTEST_STATE();

int main(const int argc, const char* argv[]) {
    vmem_init();
    return utest_main(argc, argv);
}
