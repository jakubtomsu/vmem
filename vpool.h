#if !defined(VPOOL_H_INCLUDED)
#define VPOOL_H_INCLUDED

#include <stdint.h>
#include <stddef.h> // size_t

typedef uint32_t VPoolSlotIndex;
#define VPOOL_SLOT_INDEX_INVALID ((VPoolSlotIndex)-1)

typedef struct VPool {
    uint8_t* _buf;
    size_t _total_slots;
    size_t _slot_size_bytes;
    size_t _commited_bytes;
    VPoolSlotIndex _head_slot;
    VPoolSlotIndex _first_unused_slot;
} VPool;

#if defined(__cplusplus)
extern "C" {
#endif

VPool vpool_init(int total_slots, int slot_bytes);
void vpool_deinit(VPool* pool);
int vpool_is_valid(const VPool* pool);
uint8_t* vpool_alloc(VPool* pool);
void vpool_dealloc(VPool* pool, void* ptr);
VPoolSlotIndex vpool_alloc_slot(VPool* pool);
void vpool_dealloc_slot(VPool* pool, VPoolSlotIndex index);
void vpool_clear_and_decommit(VPool* pool);
uint8_t* vpool_get_at_slot(VPool* pool, VPoolSlotIndex index);

#if defined(__cplusplus)
} // extern "C"
#endif



#if defined(__cplusplus) && !defined(VPOOL_NO_CPP_CONTAINER)
#define VPOOL_CONTAINER

// Type-safe C++ wrapper around VPool.
template<typename T>
struct VPoolContainer {
    VPool _pool;

    void init(int max_items) {
        _pool = vpool_init(max_items, sizeof(T));
    }

    void deinit() {
        vpool_deinit(&_pool);
    }

    T* alloc() {
        T* result = (T*)vpool_alloc(&_pool);
        *result = {};
        return result;
    }

    void dealloc(T* item) {
        vpool_dealloc(&_pool, item);
    }
};
#endif // defined(__cplusplus) && !defined(VPOOL_NO_CPP_CONTAINER)

#endif // !defined(VPOOL_H_INCLUDED)

#if defined(VPOOL_IMPLEMENTATION) && !defined(VPOOL_H_IMPLEMENTED)
#define VPOOL_H_IMPLEMENTED

#include "vmem.h"
#include <stdint.h>
#include <stddef.h> // size_t

#include <assert.h>

VPool vpool_init(int total_slots, int slot_size_bytes) {
    assert(slot_size_bytes >= sizeof(VPoolSlotIndex));
    VPool result = {0};
    result._buf = (uint8_t*)vmem_alloc(total_slots * slot_size_bytes);
    result._total_slots = total_slots;
    result._slot_size_bytes = slot_size_bytes;
    result._first_unused_slot = VPOOL_SLOT_INDEX_INVALID;
    return result;
}

void vpool_deinit(VPool* pool) {
    vmem_dealloc(pool->_buf, pool->_total_slots * pool->_slot_size_bytes);
    pool->_buf = 0;
}

int vpool_is_valid(const VPool* pool) {
    if(pool) {
        return pool->_buf != 0 && pool->_total_slots != 0 && pool->_slot_size_bytes > sizeof(VPoolSlotIndex);
    }
    return 0;
}

static inline void* vpool__index_to_ptr(const VPool* pool, const size_t index) {
    return pool->_buf + pool->_slot_size_bytes * index;
}

static inline size_t vpool__ptr_to_index(const VPool* pool, const void* slot_ptr) {
    return ((intptr_t)slot_ptr - (intptr_t)pool->_buf) / pool->_slot_size_bytes;
}

static inline size_t vpool__calc_bytes_used_for_size(const size_t cap) {
    return vmem_align_forward(cap, vmem_get_page_size());
}

void vpool__set_commited_slots(VPool* pool, const size_t commited_slots) {
    const size_t commited_bytes = commited_slots * pool->_slot_size_bytes;
    if(commited_bytes == pool->_commited_bytes) return;

    const size_t new_commited_bytes = vpool__calc_bytes_used_for_size(commited_bytes);
    const size_t current_commited_bytes = vpool__calc_bytes_used_for_size(pool->_commited_bytes);

    if(new_commited_bytes != current_commited_bytes) {
        // Shrink
        if(commited_bytes < pool->_commited_bytes) {
            if(new_commited_bytes < current_commited_bytes) {
                const size_t bytes_to_dealloc =
                    (size_t)((intptr_t)current_commited_bytes - (intptr_t)new_commited_bytes);
                vmem_decommit(pool->_buf + new_commited_bytes, bytes_to_dealloc);
            }
        }
        // Expand
        else {
            if(commited_slots >= pool->_total_slots) {
                // If you hit this, you likely either didn't alloc enough space up-front,
                // or have a leak that is allocating too many elements
                assert(0 && "[Pool] You've used up all the memory available.");
                return;
            }

            if(current_commited_bytes < new_commited_bytes) {
                vmem_commit(pool->_buf, new_commited_bytes);
            }
        }
    }

    pool->_commited_bytes = commited_bytes;
}

VPoolSlotIndex vpool_alloc_slot(VPool* pool) {
    // first, try to grab the top of the free list...
    VPoolSlotIndex index = pool->_first_unused_slot;
    if(index != VPOOL_SLOT_INDEX_INVALID) {
        pool->_first_unused_slot = *(VPoolSlotIndex*)(vpool__index_to_ptr(pool, index));
    } else {
        // The free list was empty, push a new entity onto the pool.
        vpool__set_commited_slots(pool, pool->_head_slot + 1);
        index = pool->_head_slot;
        pool->_head_slot++;
    }
    return index;
}

uint8_t* vpool_alloc(VPool* pool) {
    const VPoolSlotIndex index = vpool_alloc_slot(pool);
    return vpool__index_to_ptr(pool, index);
}

static inline void vpool__dealloc_slot_with_ptr(VPool* pool, const VPoolSlotIndex index, const void* ptr) {
    // releasing -> push onto free list. The slot_ptr allocation will take the top of the free list,
    // not push onto the pool.
    *(VPoolSlotIndex*)ptr = pool->_first_unused_slot;
    pool->_first_unused_slot = index;
}

void vpool_dealloc_slot(VPool* pool, VPoolSlotIndex index) {
    vpool__dealloc_slot_with_ptr(pool, index, vpool__index_to_ptr(pool, index));
}

void vpool_dealloc(VPool* pool, void* slot_ptr) {
    vpool__dealloc_slot_with_ptr(pool, vpool__ptr_to_index(pool, slot_ptr), slot_ptr);
}

void vpool_clear_and_decommit(VPool* pool) {
    vpool__set_commited_slots(pool, 0);
}

#endif // defined(VPOOL_IMPLEMENTATION) && !defined(VPOOL_H_IMPLEMENTED