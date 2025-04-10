/**
 * linked_list_benchmark.c
 *
 * Tests performance of field reordering when traversing linked lists.
 */

#include <stdlib.h>

// Number of iterations passed by command line
int ITERATIONS = 0;

// Size of linked list
#define LIST_SIZE 10000
#define SEARCH_COUNT 1000

// Node structure with different field access patterns during traversal
typedef struct Node {
    struct Node* next;    // Frequently accessed during traversal
    int key;              // Frequently accessed during search
    char data[64];        // Rarely accessed during traversal
    double value;         // Occasionally accessed
} Node;

// Global array to allocate nodes from (instead of malloc)
Node nodes[LIST_SIZE];
Node* head = NULL;

// Search targets
int search_keys[SEARCH_COUNT];

/**
 * Search the linked list for nodes with matching keys
 */
int search_list() {
    int found = 0;

    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (int s = 0; s < SEARCH_COUNT; s++) {
            int target = search_keys[s];

            // Traverse the list looking for the key
            Node* current = head;
            while (current != NULL) {
                if (current->key == target) {
                    found++;
                    // Occasionally access the value
                    if (found % 10 == 0) {
                        volatile double v = current->value;
                    }
                    break;
                }
                current = current->next;
            }
        }
    }

    return found;
}

/**
 * Main function that initializes the linked list and runs the benchmark
 */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 1;
    }

    ITERATIONS = atoi(argv[1]);
    if (ITERATIONS < 1) {
        return 1;
    }

    // Initialize the linked list
    for (int i = 0; i < LIST_SIZE; i++) {
        nodes[i].key = i;
        nodes[i].value = i * 1.1;
        nodes[i].next = (i < LIST_SIZE - 1) ? &nodes[i + 1] : NULL;

        // Fill in some data (rarely accessed)
        for (int j = 0; j < 64; j++) {
            nodes[i].data[j] = (i + j) % 256;
        }
    }

    // Set the head pointer
    head = &nodes[0];

    // Initialize search keys
    for (int i = 0; i < SEARCH_COUNT; i++) {
        // Generate some hits and some misses
        search_keys[i] = (i % 2 == 0) ? (i % LIST_SIZE) : (LIST_SIZE + i);
    }

    // Run the benchmark
    volatile int result = search_list();

    return 0;
}