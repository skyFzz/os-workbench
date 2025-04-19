// When we test your kernel implementation, the framework
// directory will be replaced. Any modifications you make
// to its files (e.g., kernel.h) will be lost. 

// Note that some code requires data structure definitions 
// (such as `sem_t`) to compile, and these definitions are
// not present in kernel.h. 

// Include these definitions in os.h.
#include <common.h>
#include "list.h"

#ifndef OS_H__
#define OS_H__

/* interrupt management for multiprocessor kernel */
struct cpu {
    int noff;   // track how many times push_off() has been called
    int intena; // stores whether interrupts were enabled before the first push_off()
};

extern struct cpu cpus[];

struct task {
  const char  *name;
  void        (*entry)(void *);
  void        *arg;
  Context     context;
  struct task *next;
  int         status;
  void        *stack;
};

typedef struct queue { 
  task_t      *head;
  task_t      *tail;
} queue_t;

struct spinlock {
  const char  *name;
  int         status;
  struct cpu  *cpu;
};

struct semaphore {
  const char  *name;
  int         value;
  spinlock_t  lk;
  queue_t     wait_list;
};

typedef struct handler_entry_t {
  list_head   list;
  int         seq;
  Event       ev; 
  Context     *(*handler)(Event ev, Context *ctx);
} handler_entry_t;

#endif
