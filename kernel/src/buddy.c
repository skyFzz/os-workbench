/* Buddy Allocator */
#include <common.h>

typedef struct {
  list_head free_list;
  unsigned long *map;
} free_area_t;

typedef struct {
  list_head list;
  int order;
} page;

page pages[NUM_PAGES];

/*
 * Initialize free lists and page structs. The memory layout divides 32000 available physcial pages. Based on the lab description, 
 * most workload are small requests no bigger than 128 bytes and less than or equal to 4 KiB. Requests of large memory are rare.
 */
void pages_init() {
  int order[] = { 0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };
  int i = 0, index = 1 << 13;
  for (;; i < index; i++) {
    list_add(free_area[0].free_list, pages[i].list);
    pages[i].order = 0;
  }
  index += 1 << 2;
  for (;; i < index; i++) {
    list_add(free_area[1].free_list, pages[i].list);
    pages[i].order = 1;
  }
  index += (1 << 2) * 3;
  for (;; i < index; i++) {
    list_add(free_area[2].free_list, pages[i].list);
    pages[i].order = 2;
  }
  index += 1 << 4;
  for (;; i < index; i++) {
    list_add(free_area[3].free_list, pages[i].list);
    pages[i].order = 3;
  }
  index += 1 << 5;
  for (;; i < index; i++) {
    list_add(free_area[4].free_list, pages[i].list);
    pages[i].order = 4;
  }
  index += 1 << 6;
  for (;; i < index; i++) {
    list_add(free_area[5].free_list, pages[i].list);
    pages[i].order = 5;
  }
  index += 1 << 7;
  for (;; i < index; i++) {
    list_add(free_area[6].free_list, pages[i].list);
    pages[i].order = 6;
  }
  index += 1 << 8;
  for (;; i < index; i++) {
    list_add(free_area[7].free_list, pages[i].list);
    pages[i].order = 7;
  }
  index += (1 << 8) * 3;
  for (;; i < index; i++) {
    list_add(free_area[8].free_list, pages[i].list);
    pages[i].order = 8;
  }
  index += 1 << 10;
  for (;; i < index; i++) {
    list_add(free_area[9].free_list, pages[i].list);
    pages[i].order = 9;
  }
  index += (1 << 10) * 3;
  for (;; i < index; i++) {
    list_add(free_area[10].free_list, pages[i].list);
    pages[i].order = 10;
  }
  index += (1 << 11) * 3;
  for (;; i < index; i++) {
    list_add(free_area[11].free_list, pages[i].list);
    pages[i].order = 11;
  }
  index += (1 << 12) * 3;
  for (;; i < index; i++) {
    list_add(free_area[12].free_list, pages[i].list);
    pages[i].order = 12;
  }
}

static free_area_t free_area[MAX_ORDER + 1];

/*
 * Initially, there are 4 free block in the pool: 7 4096-blocks, 
 * 1 2048-block, 1 1024-block, 1 256-block. The largest block 
 * has an order of 12, which can satisfy one 16 MiB request. Return
 * NULL(0) if the request cannot be satisfied. 
 */
void init_free_area() {
  for (int i = 0; i < MAX_ORDER + 1; i++) {
    list_init(&free_area[i].free_list);
    unsigned long *map;
    /* At i = 5, the list has a max of 32 pages per block, 128 blocks, 64 pairs. */
    if (i >= 5) {
      map = (unsigned long *)malloc(sizeof(unsigned long));  
      memset(map, 0, 8); 
    } else {
      // simplify (4096 / (1 << (order + 1))) / 64 bits
      int num_long = (1 << (5 - i)) * sizeof(unsigned long);
      map = (unsigned long *)malloc(num_pairs);
      memset(map, 0, num_pairs); 
    }
  }

  uintptr_t addr = HEAP_START;
  mem_block_t *block;
  for (int i = 0; i < 7; i++) {
    block = block_create(12, addr);
    list_add(free_area[12].free_list, block->next);
    addr = block->area.end; 
  }

  block = block_create(11, addr);
  list_add(free_area[11].free_list, block->next);
  addr = block->area.end;

  block = block_create(10, addr);
  list_add(free_area[10].free_list, block->next);
  addr = block->area.end;
  
  block = block_create(8, addr);
  list_add(free_area[8].free_list, block->next);
}

mem_block_t *block_create(size_t order, uintptr_t start_addr) {
  mem_block_t *block;
  list_init(block->next);
  block->area.start = start_addr;
  // e.g. start_addr = 0x300000, order = 12, end = 0x1300000
  block->area.end = start_addr + (1 << order + 12); 

  return block;
}

int log_2(int n) {
  int ans = 0;
  while (n >>= 1) ans++;
  return ans;
}

/* Find a block of the requested order */ 
mem_block_t *rmqueue(int order) {

  
}

/* Splits page blocks of higher orders until a page block of the needed order is available */
mem_block_t *expand(mem_block_t *bk, long index, int low, int high, free_area_t *area) {

}

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


