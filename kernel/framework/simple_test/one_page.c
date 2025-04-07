// DO NOT MODIFY: Will be reverted by the Online Judge.

#include <common.h>
#include <klib.h>

//#define FINITE
#define SIZE 16 
struct cpu cpus[16];

spinlock_t test_lk = spin_init("test");

// should print the same address
void simple_test() {
#ifdef FINITE
  int i = 0;
  while (i < 50) { 
    void *ptr1 = pmm->alloc(2049);
    void *ptr2 = pmm->alloc(2048);
    printf("alloced ptr1 at: %p\n", ptr1);
    spin_lock(&test_lk);
    printf("alloced ptr2 at: %p\n", ptr2);
    spin_unlock(&test_lk);
    pmm->free(ptr2);
    i++;
  }
#else
  while (1) {
    void *ptr1 = pmm->alloc(4096);
    void *ptr2 = pmm->alloc(SIZE);
    //void *ptr2 = NULL;
    spin_lock(&test_lk);
    printf("allocted ptr1 at: %p\n", ptr1);
    printf("allocted ptr2 at: %p\n", ptr2);
    spin_unlock(&test_lk);
    //pmm->free(ptr2);
    pmm->free(ptr1);
  }
#endif 
}

int main() {
    os->init();
    mpe_init(simple_test);
    return 1;
}
