/**
 * loop_access_benchmark.c
 *
 * Tests performance improvement from reordering fields based on
 * access frequency patterns in nested loops.
 */

#include <stdlib.h>

// Number of iterations passed by command line
int ITERATIONS = 0;

// Test size
#define DATA_SIZE 10000
#define INNER_LOOP 50

// Structure with fields accessed at different frequencies
typedef struct {
    int rarely_used1;      // Cold field
    double hot_field1;     // Hot field
    int rarely_used2;      // Cold field
    double hot_field2;     // Hot field
    int rarely_used3;      // Cold field
    double hot_field3;     // Hot field
} MixedAccessStruct;

// Global array for testing
MixedAccessStruct data[DATA_SIZE];

/**
 * Process data with hot/cold access pattern
 */
double process_data() {
    double sum = 0;

    // Outer iteration loop (benchmark scaling)
    for (int iter = 0; iter < ITERATIONS; iter++) {

        // Data processing loop
        for (int i = 0; i < DATA_SIZE; i++) {
            // Frequently access hot fields in tight inner loop
            for (int j = 0; j < INNER_LOOP; j++) {
                sum += data[i].hot_field1;
                sum += data[i].hot_field2;
                sum += data[i].hot_field3;
            }

            // Rarely access cold fields (only once per data item)
            sum += data[i].rarely_used1;
            sum += data[i].rarely_used2;
            sum += data[i].rarely_used3;
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
    for (int i = 0; i < DATA_SIZE; i++) {
        data[i].rarely_used1 = i % 100;
        data[i].hot_field1 = i * 1.1;
        data[i].rarely_used2 = (i + 100) % 100;
        data[i].hot_field2 = i * 2.2;
        data[i].rarely_used3 = (i + 200) % 100;
        data[i].hot_field3 = i * 3.3;
    }

    // Run the benchmark
    volatile double result = process_data();

    return 0;
}