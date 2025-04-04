/**
 * test/field_grouping_test.c
 *
 * This test demonstrates various field access patterns that should trigger
 * field grouping optimizations:
 *
 * 1. Co-accessed fields (fields frequently accessed together)
 * 2. Fields accessed in loops (hot fields)
 * 3. Fields with different access frequencies
 * 4. Access patterns across multiple functions
 * 5. Fields that should be grouped based on type size for better padding
 */

#include <stdio.h>

// Original sub-optimal struct layout with mixed field sizes and
// intentionally poor grouping of related fields
typedef struct {
    int a;           // Group 1: Frequently accessed together with c and e
    double b;        // Group 2: Frequently accessed together with d and f
    int c;           // Group 1: Frequently accessed together with a and e
    double d;        // Group 2: Frequently accessed together with b and f
    int e;           // Group 1: Frequently accessed together with a and c
    double f;        // Group 2: Frequently accessed together with b and d
    char g;          // Group 3: Rarely accessed fields
    char h;          // Group 3: Rarely accessed fields
    long i;          // Group 3: Rarely accessed fields
} TestStruct;

// Function that accesses fields in Group 1 together
void process_group1(TestStruct* ts) {
    // These fields should be grouped together after optimization
    ts->a += 10;
    ts->c += 20;
    ts->e += 30;
}

// Function that accesses fields in Group 2 together
void process_group2(TestStruct* ts) {
    // These fields should be grouped together after optimization
    ts->b *= 1.5;
    ts->d *= 2.0;
    ts->f *= 2.5;
}

// Function that accesses Group 1 fields in a hot loop
void hot_access_group1(TestStruct* ts, int iterations) {
    // This should increase the priority of Group 1 fields
    for (int i = 0; i < iterations; i++) {
        ts->a += i;
        ts->c += i * 2;
        ts->e += i * 3;
    }
}

// Function that accesses Group 2 fields in a hot loop
void hot_access_group2(TestStruct* ts, int iterations) {
    // This should increase the priority of Group 2 fields
    for (int i = 0; i < iterations; i++) {
        ts->b += i * 0.1;
        ts->d += i * 0.2;
        ts->f += i * 0.3;
    }
}

// Function that rarely accesses Group 3 fields
void rare_access_group3(TestStruct* ts) {
    // These fields should be grouped together but with lower priority
    ts->g = 'X';
    ts->h = 'Y';
    ts->i = 999;
}

// Function demonstrating interleaved access patterns
void mixed_access(TestStruct* ts) {
    // This still shows the grouping pattern despite being interleaved
    ts->a += 1;
    ts->b *= 1.1;
    ts->c += 2;
    ts->d *= 1.2;
    ts->e += 3;
    ts->f *= 1.3;
}

// Function demonstrating partial group access
void partial_group_access(TestStruct* ts) {
    // Only accesses part of Group 1
    ts->a += 5;
    ts->c += 10;

    // Only accesses part of Group 2
    ts->d *= 1.5;
    ts->f *= 2.0;
}

// Complex function with multiple access patterns
void complex_processing(TestStruct* ts, int count) {
    // Initialize with Group 3 (rare)
    ts->g = 'A';
    ts->h = 'B';
    ts->i = 100;

    // Process Group 1 (hot)
    for (int i = 0; i < count; i++) {
        ts->a += i;
        ts->c += i;
        ts->e += i;

        // Occasionally access Group 2
        if (i % 10 == 0) {
            ts->b *= 1.01;
            ts->d *= 1.01;
            ts->f *= 1.01;
        }
    }

    // Final updates using Group 1 and 2 together
    ts->a = ts->a + ts->c + ts->e;
    ts->b = ts->b + ts->d + ts->f;
}

int main() {
    TestStruct data;
    // TODO: Will cause stack smashing due to size mismatch, need to handle: https://llvm.org/docs/LangRef.html#llvm-memset-intrinsics
//    TestStruct data = {0};

    // Initialize
    data.a = 1;
    data.b = 2.0;
    data.c = 3;
    data.d = 4.0;
    data.e = 5;
    data.f = 6.0;
    data.g = '7';
    data.h = '8';
    data.i = 9;

    // Call various functions to create access patterns
    process_group1(&data);
    process_group2(&data);
    hot_access_group1(&data, 100);
    hot_access_group2(&data, 50);
    rare_access_group3(&data);
    mixed_access(&data);
    partial_group_access(&data);
    complex_processing(&data, 200);

    // Print results to prevent compiler from optimizing away our operations
    printf("Final values: a=%d, b=%.2f, c=%d, d=%.2f, e=%d, f=%.2f, g=%c, h=%c, i=%ld\n",
           data.a, data.b, data.c, data.d, data.e, data.f, data.g, data.h, data.i);

    return 0;
}