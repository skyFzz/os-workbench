#include "list.h"
#include "buddy.h"
#include "slab.h"

//#define DEBUG_BOOT
//#define DEBUG_CACHE_GROW
//#define DEBUG_FREE
#define DEBUG

#define HEAP_START 0x300000
#define HEAP_END 0x8000000
#define USER_START 0x1000000
#define TOTAL_CLASSES 10
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12 
#define MAX_SIZE (1 << 24)
#define list_entry(ptr, type, member) ((struct type *)((void *)ptr - (void *)&((type *)0)->member))
#define get_free_list(slabp) ((free_list *)((slab_t *)slabp + 1))    
#define MAX_CORE 8

extern spinlock_t global_lock;

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

static cache_t *cache_mom;

/* Allocator for allocators */
void cache_mom_create() {
  cache_mom = (cache_t *)alloc_bootmem(sizeof(cache_t)); 
  assert(cache_mom);
  list_init(&cache_mom->list);
  strncpy(cache_mom->name, "cache-mom", 10);
}

#define cc_entry(cpucache) \
        ((void **)(((cpu_cache_t*)(cpucache))+1))
#define cc_data(cachep) \
       ((cachep)->cpu_caches[cpu_current()])   // cpu_current(), am's api

static void cpu_cache_create(cache_t *cache) {
  int limit;
  int obj_size = cache->obj_size;
  
  // The size of per-cpu cache is based on the workload for this lab: most requests don't exceed 128 bytes 
  if (obj_size <= 128) {
    limit = PAGE_SIZE * 8 / obj_size;
  } else {
    limit = PAGE_SIZE * 4 / obj_size;  // e.g. cache-2048, per-cpu get 8 objs
  }

  for (int i = 0; i < MAX_CORE; i++) {
    cache->cpu_caches[i] = (cpu_cache_t *)alloc_bootmem(sizeof(cpu_cache_t) * limit * sizeof(void *));
#ifdef DEBUG_BOOT
    printf("Core %d local cache at: %p\n", i, &cache->cpu_caches[i]);
#endif
    cache->cpu_caches[i]->avail = 0;
    cache->cpu_caches[i]->limit = limit;
  }
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
  cache_new->free_slab_cnt = 0;

  // if too high, program halts early
  // if too low, reap() gets called expensively
  if (size <= 128) {
    cache_new->free_slab_limit = 8;
  } else {
    cache_new->free_slab_limit = 4;
  }

  cache_new->page_per_slab = 1;
  cache_new->total_objs = PAGE_SIZE / size;
  cache_new->obj_size = size;
  kmt->spin_init(&cache_new->cache_lock, name);
  list_init(&cache_mom->list);
  strncpy(cache_new->name, name, 15);
  // after finishing with general cache
  cpu_cache_create(cache_new);
  cache_new->batch_size = cache_new->cpu_caches[0]->limit / 2;

#ifdef DEBUG_BOOT
  printf("New cache name: %s\n", cache_new->name);
  printf("\tat: %p\n", cache_new);
  printf("\tobj_size: %d\n", cache_new->obj_size);
  printf("\ttotal_objs: %d\n", cache_new->total_objs);
  printf("\tbatch_size is %d\n", cache_new->batch_size);
#endif

  list_add(&cache_mom->list, &cache_new->list);
  return cache_new;
}

/* Alloc a new slab dynamically */
static slab_t *cache_grow(cache_t *cache) {
  int page_index;
  int num_obj = cache->total_objs;
  mem_map_t *page;

  // Using pgalloc here is better than alloc_bootmem, bootmem is not large enough to handle the amount of slabs that might be requested
  // slab_t *new_slab = (slab_t *)alloc_bootmem(sizeof(slab_t) + num_obj * sizeof(free_list));

  // access global resource, lock
  kmt->spin_lock(&global_lock); 
  slab_t *new_slab = (slab_t *)pgalloc(sizeof(slab_t) + num_obj * sizeof(free_list));
  if (!new_slab) {
    printf("Fail: run out of physical memory\n");
    halt(0);
  }
  new_slab->addr = pgalloc(PAGE_SIZE);
  kmt->spin_unlock(&global_lock);

  page_index = ((uintptr_t)(new_slab->addr - USER_START)) >> PAGE_SHIFT; 
  page = global_mem_map + page_index;
  page->cache = cache;    // set the cache and slab it belongs to
  page->slab = new_slab;
  new_slab->inuse = 0;
  new_slab->free = 0;
  cache->free_slab_cnt++;

  // init the free_list
  for (int i = 0; i < num_obj; i++) {
    get_free_list(new_slab)[i] = i + 1;
  }
  get_free_list(new_slab)[num_obj-1] = -1;

#ifdef DEBUG_CACHE_GROW
  printf("New slab from cache: %s\n", cache->name);
  printf("New slab at: %p\n", new_slab);
  printf("New slab user page at: %p\n", new_slab->addr);
#endif

  return new_slab;
}

