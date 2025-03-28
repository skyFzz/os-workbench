// emulating system header INCFLAGS += $(addprefix -I, $(INC_PATH))
// CFLAGS += -Iinclude/
#include <common.h>
#include "list.h" 

#define PAGE_SIZE 4096
#define MAX_PAGE 32000
#define HEAP_START 0x300000
#define HEAP_END 0x8000000
#define MAX_ORDER 12
#define BOOT_SIZE (1 << 19) 
#define LONG_ALIGN(n) (n & 7) ? n + -(n & 7) : n 

typedef struct free_area {
  list_head free_list;
  unsigned long *map;
} free_area_t;

typedef struct page {
  list_head list;
  unsigned long index;
} mem_map_t;

uintptr_t bootmem_track = (uintptr_t)HEAP_START;
mem_map_t *global_mem_map;
free_area_t *free_area;

static void *alloc_bootmem(size_t size) {
  void *p = (void *)bootmem_track;
  bootmem_track += size;
  return p;
}

/*
 * AbstractMachine provides 32000 pages of physcial memory. The max-size request, according to the lab description, is 16 MiB, or 4096 pages, 
 * which can be satisfied by one order-12 block. Also, given the constraint that the data structures (metadata) used to maintain the pages and 
 * free areas must also be allocated in the heap, a 0.8125 order-12 block, or 3328 pages (32000 / 4096 = 7.8125), are reserved for kernel space. 
 * The rest 7 blocks will be put on the free list for user space.
 */
static void mem_map_alloc() {
  global_mem_map = (mem_map_t *)alloc_bootmem(sizeof(struct page) * MAX_PAGE);
}

static void mem_map_init() {
  for (unsigned long i = 0; i < MAX_PAGE; i++) {
    list_init(&global_mem_map[i].list);
    global_mem_map[i].index = i;
    global_mem_map[i].inuse = 0;
  }
}

static void free_area_alloc() {
  free_area = (free_area_t *)alloc_bootmem(sizeof(free_area_t) * (MAX_ORDER + 1));
}

static void free_area_init() {
  for (unsigned long i = 0; i < MAX_ORDER + 1; i++) {
    unsigned long map_size;

    list_init(&free_area[i].free_list);
    if (i == 0) {
      for (int j = 0; j < MAX_PAGE; j++) {
        list_add(&global_mem_map[j].list);
      }
    }
    map_size = ((uintptr_t)HEAP_END - bootmem_track) / PAGE_SIZE;   // pages left available
    // ceiling of n and m equals to (n + m - 1) / m
    map_size = (map_size + (1 << (i + 1)) - 1) >> i + 1;     // for order i, each pair consists of 2^(i+1) pages, take the ceiling to track all pages
    map_size = (map_size + 7) >> 3;         // each pair takes 1 bit, get how many bytes we need, take the ceiling as well to not lose any bits
    map_size = LONG_ALIGN(map_size);  // align to long
    
    free_area[i].map = (unsigned long *)alloc_bootmem(map_size);
  }
}

static void toggle_bitmap(free_area_t *area, int order, unsigned long index) {
  int bit_index = index >> (order + 1);
  unsigned long target_byte = free_area[order]->map[target_bit / 64];
  target_byte ^= (1UL << bit_index);
}

/* Find a block of the requested order */ 
static struct page *rmqueue(int order) {
  free_area_t *area = free_area[order];
  mem_map_t *page;
  int real_order = order;
  spinlock_t lock = spin_init("lock");

  spin_lock(&lock);
  do {
    if (!list_empty(area->free_list)) {
      page = (struct page *)(area->free_list.next);
      toggle_bitmap(area, order, page->index);
      page->inuse = 1;
      list_del_entry(page->list);
      page = expand(page, (unsigned long)(page + 16), order, real_order, area);
      spin_unlock(&lock);
      return page;
    }
    area = free_area[++real_order];
  } while (real_order <= MAX_ORDER);
  spin_unlock(&lock);
  
  return NULL;
}

/* Split page blocks of higher orders until a page block of the needed order is available */
static struct page *expand(struct page *page, unsigned long index, int low, int high, free_area_t *area) {
  /* No list_del() because the first half is never added to a free list */
  while (high - low > 0) {
    unsigned long second_half_offset = 1 << (high - 1);
    high--;
    area--;
    list_add(area->free_list, (page + second_half_offset)->list);
    toggle_bitmap(area, order, page->index);
    page->inuse = 1;
    page += second_half_offset;
    index += second_half_offset;
  }

  return page;
}

/* Return a n-page block where n is the order, ranging from 0 to 12 */
void *pgalloc(int size) {
  /* no size check, only kernel can call pgalloc */
  size += -size & PAGE_SIZE - 1;  // align to the next page
  size >>= 12;
  
  return rmqueue(size);
}

void pgfree(void *page) {
  panic("not implemented");

}

