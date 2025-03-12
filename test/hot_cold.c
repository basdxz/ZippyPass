#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#define RECORDS 200000
#define ITERATIONS 1000

// Poor layout - hot/cold fields mixed together
struct CustomerGoodLayout {
    int customer_id;         // Frequently accessed (hot)
    char name[64];           // Rarely accessed (cold)
    float account_balance;   // Frequently accessed (hot)
    char address[128];       // Rarely accessed (cold)
    int transactions_count;  // Frequently accessed (hot)
    char notes[256];         // Rarely accessed (cold)
};

// Better layout - hot fields grouped together
struct CustomerPoorLayout {
    // Hot fields cluster
    int customer_id;
    float account_balance;
    int transactions_count;

    // Cold fields cluster
    char name[64];
    char address[128];
    char notes[256];
};

// Function to prevent compiler from optimizing away our calculations
volatile float sink_float;
volatile int sink_int;

double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

int main() {
    double start, end;
    double elapsed_poor, elapsed_good;

    printf("Struct sizes - Poor: %zu bytes, Good: %zu bytes\n",
           sizeof(struct CustomerPoorLayout), sizeof(struct CustomerGoodLayout));

    // Allocate arrays
    struct CustomerPoorLayout *customers_poor =
    malloc(RECORDS * sizeof(struct CustomerPoorLayout));
    struct CustomerGoodLayout *customers_good =
    malloc(RECORDS * sizeof(struct CustomerGoodLayout));

    if (!customers_poor || !customers_good) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    // Seed random number generator
    srand(42);

    customers_poor[0].account_balance = customers_good[0].account_balance = (float)(rand() % 10000) / 100.0f;

    // Initialize with test data
    for (int i = 0; i < RECORDS; i++) {
//        customers_poor[i].customer_id = customers_good[i].customer_id = i;
//        customers_poor[i].account_balance = customers_good[i].account_balance = (float)(rand() % 10000) / 100.0f;
//        customers_poor[i].transactions_count = customers_good[i].transactions_count = rand() % 500;

        // Fill name, address and notes with some data to ensure accurate memory layout
        for (int j = 0; j < 63; j++) {
          char z = 'A' + (j % 26);
            customers_poor[i].name[j] = z;
            customers_good[i].name[j] = z;
        }
//        customers_poor[i].name[63] = customers_good[i].name[63] = '\0';
//
//        for (int j = 0; j < 127; j++) {
//            customers_poor[i].address[j] = customers_good[i].address[j] = 'B' + (j % 26);
//        }
//        customers_poor[i].address[127] = customers_good[i].address[127] = '\0';
//
//        for (int j = 0; j < 255; j++) {
//            customers_poor[i].notes[j] = customers_good[i].notes[j] = 'C' + (j % 26);
//        }
//        customers_poor[i].notes[255] = customers_good[i].notes[255] = '\0';
    }

    // Warmup
    float warmup_balance = 0;
    int warmup_transactions = 0;
//    for (int i = 0; i < RECORDS / 10; i++) {
//        if (customers_poor[i].customer_id % 2 == 0) {
//            warmup_balance += customers_poor[i].account_balance;
//            warmup_transactions += customers_poor[i].transactions_count;
//        }
//        if (customers_good[i].customer_id % 2 == 0) {
//            warmup_balance += customers_good[i].account_balance;
//            warmup_transactions += customers_good[i].transactions_count;
//        }
//    }
    sink_float = warmup_balance;
    sink_int = warmup_transactions;

    // Benchmark poor layout - accessing hot fields
    float total_balance = 0.0f;
    int total_transactions = 0;

    start = get_time_ms();
//    for (int iter = 0; iter < ITERATIONS; iter++) {
//        for (int i = 0; i < RECORDS; i++) {
//            if (customers_poor[i].customer_id % 2 == 0) {
//                total_balance += customers_poor[i].account_balance;
//                total_transactions += customers_poor[i].transactions_count;
//            }
//        }
//    }
    end = get_time_ms();
    elapsed_poor = end - start;

    // Prevent optimization
    sink_float = total_balance;
    sink_int = total_transactions;

    // Benchmark good layout - accessing hot fields
    float total_balance2 = 0.0f;
    int total_transactions2 = 0;

    start = get_time_ms();
//    for (int iter = 0; iter < ITERATIONS; iter++) {
//        for (int i = 0; i < RECORDS; i++) {
//            if (customers_good[i].customer_id % 2 == 0) {
//                total_balance2 += customers_good[i].account_balance;
//                total_transactions2 += customers_good[i].transactions_count;
//            }
//        }
//    }
    end = get_time_ms();
    elapsed_good = end - start;

    // Prevent optimization
    sink_float = total_balance2;
    sink_int = total_transactions2;

    printf("Poor layout time: %.3f ms\n", elapsed_poor);
    printf("Good layout time: %.3f ms\n", elapsed_good);
    printf("Performance improvement: %.2f%%\n",
           (elapsed_poor - elapsed_good) / elapsed_poor * 100);

    free(customers_poor);
    free(customers_good);
    return 0;
}
