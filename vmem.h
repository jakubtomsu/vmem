// vmem.h - v0.1 - public domain
// no warranty implied; use at your own risk.
//
// The latest version of this library is available on GitHub:
// https://github.com/jakubtomsu/vmem
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
//
// Compile-time options
//      VMEM_IMPLEMENTATION
//          Instantiate the library implementation in the current source file.
//      VMEM_FUNC
//          Specifiers for all API functions. E.g. you can mark all functions static with `#define VMEM_FUNC static`
//      VMEM_ON_ERROR(opt_string)
//          Called when an error is encountered. By default this just calls `assert(0)`. You can disable it with
//          `#define VMEM_ON_ERROR(opt_string)`.
//      VMEM_NO_ERROR_MESSAGES
//          Disables all error messages. When you call `vmem_get_error_message` it gives you just `<Error messages
//          disabled>`.
//      VMEM_NO_ERROR_CHECKING
//          Completely disables ***all*** error checking. This might be very unsafe.

#if !defined(VMEM_H_INCLUDED)
#define VMEM_H_INCLUDED

#include <stdint.h>
#include <stddef.h> // size_t

#if !defined(VMEM_FUNC)
#define VMEM_FUNC
#endif

#if !defined(_MSC_VER)
#if defined(__cplusplus)
#define VMEM_INLINE inline
#else
#define VMEM_INLINE
#endif // __cplusplus
#else
#define VMEM_INLINE __forceinline
#endif


