/**
 * hotcold_access.c - Benchmark for hot/cold field access patterns
 * 
 * This benchmark creates a structure with fields that have different access frequencies
 * and measures the performance impact of struct field reordering.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Number of iterations to run
#define ITERATIONS 1000

// Structure with mixed hot/cold fields
typedef struct {
    int rarely_used1;          // Cold field
    long frequently_used1;     // Hot field
    int rarely_used2;          // Cold field
    double frequently_used2;   // Hot field
    int rarely_used3;          // Cold field
    float frequently_used3;    // Hot field
    int rarely_used4;          // Cold field
    char frequently_used4;     // Hot field
    int rarely_used5;          // Cold field
} HotColdStruct;

// Function that performs hot/cold access pattern
double access_hot_fields(HotColdStruct* data, int iterations) {
    double sum = 0;
    
    // Access frequently used fields many times
    for (int i = 0; i < iterations; i++) {
        sum += data->frequently_used1;
        sum += data->frequently_used2;
        sum += data->frequently_used3;
        sum += data->frequently_used4;
    }
    
    // Rarely access cold fields to maintain semantics
    if (iterations % 1000000 == 0) {
        sum += data->rarely_used1;
        sum += data->rarely_used2;
        sum += data->rarely_used3;
        sum += data->rarely_used4;
        sum += data->rarely_used5;
    }
    
    return sum;
}

int main() {
    // Initialize struct
    HotColdStruct data = {
        1,          // rarely_used1
        2,          // frequently_used1
        3,          // rarely_used2
        4.0,        // frequently_used2
        5,          // rarely_used3
        6.0f,       // frequently_used3
        7,          // rarely_used4
        8,          // frequently_used4
        9           // rarely_used5
    };
    
    // Run the benchmark
    double result = access_hot_fields(&data, ITERATIONS);
    
    // Print a result to prevent the compiler from optimizing away the calculation
    printf("Result: %.1f\n", result);
    
    return 0;
}
