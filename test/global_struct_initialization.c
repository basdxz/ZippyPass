/**
 * test_global_struct_initialization.c
 *
 * Purpose: Verify global struct initializers are correctly transformed
 * Adapted from: global_vars.c
 */

typedef struct {
    int rarely_used1;       // Cold field
    long frequently_used1;  // Hot field
    int frequently_used2;   // Hot field
    float rarely_used2;     // Cold field
    int frequently_used3;   // Hot field
    float rarely_used3;     // Cold field
} TestStruct;

// Global structs with initializers
const TestStruct g_constVar = {10, 20, 30, 40.0f, 50, 60.0f};
TestStruct g_regularVar = {100, 200, 300, 400.0f, 500, 600.0f};

// Access hot fields frequently to trigger reordering
double get_hot_field_sum() {
    double sum = 0;
    
    // Access hot fields multiple times
    for (int i = 0; i < 10; i++) {
        sum += g_constVar.frequently_used1;
        sum += g_constVar.frequently_used2;
        sum += g_constVar.frequently_used3;
        
        sum += g_regularVar.frequently_used1;
        sum += g_regularVar.frequently_used2;
        sum += g_regularVar.frequently_used3;
    }
    
    return sum;
}

// Access all fields once to verify correctness
double get_all_field_sum() {
    double sum = 0;
    
    // Sum all fields from constant global
    sum += g_constVar.rarely_used1;
    sum += g_constVar.frequently_used1;
    sum += g_constVar.frequently_used2;
    sum += g_constVar.rarely_used2;
    sum += g_constVar.frequently_used3;
    sum += g_constVar.rarely_used3;
    
    // Sum all fields from regular global
    sum += g_regularVar.rarely_used1;
    sum += g_regularVar.frequently_used1;
    sum += g_regularVar.frequently_used2;
    sum += g_regularVar.rarely_used2;
    sum += g_regularVar.frequently_used3;
    sum += g_regularVar.rarely_used3;
    
    return sum;
}

int main() {
    double hot_sum = get_hot_field_sum();
    double all_sum = get_all_field_sum();
    
    // This will only be true if all field values were preserved correctly
    // after struct field reordering
    return (hot_sum == 11000 && all_sum == 2310) ? 0 : 1;
}

/* Expected transformation:
typedef struct {
    long frequently_used1;
    int frequently_used2;
    int frequently_used3;
    int rarely_used1;
    float rarely_used2;
    float rarely_used3;
} TestStruct;

The initializer values must be correspondingly reordered:
const TestStruct g_constVar = {20, 30, 50, 10, 40.0f, 60.0f};
TestStruct g_regularVar = {200, 300, 500, 100, 400.0f, 600.0f};
*/
