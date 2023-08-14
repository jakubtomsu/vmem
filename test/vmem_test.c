#define VMEM_ON_ERROR(opt_string) // Ignore for tests
#define VMEM_IMPLEMENTATION
#include "../vmem.h"
#include "utest.h"
#include <stdio.h>

#define EXPECT_ERROR_WITH_VMEM_MSG(x) \
    EXPECT_FALSE(x);                  \
    printf("\tVmem error message: %s\n", vmem_get_error_message())

#define MANY 100000

UTEST(vmem, error_messages) {
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_alloc(0));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_alloc(~0));

    EXPECT_ERROR_WITH_VMEM_MSG(vmem_alloc_protect(1, VMemProtect_Invalid));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_alloc_protect(1, (VMemProtect)12345));

    EXPECT_ERROR_WITH_VMEM_MSG(vmem_dealloc(0, 0));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_dealloc(0, 123));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_dealloc((void*)1, 0));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_dealloc((void*)1, 1));

    EXPECT_ERROR_WITH_VMEM_MSG(vmem_lock(0, 0));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_lock(0, 123));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_lock((void*)1, 0));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_lock((void*)1, 1));

    EXPECT_ERROR_WITH_VMEM_MSG(vmem_unlock(0, 0));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_unlock(0, 123));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_unlock((void*)1, 0));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_unlock((void*)1, 1));

    EXPECT_ERROR_WITH_VMEM_MSG(vmem_protect(0, 0, VMemProtect_ReadWrite));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_protect(0, 123, VMemProtect_ReadWrite));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_protect((void*)1, 0, VMemProtect_ReadWrite));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_protect((void*)1, 1, VMemProtect_ReadWrite));

    EXPECT_ERROR_WITH_VMEM_MSG(vmem_align_forward(123, 0));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_align_forward(123, 3));

    EXPECT_ERROR_WITH_VMEM_MSG(vmem_align_backward(123, 0));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_align_backward(123, 3));

    EXPECT_ERROR_WITH_VMEM_MSG(vmem_is_aligned(3, 4));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_is_aligned(25, 4));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_is_aligned(0, 0));

    VMemRangeInfo range_info = {0};
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_query_range_info(0, 0, 0, 0));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_query_range_info((void*)1, 0, 0, 0));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_query_range_info((void*)1, 1, 0, 0));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_query_range_info((void*)1, 1, &range_info, 0));
}

UTEST(vmem, common) {
    const int size = 1024 * 1024;
    void* ptr = vmem_alloc_protect(size, VMemProtect_ReadWrite);
    ASSERT_TRUE(ptr);

    EXPECT_FALSE(vmem_lock(ptr, 1024));
    EXPECT_FALSE(vmem_protect(ptr, 1024, VMemProtect_Read));
    EXPECT_TRUE(vmem_commit(ptr, size));

    ASSERT_TRUE(vmem_dealloc(ptr, size));
}

UTEST(vmem, is_aligned) {
    ASSERT_TRUE(vmem_is_aligned(8, 4) == 1);
    ASSERT_TRUE(vmem_is_aligned(3, 4) == 0);
    ASSERT_TRUE(vmem_is_aligned(25, 4) == 0);
    ASSERT_TRUE(vmem_is_aligned(0, 4) == 1);
    ASSERT_TRUE(vmem_is_aligned(0, 0) == 0);
}

UTEST(vmem, protect_func) {
    const int size = 1024 * 1024;
    void* ptr = vmem_alloc(size);
    ASSERT_TRUE(ptr);
    ASSERT_FALSE(vmem_protect(ptr, size, VMemProtect_Read));
    ASSERT_TRUE(vmem_commit(ptr, 1024));

    EXPECT_TRUE(vmem_protect(ptr, 1024, VMemProtect_NoAccess));
    EXPECT_TRUE(vmem_protect(ptr, 1024, VMemProtect_Read));
    EXPECT_TRUE(vmem_protect(ptr, 1024, VMemProtect_ReadWrite));
    EXPECT_TRUE(vmem_protect(ptr, 1024, VMemProtect_Execute));
    EXPECT_TRUE(vmem_protect(ptr, 1024, VMemProtect_ExecuteRead));
    EXPECT_TRUE(vmem_protect(ptr, 1024, VMemProtect_ExecuteReadWrite));
    EXPECT_FALSE(vmem_protect(ptr, size, VMemProtect_ReadWrite));

    ASSERT_TRUE(vmem_dealloc(ptr, size));
}

