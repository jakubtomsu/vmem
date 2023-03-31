// Based on https://github.com/rdunnington/zig-stable-array

#pragma once
#include "vmem.h"

template<typename T>
struct VirtualArray {
    T* items;
    size_t len = 0;
    size_t capacity = 0;
    size_t _virtual_alloc_bytes = 0;

    void init(const size_t virtual_alloc_bytes) {
        len = 0;
        capacity = 0;
        _virtual_alloc_bytes = virtual_alloc_bytes;
    }

    void deinit() {
        if(items) {
            vmem_free(items, _virtual_alloc_bytes);
        }
        items = 0;
        len = 0;
        capacity = 0;
        _virtual_alloc_bytes = 0;
    }

    int insert(const T& data) {
        ensure_capacity(len + 1);
        const int index = len;
        len += 1;
        items[index] = data;
        return index;
    }

    void ensure_capacity(const size_t cap) {
        if(cap <= capacity) return;

        const size_t new_capacity_bytes = calc_bytes_used_for_capacity(cap);
        const size_t current_capacity_bytes = calc_bytes_used_for_capacity(capacity);

        if(current_capacity_bytes < new_capacity_bytes) {
            if(capacity == 0) {
                items = vmem_alloc(_virtual_alloc_bytes);
                len = 0;
            } else if(current_capacity_bytes >= self.max_virtual_alloc_bytes) {
                // If you hit this, you likely either didn't alloc enough space up-front,
                // or have a leak that is allocating too many elements
                assert(0 && "[VirtualArray] You've used up all the memory available.");
                return;
            }

            if(cap > 0) {
                vmem_commit(items, new_capacity_bytes);
            }
        }

        capacity = cap;
    }

    void shrink_and_free(const size_t new_len) {
        if(new_len > len) return;

        const int new_capacity_bytes = calc_total_bytes_for_capacity(new_len);
        const int current_capacity_bytes = calc_total_bytes_for_capacity(capacity);

        if(new_capacity_bytes < current_capacity_bytes) {
            const int bytes_to_free = current_capacity_bytes - new_capacity_bytes;
            vmem_decommit(items + new_capacity_bytes, bytes_to_free);
            // Divide by stride. Warning: this might be wrong with custom alignment, since there will be padding between items.
            self.capacity = new_capacity_bytes / sizeof(T);
        }

        len = new_len;
    }

    static inline size_t calc_bytes_used_for_capacity(const size_t cap) {
        return vmem_align_forward(cap, vmem_get_page_size());
    }

    size_t calc_bytes_used() {
        return calc_bytes_used_for_capacity(capacity);
    }
};