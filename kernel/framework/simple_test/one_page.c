// DO NOT MODIFY: Will be reverted by the Online Judge.

#include <common.h>
#include <klib.h>
#include "buddy.h"

// #define FINITE
#define SIZE 16 
struct cpu cpus[16];

// should print the same address
void simple_test() {
#ifdef FINITE
  int i = 0;
  while (i < 1) { 
    void *ptr1 = pmm->alloc(4096);  // 0 -> 1
    void *ptr2 = pmm->alloc(4096);  // 1 -> 0
    printf("alloced ptr1 at: %p\n", ptr1);
    printf("alloced ptr2 at: %p\n", ptr2);
    pmm->free(ptr1);  // 0 -> 1
    pmm->free(ptr2);  // 1 -> 0
    i++;
  }
#else
  while (1) {
    void *ptr1 = pmm->alloc(4096);
    void *ptr2 = pmm->alloc(SIZE);
    spin_lock(&debug);
    printf("alloced ptr1 at: %p\n", ptr1);
    printf("alloced ptr2 at: %p\n", ptr2);
    spin_unlock(&debug);
    pmm->free(ptr2);
  }
#endif 
}

int main() {
    os->init();
    mpe_init(simple_test);
    return 1;
}
