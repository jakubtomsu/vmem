#if !defined(VPAGEPOOL_H_INCLUDED)
#define VPAGEPOOL_H_INCLUDED

#include <stdint.h>
#include <stddef.h> // size_t


typedef uint32_t VPagePoolPageIndex;
#define VPAGEPOOL_PAGE_INDEX_INVALID ((VPagePoolPageIndex)-1)

typedef struct VPagePool {
    uint8_t* _buf;
    size_t _total_pages;
    size_t _used_pages;
    VPagePoolPageIndex _first_free_page;
} VPagePool;

#if defined(__cplusplus)
extern "C" {
#endif

VPagePool vpagepool_init(int total_pages);
void vpagepool_deinit(VPagePool* pool);
uint8_t* vpagepool_alloc(VPagePool* pool);
void vpagepool_dealloc(VPagePool* pool, void* ptr);
VPagePoolSlotIndex vpagepool_alloc_slot(VPagePool* pool);
void vpagepool_dealloc_slot(VPagePool* pool, VPagePoolSlotIndex index);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // !defined(VPAGEPOOL_H_INCLUDED)

#if defined(VPAGEPOOL_IMPLEMENTATION) && !defined(VPAGEPOOL_H_INCLUDED)
#define VPAGEPOOL_H_INCLUDED

VPagePool vpagepool_init(int total_pages) {
    VPagePool result = {0};
    result._first_free_page = VPAGEPOOL_PAGE_INDEX_INVALID;
    result._buf = vmem_alloc(total_pages * vmem_get_page_size() +);
    result._total_pages = total_pages;
}

#endif // defined(VPAGEPOOL_IMPLEMENTATION) && !defined(VPAGEPOOL_H_INCLUDED)
