// vmem.h - v0.1 - public domain
// no warranty implied; use at your own risk.
//
// Do this:
//    #define VMEM_IMPLEMENTATION
// before you include this file in *one* C or C++ file to create the implementation.
//
// i.e.it should look like this:
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

#ifndef VMEM_UNUSED
#define VMEM_UNUSED(varible) (void)(varible)
#endif

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public API
//

// Allocates (reserves but doesn't commit) a block of virtual address-space of size 'num_bytes'.
// The memory is zeroed.
// @param num_bytes: total size of the memory block. Will be rounded up to the page size by the system.
VMEM_FUNC void* vmem_reserve(size_t num_bytes);

// Frees a block of virtual memory.
// @param ptr: a pointer to the start of the memory block. Result of `vmem_reserve`.
// @param num_reserved_bytes: *must* be the value returned by `vmem_reserve`.
//      It isn't used on windows, but it's required on unix platforms.
VMEM_FUNC void vmem_release(void* ptr, size_t num_reserved_bytes);

// Commit memory pages which contain one or more bytes in [ptr...ptr+num_bytes].
// This maps the pages to physical memory.
// NOTE: you must commit the memory before using it.
// @param ptr: pointer to the pointer returned by `vmem_reserve` or shifted by [0...num_bytes].
// @param num_bytes: number of bytes to commit.
VMEM_FUNC void vmem_commit(void* ptr, size_t num_bytes);

// Decommits the memory pages which contain one or more bytes in [ptr...ptr+num_bytes].
// This unmaps the pages from physical memory.
// NOTE: if you want to use the memory region again, you need to use `vmem_commit`.
// @param ptr: pointer to the pointer returned by `vmem_reserve` or shifted by [0...num_bytes].
// @param num_bytes: number of bytes to decommit.
VMEM_FUNC void vmem_decommit(void* ptr, size_t num_bytes);

// Returns the page size (allocation granularity) in number bytes.
// Usually something like 4096.
VMEM_FUNC size_t vmem_get_page_size();

// Round an address up to the next (or current) aligned address.
// @param address: Memory address to align.
// @param align: Address alignment. Must be a power of 2 and greater than 0.
static inline uintptr_t vmem_align_forward(const uintptr_t address, const int align) {
    VMEM_ASSERT(align != 0 && "[vmem] Alignment cannot be zero.");
    const uintptr_t mask = (uintptr_t)(align - 1);
    VMEM_ASSERT((align & mask) == 0 && "Alignment has to be a power of 2.");
    return (address + mask) & ~mask;
}

// Round the `address` down to the previous (or current) aligned address.
// @param address: Memory address to align.
// @param align: Address alignment. Must be a power of 2 and greater than 0.
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



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VMEM_IMPLEMENTATION
//
#if defined(VMEM_IMPLEMENTATION)

// Includes
#include <stdint.h>
#include <stddef.h>
#if defined(VMEM_PLATFORM_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif defined(VMEM_PLATFORM_LINUX)
#include <unistd.h>
#include <sys/mman.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Windows implementation
//
#if defined(VMEM_PLATFORM_WIN32)

VMEM_FUNC void* vmem_reserve(const size_t num_bytes) {
    VMEM_ASSERT(num_bytes > 0);
    LPVOID address = VirtualAlloc(NULL, (SIZE_T)num_bytes, MEM_RESERVE, PAGE_READWRITE);
    // Note: memory is initialized to zero.
    return address;
}

VMEM_FUNC void vmem_release(void* ptr, const size_t num_reserved_bytes) {
    VMEM_ASSERT(ptr != 0);
    VMEM_ASSERT(num_reserved_bytes > 0);
    VMEM_UNUSED(num_reserved_bytes);
    VirtualFree(ptr, 0, MEM_RELEASE);
}

VMEM_FUNC void vmem_commit(void* ptr, const size_t num_bytes) {
    VMEM_ASSERT(ptr != 0);
    VirtualAlloc(ptr, num_bytes, MEM_COMMIT, PAGE_READWRITE);
}

VMEM_FUNC void vmem_decommit(void* ptr, const size_t num_bytes) {
    VirtualFree(ptr, num_bytes, MEM_DECOMMIT);
}

VMEM_FUNC size_t vmem_get_page_size() {
    SYSTEM_INFO system_info = {};
    GetSystemInfo(&system_info);
    return system_info.dwPageSize;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Linux implementation
//
#elif defined(VMEM_PLATFORM_LINUX)

VMEM_FUNC void* vmem_reserve(const size_t num_bytes) {
    const int prot = PROT_READ | PROT_WRITE;
    const int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    // Note: memory is always initialized to zero when using MAP_ANONYMOUS.
    void* result = mmap(0, num_bytes, prot, flags, -1, 0);
    assert(result != (void*)-1);
    return result;
}

VMEM_FUNC void vmem_release(void* ptr, const size_t num_reserved_bytes) {
    // https://man7.org/linux/man-pages/man3/munmap.3p.html
    const int result = munmap(ptr, num_reserved_bytes);
    VMEM_ASSERT(result == 0);
}

VMEM_FUNC void vmem_commit(void* ptr, const size_t num_bytes) {
    // This call is ignored, because on linux the pages are created in a reserved state
    // and automatically commited on the first write.
    VMEM_UNUSED(ptr);
    VMEM_UNUSED(num_bytes);
}

VMEM_FUNC void vmem_decommit(void* ptr, const size_t num_bytes) {
    const int result = madvise(ptr, num_bytes, MADV_DONTNEED);
    VMEM_ASSERT(result == 0);
}

VMEM_FUNC size_t vmem_get_page_size() {
    return (size_t)sysconf(_SC_PAGESIZE);
}

#endif

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
This is free and unencumbered software released into the public domain.
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