/* Alloc an object and return to the caller dynamically */
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
    cache->free_slab_cnt--;
    list_add(&cache->slabs_partial, &slab->list);
  }
  
  // take the next available object, update free index and slab state
  obj = slab->addr + cache->obj_size * slab->free;

  /*
   * To solve the double free problem, we can check whether the obj_index is in the free list. Linux design did not touch the array
   * during the allocation. But we can update it to be -1 so that double free can be detected.
   */
  unsigned long index = get_free_list(slab)[slab->free];
  get_free_list(slab)[slab->free] = -1;   
  slab->free = index;  

  // just allocated the last available object
  if (slab->free == -1) {
    assert(slab->inuse == cache->total_objs - 1);
    list_del_entry(&slab->list);
    list_add(&cache->slabs_full, &slab->list); 
  } 

  slab->inuse++;

  return obj;
}

static void *__local_cache_load_alloc(int batch_size, cache_t *cache, cpu_cache_t *local_cache) {
#ifdef DEBUG_SMP
  printf("Need %d objs of size %d, total %d pages needed\n", batch_size, cache->obj_size, batch_size * cache->obj_size / PAGE_SIZE);
  printf("The limit is: %d\n", local_cache->limit);
#endif
  for (int i = 0; i < batch_size; i++) {
    cc_entry(local_cache)[i] = __cache_alloc(cache);
    local_cache->avail++;
  }
  return cc_entry(local_cache)[--local_cache->avail];
}

static void *__local_cache_alloc(cache_t *cache) {
  void *obj;
  cpu_cache_t *local_cache = cache->cpu_caches[cpu_current()];

  if (local_cache->avail) {
    obj = cc_entry(local_cache)[--local_cache->avail];
    cc_entry(local_cache)[local_cache->avail] = NULL;   // set the it to NULL after being allocated to make double free check work
  } else {
    // access global cache pool for loading, lock
    kmt->spin_lock(&cache->cache_lock);
    obj = __local_cache_load_alloc(cache->batch_size, cache, local_cache); 
    kmt->spin_unlock(&cache->cache_lock);
  }

  return obj;
}

void *cache_alloc(size_t size) {
  if (size > MAX_SIZE) {
    printf("Requested size larger than 16 MiB.\n");
    return 0;
  }
  if (size > 2048) {
    // access global resource, lock
    kmt->spin_lock(&global_lock);
    void *ptr = pgalloc(size);
    kmt->spin_unlock(&global_lock);
    if (!ptr) {
      printf("Failed pgalloc: run out of physical memory\n");
      halt(0);
    }
    return ptr;
  }

  cache_t *cache = NULL;
  for (int i = 0; i < TOTAL_CLASSES; i++) {
    if (cache_sizes[i].size >= size) {
      cache = cache_sizes[i].cache;
      break;
    }
  }
  if (!cache) {
    printf("Error at cache_alloc.\n");
    halt(0);
  }
  return __local_cache_alloc(cache);
}

// what is the timing for recycle, periodically or conditionally?
  // what is recycle?
    // destroy all free slabs and release the slabs to buddy system using pgfree()
  // set a threshold for the number of free slabs the list can have?
// what is the timing for halt?
    // cannot serve the request
      // Request for buddy or slab?
        // should be buddy
static void cache_gc(cache_t *cache) {
  if (list_empty(&cache->slabs_free)) {
    printf("Error: cache_recycle\n");
    halt(0);
  }

  slab_t *slab = list_entry(cache->slabs_free.next, slab_t, list);
  assert(!slab->inuse);
  
#ifdef DEBUG_FREE
  printf("Starting with the first free slab on the list at: %p\n", slab);
#endif
  for (int i = 0; i < cache->free_slab_limit; i++) {
    slab_t *tmp = list_entry(slab->list.next, slab_t, list);
    // access global resource, lock
    kmt->spin_lock(&global_lock);
    pgfree(slab->addr);
    kmt->spin_unlock(&global_lock);
    slab = tmp;
    cache->free_slab_cnt--;
  }
  assert(!cache->free_slab_cnt);
  assert(list_empty(&cache->slabs_free));
}

