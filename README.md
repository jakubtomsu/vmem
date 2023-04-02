# 💾 vmem.h
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
If a function fails, it returns a `Vmem_Result_Failure` (which is 0 or false).
You can get a string message about the failure reason by calling `vmem_get_failure_message`.

But by default, each failure calls `assert(0)` from C standard library before returning.
To change/disable failure this behavior, you can `#define VMEM_ON_FAILURE(opt_string)`.

Example of dealing with errors:
```c
#define VMEM_ON_FAILURE(opt_string) // Disable the default assertion
#define VMEM_IMPLEMENTATION
#include "vmem.h"
#include <stdio.h> // printf

int main() {
    if(!vmem_free(0, 0)) {
        printf("vmem_free failed with message: %s\n", vmem_get_failure_message());
        // Handle the error ...
    }
    return 0;
}
```

## Compile-time options

## Tests