#if defined(__cplusplus)
extern "C" {
#endif

typedef size_t VMemSize;
typedef uint8_t VMemProtect;
// Success/Error result (VMemResult_).
// You can use this in an if statement: if(vmem_commit(...)) { do something... }
typedef int VMemResult;

typedef enum VMemResult_ {
    VMemResult_Error = 0,   // false
    VMemResult_Success = 1, // true
} VMemResult_;

typedef enum VMemProtect_ {
    VMemProtect_Invalid = 0,
    VMemProtect_NoAccess,         // The page memory cannot be accessed at all.
    VMemProtect_Read,             // You can only read from the page memory .
    VMemProtect_ReadWrite,        // You can read and write to the page memory. This is the most common option.
    VMemProtect_Execute,          // You can only execute the page memory .
    VMemProtect_ExecuteRead,      // You can execute the page memory and read from it.
    VMemProtect_ExecuteReadWrite, // You can execute the page memory and read/write to it.
    VMemProtect_COUNT,
} VMemProtect_;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public API
//

// Call at the start of your program.
// This exists only to cache result of `vmem_query_page_size` so you can use faster `vmem_get_page_size`,
// so this is completely optional. If you don't call this `vmem_get_page_size` will return 0.
// There isn't any deinit/shutdown code.
VMEM_FUNC void vmem_init(void);

// Reserves (allocates but doesn't commit) a block of virtual address-space of size `num_bytes`.
// @returns 0 on error, start address of the allocated memory block on success.
VMEM_FUNC void* vmem_alloc_protect(VMemSize num_bytes, VMemProtect protect);

// Reserves (allocates but doesn't commit) a block of virtual address-space of size `num_bytes`, in ReadWrite protection
// mode. The memory is zeroed. Dealloc with `vmem_dealloc`. Note: you must commit the memory before using it.
// To maximize efficiency, try to always use a multiple of allocation granularity (see
// `vmem_get_allocation_granularity`) for size of allocations.
// @param num_bytes: total size of the memory block.
// @returns 0 on error, start address of the allocated memory block on success.
static VMEM_INLINE void* vmem_alloc(const VMemSize num_bytes) {
    return vmem_alloc_protect(num_bytes, VMemProtect_ReadWrite);
}

// Deallocs (releases) a block of virtual memory.
// @param alloc_ptr: a pointer to the start of the memory block. Result of `vmem_alloc`.
// @param num_allocated_bytes: *must* be the value returned by `vmem_alloc`.
//  It isn't used on windows, but it's required on unix platforms.
VMEM_FUNC VMemResult vmem_dealloc(void* alloc_ptr, VMemSize num_allocated_bytes);

// Commit memory pages which contain one or more bytes in [ptr...ptr+num_bytes]. The pages will be mapped to physical
// memory.
// Decommit with `vmem_decommit`.
// @param ptr: pointer to the pointer returned by `vmem_alloc` or shifted by [0...num_bytes].
// @param num_bytes: number of bytes to commit.
// @param protect: protection mode for the newly commited pages.
VMEM_FUNC VMemResult vmem_commit_protect(void* ptr, VMemSize num_bytes, VMemProtect protect);

// Commit memory pages which contain one or more bytes in [ptr...ptr+num_bytes]. The pages will be mapped to physical
// memory. The page protection mode will be changed to ReadWrite. Use `vmem_commit_protect` to specify a different mode.
// Decommit with `vmem_decommit`.
// @param ptr: pointer to the pointer returned by `vmem_alloc` or shifted by [0...num_bytes].
// @param num_bytes: number of bytes to commit.
static VMEM_INLINE VMemResult vmem_commit(void* ptr, const VMemSize num_bytes) {
    return vmem_commit_protect(ptr, num_bytes, VMemProtect_ReadWrite);
}

// Decommits the memory pages which contain one or more bytes in [ptr...ptr+num_bytes]. The pages will be unmapped from
// physical memory.
// @param ptr: pointer to the pointer returned by `vmem_alloc` or shifted by [0...num_bytes].
// @param num_bytes: number of bytes to decommit.
VMEM_FUNC VMemResult vmem_decommit(void* ptr, VMemSize num_bytes);

// Sets protection mode for the region of pages. All of the pages must be commited.
VMEM_FUNC VMemResult vmem_protect(void* ptr, VMemSize num_bytes, VMemProtect protect);

// @returns cached value from `vmem_query_page_size`
VMEM_FUNC VMemSize vmem_get_page_size(void);

// Query the page size from the system. Usually something like 4096 bytes.
// @returns the page size in number bytes. Cannot fail.
VMEM_FUNC VMemSize vmem_query_page_size(void);

// @returns cached value from `vmem_query_allocation_granularity`
VMEM_FUNC VMemSize vmem_get_allocation_granularity(void);

// Query the allocation granularity (alignment of each allocation) from the system.
// Usually 65KB on Windows and 4KB on linux (on linux it's page size).
// @returns allocation granularity in bytes.
VMEM_FUNC VMemSize vmem_query_allocation_granularity(void);

// Locks the specified region of the process's virtual address space into physical memory, ensuring that subsequent
// access to the region will not incur a page fault.
// All pages in the specified region must be commited.
// You cannot lock pages with `VMemProtect_NoAccess`.
VMEM_FUNC VMemResult vmem_lock(void* ptr, VMemSize num_bytes);

// Unlocks a specified range of pages in the virtual address space of a process, enabling the system to swap the pages
// out to the paging file if necessary.
// If you try to unlock pages which aren't locked, this will fail.
VMEM_FUNC VMemResult vmem_unlock(void* ptr, VMemSize num_bytes);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Utilities
//

// This will return the last message after a function fails (VMemResult_Error).
VMEM_FUNC const char* vmem_get_error_message(void);

// Returns a static string for the protection mode.
// e.g. VMemProtect_ReadWrite will return "ReadWrite".
// Never fails - unknown values return "<Unknown>", never null pointer.
VMEM_FUNC const char* vmem_get_protect_name(VMemProtect protect);

// Round the `address` up to the next (or current) aligned address.
// @param address: Memory address to align.
// @param align: Address alignment. Must be a power of 2 and greater than 0.
// @returns aligned address on success, VMemResult_Error on error.
VMEM_FUNC uintptr_t vmem_align_forward(const uintptr_t address, const int align);

// Round the `address` down to the previous (or current) aligned address.
// @param address: Memory address to align.
// @param align: Address alignment. Must be a power of 2 and greater than 0.
// @returns aligned address on success, VMemResult_Error on error.
VMEM_FUNC uintptr_t vmem_align_backward(const uintptr_t address, const int align);

// Check if an address is a multiple of `align`.
VMEM_FUNC VMemResult vmem_is_aligned(const uintptr_t address, const int align);

// Faster version of `vmem_align_forward`, because it doesn't do any error checking and can be inlined.
static inline uintptr_t vmem_align_forward_fast(const uintptr_t address, const int align) {
    const uintptr_t mask = (uintptr_t)(align - 1);
    return (address + mask) & ~mask;
}

// Faster version of `vmem_align_backward`, because it doesn't do any error checking and can be inlined.
static inline uintptr_t vmem_align_backward_fast(const uintptr_t address, const int align) {
    return address & ~(align - 1);
}

// Faster version of `vmem_is_aligned`, because it doesn't do any error checking and can be inlined.
// The alignment must be a power of 2.
static inline VMemResult vmem_is_aligned_fast(const uintptr_t address, const int align) {
    return (address & (address - 1)) == 0;
}

#if defined(__cplusplus)
} // extern "C"
#endif

