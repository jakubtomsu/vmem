// Based on https://github.com/rdunnington/zig-stable-array

#if !defined(VARENA_H_INCLUDED)
#define VARENA_H_INCLUDED

#include "vmem.h"
#include <stdint.h>
#include <stddef.h> // size_t
#include <assert.h>

// Arena of virtual memory. Initialize with `varena_init` and deinitialize with `varena_deinit`.
// All of the allocations have stable address, the memory is never reallocated. Well suited
// for large arrays of data and other containers. You can commit only the memory that you need,
// so even large arenas (100GB) are completely fine.
typedef struct VArena {
    // Pointer to the allocated memory. Do not modify the pointer itself (you can read/write data from it fine).
    uint8_t* _buf;
    // Total size of the `buf` memory allocation. Do not modify.
    size_t _buf_len;
    // Number of commited bytes. Do not modify.
    size_t _commited;
    // Number of used bytes.
    size_t len;
} VArena;

#if defined(__cplusplus)
extern "C" {
#endif

// Reserve virtual memory of size `max_bytes` and initialize arena.
VArena varena_init(const size_t max_bytes);
// Dealloc the memory and reset.
void varena_deinit(VArena* arena);
size_t varena_calc_bytes_used_for_size(size_t cap);
int varena_is_valid(const VArena* arena);
void varena_set_commited(VArena* arena, size_t commited);
uint8_t* varena_alloc(VArena* arena, size_t num_bytes);

#if defined(__cplusplus)
} // extern "C"
#endif



#if defined(__cplusplus) && !defined(VARENA_NO_CPP_CONTAINER)
#define VARENA_CONTAINER

// Type-safe C++ wrapper around VArena.
template<typename T>
struct VArenaContainer {
    VArena _arena;

    void init(const size_t max_items) {
        _arena = varena_init(max_items * sizeof(T));
    }

    void deinit() {
        varena_deinit(&_arena);
    }

    bool is_valid() {
        return varena_is_valid(&_arena);
    }

    void set_commited(const size_t commited) {
        varena_set_commited(&_arena, commited);
    }

    T* alloc(const size_t num_items) {
        T* result = (T*)varena_alloc(&_arena, num_items * sizeof(T));
        *result = {};
        return result;
    }
};
#endif // defined(__cplusplus) && !defined(VARENA_NO_CPP_CONTAINER)


#endif // !defined(VARENA_H_INCLUDED)



/////////////////////////////////////////////////////////////////////////////////////////////////////
// VARENA_IMPLEMENTATION
//
#if defined(VARENA_IMPLEMENTATION) && !defined(VARENA_H_IMPLEMENTED)
#define VARENA_H_IMPLEMENTED

#include "vmem.h"
#include <stdint.h>
#include <stddef.h> // size_t

#if !defined(VARENA_ASSERT)
#include <assert.h>
#define VARENA_ASSERT(cond) assert(cond)
#endif

#if defined(__cplusplus)
extern "C" {
#endif

VArena varena_init(const size_t max_bytes) {
    VArena result = {};
    result._buf = (uint8_t*)vmem_alloc(max_bytes);
    result._buf_len = max_bytes;
    return result;
}

void varena_deinit(VArena* arena) {
    vmem_dealloc(arena->_buf, arena->_buf_len);
    arena->_buf = 0;
}

static inline size_t varena_calc_bytes_used_for_size(const size_t cap) {
    return vmem_align_forward(cap, vmem_get_page_size());
}

int varena_is_valid(const VArena* arena) {
    return arena->_buf != 0 && arena->_buf_len > 0;
}

void varena_set_commited(VArena* arena, const size_t commited) {
    if(commited == arena->_commited) return;

    const size_t new_commited_bytes = varena_calc_bytes_used_for_size(commited);
    const size_t current_commited_bytes = varena_calc_bytes_used_for_size(arena->_commited);

    if(new_commited_bytes != current_commited_bytes) {
        // Shrink
        if(commited < arena->_commited) {
            if(new_commited_bytes < current_commited_bytes) {
                const size_t bytes_to_dealloc =
                    (size_t)((intptr_t)current_commited_bytes - (intptr_t)new_commited_bytes);
                vmem_decommit(arena->_buf + new_commited_bytes, bytes_to_dealloc);
            }
        }
        // Expand
        else {
            if(commited >= arena->_buf_len) {
                // If you hit this, you likely either didn't alloc enough space up-front,
                // or have a leak that is allocating too many elements
                VARENA_ASSERT(0 && "[VArena] You've used up all the memory available.");
                return;
            }

            if(current_commited_bytes < new_commited_bytes) {
                vmem_commit(arena->_buf, new_commited_bytes);
            }
        }
    }

    arena->_commited = commited;
}

uint8_t* varena_alloc(VArena* arena, const size_t num_bytes) {
    // Ensure capacity
    varena_set_commited(arena, arena->len + num_bytes);
    const size_t start = arena->len;
    arena->len += num_bytes;
    return &arena->_buf[start];
}

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // defined(VARENA_IMPLEMENTATION) && !defined(VARENA_H_IMPLEMENTED)