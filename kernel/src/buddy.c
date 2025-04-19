// emulating system header INCFLAGS += $(addprefix -I, $(INC_PATH))
// CFLAGS += -Iinclude/
#include "list.h" 
#include "slab.h" 
#include "buddy.h" 

#define PAGE_SIZE 4096
#define TOTAL_PAGES 4096 * 7    // 7 order-12 blocks
#define HEAP_START 0x300000
#define HEAP_END 0x8000000
#define MAX_ORDER 12
#define BOOT_SIZE (1 << 19) 
#define OFFSET 3328   
#define LONG_ALIGN(n) (n + 7) & ~7

mem_map_t *global_mem_map;
static free_area_t *free_area;

uintptr_t boot_mem = (uintptr_t)HEAP_START;
uintptr_t user_mem = (uintptr_t)(HEAP_START + (OFFSET * PAGE_SIZE));

/* Sample current bit */
static int sample_bitmap(int order, unsigned long index) {
  int pair_index = index >> (order + 1);
  unsigned long *target_byte = &free_area[order].map[pair_index / 64];
  return *target_byte & (1UL << pair_index) ? 1 : 0;
}

/* Toggle the bit representing the pair and return the new bit */
static int toggle_bitmap(int order, unsigned long index) {
  int pair_index = index >> (order + 1);
  unsigned long *target_byte = &free_area[order].map[pair_index / 64];
  *target_byte ^= (1UL << pair_index);
  return *target_byte & (1UL << pair_index) ? 1 : 0; 
}
void *alloc_bootmem(size_t size) {
  size += -size & 7;  // align to the next 8 byte
  void *p = (void *)boot_mem;
  boot_mem += size;
  return p;
}

/*
 * AbstractMachine provides 32000 pages of physcial memory. The max-size request, according to the lab description, is 16 MiB, or 4096 pages, 
 * which can be satisfied by one order-12 block. Also, given the constraint that the data structures (metadata) used to maintain the pages and 
 * free areas must also be allocated in the heap, a 0.8125 order-12 block, or 3328 pages (32000 / 4096 = 7.8125), are reserved for kernel space. 
 * The rest 7 blocks will be put on the free list for user space.
 */
void mem_map_create() {
  global_mem_map = (mem_map_t *)alloc_bootmem(sizeof(struct page) * TOTAL_PAGES);

  // Part of the pages will be used for metadata like slab structures
  for (unsigned long i = 0; i < TOTAL_PAGES; i++) {
    list_init(&global_mem_map[i].list);
    global_mem_map[i].index = i;
    global_mem_map[i].slab = NULL;
    global_mem_map[i].cache = NULL;
  }
}

void free_area_create() {
  free_area = (free_area_t *)alloc_bootmem(sizeof(struct free_area) * (MAX_ORDER + 1));

  for (unsigned long i = 0; i < MAX_ORDER + 1; i++) {
    unsigned long map_size = PAGE_SIZE * 7; 
    list_init(&free_area[i].free_list);
    if (i == MAX_ORDER) {
      // list_add adds to the front, make 0~4095 block be the first 
      for (int j = 6; j >= 0; j--) {
        list_add(&free_area[i].free_list, (struct list_head *)(global_mem_map + j * PAGE_SIZE));
      }
    }

    // ceiling of n and m equals to (n + m - 1) / m
    map_size = (map_size + (1 << (i + 1)) - 1) >> (i + 1);     // for order i, each pair consists of 2^(i+1) pages, take the ceiling to track all pages
    map_size = (map_size + 63) >> 6;         // each pair takes 1 bit, get how many lu we need, take the ceiling as well to not lose any bits
    map_size = LONG_ALIGN(map_size);  // align to long
    free_area[i].map = (unsigned long *)alloc_bootmem(map_size);
    memset(free_area[i].map, 0, map_size * sizeof(unsigned long));
    // printf("pair_status at order 0 pair 0 is: %d\n", sample_bitmap(0, 0));
  }
  assert(boot_mem < user_mem);
}

/* Split page blocks of higher orders until a page block of the needed order is available */
static struct page *expand(mem_map_t *page, int low, int high, free_area_t *area) {
  while (high - low > 0) {
    high--;
    area--;
    unsigned long buddy_offset = 1 << high;
    list_head *list = &(page + buddy_offset)->list;
    /* No list_del() because the first half is never added to a free list */
    list_add(&area->free_list, list);
    toggle_bitmap(high, page->index);
  }

  return page;
}

/* Find a block of the requested order */ 
static struct page *rmqueue(int order) {
  free_area_t *area = &free_area[order];
  mem_map_t *page;
  int real_order = order;

  do {
    if (!list_empty(&area->free_list)) {
      page = (struct page *)(area->free_list.next);
      toggle_bitmap(real_order, page->index);
//    This bug is unbelievely stupid, pure stupidity
//    int pair_status = toggle_bitmap(order, page->index);
      list_del_entry(&page->list);
      page = expand(page, order, real_order, area);
      return page;
    }
    area = &free_area[++real_order];
  } while (real_order <= MAX_ORDER);
  
  return NULL;
}

/* 
 * Return a n-page block where n is the order, ranging from 0 to 12.
 * In real usage, only one page will be requested every call.
 */
void *pgalloc(int size) {
  size += -size & (PAGE_SIZE - 1);  // align to the next page
  int order = (size >> 12) - 1; 

  mem_map_t *page = rmqueue(order);
  if (!page) {
    return NULL;
  }

  unsigned long index = page->index;
  return (void *)(user_mem + (index << 12));
}

void pgfree(void *page) {
  int order = 0;
  unsigned long mask = ~(0UL);
  unsigned long index = (page - (void *)user_mem) >> 12;

  int pair_status = sample_bitmap(order, index);
  printf("[cpu: %d] index of page going to be free: %lu\n", cpu_current(), index);
  printf("pair_status: %d\n", pair_status);
  /* 
   * The current block is inuse
   * 0: both are allocated
   * 1: buddy is free
   */
  do {
    if (pair_status == 0) {
      list_add(&free_area[order].free_list, &global_mem_map[index].list);
      toggle_bitmap(order, index);
      break;
    } else {
      // remove buddy from free list and begin merging
      list_del_entry(&global_mem_map[index ^ -mask].list);
      toggle_bitmap(order, index);
    }
    order++;
    mask <<= 1;     // 1111 -> 1110
    index &= mask;  // 0011 -> 0010 
    pair_status = sample_bitmap(order, index); 
  } while (order <= MAX_ORDER);
}

