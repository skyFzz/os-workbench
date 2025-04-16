#include <common.h>
#include "list.h"

#define STACK_SIZE  8192
#define UNLOCKED    0
#define LOCKED      1
#define N           10
#define SEM_MAX     100
#define BLOCKED     0
#define RUNNABLE    1

task_t tasks[N];
task_t current;

static void kmt_init() {
  for (int i = 0; i < N; i++) {
    task_t *t = 
    
  }
}

static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
  task->name  = name;
  task->entry = entry;
  task->stack = pmm->alloc(STACK_SIZE);
}

static void kmt_teardown(task_t *task) {
  pmm->free(task->stack);
}


bool holding(spinlock_t *lk) {
  return (lk->status == LOCKED && lk->cpu == &cpus[cpu_current()]);
}

/* Save int state */
void push_off(void) {
  int old = ienabled();
  struct cpu *c = &cpus[cpu_current()];

  iset(false);
  if (c->noff == 0) {
    c->intena = old;
  }
}

/* Restore int state */
void pop_off(void) {
  struct cpu *c = &cpus[cpu_current()];

  // Never enable interrupt when holding a lock.
  if (ienabled()) {
     panic("pop_off - interruptible");
  }

  if (c->noff < 1) {
     panic("pop_off");
  }

  c->noff -= 1;
  if (c->noff == 0 && c->intena) {
     iset(true);
  }
}

static void enqueue(queue_t *q, task_t *task) {
  if (!q) return NULL;
  q->tail->next = task;
  q->tail = task;
}

static task_t *dequeue(queue_t *q) {
  if (!q) return NULL;
  task_t *rett = q->head;
  q->head = rett->next;
  return rett;
}

static int empty(queue_t *q) {
  return q->head->next == q->tail && q->tail->next == q->head;
}

static void spin_init(spinlock_t *lk, const char *name) {
  lk->name    = name;
  lk->status  = UNLOCKED;
  lk->cpu     = NULL;
}

static void spin_lock(spinlock_t *lk) {
  push_off();
  if (holding(lk)) {
    panic("acquire %s", lk->name);
  }
  while (atomic_xchg(&lk->status, LOCKED)) {
    yield();
  }
  lk->cpu = &cpus[cpu_current()];
}

static void spin_unlock(spinlock_t *lk) {
  if (!holding(lk)) {
    panic("release %s", lk->name);
  }
  lk->cpu = NULL;
  atomic_xchg(&lk->status, UNLOCKED);
  pop_off();
}

static void sem_init(sem_t *sem, const char *name, int value) {
  if (value > SEM_MAX) {
    printf("Failed at sem_init: value > SEM_MAX\n");
    halt(0);
  }
  sem->name = name;
  sem->value = value;
  spin_init(&sem->lk);
}

static void sem_wait(sem_t *sem) {
  assert(!ienabled());
  int acquired = 0;
  spin_lock(&sem->lk);
  if (sem->value <= 0) {
    enqueue(sem->wait_list, current);
    current->status = BLOCKED;
  } else {
    sem->value--;
    acquired = 1;
  }
  spin_unlock(&sem->lk);
  if (!acquired) {
    // a theard may do V here, changing the status from BLOCKED to RUNNABLE, but it's ok
    yield();
  }
  assert(!ienabled());
}

static void sem_signal(sem_t *sem) {
  spin_lock(&sem->lk);
  if (!empty(sem->wait_list)) {
    struct task_t *task = dequeue(sem->wait_list);
    task->status = RUNNABLE;
  } else {
    sem->value++;
  }
  spin_unlock(&sem->lk);
}

MODULE_DEF(kmt) = {
  .init         = kmt_init,
  .create       = kmt_create,
  .teardown     = kmt_teardown,
  .spin_init    = spin_init,
  .spin_lock    = spin_lock,
  .spin_unlock  = spin_unlock,
  .sem_init     = sem_init,
  .sem_wait     = sem_wait,
  .sem_signal   = sem_signal,
};
