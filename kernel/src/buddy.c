#include <common.h>

#define PAGE_SIZE 4096
#define MAX_PAGE 32000
#define HEAP_START 0x300000
#define HEAP_END 0x8000000
#define MAX_ORDER 12
#define BOOT_SIZE (1 << 19) 

typedef struct free_area {
  list_head free_list;
  unsigned long *map;
} free_area_t;

typedef struct page {
  list_head list;
  unsigned long index;
  unsigned long inuse;
} mem_map_t;

uintptr_t bootmem_track = (uintptr_t)HEAP_START;
mem_map_t *global_mem_map;
free_area_t *free_area;

void *alloc_bootmem(size_t size) {
  void *p = (void *)bootmem_track;
  bootmem_track += size;
  return p;
}

void mem_map_alloc() {
  global_mem_map = (mem_map_t *)alloc_bootmem(sizeof(struct page) * MAX_PAGE);
}

void mem_map_init() {
  for (unsigned long i = 0; i < MAX_PAGE; i++) {
    list_init(&global_mem_map[i].list);
    global_mem_map[i].index = i;
    global_mem_map[i].inuse = 0;
  }
}

void free_area_alloc() {
  free_area = (free_area_t *)alloc_bootmem(sizeof(struct free_area_t) * (MAX_ORDER + 1));
}

#define CEIL(n, m) (n + m - 1) / m
#define LONG_ALIGN(n) (n & 7) ? n + -(n & 7) : n 
void free_area_init() {
  for (unsigned long i = 0; i < MAX_ORDER + 1; i++) {
    unsigned long map_size;

    list_init(&free_area[i].free_list);
    if (i == 0) {
      for (int j = 0; j < MAX_PAGE; j++) {
        list_add(&global_mem_map[j].list);
      }
    }
    map_size = ((uintptr_t)HEAP_END - bootmem_track) / PAGE_SIZE;   // pages left available
    map_size = (map_size + (1 << (i + 1)) - 1) >> i + 1;     // for order i, each pair consists of 2^(i+1) pages, take the ceiling to track all pages
    map_size = (map_size + 7) >> 3;         // each pair takes 1 bit, get how many bytes we need, take the ceiling as well to not lose any bits
    map_size = LONG_ALIGN(map_size);  // align to long
    
    free_area[i].map = (unsigned long *)alloc_bootmem(map_size);
  }
}

int log_2(int n) {
  int ans = 0;
  while (n >>= 1) ans++;
  return ans;
}

void toggle_bitmap(int order, unsigned long index) {
  int bit_index = index >> (order + 1);
  unsigned long target_byte = free_area[order].map[target_bit / 64];
  target_byte ^= (1UL << bit_index);
}

struct page *get_page(list_head *list) {
  
  
}

/* Find a block of the requested order */ 
struct page *rmqueue(int order) {
  
  if (!list_empty(free_area[order].free_list)) {
    for (list_head *list = free_area[order].free_list; )
    
    
    
    toggle_bitmap(order, (unsigned long)(free_area[order].free_list))
    
  } 


  int real_order = order;
  free_area_t *target_area = free_area[real_order];
  while (list_empty(target_area->free_list)) {
    // If no blocks available at area >= requested order, merging from lower order 
    if (real_order == MAX_ORDER) {
      
    }
    target_area = free_area[++real_order];
  }
  
  mem_map_t *target_page = (mem_map_t *)((void *)target_area->free_list.next);
  return expand((struct page *)target_page.next, (unsigned long)(target_page + offsetof(mem_map_t, index)), order, real_order, target_area);
}

/* Split page blocks of higher orders until a page block of the needed order is available */
struct page *expand(struct page *page, unsigned long index, int low, int high, free_area_t *area) {
  for (int i = 0; i < high - low; i++) {
    
  }
  
}

/* Merge blocks of lower order until a request is fulfilled */
struct page *merge()


/* Return a n-page block where n is the order, ranging from 0 to 12 */
void *pgalloc(int size) {
  /* sanity check and alignment */
  size += -size & PAGE_SIZE - 1;  // align to the next page

  size >>= 12;  
  int order = 1;       
  for (int i = 0; i < 12; i++) {
    order <<= i;                  // align to the next power of 2
    if (order >= size) break; 
  }

  order = log_2(order);
  mem_block_t *bk;
  if (list_empty(free_area[order].free_list)) {
    p = split(order);
  }
  
  return bk->area.start;   
}

void pgfree(void *page) {

}


