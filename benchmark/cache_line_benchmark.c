/**
* cache_line_benchmark.c
 *
 * Tests performance improvement from grouping frequently accessed fields
 * together to improve cache locality.
 */

#include <stdlib.h>

// Number of iterations passed by command line
int ITERATIONS = 0;

// Array size for cache testing
#define ARRAY_SIZE 100000

// Structure with fields that will be accessed together
typedef struct {
    int id;
    char padding1[64];  // Forces cache line boundary
    double value;
    char padding2[64];  // Forces cache line boundary
    int count;
} CacheTestStruct;

// Global array for testing
CacheTestStruct data[ARRAY_SIZE];

/**
 * Processes the array by accessing specific fields in a pattern
 * that would benefit from better field ordering
 */
double process_data(CacheTestStruct* array, int size) {
    double sum = 0;

    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (int i = 0; i < size; i++) {
            sum += array[i].id;
            sum += array[i].value;
            sum += array[i].count;
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

    // Initialize with some values
    for (int i = 0; i < ARRAY_SIZE; i++) {
        data[i].id = i;
        data[i].value = i * 1.1;
        data[i].count = ARRAY_SIZE - i;
    }

    // Run the benchmark
    volatile double result = process_data(data, ARRAY_SIZE);

    return 0;
}