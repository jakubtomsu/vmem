# 💾 vmem.h
A simple [STB-style](https://github.com/nothings/stb/blob/master/docs/stb_howto.txt) cross-platform C/C++ library for managing virtual memory.

### Other libs
[arena.h](#arenah) - Implementation of arena allocator on top of `vmem.h`.

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

// When you don't need the whole memory block you can free it.
// To just mark it as unused, use `vmem_decommit` instead.
vmem_free(ptr, size);
```

## Supported platforms
- Windows
- Linux

I would also like to add support for Mac, iOS and Android.
Game consoles aren't supported, you will need to implement the platform backends on your own.

## Tests
Tests use the [utest.h](https://github.com/sheredom/utest.h) library.

### Build tests
```bash
# you can replace clang with 'gcc', 'zig cc' etc.
# Compiles fine with '-Wall -Werror'.
clang vmem_test.cpp -o vmem_test.exe
```
Or in `x64 Developer Command Prompt` on windows, using MSVC:
```bash
cl vmem_test.cpp /Fevmem_test.exe
```

## Error mangement
If a function fails, it returns a `Vmem_Result_Error` (which is 0/false).
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
    if(!vmem_free(0, 0)) {
        printf("vmem_free failed with message: %s\n", vmem_get_error_message());
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
VMEM_ON_ERROR(opt_string) | Called when an error is encountered. By default this just calls `assert(0)`. You can disable it with `#define VMEM_ON_ERROR(opt_string)`.
VMEM_NO_ERROR_MESSAGES    | Disables all error messages. When you call `vmem_get_error_message` it gives you just `<Error messages disabled>`.
VMEM_NO_ERROR_CHECKING    | Completely disables ***all*** error checking. This might be very unsafe.

# arena.h
A simple arena allocator implemented using `vmem.h`.

WIP
