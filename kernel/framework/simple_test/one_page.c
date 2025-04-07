// DO NOT MODIFY: Will be reverted by the Online Judge.

#include <common.h>
#include <klib.h>

//#define FINITE
struct cpu cpus[16];

spinlock_t test_lk = spin_init("test");

// should print the same address
void simple_test() {
#ifdef FINITE
  int i = 0;
  while (i < 1) { 
    void *ptr = pmm->alloc(16);
    printf("alloced ptr at: %p\n", ptr);
    pmm->free(ptr);
    i++;
  }
#else
  while (1) {
    void *ptr = pmm->alloc(2048);
    spin_lock(&test_lk);
    printf("allocted ptr at: %p\n", ptr);
    spin_unlock(&test_lk);
    //pmm->free(ptr);
  }
#endif 
}

int main() {
    os->init();
    mpe_init(simple_test);
    return 1;
}
