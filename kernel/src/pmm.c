#include <common.h>

#define HEAP_START      0x300000
#define HEAP_END        0x8000000
#define NUM_PAGES       32000
#define NUM_CPU         8
#define PAGE_SIZE       4096
#define MAX_SIZE        16 << 20
#define LIST_HEAD_INIT(name) { .next = NULL, .prev = NULL }
#define NUM_SIZE        10

const uint16_t size_class[] = { 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };

typedef struct {
  
} linked_list;

typedef struct {
  linked_list slabs_full;
  linked_list slabs_partial;
  linked_list slabs_free;
  int page_per_slab;
  int obj_per_slab;
  int obj_size;
  linked_list next;
  char name[20];
} cache;

typedef struct {
  linked_list list;
  void *obj;
  int active_objs;
} slab; 

static cache cache_8 = {
  slabs_full = LIST_HEAD_INIT(cache_8.slabs_full),
  slabs_partial = LIST_HEAD_INIT(cache_8.slabs_partial),
  slabs_free = LIST_HEAD_INIT(cache_8.slabs_free),
  page_per_slab = 1,
  obj_per_slab = 512,
  obj_size = 8,
  next = LIST_HEAD_INIT(cache_8.next),
  name = "cache-8"
};

linked_list cache_chain;

cache *cache_create(char name[], size_t size, s ) {
}

void *alloc(cache *cachep) {

}

void free(cache *cachep, void *p) {

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

  printf("Fuck YOU damn it you asshole\n");
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
