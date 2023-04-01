// vmem.h - v0.1 - public domain
// no warranty implied; use at your own risk.
//
// Do this:
//    #define VMEM_IMPLEMENTATION
// before you include this file in *one* C or C++ file to create the implementation.
//
// i.e. it should look like this:
//      #include ...
//      #include ...
//      #define VMEM_IMPLEMENTATION
//      #include "vmem.h"
//
// LICENSE
//      See end of file for license information.

#ifndef INCLUDE_VMEM_H
#define INCLUDE_VMEM_H

#include <stdint.h>
#include <stddef.h> // size_t

#define VMEM_FUNC

#ifndef VMEM_ASSERT
#include <assert.h>
#define VMEM_ASSERT(cond) assert(cond)
#endif

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public API
//

typedef uint8_t Vmem_Protect;

typedef enum Vmem_Protect_ {
    Vmem_Protect_Invalid = 0,
    Vmem_Protect_NoAccess,         // The memory cannot be accessed at all.
    Vmem_Protect_Read,             // You can only read from the memory.
    Vmem_Protect_ReadWrite,        // You can read and write to the memory. This is the most common option for memory
                                   // allocations.
    Vmem_Protect_Execute,          // You can only execute the memory.
    Vmem_Protect_ExecuteRead,      // You can execute the memory and read from it.
    Vmem_Protect_ExecuteReadWrite, // You can execute the memory and read/write to it.
    Vmem_Protect_COUNT,
} Vmem_Protect_;

// Reserves (allocates but doesn't commit) a block of virtual address-space of size `num_bytes`.
VMEM_FUNC void* vmem_alloc_protect(size_t num_bytes, Vmem_Protect protect);

// Reserves (allocates but doesn't commit) a block of virtual address-space of size `num_bytes`, in ReadWrite protection
// mode. The memory is zeroed. Free with `vmem_free`. Note: you must commit the memory before using it.
// @param num_bytes: total size of the memory block.
static inline void* vmem_alloc(const size_t num_bytes) {
    return vmem_alloc_protect(num_bytes, Vmem_Protect_ReadWrite);
}

// Frees (releases) a block of virtual memory.
// @param ptr: a pointer to the start of the memory block. Result of `vmem_alloc`.
// @param num_allocated_bytes: *must* be the value returned by `vmem_alloc`.
//  It isn't used on windows, but it's required on unix platforms.
VMEM_FUNC void vmem_free(void* ptr, size_t num_allocated_bytes);

VMEM_FUNC void vmem_commit_protect(void* ptr, size_t num_bytes, Vmem_Protect protect);

// Commit ReadWrite memory pages which contain one or more bytes in [ptr...ptr+num_bytes].
// This maps the pages to physical memory.
// Decommit with `vmem_decommit`.
// @param ptr: pointer to the pointer returned by `vmem_alloc` or shifted by [0...num_bytes].
// @param num_bytes: number of bytes to commit.
// @param protect: protection mode for the newly commited pages.
static inline void vmem_commit(void* ptr, const size_t num_bytes) {
    return vmem_commit_protect(ptr, num_bytes, Vmem_Protect_ReadWrite);
}

// Decommits the memory pages which contain one or more bytes in [ptr...ptr+num_bytes].
// This unmaps the pages from physical memory.
// Note: if you want to use the memory region again, you need to use `vmem_commit`.
// @param ptr: pointer to the pointer returned by `vmem_alloc` or shifted by [0...num_bytes].
// @param num_bytes: number of bytes to decommit.
VMEM_FUNC void vmem_decommit(void* ptr, size_t num_bytes);

// Sets protection mode for the region of pages. All of the pages must be commited.
VMEM_FUNC void vmem_set_protect(void* ptr, size_t num_bytes, Vmem_Protect protect);

// Returns the page size in number bytes.
// Uses cached value from `vmem_query_page_size`, loaded at startup time.
VMEM_FUNC size_t vmem_get_page_size();

// Returns the page size in number bytes.
// Usually something like 4096.
VMEM_FUNC size_t vmem_query_page_size();

// Locks the specified region of the process's virtual address space into physical memory, ensuring that subsequent
// access to the region will not incur a page fault.
// All pages in the specified region must be commited.
// You cannot lock pages with `Vmem_Protect_NoAccess`.
VMEM_FUNC void vmem_lock(void* ptr, size_t num_bytes);

// Unlocks a specified range of pages in the virtual address space of a process, enabling the system to swap the pages
// out to the paging file if necessary.
// If you try to unlock pages which aren't locked, this will fail.
VMEM_FUNC void vmem_unlock(void* ptr, size_t num_bytes);


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Utilities
//

// Returns a static string for the protection mode.
// e.g. Vmem_Protect_ReadWrite will return "ReadWrite".
// Unknown values return "<Unknown>".
VMEM_FUNC const char* vmem_protect_name(Vmem_Protect protect);

