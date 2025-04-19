#include <common.h>
#include <klib.h>

struct cpu cpus[16];

void simple_test() {
    // Request 26 bytes of memory
    size_t size = 26;
    void *ptr = pmm->alloc(size);

    if (ptr == NULL) {
        printf("Allocation failed");
        return;
    }

    // Fill the memory with English alphabets (A-Z)
    char *alphabet_ptr = (char *)ptr;
    for (int i = 0; i < 26; i++) {
        alphabet_ptr[i] = 'A' + i; // Fill with A, B, C, ..., Z
    }

    // Verify the contents of the memory
    int correct = 1;
    for (int i = 0; i < 26; i++) {
        if (alphabet_ptr[i] != 'A' + i) {
            printf("Memory corruption detected at offset %d: expected %c, got %c\n",
                   i, 'A' + i, alphabet_ptr[i]);
            correct = 0;
            break;
        } else {
          printf("Memory at offset %d: %c\n", i, alphabet_ptr[i]);
        }
    }

    if (correct) {
        printf("Memory test passed: All 26 bytes are correctly filled with A-Z.\n");
    }

    // Free the allocated memory
    pmm->free(ptr);
}

int main() {
    os->init();
    mpe_init(simple_test);
    return 0;
}
