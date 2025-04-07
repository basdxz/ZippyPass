/**
 * array_traverse.c - Benchmark for cache locality effects with arrays of structs
 * 
 * This benchmark tests how struct field reordering affects cache locality
 * when traversing arrays of structs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Size of the array to process
#define ARRAY_SIZE 10000
// Number of iterations to loop through the array
#define ITERATIONS 10

// Structure with fields that have different access patterns
typedef struct {
    int id;                 // Always accessed
    double rarely_used1;    // Rarely accessed
    double rarely_used2;    // Rarely accessed
    float frequently_used;  // Frequently accessed
    char rarely_used3;      // Rarely accessed
    char rarely_used4;      // Rarely accessed
    int frequently_used2;   // Frequently accessed
} DataRecord;

// Process array with specific access pattern
double process_array(DataRecord* array, int size, int iterations) {
    double sum = 0;
    
    for (int iter = 0; iter < iterations; iter++) {
        // Scan through array, accessing fields with different frequencies
        for (int i = 0; i < size; i++) {
            // Always access these fields
            sum += array[i].id;
            sum += array[i].frequently_used;
            sum += array[i].frequently_used2;
            
            // Rarely access these fields
            if (iter % 100 == 0 && i % 1000 == 0) {
                sum += array[i].rarely_used1;
                sum += array[i].rarely_used2;
                sum += array[i].rarely_used3;
                sum += array[i].rarely_used4;
            }
        }
    }
    
    return sum;
}

int main() {
    // Allocate and initialize array
    DataRecord* records = (DataRecord*)malloc(ARRAY_SIZE * sizeof(DataRecord));
    if (!records) {
        printf("Memory allocation failed\n");
        return 1;
    }
    
    // Initialize records with data
    for (int i = 0; i < ARRAY_SIZE; i++) {
        records[i].id = i;
        records[i].rarely_used1 = i * 1.1;
        records[i].rarely_used2 = i * 2.2;
        records[i].frequently_used = i * 3.3f;
        records[i].rarely_used3 = i % 128;
        records[i].rarely_used4 = (i * 2) % 128;
        records[i].frequently_used2 = i * 4;
    }
    
    // Process the array
    double result = process_array(records, ARRAY_SIZE, ITERATIONS);
    
    // Clean up
    free(records);
    
    // Print a result to prevent optimization
    printf("Result: %.1f\n", result);
    
    return 0;
}
