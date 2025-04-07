#ifndef BUDDY_H
#define BUDDY_H

#include "slab.h"

typedef struct free_area {
  list_head free_list;
  unsigned long *map;
} free_area_t;

typedef struct page {
  list_head list;
  unsigned long index;
  slab_t *slab;       // pointer to the slab manager
  cache_t *cache;     // pointer to the cache manager
} mem_map_t;

void *alloc_bootmem(size_t size);
void mem_map_create();
void free_area_create();
void *pgalloc(int size);
void pgfree(void *page);

extern mem_map_t *global_mem_map;

#endif
