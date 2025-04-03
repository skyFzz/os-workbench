#include <common.h>
#include "list.h"
#include "slab.h"
#include "buddy.h"

#define TOTAL_CLASSES 10

char* size_class_str[] = { "cache-4", "cache-8", "cache-16", "cache-32", "cache-64", "cache-128", "cache-256", "cache-512", "cache-1024", "cache-2048" };
extern cache_sizes_t cache_sizes[TOTAL_CLASSES];

static void *kalloc(size_t size) {
  return cache_alloc(size);
}

static void kfree(void *ptr) {
  return cache_free(ptr);
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
  
  cache_mom_alloc();
  cache_mom_init();
  for (int i = 0; i < TOTAL_CLASSES; i++) {
    cache_sizes[i].cache = cache_create(size_class_str[i], cache_sizes[i].size);
  }
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
