// DO NOT MODIFY: Will be reverted by the Online Judge.

#include <kernel.h>
#include <common.h>
#include <klib.h>

enum ops { OP_ALLOC = 1, OP_FREE };

struct malloc_op {
    enum ops type;
    union {
        size_t sz;  // OP_ALLOC: size
        void *addr; // OP_FREE: address
    };
};

void stress_test() {
    while (1) {
        // 根据 workload 生成操作
        struct malloc_op op = random_op();

        switch (op.type) {
            case OP_ALLOC: {
                void *ptr = pmm->alloc(op.sz);
                alloc_check(ptr, op.sz);
                break;
            }
            case OP_FREE:
                pmm->free(op.addr);
                break;
        }
    }
}

int main() {
    os->init();
    mpe_init(stress_test);
    return 1;
}
