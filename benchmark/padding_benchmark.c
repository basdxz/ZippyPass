/**
* padding_benchmark.c
 *
 * Tests performance improvement from reordering fields by size
 * to minimize padding between fields.
 */

#include <stdlib.h>

// Number of iterations passed by command line
int ITERATIONS = 0;

// Array size for testing
#define ARRAY_SIZE 50000

// Structure with interleaved field sizes causing padding
typedef struct {
    char a;        // 1 byte + 7 bytes padding
    double b;      // 8 bytes
    char c;        // 1 byte + 7 bytes padding
    double d;      // 8 bytes
    char e;        // 1 byte + 7 bytes padding
    double f;      // 8 bytes
} PaddingStruct;  // Expected size: 48 bytes with padding

// Global array for testing
PaddingStruct data[ARRAY_SIZE];

/**
 * Processes the array by accessing all fields in order
 */
double process_data(PaddingStruct* array, int size) {
    double sum = 0;

    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (int i = 0; i < size; i++) {
            sum += array[i].a;
            sum += array[i].b;
            sum += array[i].c;
            sum += array[i].d;
            sum += array[i].e;
            sum += array[i].f;
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
        data[i].a = i % 128;
        data[i].b = i * 1.1;
        data[i].c = (i + 10) % 128;
        data[i].d = i * 2.2;
        data[i].e = (i + 20) % 128;
        data[i].f = i * 3.3;
    }

    // Run the benchmark
    volatile double result = process_data(data, ARRAY_SIZE);

    return 0;
}