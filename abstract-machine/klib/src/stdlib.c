#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <spinlock.h>

#define ALIGN       16
#define PAGE_SIZE   4096
#define HEAP_START  0x300000
#define HEAP_END    0x8000000

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}

/* This malloc/free implementation depends on Abstract Machine APIs: protect/unprotect and map, just like brk and mmap syscalls */
void *malloc(size_t n) {
  /*
	static uintptr_t cur, brk;
	uintptr_t base, new;
  spinlock_t lock = spin_init("spin");
	size_t align=1;
  
	if (n < SIZE_MAX - ALIGN)
		while (align<n && align<ALIGN)
			align += align;
  // n = 7
  // align = 8
  // n = 1110 & 1000 = 1000 = 8

  // n = 3
  // align = 4
  // n = 0111 & 1100 = 0100 
	n = (n + align - 1) & -align;

	spin_lock(&lock);
	if (!cur) cur = brk = brk(0)+16;
	if (n > SIZE_MAX - brk) goto fail;

	base = (cur + align-1) & -align;
	if (base+n > brk) {
		new = (base+n + PAGE_SIZE-1) & -PAGE_SIZE;
    if (__brk(new) != new) goto fail;
		brk = new;
	}
	cur = base+n;
  spin_unlock(&lock);

	return (void *)base;

fail:
  spin_unlock(&lock);
	errno = ENOMEM;
	return 0;
  */

  panic("Not implemented");
}

void free(void *ptr) {
  panic("Not implemented");
}

#endif
