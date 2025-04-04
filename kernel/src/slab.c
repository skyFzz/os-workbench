#include <common.h>
#include "list.h"
#include "buddy.h"
#include "slab.h"

// #define DEBUG_CACHE_INIT
#define DEBUG_CACHE_GROW

#define USER_START 0x1000000
#define TOTAL_CLASSES 10
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12 
#define MAX_SIZE (1 << 24)
#define list_entry(ptr, type, member) \
        ((struct type *)((void *)ptr - (void *)&((type *)0)->member))

//static spinlock_t lk = spin_init("Big Lock");

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
  int page_index;
  int num_obj = cache_p->total_objs;
  mem_map_t *page;

  slab_t *new_slab = (slab_t *)alloc_bootmem(sizeof(slab_t) + num_obj * sizeof(free_list));
  new_slab->addr = pgalloc(PAGE_SIZE);
  page_index = ((uintptr_t)(new_slab->addr - USER_START)) >> PAGE_SHIFT; 
  page = global_mem_map + page_index;
  page->cache = cache_p;    // set the cache and slab it belongs to
  page->slab = new_slab;
  new_slab->inuse = 0;
  new_slab->free = 0;

  // init the free_list
  for (int i = 0; i < num_obj; i++) {
    get_free_list(new_slab)[i] = i + 1;
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

  slab->inuse++;

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
static void __cache_free(cache_t *cache_p, slab_t *slab_p, void *obj_p) {
  // Update free_list status
  int obj_index = ((uintptr_t)obj_p & (PAGE_SIZE - 1)) / cache_p->obj_size; 
  get_free_list(slab_p)[slab_p->free] = slab_p->free;   // first save the original next-free index back to the list
  slab_p->free = obj_index;   

  // Update slab status
  if (slab_p->inuse == cache_p->total_objs) {
    list_del_entry(&slab_p->list);
    list_add(&cache_p->slabs_partial, &slab_p->list);
  } else if (slab_p->inuse == 1) {
    list_del_entry(&slab_p->list);
    list_add(&cache_p->slabs_free, &slab_p->list);
  }
  slab_p->inuse--;
}

void cache_free(void *ptr) {
  // Locate the cache ptr belongs
  mem_map_t *page = global_mem_map + (((uintptr_t)(ptr - USER_START)) >> PAGE_SHIFT);
  cache_t *cache = page->cache;
  slab_t *slab = page->slab; 

  return __cache_free(cache, slab, ptr);
}
