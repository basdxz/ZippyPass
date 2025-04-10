/**
 * array_traversal_benchmark.c
 *
 * Tests performance improvement when processing arrays of structs
 * with different field access patterns.
 */

#include <stdlib.h>

// Number of iterations passed by command line
int ITERATIONS = 0;

// Size of array to process
#define ARRAY_SIZE 100000

// Structure with mixed field access patterns
typedef struct {
    int id;                // Always accessed
    double payload1;       // Rarely accessed
    float frequently_used; // Frequently accessed
    char tag;              // Rarely accessed
    long payload2;         // Rarely accessed
    int count;             // Frequently accessed
} ArrayElement;

// Global array for testing
ArrayElement elements[ARRAY_SIZE];

/**
 * Process array with specific access pattern
 * Some fields are always accessed, others only occasionally
 */
double process_array() {
    double sum = 0;

    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (int i = 0; i < ARRAY_SIZE; i++) {
            // Always access these fields (hot fields)
            sum += elements[i].id;
            sum += elements[i].frequently_used;
            sum += elements[i].count;

            // Rarely access these fields (cold fields)
            if (i % 100 == 0) {
                sum += elements[i].payload1;
                sum += elements[i].tag;
                sum += elements[i].payload2;
            }
        }
    }

    return sum;
}

/**
 * Main function that initializes data and runs the benchmark
 */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 1;
    }

    ITERATIONS = atoi(argv[1]);
    if (ITERATIONS < 1) {
        return 1;
    }

    // Initialize array with data
    for (int i = 0; i < ARRAY_SIZE; i++) {
        elements[i].id = i;
        elements[i].payload1 = i * 1.1;
        elements[i].frequently_used = i * 2.2f;
        elements[i].tag = i % 128;
        elements[i].payload2 = i * 3;
        elements[i].count = ARRAY_SIZE - i;
    }

    // Run the benchmark
    volatile double result = process_array();

    return 0;
}