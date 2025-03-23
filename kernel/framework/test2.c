#include <kernel.h>
#include <common.h>
#include <klib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

enum ops { OP_ALLOC = 1, OP_FREE };

struct malloc_op {
    enum ops type;
    union {
        size_t sz;  // OP_ALLOC: size
        void *addr; // OP_FREE: address
    };
};

#define MAX_ALLOCATED_BLOCKS 1000

struct allocated_block {
    void *addr;
    size_t size;
};

struct allocated_block allocated_blocks[MAX_ALLOCATED_BLOCKS];
size_t num_allocated_blocks = 0;

// Add an allocated block to the list
void add_allocated_block(void *addr, size_t size) {
    if (num_allocated_blocks < MAX_ALLOCATED_BLOCKS) {
        allocated_blocks[num_allocated_blocks].addr = addr;
        allocated_blocks[num_allocated_blocks].size = size;
        num_allocated_blocks++;
    }
}

// Get a random address from the list of allocated blocks
void *get_random_allocated_address() {
    if (num_allocated_blocks == 0) {
        return NULL; // No blocks allocated yet
    }
    size_t index = rand() % num_allocated_blocks;
    return allocated_blocks[index].addr;
}

// Remove a freed block from the list
void remove_freed_block(void *addr) {
    for (size_t i = 0; i < num_allocated_blocks; i++) {
        if (allocated_blocks[i].addr == addr) {
            // Shift the remaining blocks to fill the gap
            for (size_t j = i; j < num_allocated_blocks - 1; j++) {
                allocated_blocks[j] = allocated_blocks[j + 1];
            }
            num_allocated_blocks--;
            break;
        }
    }
}

// Generate a random malloc operation
struct malloc_op random_op() {
    struct malloc_op op;

    // Randomly choose between ALLOC and FREE
    op.type = (rand() % 2 == 0) ? OP_ALLOC : OP_FREE;

    if (op.type == OP_ALLOC) {
        // Generate a random size between 1 and 4096
        op.sz = (size_t)(rand() % 4096) + 1;
    } else {
        // For FREE, choose a random address from previously allocated blocks
        op.addr = get_random_allocated_address();
    }

    return op;
}

// Verify that the allocated memory is valid
void alloc_check(void *ptr, size_t size) {
    if (ptr == NULL) {
        // Allocation failed
        printf("Allocation failed for size %zu\n", size);
        return;
    }

    // Write a pattern to the allocated memory
    memset(ptr, 0xAA, size);

    // Verify the pattern
    for (size_t i = 0; i < size; i++) {
        if (((uint8_t*)ptr)[i] != 0xAA) {
            printf("Memory corruption detected at offset %zu\n", i);
            return;
        }
    }

    // Add the allocated block to the list
    add_allocated_block(ptr, size);
}

void stress_test() {
    while (1) {
        // Generate a random malloc operation
        struct malloc_op op = random_op();

        switch (op.type) {
            case OP_ALLOC: {
                void *ptr = pmm->alloc(op.sz);
                alloc_check(ptr, op.sz);
                break;
            }
            case OP_FREE:
                pmm->free(op.addr);
                remove_freed_block(op.addr); // Remove the freed block from the list
                break;
        }
    }
}

int main() {
    os->init();
    mpe_init(stress_test);
    return 1;
}
