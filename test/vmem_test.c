#include "utest.h"
#include <stdio.h>

#define EXPECT_ERROR_WITH_VMEM_MSG(x) \
    EXPECT_FALSE(x);                  \
    printf("\tVmem error message: %s\n", vmem_get_error_message())

#define MANY 100000

UTEST(vmem, error_messages) {
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_alloc(0));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_alloc(~0));

    EXPECT_ERROR_WITH_VMEM_MSG(vmem_alloc_protect(1, Vmem_Protect_Invalid));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_alloc_protect(1, (Vmem_Protect)12345));

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

    EXPECT_ERROR_WITH_VMEM_MSG(vmem_protect(0, 0, Vmem_Protect_ReadWrite));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_protect(0, 123, Vmem_Protect_ReadWrite));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_protect((void*)1, 0, Vmem_Protect_ReadWrite));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_protect((void*)1, 1, Vmem_Protect_ReadWrite));

    EXPECT_ERROR_WITH_VMEM_MSG(vmem_align_forward(123, 0));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_align_forward(123, 3));

    EXPECT_ERROR_WITH_VMEM_MSG(vmem_align_backward(123, 0));
    EXPECT_ERROR_WITH_VMEM_MSG(vmem_align_backward(123, 3));
}

UTEST(vmem, common) {
    const int size = 1024 * 1024;
    void* ptr = vmem_alloc_protect(size, Vmem_Protect_ReadWrite);
    ASSERT_TRUE(ptr);

    EXPECT_FALSE(vmem_lock(ptr, 1024));
    EXPECT_FALSE(vmem_protect(ptr, 1024, Vmem_Protect_Read));
    EXPECT_TRUE(vmem_commit(ptr, size));

    ASSERT_TRUE(vmem_dealloc(ptr, size));
}

UTEST(vmem, protect_func) {
    const int size = 1024 * 1024;
    void* ptr = vmem_alloc(size);
    ASSERT_TRUE(ptr);
    ASSERT_FALSE(vmem_protect(ptr, size, Vmem_Protect_Read));
    ASSERT_TRUE(vmem_commit(ptr, 1024));

    EXPECT_TRUE(vmem_protect(ptr, 1024, Vmem_Protect_NoAccess));
    EXPECT_TRUE(vmem_protect(ptr, 1024, Vmem_Protect_Read));
    EXPECT_TRUE(vmem_protect(ptr, 1024, Vmem_Protect_ReadWrite));
    EXPECT_TRUE(vmem_protect(ptr, 1024, Vmem_Protect_Execute));
    EXPECT_TRUE(vmem_protect(ptr, 1024, Vmem_Protect_ExecuteRead));
    EXPECT_TRUE(vmem_protect(ptr, 1024, Vmem_Protect_ExecuteReadWrite));
    EXPECT_FALSE(vmem_protect(ptr, size, Vmem_Protect_ReadWrite));

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
    ASSERT_TRUE(vmem_commit_protect(ptr, 1024, Vmem_Protect_NoAccess));
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
    ASSERT_FALSE(vmem_protect(ptr + vmem_get_page_size(), vmem_get_page_size(), Vmem_Protect_Read));

    ASSERT_TRUE(vmem_commit(ptr, size));
    ASSERT_TRUE(vmem_protect(ptr, size, Vmem_Protect_Read));

    ASSERT_TRUE(vmem_dealloc(ptr, size));
}