/* Free an object from the global size cache */
static void __cache_free(cache_t *cache, slab_t *slab, void *obj) {
  // Update free_list status
  int obj_index = ((uintptr_t)obj & (PAGE_SIZE - 1)) / cache->obj_size; 

  get_free_list(slab)[slab->free] = slab->free;   // first save the original next-free index back to the list
  slab->free = obj_index;   

  // Update slab status
  if (slab->inuse == cache->total_objs) {
    list_del_entry(&slab->list);
    list_add(&cache->slabs_partial, &slab->list);
  } else if (slab->inuse == 1) {
    /*
     * Three conditions used here to trigger gc: 
     * 1. caller must call free
     * 2. when the slab will be moved to the free list
     * 3. when the number of free slabs reach the limit 
     */
    list_del_entry(&slab->list);
    list_add(&cache->slabs_free, &slab->list);
    if (++cache->free_slab_cnt == cache->free_slab_limit) {
      cache_gc(cache);
    }
  }
  slab->inuse--;

#ifdef DEBUG_FREE
  printf("Slab at %p has:\n", slab->addr);
  printf("\ttotal objs limit: %d\n", cache->total_objs);
  printf("\ttotal objs in use: %d\n", slab->inuse);
  printf("\tavailable objs: %d\n", cache->total_objs - slab->inuse);
#endif
}

// Follow kernel design that returns half objs to the global cache from the local cache
static void __local_cache_free_batch(cpu_cache_t *local_buf, int batch_size, void *new_ptr) {
  assert(local_buf->avail == local_buf->limit);
  for (int i = 0; i < batch_size; i++) {
    /*
     * It is possible that the objects are from different slabs. So it is necessary to recompute the slab.
     */
    void *old_ptr = cc_entry(local_buf)[--local_buf->avail];    // starting from the end
    mem_map_t *page = global_mem_map + (((uintptr_t)(new_ptr - USER_START)) >> PAGE_SHIFT);
    cache_t *cache = page->cache;
    slab_t *slab = page->slab; 
    __cache_free(cache, slab, old_ptr);
  }

  cc_entry(local_buf)[local_buf->avail] = new_ptr;
}

static void __local_cache_free(cache_t *cache, void *ptr) {
  cpu_cache_t *local_buf = cache->cpu_caches[cpu_current()];

  // Double free check. Since we are not iterating the entire buffer, the cost is acceptable.
  for (int i = 0; i < local_buf->avail; i++) {
    if (cc_entry(local_buf)[i] == ptr) {
      printf("Found the ptr in the buffer: %s\n", cc_entry(local_buf)[i]);
      printf("The ptr going to be freed: %s\n", ptr);
      printf("Failed at local_cache_free: double free detected\n");
      halt(0);
    }
  }

  if (local_buf->avail == local_buf->limit) {
    printf("Local cache already full, time to free a batch...\n");

    // access global cache, lock
    kmt->spin_lock(&cache->cache_lock);
    __local_cache_free_batch(local_buf, cache->batch_size, ptr);
    kmt->spin_unlock(&cache->cache_lock);

    printf("Finish freeing a batch...\n");
  } else {
    cc_entry(local_buf)[local_buf->avail++] = ptr;
  }
}

void cache_free(void *ptr) {
  if (!ptr) {
    printf("Fail at cache_free: try to free a NULL pointer\n");
    halt(0);
  }

  mem_map_t *page = global_mem_map + (((uintptr_t)(ptr - USER_START)) >> PAGE_SHIFT);

  if ((uintptr_t)page < HEAP_START || (uintptr_t)page > USER_START) {
    printf("Fail at cache_free: invalid ptr, out of bound\n");
    halt(0);
  }

  cache_t *cache = page->cache;
  // all pages allocated for slabs must tie to a cache, otherwise the page is directly from pgalloc
  if (!cache) {
    // access global resource, lock
    kmt->spin_lock(&global_lock);
    pgfree(ptr);
    kmt->spin_unlock(&global_lock);
    return;
  }
  return __local_cache_free(cache, ptr);
}
