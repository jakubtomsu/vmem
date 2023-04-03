#define VPOOL_IMPLEMENTATION
#include "../vpool.h"
#include "utest.h"
#include <stdio.h>

UTEST(vpool, common) {
    VPool pool = vpool_init(1024 * 1024, 64);
    ASSERT_TRUE(vpool_is_valid(&pool));

    ASSERT_TRUE(vpool_alloc(&pool));
    ASSERT_TRUE(pool._head_slot == 1);
    void* second = vpool_alloc(&pool);
    ASSERT_TRUE(second);
    ASSERT_TRUE(pool._head_slot == 2);
    vpool_dealloc(&pool, second);
    ASSERT_TRUE(pool._head_slot == 2);
    ASSERT_TRUE(pool._first_unused_slot = 1);
    ASSERT_TRUE(second == vpool_alloc(&pool));
    ASSERT_TRUE(pool._head_slot == 2);
    ASSERT_TRUE(pool._first_unused_slot = VPOOL_SLOT_INDEX_INVALID);

    vpool_deinit(&pool);
}