/*
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#define SIZE (1024 * 512) // 0.5MB

#define PRINT_JUST_SHORT_INFO 1

#if defined(VMEM_PLATFORM_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static const char* protect_mode_name_win32(const ULONG protect) {
    switch(protect) {
        case 0: return "<Can't access>";
        case PAGE_NOACCESS: return "NOACCESS";
        case PAGE_EXECUTE: return "EXECUTE";
        case PAGE_READONLY: return "READONLY";
        case PAGE_READWRITE: return "READWRITE";
        case PAGE_GUARD: return "GUARD";
        case PAGE_NOCACHE: return "NOCACHE";
        case PAGE_WRITECOMBINE: return "WRITECOMBINE";
    }
    return "<Unknown>";
}

// Print allocation regions and page states.
static void print_allocation_info_win32(void* ptr, const size_t num_bytes_to_scan) {
    printf("[print_allocation_info_win32] Win32 VirtualQuery info for %p\n", ptr);

    static char overview[1024 * 8] = {};
    int overview_len = 0;

    for(size_t i = 0; i < num_bytes_to_scan;) {
        static MEMORY_BASIC_INFORMATION info = {};

        void* p = (void*)((uintptr_t)ptr + i);
        SIZE_T num_bytes = VirtualQuery(p, &info, sizeof(info));
        assert(num_bytes != 0);

        const char* state_str = "<Unknown>";
        switch(info.State) {
            case MEM_COMMIT: state_str = "COMMIT"; break;
            case MEM_DEALLOC: state_str = "DEALLOC"; break;
            case MEM_RESERVE: state_str = "RESERVE"; break;
        }

        const char* type_str = "<Unknown>";
        switch(info.Type) {
            case MEM_IMAGE: type_str = "IMAGE"; break;
            case MEM_MAPPED: type_str = "MAPPED"; break;
            case MEM_PRIVATE: type_str = "PRIVATE"; break;
        }

        const char* initial_protect_str = protect_mode_name_win32(info.AllocationProtect);
        const char* protect_str = protect_mode_name_win32(info.Protect);

        const int region_pages = info.RegionSize / vmem_get_page_size();

#if PRINT_JUST_SHORT_INFO
        VMEM_UNUSED(initial_protect_str);
        // 'p' stands for pages
        printf(
            "\t\tOffs: %06llib (%04ip), Size: %06llib (%04ip), State: %10s, Protect: %16s, Type: %10s\n",
            (intptr_t)info.BaseAddress - (intptr_t)ptr,
            (int)(((intptr_t)info.BaseAddress - (intptr_t)ptr) / vmem_get_page_size()),
            info.RegionSize,
            region_pages,
            state_str,
            protect_str,
            type_str);
#else
        printf(
            "\tinfo at offset %llu bytes\n"
            "\t\tBaseAddress:          %p (%lli bytes from start ptr)\n"
            "\t\tAllocationBase:       %p\n"
            "\t\tAllocationProtect:    %i (%s)\n"
            "\t\tRegionSize:           %llu bytes (%i pages)\n"
            "\t\tState:                %i (%s)\n"
            "\t\tProtect:              %i (%s)\n"
            "\t\tType:                 %i (%s)\n"
            "\n",
            i,
            info.BaseAddress,
            (intptr_t)info.BaseAddress - (intptr_t)ptr,
            info.AllocationBase,
            (int)info.AllocationProtect,
            initial_protect_str,
            info.RegionSize,
            region_pages,
            (int)info.State,
            state_str,
            (int)info.Protect,
            protect_str,
            (int)info.Type,
            type_str);
#endif

        if(overview_len + region_pages < sizeof(overview)) {
            char overview_char = '?';
            switch(info.State) {
                case MEM_COMMIT: overview_char = 'C'; break;
                case MEM_DEALLOC: overview_char = 'F'; break;
                case MEM_RESERVE: overview_char = 'r'; break;
            }
            for(int j = 0; j < region_pages; j++) {
                overview[overview_len + j] = overview_char;
            }
            overview_len += region_pages;
        }

        i += info.RegionSize;
    }

    if(overview_len < sizeof(overview)) {
        overview[overview_len] = '\0';
    } else {
        overview[sizeof(overview) - 4] = '.';
        overview[sizeof(overview) - 3] = '.';
        overview[sizeof(overview) - 2] = '.';
        overview[sizeof(overview) - 1] = '\0';
    }

    printf("\tPage state overview: (C: commited, F: dealloc, r: reserved)\n\t\t%s\n", overview);
}
#endif

int main() {
    assert(vmem_get_page_size() == vmem_query_page_size());

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

    const size_t page_size = vmem_get_page_size();

    // Basic stuff
    printf("Test basic...\n");
    {
        printf("Page size: %llu\n", page_size);

        uint8_t* ptr = (uint8_t*)vmem_alloc(SIZE);
        vmem_commit(ptr, page_size * 2);
        for(int i = 0; i < page_size * 2; i++) {
            ptr[i] = 0xfa;
        }

        vmem_decommit(ptr, page_size);

        for(int i = page_size; i < page_size * 2; i++) {
            ptr[i] = 0xff;
        }

        vmem_dealloc(ptr, SIZE);
    }

    // Windows query virtual memory info
#if defined(VMEM_PLATFORM_WIN32)
    {
        uint8_t* ptr = (uint8_t*)vmem_alloc(SIZE);

        print_allocation_info_win32(ptr, SIZE);

        vmem_commit(ptr, vmem_get_page_size() * 4);

        print_allocation_info_win32(ptr, SIZE);

        for(int i = 0; i < 20; i++) {
            vmem_commit(ptr + i * vmem_get_page_size() * 2, 1);
        }

        print_allocation_info_win32(ptr, SIZE);

        vmem_dealloc(ptr, SIZE);
    }
#endif

    // Locking/unlocking
    {
        void* ptr = vmem_alloc(SIZE);

        vmem_commit(ptr, SIZE);
        vmem_lock(ptr, SIZE);
        vmem_unlock(ptr, SIZE);

        vmem_dealloc(ptr, SIZE);
    }

    // Protection
    {
        uint64_t* ptr = (uint64_t*)vmem_alloc_protect(SIZE, Vmem_Protect_NoAccess);

        vmem_commit_protect(ptr, SIZE, Vmem_Protect_ReadWrite);

        for(int i = 0; i < 200; i++) {
            ptr[i] = i;
        }

        vmem_set_protect(ptr, SIZE, Vmem_Protect_Read);

        for(int i = 0; i < 200; i++) {
            printf("%llx ", ptr[i]);
        }
        printf("\n");

        vmem_dealloc(ptr, SIZE);
    }

    // Huge allocation
    {
        // 100TB
        const size_t size = 1024LLU * 1024LLU * 1024LLU * 1024LLU * 100LLU;
        printf(
            "Huge allocation size: %llu bytes (%lluGB, %lluTB)\n",
            size,
            size / (1024LLU * 1024LLU * 1024LLU),
            size / (1024LLU * 1024LLU * 1024LLU * 1024LLU));
        void* ptr = vmem_alloc(size);

        vmem_commit(ptr, 1024 * 100);

#if defined(VMEM_PLATFORM_WIN32)
        print_allocation_info_win32(ptr, size);
#endif

        vmem_dealloc(ptr, size);
    }

    return 0;
}
*/