#define VARENA_IMPLEMENTATION
#include "../varena.h"
#include "utest.h"
#include <stdio.h>

UTEST(varena, common) {
    const int size = 1024 * 1024;
    VArena arena = varena_init(size);
    ASSERT_TRUE(varena_is_valid(arena));
    ASSERT_EQ(arena._buf_len, size);
    ASSERT_TRUE(varena_push(&arena, 1024));
    ASSERT_EQ(arena.len, 1024);
    ASSERT_EQ(arena._commited, 1024);
    for(int i = 0; i < 10; i++) {
        ASSERT_TRUE(varena_push(&arena, vmem_get_page_size() * 2));
    }
    varena_deinit(&arena);
}