// Round the `address` up to the next (or current) aligned address.
// @param address: Memory address to align.
// @param align: Address alignment. Must be a power of 2 and greater than 0.
static inline uintptr_t vmem_align_forward(const uintptr_t address, const int align) {
    VMEM_ASSERT(align != 0 && "[vmem_align_forwards] `align` paramter cannot be zero.");
    const uintptr_t mask = (uintptr_t)(align - 1);
    VMEM_ASSERT((align & mask) == 0 && "[vmem_align_forwards] `align` parameter has to be a power of 2.");
    return (address + mask) & ~mask;
}

// Round the `address` down to the previous (or current) aligned address.
// @param address: Memory address to align.
// @param align: Address alignment. Must be a power of 2 and greater than 0.
static inline uintptr_t vmem_align_backward(const uintptr_t address, const int align) {
    VMEM_ASSERT(align != 0 && "[vmem_align_backwards] `align` paramter cannot be zero.");
    VMEM_ASSERT((align & (align - 1)) == 0 && "[vmem_align_backwards] `align` parameter has to be a power of 2.");
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



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VMEM_IMPLEMENTATION
//
#if defined(VMEM_IMPLEMENTATION)

#ifndef VMEM_UNUSED
#define VMEM_UNUSED(varible) (void)(varible)
#endif

// Includes
#include <stdint.h>
#include <stddef.h>
#if defined(VMEM_PLATFORM_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif defined(VMEM_PLATFORM_LINUX)
#include <unistd.h>
#include <sys/mman.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Cached page size.
size_t vmem__g_page_size = vmem_query_page_size();

VMEM_FUNC size_t vmem_get_page_size() {
    return vmem__g_page_size;
}

VMEM_FUNC const char* vmem_protect_name(const Vmem_Protect protect) {
    switch(protect) {
        case Vmem_Protect_Invalid: return "INVALID";
        case Vmem_Protect_NoAccess: return "NoAccess";
        case Vmem_Protect_Read: return "Read";
        case Vmem_Protect_ReadWrite: return "ReadWrite";
        case Vmem_Protect_Execute: return "Execute";
        case Vmem_Protect_ExecuteRead: return "ExecuteRead";
        case Vmem_Protect_ExecuteReadWrite: return "ExecuteReadWrite";
    }
    return "<Unknown>";
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Windows implementation
//
#if defined(VMEM_PLATFORM_WIN32)
static DWORD vmem__win32_protect(const Vmem_Protect protect) {
    VMEM_ASSERT(protect != Vmem_Protect_Invalid);
    switch(protect) {
        case Vmem_Protect_NoAccess: return PAGE_NOACCESS;
        case Vmem_Protect_Read: return PAGE_READONLY;
        case Vmem_Protect_ReadWrite: return PAGE_READWRITE;
        case Vmem_Protect_Execute: return PAGE_EXECUTE;
        case Vmem_Protect_ExecuteRead: return PAGE_EXECUTE_READ;
        case Vmem_Protect_ExecuteReadWrite: return PAGE_EXECUTE_READWRITE;
    }
    VMEM_ASSERT(0 && "[vmem__win32_protect] Unknown protect mode.");
    // Note: what should be the behavior on invalid protection mode (if the above assertion is turned off?).
    return PAGE_READWRITE;
}

VMEM_FUNC void* vmem_alloc_protect(const size_t num_bytes, const Vmem_Protect protect) {
    if(num_bytes == 0) return 0;

    LPVOID address = VirtualAlloc(NULL, (SIZE_T)num_bytes, MEM_RESERVE, vmem__win32_protect(protect));
    VMEM_ASSERT(address != NULL && "[vmem_alloc_protect] VirtualAlloc failed");
    // Note: memory is initialized to zero.
    return address;
}

VMEM_FUNC void vmem_free(void* ptr, const size_t num_allocated_bytes) {
    VMEM_ASSERT(ptr != 0);
    VMEM_ASSERT(num_allocated_bytes > 0);
    VMEM_UNUSED(num_allocated_bytes);

    const BOOL result = VirtualFree(ptr, 0, MEM_RELEASE);
    VMEM_ASSERT(result != 0 && "[vmem_free] VirtualFree failed.");
}

VMEM_FUNC void vmem_commit_protect(void* ptr, const size_t num_bytes, const Vmem_Protect protect) {
    VMEM_ASSERT(ptr != 0);
    if(num_bytes == 0) return;

    const LPVOID result = VirtualAlloc(ptr, num_bytes, MEM_COMMIT, vmem__win32_protect(protect));
    VMEM_ASSERT(result != NULL && "[vmem_commit_protect] VirtualAlloc failed to commit memory.");
}

VMEM_FUNC void vmem_decommit(void* ptr, const size_t num_bytes) {
    VMEM_ASSERT(ptr != 0);
    if(num_bytes == 0) return;

    const BOOL result = VirtualFree(ptr, num_bytes, MEM_DECOMMIT);
    VMEM_ASSERT(result != 0 && "[vmem_decommit] VirtualFree failed.");
}

VMEM_FUNC void vmem_set_protect(void* ptr, const size_t num_bytes, const Vmem_Protect protect) {
    VMEM_ASSERT(ptr != 0);
    if(num_bytes == 0) return;

    DWORD old_protect = 0;
    const BOOL result = VirtualProtect(ptr, num_bytes, vmem__win32_protect(protect), &old_protect);
    VMEM_ASSERT(result != 0 && "[vmem_set_protect] VirtualProtect failed");
}

VMEM_FUNC size_t vmem_query_page_size() {
    SYSTEM_INFO system_info = {};
    GetSystemInfo(&system_info);
    return system_info.dwPageSize;
}

VMEM_FUNC void vmem_lock(void* ptr, const size_t num_bytes) {
    VMEM_ASSERT(ptr != 0);
    if(num_bytes == 0) return;

    const BOOL result = VirtualLock(ptr, num_bytes);
    VMEM_ASSERT(result == 0 && "[vmem_lock] VirtualLock failed.");
}

VMEM_FUNC void vmem_unlock(void* ptr, const size_t num_bytes) {
    VMEM_ASSERT(ptr != 0);
    if(num_bytes == 0) return;

    const BOOL result = VirtualUnlock(ptr, num_bytes);
    VMEM_ASSERT(result == 0 && "[vmem_unlock] VirtualUnlock failed.");
}

#endif // defined(VMEM_PLATFORM_WIN32)



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Linux implementation
//
#if defined(VMEM_PLATFORM_LINUX)
static int vmem__linux_protect(const Vmem_Protect protect) {
    VMEM_ASSERT(protect != Vmem_Protect_Invalid);
    switch(protect) {
        case Vmem_Protect_NoAccess: return PROT_NONE;
        case Vmem_Protect_Read: return PROT_READ;
        case Vmem_Protect_ReadWrite: return PROT_READ | PROT_WRITE;
        case Vmem_Protect_Execute: return PROT_EXEC;
        case Vmem_Protect_ExecuteRead: return PROT_EXEC | PROT_READ;
        case Vmem_Protect_ExecuteReadWrite: return PROT_EXEC | PROT_READ | PROT_WRITE;
    }
    VMEM_ASSERT(0 && "[vmem__linux_protect] Unknown protect mode.");
    return PROT_NONE;
}

VMEM_FUNC void* vmem_alloc_protect(const size_t num_bytes, const Vmem_Protect protect) {
    if(num_bytes == 0) return 0;

    const int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    const int prot = vmem__linux_protect(protect);
    // Note: memory is always initialized to zero when using MAP_ANONYMOUS.
    void* result = mmap(0, num_bytes, prot, flags, -1, 0);
    VMEM_ASSERT(result != (void*)-1 && "[vmem_alloc_protect] mmap failed.");
    return result;
}

VMEM_FUNC void vmem_free(void* ptr, const size_t num_allocated_bytes) {
    VMEM_ASSERT(ptr != 0);
    VMEM_ASSERT(num_allocated_bytes > 0);

    // https://man7.org/linux/man-pages/man3/munmap.3p.html
    const int result = munmap(ptr, num_allocated_bytes);
    VMEM_ASSERT(result == 0 && "[vmem_free] munmap failed.");
}

VMEM_FUNC void vmem_commit_protect(void* ptr, const size_t num_bytes, const Vmem_Protect protect) {
    VMEM_ASSERT(ptr != 0);
    if(num_bytes == 0) return;

    // On linux the pages are created in a reserved state and automatically commited on the first write, so we don't
    // need to commit anything.
    // But for compatibility with other platforms, we have to set the protection level.
    vmem_set_protect(ptr, num_bytes, protect);
}

VMEM_FUNC void vmem_decommit(void* ptr, const size_t num_bytes) {
    VMEM_ASSERT(ptr != 0);
    if(num_bytes == 0) return;

    const int result = madvise(ptr, num_bytes, MADV_DONTNEED);
    VMEM_ASSERT(result == 0 && "[vmem_decommit] madvise failed.");
}

VMEM_FUNC void vmem_set_protect(void* ptr, const size_t num_bytes, const Vmem_Protect protect) {
    VMEM_ASSERT(ptr != 0);
    if(num_bytes == 0) return;

    const int result = mprotect(ptr, num_bytes, vmem__linux_protect(protect));
    VMEM_ASSERT(result == 0 && "[vmem_set_protect] mprotect failed.");
}

VMEM_FUNC size_t vmem_query_page_size() {
    return (size_t)sysconf(_SC_PAGESIZE);
}

VMEM_FUNC void vmem_lock(void* ptr, const size_t num_bytes) {
    VMEM_ASSERT(ptr != 0);
    if(num_bytes == 0) return;

    const int result = mlock(ptr, num_bytes);
    VMEM_ASSERT(result != 0 && "[vmem_lock] mlock failed.");
}

VMEM_FUNC void vmem_unlock(void* ptr, const size_t num_bytes) {
    VMEM_ASSERT(ptr != 0);
    if(num_bytes == 0) return;

    const int result = munlock(ptr, num_bytes);
    VMEM_ASSERT(result != 0 && "[vmem_lock] munlock failed.");
}
#endif // defined(VMEM_PLATFORM_LINUX)



#ifdef __cplusplus
}
#endif

#endif // defined(VMEM_IMPLEMENTATION)

/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2023 Jakub Tomšů
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software freed into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
