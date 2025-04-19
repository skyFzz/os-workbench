#include <os.h>
#include <limits.h>

#define DEBUG_LOCAL
#define NPROD       1
#define NCONS       1
#define N           4
#define list_entry(ptr, type, member) ((struct type *)((void *)ptr - (void *)&((type *)0)->member))

sem_t             empty, fill;
handler_entry_t   *dummy;

static void T_produce(void *arg) {
  iset(true);
  putch('1');
  while (1) {
    kmt->sem_wait(&empty);
    putch('(');
    kmt->sem_signal(&fill);
  }
}

static void T_consume(void *arg) {
  printf("Inside T_consume entry...\n");
  while (1) {
    kmt->sem_wait(&fill);
    putch(')');
    kmt->sem_signal(&empty);
  }
}


static inline task_t *task_alloc() {
  return pmm->alloc(sizeof(task_t));
}

static void run_test1() {
  kmt->sem_init(&empty, "empty", N);
  kmt->sem_init(&fill, "fill", 0);
  for (int i = 0; i < NPROD; i++) {
    kmt->create(task_alloc(), "producer", T_produce, NULL);
  }
  for (int i = 0; i < NCONS; i++) {
    kmt->create(task_alloc(), "consumer", T_consume, NULL);
  }
  yield();
}

static void os_init() {
  pmm->init();
  printf("Finish pmm->init\n");
  printf("-------------------------------------------------------------------------------------------------\n");

  dummy = pmm->alloc(sizeof(handler_entry_t));
  assert(dummy != NULL);
  dummy->seq = INT_MIN;
  list_init(&dummy->list);

  kmt->init();
  printf("Finish kmt->init\n");
  printf("-------------------------------------------------------------------------------------------------\n");
 // dev->init();
  
#ifdef DEBUG_LOCAL
  printf("Start local testing...\n");
  run_test1();
#endif
}

static void os_run() {
  for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
      putch(*s == '*' ? '0' + cpu_current() : *s);
  }
  while (1) ;
}

static Context *os_trap(Event ev, Context *ctx) {
  printf("int status: %d\n", ienabled());
  Context *next = NULL;
  handler_entry_t *entry = list_entry(dummy->list.next, handler_entry_t, list);

  while (entry) {
    if (entry->ev.event == EVENT_NULL || entry->ev.event == ev.event) {
      Context *ret_ctx = entry->handler(ev, ctx);
      panic_on(ret_ctx && next, "return to multiple contexts");
      if (ret_ctx) next = ret_ctx;
    }
    entry = list_entry(entry->list.next, handler_entry_t, list);
  }
  printf("Returning from os_trap...\n");
  panic_on(!next, "return to NULL context");
  //panic_on(sane_context(next), "return to invalid context");
  return next;
}

/* Register an event handler by inserting it to the sorted list */
static void os_on_irq(int seq, int event, handler_t handler) {
  handler_entry_t *new_handler = pmm->alloc(sizeof(handler_entry_t));
  assert(new_handler != NULL);
  printf("new_handler allocated at: %p\n", new_handler);
  list_init(&new_handler->list);
  new_handler->seq = seq;
  new_handler->ev.event = event;
  new_handler->handler = handler;

  // dummy -> MIN
  handler_entry_t *cur = list_entry(dummy->list.next, handler_entry_t, list);
  while (seq > cur->seq && cur->list.next != NULL) {
    printf("Advancing...\n");
    cur = list_entry(cur->list.next, handler_entry_t, list);
  }
  list_add(&cur->list, &new_handler->list);
  if (new_handler->list.next == &cur->list) new_handler->list.next = NULL;
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
  .trap = os_trap,
  .on_irq = os_on_irq,
};
