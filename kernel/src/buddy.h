void *alloc_bootmem(size_t size);
void mem_map_alloc();
void mem_map_init();
void free_area_alloc();
void free_area_init();
void *pgalloc(int size);
void pgfree(void *page);