UTEST(vmem, lock_func) {
    const int size = 1024 * 1024;
    void* ptr = vmem_alloc(size);
    ASSERT_TRUE(ptr);

    EXPECT_FALSE(vmem_lock(0, 0));
    EXPECT_FALSE(vmem_lock(0, 123));
    EXPECT_FALSE(vmem_lock((void*)1, 0));
    EXPECT_FALSE(vmem_lock((void*)1, 1));

    EXPECT_FALSE(vmem_unlock(0, 0));
    EXPECT_FALSE(vmem_unlock(0, 123));
    EXPECT_FALSE(vmem_unlock((void*)1, 0));
    EXPECT_FALSE(vmem_unlock((void*)1, 1));

    EXPECT_FALSE(vmem_lock(ptr, 1024));
    ASSERT_TRUE(vmem_commit(ptr, 1024));
    EXPECT_TRUE(vmem_lock(ptr, 1024));
    ASSERT_TRUE(vmem_commit_protect(ptr, 1024, VMemProtect_NoAccess));
    EXPECT_FALSE(vmem_lock(ptr, 1024));

    ASSERT_TRUE(vmem_dealloc(ptr, size));
}

UTEST(vmem, page_size_func) {
    ASSERT_TRUE(vmem_get_page_size() > 0);
    ASSERT_TRUE(vmem_query_page_size() > 0);
    ASSERT_TRUE(vmem_query_page_size() == vmem_get_page_size());
}

UTEST(vmem, allocation_granularity) {
    ASSERT_TRUE(vmem_get_allocation_granularity() > 0);
    ASSERT_TRUE(vmem_query_allocation_granularity() > 0);
    ASSERT_TRUE(vmem_query_allocation_granularity() == vmem_get_allocation_granularity());
}

UTEST(vmem, many_allocs_deallocs_perf) {
    for(int i = 1; i < MANY; i++) {
        void* ptr = vmem_alloc(i);
        ASSERT_TRUE(ptr);
        ASSERT_TRUE(vmem_dealloc(ptr, i));
    }
}

UTEST(vmem, many_allocs_commits_deallocs_perf) {
    for(int i = 1; i < MANY; i++) {
        void* ptr = vmem_alloc(i);
        ASSERT_TRUE(ptr);
        ASSERT_TRUE(vmem_commit(ptr, i));
        ASSERT_TRUE(vmem_dealloc(ptr, i));
    }
}

UTEST(vmem, many_small_recommits_perf) {
    void* ptr = vmem_alloc(MANY);
    ASSERT_TRUE(ptr);
    for(int i = 1; i < MANY; i++) {
        ASSERT_TRUE(vmem_commit(ptr, i));
    }
    ASSERT_TRUE(vmem_dealloc(ptr, MANY));
}

UTEST(vmem, page_commits_perf) {
    const int num_pages = 1000;
    const int size = num_pages * vmem_get_page_size();
    void* ptr = vmem_alloc(size);
    ASSERT_TRUE(ptr);
    for(int i = 1; i < num_pages; i++) {
        ASSERT_TRUE(vmem_commit(ptr, i * vmem_get_page_size()));
    }
    ASSERT_TRUE(vmem_dealloc(ptr, size));
}

UTEST(vmem, overlapped_page) {
    const int size = 2 * vmem_get_page_size();
    uint8_t* ptr = (uint8_t*)vmem_alloc(size);
    ASSERT_TRUE(ptr);

    ASSERT_TRUE(vmem_commit(ptr, vmem_get_page_size()));
    // This should error because only the first page should be commited.
    ASSERT_FALSE(vmem_protect(ptr + vmem_get_page_size(), vmem_get_page_size(), VMemProtect_Read));

    ASSERT_TRUE(vmem_commit(ptr, size));
    ASSERT_TRUE(vmem_protect(ptr, size, VMemProtect_Read));

    ASSERT_TRUE(vmem_dealloc(ptr, size));
}

