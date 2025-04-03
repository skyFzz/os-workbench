// emulating system header INCFLAGS += $(addprefix -I, $(INC_PATH))
// CFLAGS += -Iinclude/
#include <common.h>
#include "list.h" 

#define PAGE_SIZE 4096
#define TOTAL_PAGES 4096 * 7    // 7 order-12 blocks
#define HEAP_START 0x300000
#define HEAP_END 0x8000000
#define MAX_ORDER 12
#define BOOT_SIZE (1 << 19) 
#define OFFSET 3328   
#define LONG_ALIGN(n) (n + 7) & ~7

typedef struct free_area {
  list_head free_list;
  unsigned long *map;
} free_area_t;

typedef struct page {
  list_head list;
  unsigned long index;
} mem_map_t;

static mem_map_t *global_mem_map;
static free_area_t *free_area;

// lock should be global
static spinlock_t lk = spin_init("Big Lock for rmqueue and expand");

uintptr_t boot_mem = (uintptr_t)HEAP_START;
uintptr_t user_mem = (uintptr_t)(HEAP_START + (OFFSET * PAGE_SIZE));

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
void mem_map_alloc() {
  global_mem_map = (mem_map_t *)alloc_bootmem(sizeof(struct page) * TOTAL_PAGES);
}

void mem_map_init() {
  for (unsigned long i = 0; i < TOTAL_PAGES; i++) {
    list_init(&global_mem_map[i].list);
    global_mem_map[i].index = i;
  }
}

void free_area_alloc() {
  free_area = (free_area_t *)alloc_bootmem(sizeof(struct free_area) * (MAX_ORDER + 1));
}

void free_area_init() {
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
  }
  assert(boot_mem < user_mem);
}

/* Toggle the bit representing the pair and return the new bit */
static int toggle_bitmap(int order, unsigned long index) {
  int pair_index = index >> (order + 1);
  unsigned long *target_byte = &free_area[order].map[pair_index / 64];
  *target_byte ^= (1UL << pair_index);
  return *target_byte & (1UL << pair_index) ? 1 : 0; 
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

  spin_lock(&lk);
  do {
    if (!list_empty(&area->free_list)) {
      page = (struct page *)(area->free_list.next);
      toggle_bitmap(order, page->index);
      list_del_entry(&page->list);
      page = expand(page, order, real_order, area);
      spin_unlock(&lk);
      return page;
    }
    area = &free_area[++real_order];
  } while (real_order <= MAX_ORDER);
  spin_unlock(&lk);
  
  return NULL;
}

/* 
 * Return a n-page block where n is the order, ranging from 0 to 12.
 * In real usage, only one page will be requested every call.
 */
void *pgalloc(int size) {
  /* no size check, only kernel can call pgalloc */
  size += -size & (PAGE_SIZE - 1);  // align to the next page
  int order = (size >> 12) - 1; 
  unsigned long index = rmqueue(order)->index;
  return (void *)(user_mem + (index << 12));
}

/* 
 * jyy设计的api只针对一个物理页面的分配和释放。实际am的源码对pgalloc的调用
 * 每次也只要一个物理页，符合pgfree api的设计。如果要允许释放一个来自order
 * 大于0的block， 那么pgfree参数还需要order，因为只有caller知道size。
 */
void pgfree(void *page) {
  int order = 0;
  unsigned long mask = ~(0UL);
  unsigned long index = (page - (void *)user_mem) >> 12;

  spin_lock(&lk);
  int new_bit = toggle_bitmap(order, index);
  /* 
   * After toggle, 1 means the buddy is still in use, 0 means both buddies 
   * are free, and can be coalesced into a new block of (order + 1).
   */
  do {
    if (!new_bit) {
      list_del_entry(&global_mem_map[index ^ -mask].list);
    } else {
      list_add(&free_area[order].free_list, &global_mem_map[index].list);
      break;
    } 
    order++;
    mask <<= 1;     // 1111 -> 1110
    index &= mask;  // 0011 -> 0010 
    new_bit = toggle_bitmap(order, index); 
  } while (order <= MAX_ORDER);
  spin_unlock(&lk);
}

