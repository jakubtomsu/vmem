#define ARENA_IMPLEMENTATION
#include "../arena.h"
#include "utest.h"
#include <stdio.h>

UTEST(arena, common) {
    const int size = 1024 * 1024;
    Arena arena = arena_init(size);
    ASSERT_TRUE(arena_is_valid(arena));
    ASSERT_EQ(arena._buf_len, size);
    ASSERT_TRUE(arena_push(&arena, 1024));
    ASSERT_EQ(arena.len, 1024);
    ASSERT_EQ(arena._commited, 1024);
    for(int i = 0; i < 10; i++) {
        ASSERT_TRUE(arena_push(&arena, vmem_get_page_size() * 2));
    }
    arena_deinit(&arena);
}
