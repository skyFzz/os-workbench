#include <common.h>

#define HEAP_START      0x300000
#define HEAP_END        0x8000000
#define NUM_PAGES       32000
#define NUM_CPU         8
#define PAGE_SIZE       4096
#define MAX_SIZE        16 << 20
#define LIST_HEAD_INIT(name) { .next = NULL, .prev = NULL }
#define NUM_SIZE        10
#define MAX_ORDER       13


const uint16_t size_class[] = { 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };

/* linked list helper */
typedef struct list_head {
  struct list_head *next, *prev;
} list_head;

static void list_init(list_head *list) {
  list->next = list;
  list->prev = list;
}

static void list_add(list_head *head, list_head *new) {
  list_head *head_next = head->next;
  list_head *head_prev = head;

  new->next = head_next;
  new->prev = head_prev;
  head_next->prev = new;
  head_prev->next = new;
}

static void list_del(list_head *prev, list_head *next) {
  next->prev = prev;
  prev->next = next;
}

static void list_del_entry(list_head *entry) {
  list_del(entry->prev, entry->next);
}

/* buddy allocator structs */
typedef struct {
  list_head free_list;
  unsigned long *map;
} free_area_t;

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
  cache_t *cache_new = (cache_t *)malloc(sizeof(cache_t));

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

/*
 * Initially, there is only one free block exists in the free_list 
 * of the order 12. That big block can satisfy one 16
 * MiB request. Smaller requests are satisfied by breaking
 * that block. Return NULL(0) if the request cannot be satisfied. 
 */
static free_area_t free_lists[MAX_ORDER + 1];

void init_free_lists() {
  for (int i = 0; i < MAX_ORDER + 1; i++) {
    list_init(&free_lists[i].free_list);
    unsigned long *map;
    if (i >= 5) {
      map = (unsigned long *)malloc(sizeof(unsigned long));  
      memset(map, 0, 8); 
    } else {
      int num_pairs = 1 << (5 - i) * sizeof(unsigned long);
      map = (unsigned long *)malloc(num_pairs);
      memset(map, 0, num_pairs); 
    }
  }
}

/* Binary Buddy Allocator */
void *pgalloc(int size) {
  
  return (void *)HEAP_START;
}

void pgfree(void *page) {

}

/* Alloc a new slab */
slab_t *cache_grow(cache_t *cache_p) {
  void *new_frame = pgalloc();
  int num_objs = cache_p->total_objs;
  
  slab_t *new_slab = (slab_t *)malloc(sizeof(slab_t) + num_objs * sizeof(unsigned int));
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

static void *kalloc(size_t size) {
  if (size > MAX_SIZE) {
    printf("Requested size larger than 16 MiB.\n");
    return 0;
  }

  for (int i = 0; i < NUM_SIZE; i++) {
    size = size <= class_size[i] ? class_size[i] : size;
  }

  return alloc(&cache_8);
}

static void kfree(void *ptr) {
  return free(ptr);
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

  cache_init();
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
