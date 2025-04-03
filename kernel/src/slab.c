#include <common.h>
#include "list.h"
#include "buddy.h"
#include "slab.h"

//#define DEBUG_CACHE_INIT
//#define DEBUG_CACHE_GROW

#define TOTAL_CLASSES 10
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12 
#define virt_to_page(kaddr) (global_mem_map + (__pa(kaddr) >> PAGE_SHIFT))
#define MAX_SIZE (1 << 24)
#define list_entry(ptr, type, member) \
        ((struct type *)((void *)ptr - (void *)&((type *)0)->member))

cache_sizes_t cache_sizes[] = {
	{ 4,	NULL },
	{ 8,	NULL },
	{ 16,	NULL },
	{ 32,	NULL },
	{ 64,	NULL },
	{ 128,	NULL },
	{ 256,	NULL },
	{ 512,	NULL },
	{ 1024,	NULL },
	{ 2048,	NULL },
};

// ((slab_t *)slabp + 1) advances the pointer by sizeof(slab_t) bytes
// (slabp + 1) moves to the next slab_t in memory
#define get_free_list(slabp) \
        ((free_list *)((slab_t *)slabp + 1))    

static cache_t *cache_mom;

/* Allocator for allocators */
void cache_mom_alloc() {
  cache_mom = (cache_t *)alloc_bootmem(sizeof(cache_t)); 
  assert(cache_mom);
}

void cache_mom_init() {
  list_init(&cache_mom->slabs_full);
  list_init(&cache_mom->slabs_partial);
  list_init(&cache_mom->slabs_free);
  cache_mom->page_per_slab = 0;
  cache_mom->total_objs = 0;
  cache_mom->obj_size = 0;
  list_init(&cache_mom->list);
  strncpy(cache_mom->name, "cache-mom", 10);
}

/* Cache creation for different sizes */
cache_t *cache_create(char *name, size_t size) {
  cache_t *cache_new = (cache_t *)alloc_bootmem(sizeof(cache_t));
  if (!cache_new) {
    printf("Fail: cache_create\n");
    halt(0);
  }

  list_init(&cache_new->slabs_full);
  list_init(&cache_new->slabs_partial);
  list_init(&cache_new->slabs_free);
  cache_new->page_per_slab = 1;
  cache_new->total_objs = PAGE_SIZE / size;
  cache_new->obj_size = size;
  list_init(&cache_mom->list);
  strncpy(cache_new->name, name, 15);
  
#ifdef DEBUG_CACHE_INIT
  printf("New cache name: %s\n", cache_new->name);
  printf("New cache at: %p\n", cache_new);
  printf("New cache obj_size: %d\n", cache_new->obj_size);
  printf("New cache total_objs: %d\n", cache_new->total_objs);
#endif

  list_add(&cache_mom->list, &cache_new->list);
  return cache_new;
}

/* Create a new slab */
static slab_t *cache_grow(cache_t *cache_p) {
  int num_obj = cache_p->total_objs;

  slab_t *new_slab = (slab_t *)alloc_bootmem(sizeof(slab_t) + num_obj * sizeof(free_list));

  list_add(&cache_p->slabs_free, &new_slab->list);
  new_slab->addr = pgalloc(PAGE_SIZE);
  new_slab->inuse = 0;
  new_slab->free = 0;
  
  // init the free_list
  for (int i = 0; i < num_obj; i++) {
    get_free_list(new_slab)[i] = i + 1;
    // printf("free_list[%d] has object index: %lu\n", i, get_free_list(new_slab)[i]);
  }
  get_free_list(new_slab)[num_obj-1] = -1;

#ifdef DEBUG_CACHE_GROW
  printf("New slab from cache: %s\n", cache_p->name);
  printf("New slab at: %p\n", new_slab);
  printf("New slab user page at: %p\n", new_slab->addr);
#endif

  return new_slab;
}

/* Alloc an object and return to the caller */
static void *__cache_alloc(cache_t *cache) {
  slab_t *slab = list_entry(cache->slabs_partial.next, slab_t, list);
  void *obj;

  if (list_empty(&cache->slabs_partial)) {
    if (list_empty(&cache->slabs_free)) {
      slab = cache_grow(cache);
      list_add(&cache->slabs_free, &slab->list);
    } else {
      slab = list_entry(cache->slabs_free.next, slab_t, list);
    }
    list_del_entry(&slab->list);
    list_add(&cache->slabs_partial, &slab->list);
  }
  
  // take the next available object, update free index and slab state
  obj = slab->addr + cache->obj_size * slab->free;
  slab->free = get_free_list(slab)[slab->free];   // macro used to here to update the index of the next free page 

  // just allocated the last available object
  if (slab->free == -1) {
    list_del_entry(&slab->list);
    list_add(&cache->slabs_full, &slab->list); 
  } 

  return obj;
}

void *cache_alloc(size_t size) {
  if (size > MAX_SIZE) {
    printf("Requested size larger than 16 MiB.\n");
    return 0;
  }

  cache_t *cache = cache_mom;

  if (size < PAGE_SIZE) {
    for (int i = 0; i < TOTAL_CLASSES; i++) {
      if (cache_sizes[i].size >= size) {
        cache = cache_sizes[i].cache;
        break;
      }
    }
    return __cache_alloc(cache);
  } else {
    return pgalloc(size);
  }

  return NULL;
}

/* Free an object from the size cache */
static void __cache_free(cache_t *cache_p, void *obj_p) {
  panic("not yet");
}

void cache_free(void *ptr) {
  __cache_free(cache_mom, (void *)0);
  panic("not yet");
}
