// DO NOT MODIFY: Will be reverted by the Online Judge.

#include <common.h>
#include <klib.h>

void simple_test() {
  void *ptr = pmm->alloc(4096);
  if (ptr) printf("ptr not null");
}

int main() {
    os->init();
    mpe_init(simple_test);
    return 1;
}
