// DO NOT MODIFY: Will be reverted by the Online Judge.

#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>
#include <am.h>
#include <common.h>
#include <os.h>

int main(const char *args) {
    ioe_init();
    cte_init(os->trap);
    os->init();
//    init_workload(args);
    mpe_init(os->run);
}
