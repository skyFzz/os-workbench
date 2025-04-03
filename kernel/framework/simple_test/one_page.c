// DO NOT MODIFY: Will be reverted by the Online Judge.

#include <common.h>
#include <klib.h>

struct cpu cpus[16];

// spinlock_t test_lk = spin_init("test");

// should print the same address
void simple_test() {
  int i = 0;
  while (i < 10) { 
    void *ptr = pmm->alloc(4096*16);
    // spin_lock(&test_lk);
    printf("alloced ptr at: %p\n", ptr);
    // spin_unlock(&test_lk);
    i++;
  }
}

int main() {
    os->init();
    mpe_init(simple_test);
    return 1;
}
