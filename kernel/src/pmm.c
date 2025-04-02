#include <common.h>

#define MAX_SIZE (1 << 24)

void *pgalloc(int size);
void pgfree(void *page);
void free_area_alloc();
void free_area_init();
void mem_map_alloc();
void mem_map_init();

static void *kalloc(size_t size) {
  if (size > MAX_SIZE) {
    printf("Requested size larger than 16 MiB.\n");
    return 0;
  }

  return pgalloc(size);
}

static void kfree(void *ptr) {
  return pgfree(ptr);
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

  mem_map_alloc();
  mem_map_init();
  free_area_alloc();
  free_area_init();
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
