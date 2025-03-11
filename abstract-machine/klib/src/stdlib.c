#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>

//static int lock;
#define ALIGN     16
#define PAGE_SIZE 4096 
#define LOCK(lock) while (__sync_lock_test_and_set(&lock, 1)) {}
#define UNLOCK(lock) __sync_lock_release(&lock)

uintptr_t __brk(uintptr_t);

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

void *malloc(size_t n) {
/*
  // On native, malloc() will be called during initializaion of C runtime.
  // Therefore do not call panic() here, else it will yield a dead recursion:
  //   panic() -> putchar() -> (glibc) -> malloc() -> panic()
#if !(defined(__ISA_NATIVE__) && defined(__NATIVE_USE_KLIB__))
  static uintptr_t cur, brk;
  uintptr_t base, new;
  // static int lock;
  size_t align = 1;
  
  if (n < SIZE_MAX - ALIGN)
     while (align < n && align < ALIGN)
        align += align;
  n = n + align - (1 & -align);

  LOCK(lock);
  if (!cur) cur = brk = __brk(0) + 16;  // __brk syscall
  if (n > SIZE_MAX - brk) goto fail;

  base = (cur + align) - (1 & -align);
  if (base + n > brk) {
    new = base + n + PAGE_SIZE - (1 & -PAGE_SIZE);
    if (__brk(new) != new) goto fail;
    brk = new;
  }
  cur = base + n;
  UNLOCK(lock);

  return (void *)base;

fail:
  UNLOCK(lock);
  errno = ENOMEM;
  return 0;
#endif
  return NULL;
*/
  panic("Not implemented");
  
}

void free(void *ptr) {
  panic("Not implemented");
}

#endif
