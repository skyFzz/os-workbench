// DO NOT MODIFY: Will be reverted by the Online Judge.

#include <common.h>
#include <klib.h>

#define FIN
struct cpu cpus[16];

// spinlock_t test_lk = spin_init("test");

// should print the same address
void simple_test() {
#ifdef FIN 
  int i = 0;
  while (i < 32) { 
    void *ptr = pmm->alloc(256);
    printf("alloced ptr at: %p\n", ptr);
    pmm->free(ptr);
    i++;
  }
#else
  while (1) {
    void *ptr = pmm->alloc(256);
    printf("allocted ptr at: %p\n", ptr);
  }
#endif 
}

int main() {
    os->init();
    mpe_init(simple_test);
    return 1;
}
