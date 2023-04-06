#pragma once
#include "../vmem.h"

template<typename INDEX, typename T>
struct VPool {
    static_assert(sizeof(T) >= sizeof(INDEX), "T has to be at least as large as INDEX");
    static constexpr INDEX INVALID_INDEX = (INDEX)-1;

    VMemArena arena = {};
    INDEX head_slot = 0;
    INDEX first_free_slot = INVALID_INDEX;

    void init(void* mem, VMemSize size_bytes) {
        arena = vmem_arena_init(mem, size_bytes);
    }

    void init_alloc(const int max_slots) {
        arena = vmem_arena_init_alloc(max_slots * sizeof(T));
    }

    void deinit_dealloc() {
        vmem_arena_deinit_dealloc(&arena);
    }

    bool is_valid() {
        return vmem_arena_is_valid(&arena);
    }

    inline bool is_in_bounds(const int slot) {
        return slot >= 0 && slot <= head_slot;
    }

    inline T* get_slots() {
        return (T*)arena.mem;
    }

    T& get(const int slot) {
        if(is_in_bounds(slot)) {
            return get_slots()[slot];
        }
        return {};
    }

    INDEX* _slot_index_ptr(const INDEX slot) {
        return (INDEX*)(&get_slots()[slot]);
    }

    int put(const T& value) {
        INDEX slot = first_free_slot;
        if(slot != INVALID_INDEX) {
            first_free_slot = *_slot_index_ptr(first_free_slot);
        } else {
            // The free list was empty, push a new entity onto the pool.
            vmem_arena_set_commited(&arena, head_slot + 1);
            slot = head_slot;
            head_slot++;
        }
        return slot;
    }

    void remove(const int slot) {
        *_slot_index_ptr(slot) = first_free_slot;
        first_free_slot = slot;
    }
};