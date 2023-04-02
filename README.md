# 💾 vmem.h
A simple [STB-style](https://github.com/nothings/stb/blob/master/docs/stb_howto.txt) cross-platform library for virtual memory management written in C. You can use this to implement arenas and other memory allocators, which work directly with virtual address space.

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
uint8_t* ptr = vmem_alloc(size);
// Mark some memory as ready to use.
vmem_commit(ptr, 2048);

// do something with the data, now you can use bytes in [0..<2048]

// When you don't need the whole memory block you can free it.
// To just mark it as unused, use `vmem_decommit` instead.
vmem_free(ptr, size);
```

You can find more example code in [vmem_test.cpp](tests/vmem_test.cpp), but currently it's a very simple example.

## Supported platforms
- Windows
- Linux

The plan is to also add Mac, iOS and Android.

Game consoles aren't supported, you will need to implement the platform backends on your own.

## Public API
### Main functions
`vmem_alloc`: Allocates a block of memory initialized to zero. You must use `vmem_commit` to use the memory.

`vmem_free`: Frees a block of virtual memory.

`vmem_commit`: Marks the memory for usage. This maps pages to physical memory.

`vmem_decommit`: This unmaps the pages from physical memory. To use the region again, use `vmem_commit`.

`vmem_get_page_size`: Returns the page size in number bytes. Usually something like 4096.

### Utilities

`vmem_align_forward`: Round the address up to the next (or current) aligned address.

`vmem_align_backward`: Round the address down to the previous (or current) aligned address.

You can find more documentation directly in [vmem.h](vmem.h)

## Building tests
```bash
# you can replace clang with 'gcc', 'zig cc' etc.
# Compiles fine with '-Wall -Werror'.
clang vmem_test.cpp -o vmem_test.exe
```
Or in `x64 Developer Command Prompt` on windows, using MSVC:
```bash
cl vmem_test.cpp /Fevmem_test.exe
```

