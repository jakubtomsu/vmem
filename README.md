# `vmem.h` - cross-platform virtual memory API
This is a simple [STB-style](https://github.com/nothings/stb/blob/master/docs/stb_howto.txt) cross-platform library for virtual memory management. You can use this to implement arenas and other high-performance datastructures, which work directly with virtual address space.

# Documentation
## How to use
Before including `vmem.h`, define `VMEM_IMPLEMENTATION` in only *one* C or C++ source file to create the implementation.

i.e. one file should look like this:
```c
#include ...
#define VMEM_IMPLEMENTATION
#include "vmem.h"
```
And all other files like this:
```c
#include ...
#include "vmem.h"
```
And then you can use the public API. For example:
```c
const int size = 1024 * 1024;
// Allocate a block of virtual addresses. Note: you can't use this memory *yet*.
uint8_t* ptr = vmem_reserve(size);
// Mark some memory as ready to use.
vmem_commit(ptr, 2048);

// do something with the data...

vmem_release(ptr, size);
```

## Public API
### vmem_reserve
Allocates (reserves but doesn't commit) a block of virtual address-space of size `num_bytes`.
Note: memory is initialized to zero.
Note: you must commit the memory before using it.

### vmem_release
Frees a block of virtual memory.

### vmem_commit
Commit memory pages which contain one or more bytes in `[ptr...ptr+num_bytes]`.
This maps the pages to physical memory.

### vmem_decommit
Decommits the memory pages which contain one or more bytes in `[ptr...ptr+num_bytes]`.
This unmaps the pages from physical memory.
Note: if you want to use the memory region again, you need to use `vmem_commit`.

### vmem_get_page_size
Returns the page size in number bytes. Usually something like 4096.

### vmem_align_forward
Round the `address` up to the next (or current) aligned address.

### vmem_align_backward
Round the `address` down to the previous (or current) aligned address.