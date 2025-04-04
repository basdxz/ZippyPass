/**
 * test_cross_function_structs.c
 *
 * Purpose: Verify structs passed between functions
 * Adapted from: field_grouping_test.c
 */

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
} CrossFunctionStruct;

// Function that accesses fields in Group 1
void process_group1(CrossFunctionStruct* ts) {
    // These fields should be grouped together after optimization
    ts->a += 10;
    ts->c += 20;
    ts->e += 30;
}

// Function that accesses fields in Group 2
void process_group2(CrossFunctionStruct* ts) {
    // These fields should be grouped together after optimization
    ts->b *= 1.5;
    ts->d *= 2.0;
    ts->f *= 2.5;
}

// Function that accesses Group 1 fields in a hot loop
void hot_access_group1(CrossFunctionStruct* ts, int iterations) {
    // This should increase the priority of Group 1 fields
    for (int i = 0; i < iterations; i++) {
        ts->a += i;
        ts->c += i * 2;
        ts->e += i * 3;
    }
}

// Function that rarely accesses Group 3 fields
void rare_access_group3(CrossFunctionStruct* ts) {
    // These fields should be grouped together but with lower priority
    ts->g = 'X';
    ts->h = 'Y';
    ts->i = 999;
}

// Function demonstrating partial group access
void partial_group_access(CrossFunctionStruct* ts) {
    // Only accesses part of Group 1
    ts->a += 5;
    ts->c += 10;

    // Only accesses part of Group 2
    ts->d *= 1.5;
    ts->f *= 2.0;
}

int verify_results(CrossFunctionStruct* ts) {
    // Expected final values based on the operations performed
    return (ts->a == 61 && 
           ts->b == 3.0 && 
           ts->c == 135 && 
           ts->d == 6.0 && 
           ts->e == 209 && 
           ts->f == 30.0 && 
           ts->g == 'X' && 
           ts->h == 'Y' && 
           ts->i == 999);
}

int main() {
    // Initialize with known values
    CrossFunctionStruct data = {1, 2.0, 3, 2.0, 4, 6.0, 'A', 'B', 100};
    
    // Call various functions to create access patterns
    process_group1(&data);
    process_group2(&data);
    hot_access_group1(&data, 5);
    rare_access_group3(&data);
    partial_group_access(&data);
    
    // Check if all operations produced expected results
    return verify_results(&data) ? 0 : 1;
}

/* Expected transformation based on access patterns:
The optimizer should identify three groups of fields with different access patterns:

typedef struct {
    // Group 1: Most frequently accessed (accessed in hot loops)
    int a;
    int c;
    int e;
    
    // Group 2: Frequently accessed
    double b;
    double d;
    double f;
    
    // Group 3: Rarely accessed
    char g;
    char h;
    long i;
} CrossFunctionStruct;
*/
