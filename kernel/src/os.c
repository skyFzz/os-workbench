#include <common.h>

#define DEBUG_LOCAL
#define NPROD       2
#define NCONS       2
#define N           1

sem_t empty, fill;

static void producer(void *arg) {
  while (1) {
    kmt->sem_wait(&empty);
    putch('(');
    kmt->sem_signal(&fill);
  }
}

static void consumer(void *arg) {
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
}

static void os_init() {
  pmm->init();
  kmt->init();
 // dev->init();

#ifdef DEBUG_LOCAL
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
  Context *next = NULL;
  for (auto &h:handlers_sorted_by_seq) {
    if (h.event == EVENT_NULL || h.event == ev.event) {
      Context *r = h.handler(ev, ctx);
      panic_on(r && next, "return to multiple contexts");
      if (r) next = r;
    }
  }
  panic_on(!next, "return to NULL context");
  panic_on(sane_context(next), "return to invalid context");
  return next;
}

static void on_irq(int seq, int event, handler_t handler) {
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
  .trap = os_trap,
  .on_irq = os_irq,
};
