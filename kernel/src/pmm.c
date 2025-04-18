#include "list.h"
#include "slab.h"
#include "buddy.h"

#define TOTAL_CLASSES 10
#define PAGE_SIZE 4096

/*
 * This assignment largely follows the mm design from Linux 2.4.22, mainly the slab allocator and buddy allocator. The following resources have been really helpful:
 * - Linux-2.4.22 source code
 * - https://www.kernel.org/doc/gorman/html/understand/understand011.html#fig:%20Layout%20of%20the%20Slab%20Allocator
 */ 

char* size_class_str[] = { "cache-4", "cache-8", "cache-16", "cache-32", "cache-64", "cache-128", "cache-256", "cache-512", "cache-1024", "cache-2048" };
extern cache_sizes_t cache_sizes[TOTAL_CLASSES];
spinlock_t global_lock;

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

  /* Global pmm lock */
  kmt->spin_init(&global_lock, "Big buddy lock");

  /* Buddy */
  printf("Start booting buddy allocator...\n");
  mem_map_create();
  free_area_create();
  printf("Finish booting buddy allocator...\n");

  /* Slab */
  printf("Start booting general and local caches...\n");
  cache_mom_create();
  for (int i = 0; i < TOTAL_CLASSES; i++) {
    cache_sizes[i].cache = cache_create(size_class_str[i], cache_sizes[i].size);
  }
  printf("Finish booting caches...\n");
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
