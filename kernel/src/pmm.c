#include <common.h>

#define HEAP_START      0x300000
#define HEAP_END        0x8000000
#define NUM_PAGES       32000
#define NUM_CPU         8
#define PAGE_SIZE       4096
#define MAX_SIZE        16 << 20
#define LIST_HEAD_INIT(name) { .next = NULL, .prev = NULL }
#define NUM_SIZE        10
#define MAX_ORDER       13

static void *kalloc(size_t size) {
  if (size > MAX_SIZE) {
    printf("Requested size larger than 16 MiB.\n");
    return 0;
  }

  for (int i = 0; i < NUM_SIZE; i++) {
    size = size <= class_size[i] ? class_size[i] : size;
  }

  return alloc(&cache_8);
}

static void kfree(void *ptr) {
  return free(ptr);
}


static void pmm_init() {
  uintptr_t pmsize = (
      (uintptr_t)heap.end
      - (uintptr_t)heap.start
  );

  printf(
      "Got %d MiB heap: [%p, %p)\n",
      pmsize >> 20, heap.start, heap.end
  );

  cache_init();
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
