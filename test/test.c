#define VMEM_ON_ERROR(opt_string) // Ignore for tests
#define VMEM_IMPLEMENTATION
#include "../vmem.h"

#include "utest.h"

// Tests
#include "vmem_test.c"

UTEST_STATE();

int main(const int argc, const char* argv[]) {
    vmem_init();
    return utest_main(argc, argv);
}