UTEST(vmem, arena_common) {
    VMemArena arena = {0};
    ASSERT_FALSE(vmem_arena_is_valid(&arena));
    arena = vmem_arena_init_alloc(1024 * 1024);
    ASSERT_TRUE(vmem_arena_is_valid(&arena));
    ASSERT_TRUE(vmem_arena_set_commited(&arena, 1024 * 128));
    ASSERT_TRUE(vmem_arena_set_commited(&arena, 1024 * 64));
    ASSERT_TRUE(vmem_arena_set_commited(&arena, 0));
    ASSERT_TRUE(vmem_arena_set_commited(&arena, arena.size_bytes));
    ASSERT_TRUE(vmem_arena_deinit_dealloc(&arena));
    ASSERT_FALSE(vmem_arena_is_valid(&arena));
}

UTEST(vmem, usage_status) {
    VMemUsageStatus status = vmem_query_usage_status();
    ASSERT_GT(status.total_physical_bytes, 0);
    ASSERT_GT(status.avail_physical_bytes, 0);
    printf(
        "VMemUsageStatus { total_physical_bytes: %zu, avail_physical_bytes %zu }\n",
        status.total_physical_bytes,
        status.avail_physical_bytes);
    printf("Total: %iGB\n", (int)((status.total_physical_bytes) / (1024 * 1024 * 1024)));
}

UTEST(vmem, range_info) {
    size_t size = 1024 * 1024;
    void* ptr = vmem_alloc_protect(size, VMemProtect_NoAccess);
    ASSERT_TRUE(ptr);

    VMemRangeInfo info_buf[256] = {0};

    {
        VMemSize info_len = vmem_query_range_info(ptr, size, &info_buf[0], 256);
        ASSERT_EQ(info_len, 1);

        ASSERT_EQ(info_buf[0].ptr, ptr);
        ASSERT_EQ(info_buf[0].size_bytes, size);
        ASSERT_EQ(info_buf[0].is_commited, 0);
        ASSERT_EQ(info_buf[0].protect, VMemProtect_NoAccess);
    }

    ASSERT_TRUE(vmem_commit_protect((void*)((uintptr_t)ptr + vmem_get_page_size()), 1, VMemProtect_Read));

    {
        VMemSize info_len = vmem_query_range_info(ptr, size, &info_buf[0], 256);
        ASSERT_EQ(info_len, 3);

        ASSERT_EQ(info_buf[0].ptr, ptr);
        ASSERT_EQ(info_buf[0].size_bytes, vmem_get_page_size());
        ASSERT_EQ(info_buf[0].is_commited, 0);
        ASSERT_EQ(info_buf[0].protect, VMemProtect_NoAccess);

        ASSERT_EQ(info_buf[1].ptr, (void*)((uintptr_t)ptr + vmem_get_page_size()));
        ASSERT_EQ(info_buf[1].size_bytes, vmem_get_page_size());
        ASSERT_EQ(info_buf[1].is_commited, 1);
        ASSERT_EQ(info_buf[1].protect, VMemProtect_Read);

        ASSERT_EQ(info_buf[2].ptr, (void*)((uintptr_t)ptr + vmem_get_page_size() * 2));
        ASSERT_EQ(info_buf[2].size_bytes, size - vmem_get_page_size() * 2);
        ASSERT_EQ(info_buf[2].is_commited, 0);
        ASSERT_EQ(info_buf[2].protect, VMemProtect_NoAccess);
    }

    {
        VMemSize info_len = vmem_query_range_info(ptr, vmem_get_page_size(), &info_buf[0], 256);
        ASSERT_EQ(info_len, 1);

        ASSERT_EQ(info_buf[0].ptr, ptr);
        ASSERT_EQ(info_buf[0].size_bytes, vmem_get_page_size());
        ASSERT_EQ(info_buf[0].is_commited, 0);
        ASSERT_EQ(info_buf[0].protect, VMemProtect_NoAccess);
    }

    ASSERT_TRUE(vmem_dealloc(ptr, size));
}

UTEST_STATE();

int main(const int argc, const char* argv[]) {
    vmem_init();
    return utest_main(argc, argv);
}
