#ifndef KERNEL_H__
#define KERNEL_H__

struct cpu {
    /* interrupt management for multiprocessor kernel */
    int noff;   // track how many times push_off() has been called  计数
    int intena; // stores whether interrupts were enabled before the first push_off()   关中断前状态
};

extern struct cpu cpus[];
#define mycpu (&cpus[cpu_current()])

#ifndef panic
#define panic(...) \
    do { \
        printf("Panic: " __VA_ARGS__); \
        halt(1); \
    } while (0)
#endif
#endif
