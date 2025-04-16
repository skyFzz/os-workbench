// DO NOT MODIFY: Will be reverted by the Online Judge.

#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>
#include <am.h>
#include <common.h>
#include <os.h>

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

static void create_threads() {
  kmt->create(pmm->alloc(sizeof(task_t)), "test-thread-1", producer, xxx);
  kmt->create(pmm->alloc(sizeof(task_t)), "test-thread-2", consumer, yyy);
}

int main(const char *args) {
    ioe_init();
    cte_init(os->trap);
    os->init();
    create_threads();
    mpe_init(os->run);
    return 1;
}
