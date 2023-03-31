#ifndef INCLUDE_VMEM_H
#define INCLUDE_VMEM_H

#include <stdint.h>

#define VMEM_FUNC

#ifndef VMEM_ASSERT
#include <assert.h>
#define VMEM_ASSERT(cond) assert(cond)
#endif


#ifdef __cplusplus
extern "C" {
#endif

// Allocates (reserves but doesn't commit) a block of virtual memory of size 'num_bytes'
VMEM_FUNC void* vmem_reserve(size_t num_bytes);

// Frees a block of virtual memory.
// ptr: a pointer to the start of the memory block. Result of `vmem_reserve`
VMEM_FUNC void vmem_release(void* ptr);

// Commit memory pages which contain one or more bytes in [ptr...ptr+num_bytes].
// This maps the pages to physical memory.
// NOTE: you must commit the memory before using it.
VMEM_FUNC void vmem_commit(void* ptr, size_t num_bytes);

// Decommits the memory pages which contain one or more bytes in [ptr...ptr+num_bytes].
// This unmaps the pages from physical memory.
// NOTE: if you want to use the memory region again, you need to use `vmem_commit`.
VMEM_FUNC void vmem_decommit(void* ptr, size_t num_bytes);

// Returns the page size (allocation granularity) in number bytes.
// Usually something like 4096.
VMEM_FUNC size_t vmem_get_page_size();

// Round an address up to the next (or current) aligned address.
// The `align` parameter must be a power of 2 and greater than 0.
static inline uintptr_t vmem_align_forward(const uintptr_t address, const int align) {
    VMEM_ASSERT(align != 0 && "[vmem] Alignment cannot be zero.");
    const uintptr_t mask = (uintptr_t)(align - 1);
    VMEM_ASSERT((align & mask) == 0 && "Alignment has to be a power of 2.");
    return (address + mask) & ~mask;
}

// Round the `address` down to the previous (or current) aligned address.
// The `align` parameter must be a power of 2 and greater than 0.
static inline uintptr_t vmem_align_backward(const uintptr_t address, const int align) {
    VMEM_ASSERT(align != 0 && "[vmem] Alignment cannot be zero.");
    VMEM_ASSERT((align & (align - 1)) == 0 && "Alignment has to be a power of 2.");
    return address & ~(align - 1);
}

#ifdef __cplusplus
}
#endif

#if !defined(VMEM_NO_AUTO_PLATFORM)
#if defined(_WIN32)
#define VMEM_PLATFORM_WIN32
#elif defined(__linux__) || defined(__unix__)
#define VMEM_PLATFORM_LINUX
#else
#error "[vmem] Unknown platform."
#endif
#endif // !defined(VMEM_NO_AUTO_PLATFORM)

#endif // INCLUDE_VMEM_H

//
// IMPLEMENTATION
//

#if defined(VMEM_IMPLEMENTATION)

// Includes
#if defined(VMEM_PLATFORM_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif defined(VMEM_PLATFORM_LINUX)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(VMEM_PLATFORM_WIN32)

VMEM_FUNC void* vmem_reserve(size_t num_bytes) {
    LPVOID address = VirtualAlloc(NULL, (SIZE_T)num_bytes, MEM_RESERVE, PAGE_READWRITE);
    return address;
}

VMEM_FUNC void vmem_release(void* ptr) {
    VirtualFree(ptr, 0, MEM_RELEASE);
}

VMEM_FUNC void vmem_commit(void* ptr, size_t num_bytes) {
    VirtualAlloc(ptr, num_bytes, MEM_COMMIT, PAGE_READWRITE);
}

VMEM_FUNC void vmem_decommit(void* ptr, size_t num_bytes) {
    VirtualFree(ptr, num_bytes, MEM_DECOMMIT);
}

VMEM_FUNC size_t vmem_get_page_size() {
    SYSTEM_INFO system_info = {0};
    GetSystemInfo(&system_info);
    return system_info.dwAllocationGranularity;
}

#elif defined(VMEM_PLATFORM_LINUX)
#endif

#ifdef __cplusplus
}
#endif

#endif // defined(VMEM_IMPLEMENTATION)