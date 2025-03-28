#include <common.h>

const uint16_t size_class[] = { 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };

/* slab allocator structs */
typedef struct {
  list_head slabs_full;
  list_head slabs_partial;
  list_head slabs_free;
  int page_per_slab;
  int total_objs;
  int obj_size;
  list_head next;    // cache-chain
  char name[20];
} cache_t;

typedef struct {
  list_head next;      // marks the full/partial/free list it belongs to; slab-chain
  void *addr;        // page frame address
  int free; // current free obj index
  int free_list[];  // next free obj indices
} slab_t; 

/* Allocator for allocators initialization */
static cache_t cache_mom = {
  .slabs_full = list_init(cache_mom.slabs_full);
  .slabs_partial = list_init(cache_mom.slabs_partial);
  .slabs_free = list_init(cache_mom.slabs_free);
  .name = "cache-mom"
  .next = list_init(cache_mom.next);
};

/* Runtime cache creation for different sizes */
cache_t *cache_create(char *name, size_t size) {
  cache_t *cache_new = (cache_t *)pgalloc(sizeof(cache_t));

  cache_new->name = name;
  cache_new->obj_size = size;
  cache_new->page_per_slab = 1;
  cache_new->total_objs = PAGE_SIZE / size;
  cache_new->slabs_full = list_init();
  cache_new->slabs_partial = list_init();
  cache_new->slabs_free = list_init();
  cache_new->next = list_init(cache_new->next);
  list_add(cache_mom->next, cache_new->next);

  return &cache_new;
}

/* Alloc an object and return to the caller */
void *cache_alloc(cache_t *cache_p) {
  slab_t *target_slab;

  // check partial list
  if (cache_p->slabs_paritial->next == cache_p->slabs_partial) {
    // check free list
    if (cache_p->slabs_free->next == cache_p->slabs_free) {
      target_slab = cache_grow(cache_p)->next;
      list_add(cache_p->slabs_free, target_slab);
    }
    list_del_entry(target_slab);
    list_add(cache_p->slabs_partial, target_slab);
  }
  
  // take the next available object based on slab->free, update index and slab category
  obj_p = target_slab->addr + cache_p->total_objs * target_slab->free;
  target_slab->free = target_slab->free_list[target_slab->free];
  if (target_slab->free_list[target_slab->free] == -1) {
    list_del_entry(target_slab);
    list_add(cache_p->slabs_full, target_slab); 
  } 

  return obj_p;
}

/* Alloc a new slab */
slab_t *cache_grow(cache_t *cache_p) {
  void *new_frame = pgalloc();
  int num_objs = cache_p->total_objs;
  
  slab_t *new_slab = (slab_t *)pgalloc(sizeof(slab_t) + num_objs * sizeof(unsigned int));
  list_add(cache_p->slabs_free, new_slab->next);
  new_slab->addr = new_frame;
  new_slab->free_cnt = num_objs;
  for (int i = 0; i < num_objs; i++) free[i] = i + 1;
  // the last element represents an invalid index, indicating that the slab has no free objs left
  free[num_objs - 1] = -1;

  return new_slab;
}

void cache_free(cache_t *cache_p, void *obj_p) {
}
