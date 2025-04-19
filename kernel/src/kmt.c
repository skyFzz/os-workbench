#include <os.h>
#include <limits.h>
#include "list.h"

#define STACK_SIZE  4096
#define UNLOCKED    0
#define LOCKED      1
#define T_MAX       10
#define SEM_MAX     100
#define BLOCKED     0
#define RUNNABLE    1
#define SMP         1

/* Each core has a current pointer, representing the workload it is currently running */
task_t          *current[SMP];
#define CURRENT current[0]      // mpe needed for cpu_current()
queue_t         *ready_queue;
int             cnt;
struct cpu      cpus[SMP];

static int empty(queue_t *q) {
  return q->head == NULL && q->tail == NULL;
}

static void enqueue(queue_t *q, task_t *task) {
  task->next = NULL;
  if (q->tail) {
    q->tail->next = task;
  } else {
    q->head = task;
  }
  q->tail = task;
}

static task_t *dequeue(queue_t *q) {
  if (empty(q)) {
    printf("Failed at dequeue: empty queue\n");
    halt(0);
  }
  task_t *ret = q->head;
  if (q->tail == ret) {
    q->head = q->tail = NULL;
  } else {
    q->head = ret->next; 
  }
  return ret;
}

Context *context_save(Event ev, Context *ctx) {
  printf("Begin context save...\n");
  CURRENT->context = *ctx;
  return NULL;
}

Context *schedule(Event ev, Context *ctx) {
  printf("Begin schedule handling...\n");
  printf("Old task name: %s\n", CURRENT->name);
  CURRENT = dequeue(ready_queue);
  while (CURRENT->status != RUNNABLE) {
    CURRENT = dequeue(ready_queue);
  }
  printf("New task name: %s\n", CURRENT->name);
  return &CURRENT->context;
}

/* Init bootstrap thread */ 
static void kmt_init() {
  ready_queue   = pmm->alloc(sizeof(queue_t));
  ready_queue->head = ready_queue->tail = NULL;
  task_t *boot  = pmm->alloc(sizeof(task_t));
  boot->stack   = pmm->alloc(sizeof(STACK_SIZE));
  boot->name    = "_";
  boot->entry   = NULL;
  boot->arg     = NULL;
  boot->status  = BLOCKED;
  cnt           = 1;

  enqueue(ready_queue, boot);
  for (int i = 0; i < SMP; i++) {
    current[i] = boot;
    cpus[i].noff = 0;
    cpus[i].intena = ienabled();
  }
  handler_t kmt_context_save = context_save;
  handler_t kmt_schedule = schedule;
  os->on_irq(INT_MIN, EVENT_NULL, kmt_context_save);
  os->on_irq(INT_MAX, EVENT_NULL, kmt_schedule);
}

static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
  if (cnt == T_MAX) {
    printf("Failed at kmt_create: cnt == T_MAX\n");
    halt(0);
  }
  task->name        = name;
  task->entry       = entry;
  task->arg         = arg;
  task->stack       = pmm->alloc(STACK_SIZE);
  //printf("task->stack allocated start at: %p\n", task->stack);
  //printf("task->stack allocated ends at: %p\n", task->stack + STACK_SIZE);
  task->context     = *kcontext( (Area) { .start = task->stack, .end = task->stack + STACK_SIZE }, entry, NULL);
  //printf("task->entry allocated at: %p\n", task->entry);
  //printf("task->context allocated at: %p\n", &task->context);
  task->status      = RUNNABLE;
  enqueue(ready_queue, task);

  printf("Task %s is ready for physical execution\n", task->name);
  return 1;
}

static void kmt_teardown(task_t *task) {
}

/* Save int state */
void push_off(void) {
  int old = ienabled();
    printf("c->intena is: %d\n", ienabled());
  struct cpu *c = &cpus[cpu_current()];

  iset(false);
  if (c->noff == 0) {
    c->intena = old;
  }
  c->noff++;
}

/* Restore int state */
void pop_off(void) {
  struct cpu *c = &cpus[cpu_current()];

  // Never enable interrupt when holding a lock.
  if (ienabled()) {
     printf("pop_off - interruptible\n");
     halt(0);
  }

  if (c->noff < 1) {
     printf("pop_off\n");
     halt(0);
  }

  c->noff -= 1;
  if (c->noff == 0 && c->intena) {
     iset(true);
  }
}

bool holding(spinlock_t *lk) {
  return (lk->status == LOCKED && lk->cpu == &cpus[cpu_current()]);
}

static void spin_init(spinlock_t *lk, const char *name) {
  lk->name    = name;
  lk->status  = UNLOCKED;
  lk->cpu     = NULL;
}

static void spin_lock(spinlock_t *lk) {
  push_off();
  if (holding(lk)) {
    printf("acquire %s\n", lk->name);
    halt(0);
  }
  while (atomic_xchg(&lk->status, LOCKED)) {
    yield();
  }
  lk->cpu = &cpus[cpu_current()];
}

static void spin_unlock(spinlock_t *lk) {
  if (!holding(lk)) {
    printf("release %s\n", lk->name);
    halt(0);
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
  spin_init(&sem->lk, "sem_lock");
  sem->wait_list.head->next = sem->wait_list.tail;
  sem->wait_list.tail->next = sem->wait_list.head;
}

static void sem_wait(sem_t *sem) {
  int acquired = 0;
  spin_lock(&sem->lk);
  if (sem->value <= 0) {
    enqueue(&sem->wait_list, CURRENT);
    CURRENT->status = BLOCKED;
  } else {
    sem->value--;
    acquired = 1;
  }
  spin_unlock(&sem->lk);
  if (!acquired) {
    // a theard may do V here, changing the status from BLOCKED to RUNNABLE, but it's ok
    yield();
  }
}

static void sem_signal(sem_t *sem) {
  spin_lock(&sem->lk);
  if (!empty(&sem->wait_list)) {
    task_t *task = dequeue(&sem->wait_list);
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
