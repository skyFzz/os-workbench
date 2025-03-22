#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>

#define ALIGN       16
#define PAGE_SIZE   4096

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

uintptr_t __brk(uintptr_t);

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

uintptr_t __brk(uintptr_t newbrk) {
    uintptr_t result;
    asm volatile (
        "movq $12, %%rax\n" // __NR_brk = 12 (x86-64)
        "movq %1, %%rdi\n"  // newbrk -> RDI
        "syscall\n"         // Invoke the system call
        "movq %%rax, %0"    // Result -> result
        : "=r"(result)      // Output
        : "r"(newbrk)       // Input
        : "rax", "rdi", "rcx", "memory" // Clobbered registers
    );
    return result;
}

/* This malloc/free implementation depends on Abstract Machine APIs: protect/unprotect and map, just like brk and mmap syscalls */
void *malloc(size_t n) {
  /*
	static uintptr_t cur, brk;
	uintptr_t base, new;
	//static int lock;
	size_t align=1;


	if (n < SIZE_MAX - ALIGN)
		while (align<n && align<ALIGN)
			align += align;
	n = (n + align - 1) & -align;

//	LOCK(&lock);
	if (!cur) cur = brk = __brk(0)+16;
	if (n > SIZE_MAX - brk) {
    printf("alloc error\n");
    return 0;
  }

	base = (cur + align-1) & -align;
	if (base+n > brk) {
		new = (base+n + PAGE_SIZE-1) & -PAGE_SIZE;
    if (__brk(new) != new) {
      printf("alloc error\n");
       return 0;
    }

		brk = new;
	}
	cur = base+n;
//	UNLOCK(&lock);

	return (void *)base;

  */
/*
fail:
//	UNLOCK(&lock);
	errno = ENOMEM;
	return 0;
*/
  
  panic("not implemented");
}

void free(void *ptr) {
  panic("Not implemented");
}

#endif
