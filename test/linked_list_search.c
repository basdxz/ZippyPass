#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

// Poor layout - frequently accessed search key is far from the pointer
struct NodePoorLayout {
    struct NodePoorLayout *next;    // Node pointer (8 bytes)
    char data[1024];                  // Application data (64 bytes)
    int key;                        // Search key (4 bytes, frequently accessed)
    int value;                      // Associated value (4 bytes)
};

// Better layout - frequently accessed fields are grouped together
struct NodeGoodLayout {
    struct NodeGoodLayout *next;    // Node pointer (8 bytes)
    int key;                        // Search key (4 bytes, frequently accessed)
    int value;                      // Associated value (4 bytes)
    char data[1024];                  // Application data (64 bytes)
};

#define LIST_SIZE 10000
#define SEARCHES 100000

// Function prototypes
struct NodePoorLayout* binary_search_poor(struct NodePoorLayout* head, int length, int target);
struct NodeGoodLayout* binary_search_good(struct NodeGoodLayout* head, int length, int target);
struct NodePoorLayout* get_node_at_poor(struct NodePoorLayout* start, int steps);
struct NodeGoodLayout* get_node_at_good(struct NodeGoodLayout* start, int steps);

// To prevent optimization
volatile int sink;

double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

int main() {
    double start, end;
    double elapsed_poor, elapsed_good;

    printf("Struct sizes - Poor layout: %zu bytes, Good layout: %zu bytes\n",
           sizeof(struct NodePoorLayout), sizeof(struct NodeGoodLayout));

    // Create sorted linked list with poor layout
    struct NodePoorLayout *poor_head = NULL, *poor_current = NULL;
    for (int i = 0; i < LIST_SIZE; i++) {
        struct NodePoorLayout *node = malloc(sizeof(struct NodePoorLayout));
        node->key = i;
        node->value = i * 10;
        node->next = NULL;
        // Fill data with some pattern
        for (int j = 0; j < 64; j++) {
            node->data[j] = (char)(j % 26 + 'a');
        }

        if (poor_head == NULL) {
            poor_head = node;
        } else {
            poor_current->next = node;
        }
        poor_current = node;
    }

    // Create identical sorted linked list with good layout
    struct NodeGoodLayout *good_head = NULL, *good_current = NULL;
    for (int i = 0; i < LIST_SIZE; i++) {
        struct NodeGoodLayout *node = malloc(sizeof(struct NodeGoodLayout));
        node->key = i;
        node->value = i * 10;
        node->next = NULL;
        // Fill data with same pattern
        for (int j = 0; j < 64; j++) {
            node->data[j] = (char)(j % 26 + 'a');
        }

        if (good_head == NULL) {
            good_head = node;
        } else {
            good_current->next = node;
        }
        good_current = node;
    }

    // Generate random search targets
    int *targets = malloc(SEARCHES * sizeof(int));
    srand(42);  // For reproducibility
    for (int i = 0; i < SEARCHES; i++) {
        targets[i] = rand() % (LIST_SIZE * 2);  // Some will miss, some will hit
    }

    // Warmup to ensure caches are primed
    int warmup = 0;
    for (int i = 0; i < SEARCHES / 10; i++) {
        struct NodePoorLayout *result1 = binary_search_poor(poor_head, LIST_SIZE, targets[i]);
        struct NodeGoodLayout *result2 = binary_search_good(good_head, LIST_SIZE, targets[i]);
        if (result1) warmup += result1->value;
        if (result2) warmup += result2->value;
    }
    sink = warmup;

    // Benchmark poor layout
    int found_poor = 0;
    start = get_time_ms();
    for (int i = 0; i < SEARCHES; i++) {
        struct NodePoorLayout *result = binary_search_poor(poor_head, LIST_SIZE, targets[i]);
        if (result) found_poor++;
    }
    end = get_time_ms();
    elapsed_poor = end - start;

    // Benchmark good layout
    int found_good = 0;
    start = get_time_ms();
    for (int i = 0; i < SEARCHES; i++) {
        struct NodeGoodLayout *result = binary_search_good(good_head, LIST_SIZE, targets[i]);
        if (result) found_good++;
    }
    end = get_time_ms();
    elapsed_good = end - start;

    // Prevent optimization
    sink = found_poor + found_good;

    printf("Poor layout time: %.3f ms (found %d items)\n", elapsed_poor, found_poor);
    printf("Good layout time: %.3f ms (found %d items)\n", elapsed_good, found_good);
    printf("Performance improvement: %.2f%%\n",
           (elapsed_poor - elapsed_good) / elapsed_poor * 100);

    // Free memory
    struct NodePoorLayout *p_temp;
    while (poor_head) {
        p_temp = poor_head;
        poor_head = poor_head->next;
        free(p_temp);
    }

    struct NodeGoodLayout *g_temp;
    while (good_head) {
        g_temp = good_head;
        good_head = good_head->next;
        free(g_temp);
    }

    free(targets);
    return 0;
}

// Binary search in a linked list - poor layout
struct NodePoorLayout* binary_search_poor(struct NodePoorLayout* head, int length, int target) {
    int left = 0;
    int right = length - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        struct NodePoorLayout* mid_node = get_node_at_poor(head, mid);

        if (mid_node->key == target) {
            return mid_node;
        } else if (mid_node->key < target) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return NULL;
}

// Binary search in a linked list - good layout
struct NodeGoodLayout* binary_search_good(struct NodeGoodLayout* head, int length, int target) {
    int left = 0;
    int right = length - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        struct NodeGoodLayout* mid_node = get_node_at_good(head, mid);

        if (mid_node->key == target) {
            return mid_node;
        } else if (mid_node->key < target) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return NULL;
}

// Get node at specific position - poor layout
struct NodePoorLayout* get_node_at_poor(struct NodePoorLayout* start, int steps) {
    struct NodePoorLayout* current = start;
    for (int i = 0; i < steps && current; i++) {
        current = current->next;
    }
    return current;
}

// Get node at specific position - good layout
struct NodeGoodLayout* get_node_at_good(struct NodeGoodLayout* start, int steps) {
    struct NodeGoodLayout* current = start;
    for (int i = 0; i < steps && current; i++) {
        current = current->next;
    }
    return current;
}
