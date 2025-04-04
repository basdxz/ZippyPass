/**
 * test_global_struct_references.c
 *
 * Purpose: Verify references to global structs work correctly
 * Adapted from: global_vars.c
 */

typedef struct {
    int rarely_used;           // Cold field
    long frequently_used1;     // Hot field
    int frequently_used2;      // Hot field
    float rarely_used2;        // Cold field
    int frequently_used3;      // Hot field
    float rarely_used3;        // Cold field
} GlobalStruct;

// Global variables with different initialization patterns
GlobalStruct g_initialized = {1, 2, 3, 4.0f, 5, 6.0f};
GlobalStruct g_zero = {0};  // Zero-initialized
GlobalStruct g_uninitialized;  // Uninitialized

// Function that directly modifies global struct fields
void modify_global_direct() {
    // Access hot fields frequently
    for (int i = 0; i < 5; i++) {
        g_initialized.frequently_used1 += i;
        g_initialized.frequently_used2 += i;
        g_initialized.frequently_used3 += i;
    }
    
    // Access cold fields once
    g_initialized.rarely_used = 100;
    g_initialized.rarely_used2 = 200.0f;
    g_initialized.rarely_used3 = 300.0f;
}

// Function that modifies global struct through pointer
void modify_global_pointer(GlobalStruct* gs) {
    // Access hot fields frequently
    for (int i = 0; i < 3; i++) {
        gs->frequently_used1 += 10;
        gs->frequently_used2 += 20;
        gs->frequently_used3 += 30;
    }
    
    // Access cold fields once
    gs->rarely_used = 1000;
}

// Function that copies between global structs
void copy_global_structs() {
    // Copy initialized to zero with field-by-field access
    g_zero.frequently_used1 = g_initialized.frequently_used1;
    g_zero.frequently_used2 = g_initialized.frequently_used2;
    g_zero.frequently_used3 = g_initialized.frequently_used3;
    g_zero.rarely_used = g_initialized.rarely_used;
    g_zero.rarely_used2 = g_initialized.rarely_used2;
    g_zero.rarely_used3 = g_initialized.rarely_used3;
    
    // Then copy zero to uninitialized with assignment
    g_uninitialized = g_zero;
    
    // Modify some fields in uninitialized
    g_uninitialized.frequently_used1 = 9999;
}

// Verify all structures have expected values
int verify_globals() {
    // Check initialized struct
    if (g_initialized.rarely_used != 1000 ||
        g_initialized.frequently_used1 != 40 ||  // 2 + 0+1+2+3+4 + 30
        g_initialized.frequently_used2 != 73 ||  // 3 + 0+1+2+3+4 + 60
        g_initialized.frequently_used3 != 105 || // 5 + 0+1+2+3+4 + 90
        g_initialized.rarely_used2 != 200.0f ||
        g_initialized.rarely_used3 != 300.0f) {
        return 0;
    }
    
    // Check g_zero
    if (g_zero.rarely_used != 1000 ||
        g_zero.frequently_used1 != 40 ||
        g_zero.frequently_used2 != 73 ||
        g_zero.frequently_used3 != 105 ||
        g_zero.rarely_used2 != 200.0f ||
        g_zero.rarely_used3 != 300.0f) {
        return 0;
    }
    
    // Check g_uninitialized (after copying)
    if (g_uninitialized.rarely_used != 1000 ||
        g_uninitialized.frequently_used1 != 9999 || // Specially modified
        g_uninitialized.frequently_used2 != 73 ||
        g_uninitialized.frequently_used3 != 105 ||
        g_uninitialized.rarely_used2 != 200.0f ||
        g_uninitialized.rarely_used3 != 300.0f) {
        return 0;
    }
    
    return 1; // All checks passed
}

int main() {
    // Perform operations on global structs
    modify_global_direct();
    modify_global_pointer(&g_initialized);
    copy_global_structs();
    
    // Verify all globals have expected values
    return verify_globals() ? 0 : 1;
}

/* Expected transformation:
typedef struct {
    // Hot fields should be grouped together
    long frequently_used1;
    int frequently_used2;
    int frequently_used3;
    
    // Cold fields should be grouped together
    int rarely_used;
    float rarely_used2;
    float rarely_used3;
} GlobalStruct;

The initializers should also be reordered accordingly:
GlobalStruct g_initialized = {2, 3, 5, 1, 4.0f, 6.0f};
*/
