#include <kernel.h>
#include <common.h>
#include <stdio.h>
#include <string.h>

void simple_test_case() {
    // Request 26 bytes of memory
    size_t size = 26;
    void *ptr = pmm->alloc(size);

    if (ptr == NULL) {
        printf("Allocation failed for %zu bytes\n", size);
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
    mpe->init(simple_test_case);
    return 0;
}
