/**
 * padding_effects.c - Benchmark for measuring the impact of reduced padding
 * 
 * This benchmark tests how struct field reordering can reduce padding and
 * improve memory usage and access time.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Number of instances to create
#define NUM_INSTANCES 1000000
// Number of iterations to process
#define ITERATIONS 100

// Structure with inefficient field ordering that causes padding
typedef struct {
    char a;        // 1 byte + 7 bytes padding
    double b;      // 8 bytes
    char c;        // 1 byte + 7 bytes padding
    double d;      // 8 bytes
    char e;        // 1 byte + 7 bytes padding
    double f;      // 8 bytes
    char g;        // 1 byte + 7 bytes padding
    double h;      // 8 bytes
} PaddingStruct;   // Total expected to be ~64 bytes with padding

// Function to process an array of structs
double process_structs(PaddingStruct* array, int size, int iterations) {
    double sum = 0;
    
    for (int iter = 0; iter < iterations; iter++) {
        for (int i = 0; i < size; i++) {
            // Access all fields to ensure they are loaded into cache
            sum += array[i].a;
            sum += array[i].b;
            sum += array[i].c;
            sum += array[i].d;
            sum += array[i].e;
            sum += array[i].f;
            sum += array[i].g;
            sum += array[i].h;
        }
    }
    
    return sum;
}

int main() {
    // Print actual size of struct to see padding effect
    printf("Size of PaddingStruct: %zu bytes\n", sizeof(PaddingStruct));
    
    // Allocate memory for array of structs
    PaddingStruct* structs = (PaddingStruct*)malloc(NUM_INSTANCES * sizeof(PaddingStruct));
    if (!structs) {
        printf("Memory allocation failed\n");
        return 1;
    }
    
    // Initialize with values
    for (int i = 0; i < NUM_INSTANCES; i++) {
        structs[i].a = i % 128;
        structs[i].b = i * 1.1;
        structs[i].c = (i + 10) % 128;
        structs[i].d = i * 2.2;
        structs[i].e = (i + 20) % 128;
        structs[i].f = i * 3.3;
        structs[i].g = (i + 30) % 128;
        structs[i].h = i * 4.4;
    }
    
    // Process the structs
    double result = process_structs(structs, NUM_INSTANCES, ITERATIONS);
    
    // Clean up
    free(structs);
    
    // Print result to prevent compiler from optimizing away
    printf("Result: %.1f\n", result);
    
    return 0;
}
