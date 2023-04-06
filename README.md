# 💾 vmem.h 0.2
A simple [STB-style](https://github.com/nothings/stb/blob/master/docs/stb_howto.txt) cross-platform C/C++ library for managing virtual memory.

## Usage
Define `VMEM_IMPLEMENTATION` in only *one* C or C++ source file before including `vmem.h` to create the implementation.

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
void* ptr = vmem_alloc(size);
// Mark some memory as ready to use.
vmem_commit(ptr, 2048);

// do something with the data, now you can use bytes in [0..<2048]

// When you don't need the whole memory block you can dealloc it.
// To just mark it as unused, use `vmem_decommit` instead.
vmem_dealloc(ptr, size);
```

## Features
- Reserving, committing, decommiting and releasing memory
- Page protection levels
- Querying page size and allocation granularity
- Memory usage status (total physical memory, available physical memory)
- Address math utilities - aligning forwards, backwards, checking alignment
- Arena allocation, see [Arena](#arena)

## Supported platforms
- Windows
- Linux

I would also like to add support for Mac, iOS and Android.
Game consoles aren't supported, you will need to implement the platform backends on your own.

## Dependencies
There are zero dependencies apart from the C standard library.

## Arena
Arena is a bump allocator on a linear block of memory.
When working with virtual memory, only the used part of the arena stays commited.

You can read more about advantages of arenas in [this blog post by Ryan Fleury](https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator).

Example usage:
```c
VMemArena arena = vmem_arena_init_alloc(1024 * 1024); // Allocate an arena.
vmem_arena_set_commited(&arena, 32 * sizeof(int); // Commit part of the arena.
for(int i = 0; i < 32; i++) {
    *(int*)arena.mem = i;
}
vmem_arena_deinit_dealloc(&arena); // Free the arena memory.
```

## Samples
The [samples/](samples/) folder contains a number of containers built using arena allocation.

Note: currently the samples are mostly WIP, there will be more in the future.

Tests for the samples are in [tests/samples_test.cpp](tests/samples_test.cpp)

## Tests
Tests use the [utest.h](https://github.com/sheredom/utest.h) library.
You can find all the vmem.h tests in [tests/vmem_test.c](tests/vmem_test.c).

### Build tests
```bash
# Note: you can replace clang with 'gcc', 'zig cc' etc.
clang tests/test.c -o test.exe
# Other options
clang++ -x c++ test/test.c -o test.exe -Wall -Werror -fsanitize=address,undefined -std=c++20
clang test/test.c -o test.exe -Wall -Werror -fsanitize=address,undefined -std=c99
```
Or in `x64 Developer Command Prompt` on windows, using MSVC:
```bash
cl vmem_test.cpp /Fevmem_test.exe
```

## Error mangement
If a function fails, it returns a `VMemResult_Error` (which is 0/false).
You can get a string message about the error reason by calling `vmem_get_error_message`.

But by default, each error calls `assert(0)` from C standard library before returning.
To change/disable this behavior, you can `#define VMEM_ON_ERROR(opt_string) ...`.

Example of dealing with errors:
```c
#define VMEM_ON_ERROR(opt_string) // Disable the default assertion
#define VMEM_IMPLEMENTATION
#include "vmem.h"
#include <stdio.h> // printf

int main() {
    if(!vmem_dealloc(0, 0)) {
        printf("vmem_dealloc failed with message: %s\n", vmem_get_error_message());
        // Handle the error ...
    }
    return 0;
}
```

## Compile-time options
#define | Description
------- | -----------
VMEM_IMPLEMENTATION       | Instantiate the library implementation in the current source file.
VMEM_FUNC                 | Specifiers for all API functions. E.g. you can mark all functions static with `#define VMEM_FUNC static`
VMEM_INLINE               | Inline qualifier for short static functions defined in the header. Defined to platform-specific inline or forceinline specifier.
VMEM_ON_ERROR(opt_string) | Called when an error is encountered. By default this just calls `assert(0)`. You can disable it with `#define VMEM_ON_ERROR(opt_string)`.
VMEM_NO_ERROR_MESSAGES    | Disables all error messages. When you call `vmem_get_error_message` it gives you just `<Error messages disabled>`.
VMEM_NO_ERROR_CHECKING    | Completely disables ***all*** error checking. This might be very unsafe.


## Language support
- C99
- C11
- C++11
- C++14
- C++17
- C++20
