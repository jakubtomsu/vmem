#pragma once
#include "../vmem.h"

template<typename T>
struct VArray {
    VMemArena arena = {};
    int len = 0;

    void init(void* mem, VMemSize size_bytes) {
        arena = vmem_arena_init(mem, size_bytes);
    }

    void init_alloc(const int max_items) {
        arena = vmem_arena_init_alloc(max_items * sizeof(T));
    }

    void deinit_dealloc() {
        vmem_arena_deinit_dealloc(&arena);
    }

    bool is_valid() {
        return vmem_arena_is_valid(&arena);
    }

    inline bool is_in_bounds(const int index) {
        return index >= 0 && index <= len;
    }

    inline T* get_items() {
        return (T*)arena.mem;
    }

    T get(const int index) {
        if(is_in_bounds(index)) {
            return get_items()[index];
        }
        return T();
    }

    bool try_get(const int index, T& out_value) {
        if(is_in_bounds(index)) {
            out_value = get_items()[index];
            return true;
        }
        return false;
    }

    int put(const T& value) {
        vmem_arena_set_commited(&arena, len + 1);
        const int index = len;
        len += 1;
        get_items()[index] = value;
        return index;
    }

    void swap_remove(const int index) {
        if(is_in_bounds(index)) {
            len -= 1;
            const int last = len;
            T* items = get_items();
            items[index] = items[last];
        }
    }
};