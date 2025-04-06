#ifndef SLAB_H
#define SLAB_H

#include "list.h"

#define MAX_CORE 8
#define NAME_LEN 20
#define CACHE_CLASS 10

typedef unsigned long free_list;

/* SMP */
typedef struct cpu_cache_t {
  int limit;
  int avail;
} cpu_cache_t;

/* slab allocator structs */
typedef struct cache_t {
  list_head slabs_full;
  list_head slabs_partial;
  list_head slabs_free;
  unsigned long free_slab_cnt;
  unsigned long free_slab_limit;
  spinlock_t cache_lock;
  int page_per_slab;
  int total_objs;
  int obj_size;
  list_head list;    // cache-chain
  char name[NAME_LEN];
  int batch_size;   // number of objs loaded into the cache
  cpu_cache_t *cpu_caches[MAX_CORE];    // ptr 1 -> cpu_cache_t, ptr 2 -> cpu_cache_t
} cache_t;

typedef struct slab_t {
  list_head list;       // marks the full/partial/free list it belongs to; slab-chain
  void *addr;           // starting page address
  unsigned int inuse;
  free_list free;       // an implementation of singly-linked list using a flexible array member
} slab_t; 

/* Size description struct for general caches. */
typedef struct cache_sizes {
	size_t size;
	cache_t *cache;
} cache_sizes_t;

void cache_mom_alloc();
void cache_mom_init();
cache_t *cache_create(char *name, size_t size);
void *cache_alloc(size_t size);
void cache_free(void *ptr);

extern cache_sizes_t cache_sizes[CACHE_CLASS];

#endif
