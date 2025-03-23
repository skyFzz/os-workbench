#ifndef KERNEL_H__
#define KERNEL_H__

struct cpu {
    /* interrupt management for multiprocessor kernel */
    int noff;   // track how many times push_off() has been called
    int intena; // stores whether interrupts were enabled before the first push_off()
};

extern struct cpu cpus[];
#define mycpu (&cpus[cpu_current()])

#define panic(...) \
    do { \
        printf("Panic: " __VA_ARGS__); \
        halt(1); \
    } while (0)

#endif
