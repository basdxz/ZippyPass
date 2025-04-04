#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

// Poor layout - mixing types causes padding
struct BadLayout {
    char a;        // 1 byte + 7 bytes padding
    double b;      // 8 bytes
    char c;        // 1 byte + 7 bytes padding
    double d;      // 8 bytes
    char e;        // 1 byte + 7 bytes padding
    double f;      // 8 bytes
};  // Total: 48 bytes

// Better layout - grouping by size
struct GoodLayout {
    double b;      // 8 bytes
    double d;      // 8 bytes
    double f;      // 8 bytes
    char a;        // 1 byte
    char c;        // 1 byte
    char e;        // 1 byte
    // 5 bytes padding
};  // Total: 32 bytes

#define ARRAY_SIZE 5000000
#define ITERATIONS 200

// Function to prevent compiler from optimizing away our calculations
volatile double sink;

double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

int main() {
    double start, end;
    double elapsed_bad, elapsed_good;

    printf("Struct sizes - Bad: %zu bytes, Good: %zu bytes\n",
           sizeof(struct BadLayout), sizeof(struct GoodLayout));

    // Allocate arrays
    struct BadLayout *bad_array = malloc(ARRAY_SIZE * sizeof(struct BadLayout));
    struct GoodLayout *good_array = malloc(ARRAY_SIZE * sizeof(struct GoodLayout));

    if (!bad_array || !good_array) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    // Initialize with same values
    for (int i = 0; i < ARRAY_SIZE; i++) {
        bad_array[i].a = good_array[i].a = (char)(i & 0xFF);
        bad_array[i].b = good_array[i].b = (double)i;
        bad_array[i].c = good_array[i].c = (char)((i >> 8) & 0xFF);
        bad_array[i].d = good_array[i].d = (double)(i * 2);
        bad_array[i].e = good_array[i].e = (char)((i >> 16) & 0xFF);
        bad_array[i].f = good_array[i].f = (double)(i * 3);
    }

    // Warmup to eliminate cache effects
    double warmup_sum = 0;
    for (int i = 0; i < ARRAY_SIZE / 10; i++) {
        warmup_sum += bad_array[i].b + bad_array[i].d + bad_array[i].f;
        warmup_sum += good_array[i].b + good_array[i].d + good_array[i].f;
    }
    sink = warmup_sum;

    // Benchmark bad layout
    double sum_bad = 0;
    start = get_time_ms();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (int i = 0; i < ARRAY_SIZE; i++) {
            sum_bad += bad_array[i].b + bad_array[i].d + bad_array[i].f +
            (double)(bad_array[i].a + bad_array[i].c + bad_array[i].e);
        }
    }
    end = get_time_ms();
    elapsed_bad = end - start;

    // Benchmark good layout
    double sum_good = 0;
    start = get_time_ms();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (int i = 0; i < ARRAY_SIZE; i++) {
            sum_good += good_array[i].b + good_array[i].d + good_array[i].f +
            (double)(good_array[i].a + good_array[i].c + good_array[i].e);
        }
    }
    end = get_time_ms();
    elapsed_good = end - start;

    // Ensure our calculations aren't optimized away
    sink = sum_bad;
    sink = sum_good;

    printf("Bad layout time: %.3f ms\n", elapsed_bad);
    printf("Good layout time: %.3f ms\n", elapsed_good);
    printf("Performance improvement: %.2f%%\n",
           (elapsed_bad - elapsed_good) / elapsed_bad * 100);

    free(bad_array);
    free(good_array);
    return 0;
}
