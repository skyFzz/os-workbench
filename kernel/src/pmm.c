#include <common.h>
#include "list.h"
#include "slab.h"
#include "buddy.h"

#define TOTAL_CLASSES 10
#define PAGE_SIZE 4096

char* size_class_str[] = { "cache-4", "cache-8", "cache-16", "cache-32", "cache-64", "cache-128", "cache-256", "cache-512", "cache-1024", "cache-2048" };
extern cache_sizes_t cache_sizes[TOTAL_CLASSES];

static void *kalloc(size_t size) {
  return size >= PAGE_SIZE ? pgalloc(size) : cache_alloc(size);
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

  /* Buddy */
  mem_map_create();
  free_area_create();

  /* Slab */
  cache_mom_create();
  for (int i = 0; i < TOTAL_CLASSES; i++) {
    cache_sizes[i].cache = cache_create(size_class_str[i], cache_sizes[i].size);
  }
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
