#include <stdlib.h>

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
//struct GoodLayout {
//    double b;      // 8 bytes
//    double d;      // 8 bytes
//    double f;      // 8 bytes
//    char a;        // 1 byte
//    char c;        // 1 byte
//    char e;        // 1 byte
//    // 5 bytes padding
//};  // Total: 32 bytes

#define ARRAY_SIZE 5000000
#define ITERATIONS 200

// Function to prevent compiler from optimizing away our calculations
volatile double sink;

int main() {
    // Allocate arrays
    struct BadLayout *bad_array = malloc(ARRAY_SIZE * sizeof(struct BadLayout));

    if (!bad_array) {
        return 1;
    }

    // Initialize with same values
    for (int i = 0; i < ARRAY_SIZE; i++) {
        bad_array[i].a = (char)(i & 0xFF);
        bad_array[i].b = (double)i;
        bad_array[i].c = (char)((i >> 8) & 0xFF);
        bad_array[i].d = (double)(i * 2);
        bad_array[i].e = (char)((i >> 16) & 0xFF);
        bad_array[i].f = (double)(i * 3);
    }

    // Benchmark bad layout
    double sum_bad = 0;
    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (int i = 0; i < ARRAY_SIZE; i++) {
            sum_bad += bad_array[i].b + bad_array[i].d + bad_array[i].f +
            (double)(bad_array[i].a + bad_array[i].c + bad_array[i].e);
        }
    }


    // Ensure our calculations aren't optimized away
    sink = sum_bad;


    free(bad_array);
    return 0;
}
