// Based on https://github.com/rdunnington/zig-stable-array

#if !defined(ARENA_H_INCLUDED)
#define ARENA_H_INCLUDED

#include "vmem.h"
#include <stdint.h>
#include <assert.h>

// Arena of virtual memory. Initialize with `arena_init` and deinitialize with `arena_deinit`.
// All of the allocations have stable address, the memory is never reallocated. Well suited
// for large arrays of data and other containers. You can commit only the memory that you need,
// so even large arenas (100GB) are completely fine.
typedef struct Arena {
    // Pointer to the allocated memory. Do not modify the pointer itself (you can read/write data from it fine).
    uint8_t* _buf;
    // Total size of the `buf` memory allocation. Do not modify.
    size_t _buf_len;
    // Number of commited bytes. Do not modify.
    size_t _commited;
    // Number of used bytes.
    size_t len;
} Arena;

#if defined(__cplusplus)
extern "C" {
#endif

// Reserve virtual memory of size `max_bytes` and initialize arena.
Arena arena_init(const size_t max_bytes);
// Free the memory and reset.
void arena_deinit(Arena* arena);
size_t arena_calc_bytes_used_for_size(size_t cap);
int arena_is_valid(Arena arena);
void arena_set_commited(Arena* arena, size_t commited);
uint8_t* arena_push(Arena* arena, size_t num_bytes);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // !defined(ARENA_H_INCLUDED)



/////////////////////////////////////////////////////////////////////////////////////////////////////
// ARENA_IMPLEMENTATION
//
#if defined(ARENA_IMPLEMENTATION) && !defined(ARENA_H_IMPLEMENTED)
#define ARENA_H_IMPLEMENTED

#include "vmem.h"
#include <stdint.h>

#if !defined(ARENA_ASSERT)
#include <assert.h>
#define ARENA_ASSERT(cond) assert(cond)
#endif

#if defined(__cplusplus)
extern "C" {
#endif

Arena arena_init(const size_t max_bytes) {
    Arena result = {};
    result._buf = (uint8_t*)vmem_alloc(max_bytes);
    result._buf_len = max_bytes;
    return result;
}

void arena_deinit(Arena* arena) {
    vmem_free(arena->_buf, arena->_buf_len);
}

static inline size_t arena_calc_bytes_used_for_size(const size_t cap) {
    return vmem_align_forward(cap, vmem_get_allocation_granularity());
}

int arena_is_valid(const Arena arena) {
    return arena._buf != 0 && arena._buf_len > 0;
}

void arena_set_commited(Arena* arena, const size_t commited) {
    if(commited == arena->_commited) return;

    const size_t new_commited_bytes = arena_calc_bytes_used_for_size(commited);
    const size_t current_commited_bytes = arena_calc_bytes_used_for_size(arena->_commited);

    if(new_commited_bytes != current_commited_bytes) {
        // Shrink
        if(commited < arena->_commited) {
            if(new_commited_bytes < current_commited_bytes) {
                const size_t bytes_to_free = (size_t)((intptr_t)current_commited_bytes - (intptr_t)new_commited_bytes);
                vmem_decommit(arena->_buf + new_commited_bytes, bytes_to_free);
            }
        }
        // Expand
        else {
            if(commited >= arena->_buf_len) {
                // If you hit this, you likely either didn't alloc enough space up-front,
                // or have a leak that is allocating too many elements
                ARENA_ASSERT(0 && "[Arena] You've used up all the memory available.");
                return;
            }

            if(current_commited_bytes < new_commited_bytes) {
                vmem_commit(arena->_buf, new_commited_bytes);
            }
        }
    }

    arena->_commited = commited;
}

uint8_t* arena_push(Arena* arena, const size_t num_bytes) {
    // Ensure capacity
    arena_set_commited(arena, arena->len + num_bytes);
    const size_t start = arena->len;
    arena->len += num_bytes;
    return &arena->_buf[start];
}

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // defined(ARENA_IMPLEMENTATION) && !defined(ARENA_H_IMPLEMENTED)