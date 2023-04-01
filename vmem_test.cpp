// This is a simple program to demonstrate the API and test it's features for correctness.

#define VMEM_IMPLEMENTATION
#include "vmem.h"

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#define SIZE (1024 * 512)

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
static void print_allocation_info_win32(void* ptr, const int num_bytes_to_scan) {
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
            case MEM_FREE: state_str = "FREE"; break;
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
                case MEM_FREE: overview_char = 'F'; break;
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
        overview[sizeof(overview) - 1] = '\0';
    }

    printf("\tPage state overview: (C: commited, F: free, r: reserved)\n\t\t%s\n", overview);
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

        vmem_free(ptr, SIZE);
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

        vmem_free(ptr, SIZE);
    }
#endif

    // Locking/unlocking
    {
        void* ptr = vmem_alloc(SIZE);

        vmem_commit(ptr, SIZE);
        vmem_lock(ptr, SIZE);
        vmem_unlock(ptr, SIZE);

        vmem_free(ptr, SIZE);
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

        vmem_free(ptr, SIZE);
    }

    return 0;
}