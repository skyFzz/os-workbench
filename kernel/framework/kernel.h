// DO NOT MODIFY: Will be reverted by the Online Judge.

#include <am.h>

/*
    MODULE(pmm) {
      void  (*init)();
      void *(*alloc)(size_t size);
      void  (*free)(void *ptr);
    };

    typedef struct pmm_pmm_t pmm_pmm_t;
    extern pmm_pmm_t *pmm;
    struct pmm_pmm_t {
      void  (*init)();
      void *(*alloc)(size_t size);
      void  (*free)(void *ptr);
    };
*/
#define MODULE(mod) \
    typedef struct mod_##mod##_t mod_##mod##_t; \
    extern mod_##mod##_t *mod; \
    struct mod_##mod##_t

/*
    MODULE_DEF(pmm) = {
      .init = pmm_init,
      .alloc = kalloc,
      .free = kfree,
    };

    extern pmm_pmm_t  __pmm_obj;
    pmm_pmm_t *pmm = &__pmm_obj;
    pmm_pmm_t __pmm_obj = {
      .init = pmm_init,
      .alloc = kalloc,
      .free = kfree,
    };
*/

#define MODULE_DEF(mod) \
    extern mod_##mod##_t __##mod##_obj; \
    mod_##mod##_t *mod = &__##mod##_obj; \
    mod_##mod##_t __##mod##_obj

typedef Context *(*handler_t)(Event, Context *);
MODULE(os) {
    void (*init)();
    void (*run)();
    // 系统唯一中断/系统调用的入口
    Context *(*trap)(Event ev, Context *context);
    // 中断处理程序的回调注册
    void (*on_irq)(int seq, int event, handler_t handler);
};

MODULE(pmm) {
    void  (*init)();
    void *(*alloc)(size_t size);
    void  (*free)(void *ptr);
};

// forward declaration
typedef struct task task_t;
typedef struct spinlock spinlock_t;
typedef struct semaphore sem_t;
MODULE(kmt) {
    void (*init)();
    int  (*create)(task_t *task, const char *name, void (*entry)(void *arg), void *arg);
    void (*teardown)(task_t *task);
    void (*spin_init)(spinlock_t *lk, const char *name);
    void (*spin_lock)(spinlock_t *lk);
    void (*spin_unlock)(spinlock_t *lk);
    void (*sem_init)(sem_t *sem, const char *name, int value);
    void (*sem_wait)(sem_t *sem);
    void (*sem_signal)(sem_t *sem);
};

typedef struct device device_t;
MODULE(dev) {
    void (*init)();
    device_t *(*lookup)(const char *name);
};