#if defined(_WIN32)
#define VMEM_PLATFORM_WIN32
#elif defined(__linux__) || defined(__unix__)
#define VMEM_PLATFORM_LINUX
#else
#error "[vmem] Unknown platform."
#endif

#endif // !defined(VMEM_H_INCLUDED)



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VMEM_IMPLEMENTATION
//
#if defined(VMEM_IMPLEMENTATION) && !defined(VMEM_H_IMPLEMENTED)
#define VMEM_H_IMPLEMENTED

#define _CRT_SECURE_NO_WARNINGS 1

#if !defined(VMEM_UNUSED)
#define VMEM_UNUSED(varible) (void)(varible)
#endif

// Includes
#include <stdint.h>
#include <stddef.h>

#if !defined(VMEM_NO_ERROR_MESSAGES)
#include <string.h> // strerror_s, ...
#endif

#if defined(VMEM_PLATFORM_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif defined(VMEM_PLATFORM_LINUX)
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#endif

#if !defined(VMEM_THREAD_LOCAL)
#if defined(__cplusplus) && __cplusplus >= 201103L
#define VMEM_THREAD_LOCAL thread_local
#elif defined(__GNUC__) && __GNUC__ < 5
#define VMEM_THREAD_LOCAL __thread
#elif defined(_MSC_VER)
#define VMEM_THREAD_LOCAL __declspec(thread)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_THREADS__)
#define VMEM_THREAD_LOCAL _Thread_local
#endif

#if !defined(VMEM_THREAD_LOCAL)
#if defined(__GNUC__)
#define VMEM_THREAD_LOCAL __thread
#endif
#endif

#if !defined(VMEM_THREAD_LOCAL)
// Note: should this trigger warning or maybe error?
#define VMEM_THREAD_LOCAL // Ignore
#endif
#endif

#if !defined(VMEM_ON_ERROR)
#include <assert.h>
#define VMEM_ON_ERROR(opt_string) assert(0 && (opt_string))
#endif

#if !defined(VMEM_NO_ERROR_CHECKING)
// clang-format off
#if !defined(VMEM_NO_ERROR_MESSAGES)
#define VMEM_ERROR_IF(cond, write_message) do { if(cond) { write_message; VMEM_ON_ERROR(#cond); return 0; } } while(0)
#else
#define VMEM_ERROR_IF(cond, write_message) do { if(cond) { VMEM_ON_ERROR(#cond); return 0; } } while(0)
#endif
// clang-format on
#else
#define VMEM_ERROR_IF(cond, write_message) // Ignore
#endif

#if defined(__cplusplus)
extern "C" {
#endif

// Cached global page size.
static VMemSize vmem__g_page_size = 0;
static VMemSize vmem__g_allocation_granularity = 0;

VMEM_FUNC void vmem_init(void) {
    // Note: this will be 2 syscalls on windows.
    vmem__g_page_size = vmem_query_page_size();
    vmem__g_allocation_granularity = vmem_query_allocation_granularity();
}

VMEM_FUNC VMemSize vmem_get_page_size(void) {
    return vmem__g_page_size;
}

VMEM_FUNC VMemSize vmem_get_allocation_granularity(void) {
    return vmem__g_allocation_granularity;
}

#if !defined(VMEM_NO_ERROR_MESSAGES)
VMEM_THREAD_LOCAL char vmem__g_error_message[1024] = {};

static void vmem__write_error_message(const char* str) {
    int i = 0;
    for(; i < sizeof(vmem__g_error_message); i++) {
        if(str[i] == 0) break;
        vmem__g_error_message[i] = str[i];
    }
    vmem__g_error_message[i] = 0;
}

VMEM_FUNC const char* vmem_get_error_message(void) {
    return &vmem__g_error_message[0];
}
#else
#define vmem__write_error_message(message) // Ignore

VMEM_FUNC const char* vmem_get_error_message(void) {
    return "<Error messages disabled>";
}
#endif                                     // !defined(VMEM_NO_ERROR_MESSAGES)

VMEM_FUNC uintptr_t vmem_align_forward(const uintptr_t address, const int align) {
    VMEM_ERROR_IF(align == 0, vmem__write_error_message("Alignment cannot be zero."));
    VMEM_ERROR_IF((align & (align - 1)) != 0, vmem__write_error_message("Alignment has to be a power of 2."));
    return vmem_align_forward_fast(address, align);
}

VMEM_FUNC uintptr_t vmem_align_backward(const uintptr_t address, const int align) {
    VMEM_ERROR_IF(align == 0, vmem__write_error_message("Alignment cannot be zero."));
    VMEM_ERROR_IF((align & (align - 1)) != 0, vmem__write_error_message("Alignment has to be a power of 2."));
    return vmem_align_backward_fast(address, align);
}

VMEM_FUNC VMemResult vmem_is_aligned(const uintptr_t address, const int align) {
    VMEM_ERROR_IF(align == 0, vmem__write_error_message("Alignment cannot be zero."));
    VMEM_ERROR_IF((align & (align - 1)) != 0, vmem__write_error_message("Alignment has to be a power of 2."));
    return vmem_is_aligned_fast(address, align);
}

VMEM_FUNC const char* vmem_get_protect_name(const VMemProtect protect) {
    switch(protect) {
        case VMemProtect_Invalid: return "INVALID";
        case VMemProtect_NoAccess: return "NoAccess";
        case VMemProtect_Read: return "Read";
        case VMemProtect_ReadWrite: return "ReadWrite";
        case VMemProtect_Execute: return "Execute";
        case VMemProtect_ExecuteRead: return "ExecuteRead";
        case VMemProtect_ExecuteReadWrite: return "ExecuteReadWrite";
    }
    return "<Unknown>";
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Windows implementation
//
#if defined(VMEM_PLATFORM_WIN32)
static DWORD vmem__win32_protect(const VMemProtect protect) {
    switch(protect) {
        case VMemProtect_NoAccess: return PAGE_NOACCESS;
        case VMemProtect_Read: return PAGE_READONLY;
        case VMemProtect_ReadWrite: return PAGE_READWRITE;
        case VMemProtect_Execute: return PAGE_EXECUTE;
        case VMemProtect_ExecuteRead: return PAGE_EXECUTE_READ;
        case VMemProtect_ExecuteReadWrite: return PAGE_EXECUTE_READWRITE;
    }
    vmem__write_error_message("Invalid protect mode.");
    return VMemResult_Error;
}

#if !defined(VMEM_NO_ERROR_MESSAGES)
static void vmem__write_win32_error_message(void) {
    const DWORD result = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        vmem__g_error_message,
        sizeof(vmem__g_error_message),
        NULL);

    if(result == 0) {
        vmem__write_error_message("<Failed to format Win32 error>");
    } else {
        // Rewrite the last \n to zero
        vmem__g_error_message[(int)result - 1] = '\0';
    }
}
#endif

VMEM_FUNC void* vmem_alloc_protect(const VMemSize num_bytes, const VMemProtect protect) {
    VMEM_ERROR_IF(num_bytes == 0, vmem__write_error_message("Cannot allocate memory block with size 0 bytes."));

    const DWORD protect_win32 = vmem__win32_protect(protect);
    if(protect_win32) {
        LPVOID address = VirtualAlloc(NULL, (SIZE_T)num_bytes, MEM_RESERVE, protect_win32);
        VMEM_ERROR_IF(address == NULL, vmem__write_win32_error_message());
        // Note: memory is initialized to zero.
        return address;
    }

    return 0;
}

VMEM_FUNC VMemResult vmem_dealloc(void* ptr, const VMemSize num_allocated_bytes) {
    VMEM_ERROR_IF(ptr == 0, vmem__write_error_message("Ptr cannot be null."));
    VMEM_ERROR_IF(
        num_allocated_bytes == 0,
        vmem__write_error_message("Cannot dealloc a memory block of size 0 (num_allocated_bytes is 0)."));

    const BOOL result = VirtualFree(ptr, 0, MEM_RELEASE);
    VMEM_ERROR_IF(result == 0, vmem__write_win32_error_message());
    return VMemResult_Success;
}

VMEM_FUNC VMemResult vmem_commit_protect(void* ptr, const VMemSize num_bytes, const VMemProtect protect) {
    VMEM_ERROR_IF(ptr == 0, vmem__write_error_message("Ptr cannot be null."));
    VMEM_ERROR_IF(num_bytes == 0, vmem__write_error_message("Size (num_bytes cannot be null."));

    const LPVOID result = VirtualAlloc(ptr, num_bytes, MEM_COMMIT, vmem__win32_protect(protect));
    VMEM_ERROR_IF(result == 0, vmem__write_win32_error_message());
    return VMemResult_Success;
}

VMEM_FUNC VMemResult vmem_decommit(void* ptr, const VMemSize num_bytes) {
    VMEM_ERROR_IF(ptr == 0, vmem__write_error_message("Ptr cannot be null."));
    VMEM_ERROR_IF(num_bytes == 0, vmem__write_error_message("Size (num_bytes cannot be null."));

    const BOOL result = VirtualFree(ptr, num_bytes, MEM_DECOMMIT);
    VMEM_ERROR_IF(result == 0, vmem__write_win32_error_message());
    return VMemResult_Success;
}

VMEM_FUNC VMemResult vmem_protect(void* ptr, const VMemSize num_bytes, const VMemProtect protect) {
    VMEM_ERROR_IF(ptr == 0, vmem__write_error_message("Ptr cannot be null."));
    VMEM_ERROR_IF(num_bytes == 0, vmem__write_error_message("Size (num_bytes cannot be null."));

    DWORD old_protect = 0;
    const BOOL result = VirtualProtect(ptr, num_bytes, vmem__win32_protect(protect), &old_protect);
    VMEM_ERROR_IF(result == 0, vmem__write_win32_error_message());
    return VMemResult_Success;
}

VMEM_FUNC VMemSize vmem_query_page_size(void) {
    SYSTEM_INFO system_info = {};
    GetSystemInfo(&system_info);
    return system_info.dwPageSize;
}

VMEM_FUNC VMemSize vmem_query_allocation_granularity(void) {
    SYSTEM_INFO system_info = {};
    GetSystemInfo(&system_info);
    return system_info.dwAllocationGranularity;
}

VMEM_FUNC VMemResult vmem_lock(void* ptr, const VMemSize num_bytes) {
    VMEM_ERROR_IF(ptr == 0, vmem__write_error_message("Ptr cannot be null."));
    VMEM_ERROR_IF(num_bytes == 0, vmem__write_error_message("Size (num_bytes cannot be null."));

    const BOOL result = VirtualLock(ptr, num_bytes);
    VMEM_ERROR_IF(result == 0, vmem__write_win32_error_message());
    return VMemResult_Success;
}

VMEM_FUNC VMemResult vmem_unlock(void* ptr, const VMemSize num_bytes) {
    VMEM_ERROR_IF(ptr == 0, vmem__write_error_message("Ptr cannot be null."));
    if(num_bytes == 0) return 0;

    const BOOL result = VirtualUnlock(ptr, num_bytes);
    VMEM_ERROR_IF(result == 0, vmem__write_win32_error_message());
    return VMemResult_Success;
}

#endif // defined(VMEM_PLATFORM_WIN32)



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Linux implementation
//
#if defined(VMEM_PLATFORM_LINUX)
static int vmem__linux_protect(const VMemProtect protect) {
    switch(protect) {
        case VMemProtect_NoAccess: return PROT_NONE;
        case VMemProtect_Read: return PROT_READ;
        case VMemProtect_ReadWrite: return PROT_READ | PROT_WRITE;
        case VMemProtect_Execute: return PROT_EXEC;
        case VMemProtect_ExecuteRead: return PROT_EXEC | PROT_READ;
        case VMemProtect_ExecuteReadWrite: return PROT_EXEC | PROT_READ | PROT_WRITE;
    }
    vmem__write_error_message("Invalid protect mode.");
    return VMemResult_Error;
}

#if !defined(VMEM_NO_ERROR_MESSAGES)
static void vmem__write_linux_error_reason(void) {
    // https://linux.die.net/man/3/strerror_r
#ifdef STRERROR_R_CHAR_P
    // GNU variant can return a pointer outside the passed buffer
    return strerror_r(errno, vmem__g_error_message, sizeof(vmem__g_error_message));
#else
    // POSIX variant always returns message in buffer
    if(strerror_r(errno, vmem__g_error_message, sizeof(vmem__g_error_message)) != 0) {
        vmem__write_error_message("<Failed to retrieve errno string>");
    }
#endif
}
#endif

VMEM_FUNC void* vmem_alloc_protect(const VMemSize num_bytes, const VMemProtect protect) {
    VMEM_ERROR_IF(num_bytes == 0, vmem__write_error_message("Size (num_bytes cannot be null."));

    const int prot = vmem__linux_protect(protect);
    if(prot) {
        const int flags = MAP_PRIVATE | MAP_ANONYMOUS;
        // Note: memory is always initialized to zero when using MAP_ANONYMOUS.
        void* result = mmap(0, num_bytes, prot, flags, -1, 0);
        VMEM_ERROR_IF(result == MAP_FAILED, vmem__write_linux_error_reason());
        return result;
    }
    return 0; // VMemResult_Error
}

VMEM_FUNC VMemResult vmem_dealloc(void* ptr, const VMemSize num_allocated_bytes) {
    VMEM_ERROR_IF(ptr == 0, vmem__write_error_message("Ptr cannot be null."));
    VMEM_ERROR_IF(
        num_allocated_bytes == 0,
        vmem__write_error_message("Cannot dealloc a memory block of size 0 (num_allocated_bytes is 0)."));

    const int result = munmap(ptr, num_allocated_bytes);
    VMEM_ERROR_IF(result != 0, vmem__write_linux_error_reason());
    return VMemResult_Success;
}

VMEM_FUNC VMemResult vmem_commit_protect(void* ptr, const VMemSize num_bytes, const VMemProtect protect) {
    VMEM_ERROR_IF(ptr == 0, vmem__write_error_message("Ptr cannot be null."));
    VMEM_ERROR_IF(num_bytes == 0, vmem__write_error_message("Size (num_bytes cannot be null."));

    // On linux the pages are created in a reserved state and automatically commited on the first write, so we don't
    // need to commit anything.
    // But for compatibility with other platforms, we have to set the protection level.
    vmem_protect(ptr, num_bytes, protect);
    return VMemResult_Success;
}

VMEM_FUNC VMemResult vmem_decommit(void* ptr, const VMemSize num_bytes) {
    VMEM_ERROR_IF(ptr == 0, vmem__write_error_message("Ptr cannot be null."));
    VMEM_ERROR_IF(num_bytes == 0, vmem__write_error_message("Size (num_bytes cannot be null."));

    const int result = madvise(ptr, num_bytes, MADV_DONTNEED);
    VMEM_ERROR_IF(result != 0, vmem__write_linux_error_reason());
    return VMemResult_Success;
}

VMEM_FUNC VMemResult vmem_protect(void* ptr, const VMemSize num_bytes, const VMemProtect protect) {
    VMEM_ERROR_IF(ptr == 0, vmem__write_error_message("Ptr cannot be null."));
    VMEM_ERROR_IF(num_bytes == 0, vmem__write_error_message("Size (num_bytes cannot be null."));

    const int result = mprotect(ptr, num_bytes, vmem__linux_protect(protect));
    VMEM_ERROR_IF(result != 0, vmem__write_linux_error_reason());
    return VMemResult_Success;
}

VMEM_FUNC VMemSize vmem_query_page_size(void) {
    return (VMemSize)sysconf(_SC_PAGESIZE);
}

VMEM_FUNC VMemSize vmem_query_allocation_granularity(void) {
    return (VMemSize)sysconf(_SC_PAGESIZE);
}

VMEM_FUNC VMemResult vmem_lock(void* ptr, const VMemSize num_bytes) {
    VMEM_ERROR_IF(ptr == 0, vmem__write_error_message("Ptr cannot be null."));
    VMEM_ERROR_IF(num_bytes == 0, vmem__write_error_message("Size (num_bytes cannot be null."));

    const int result = mlock(ptr, num_bytes);
    VMEM_ERROR_IF(result != 0, vmem__write_linux_error_reason());
    return VMemResult_Success;
}

VMEM_FUNC VMemResult vmem_unlock(void* ptr, const VMemSize num_bytes) {
    VMEM_ERROR_IF(ptr == 0, vmem__write_error_message("Ptr cannot be null."));
    VMEM_ERROR_IF(num_bytes == 0, vmem__write_error_message("Size (num_bytes cannot be null."));

    const int result = munlock(ptr, num_bytes);
    VMEM_ERROR_IF(result != 0, vmem__write_linux_error_reason());
    return VMemResult_Success;
}
#endif // defined(VMEM_PLATFORM_LINUX)

#if defined(__cplusplus)
}
#endif

#endif // defined(VMEM_IMPLEMENTATION) && !defined(VMEM_H_IMPLEMENTED)

/*
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
