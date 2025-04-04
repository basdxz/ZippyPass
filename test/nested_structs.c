/**
 * test_nested_structs.c
 *
 * Purpose: Verify handling of nested struct types
 * Adapted from: shapes.c
 */

// Inner struct with its own field access patterns
struct Inner {
    int rarely_used;
    float frequently_used;  // Accessed frequently
    char very_rarely_used;
};

// Outer struct with nested structs
struct Outer {
    struct Inner first;          // Accessed frequently
    int rarely_accessed;
    struct Inner second;         // Accessed rarely
    float frequently_accessed;   // Accessed frequently
};

// Function to access inner struct fields frequently
int access_inner(struct Inner* inner) {
    float sum = 0;
    
    // Access frequently_used multiple times
    for (int i = 0; i < 10; i++) {
        sum += inner->frequently_used;
    }
    
    // Access rarely_used once
    sum += inner->rarely_used;
    
    // Very rarely used not accessed
    
    return (int)sum;
}

// Function to access outer struct fields with specific pattern
int process_outer(struct Outer* outer) {
    float sum = 0;
    
    // Access first inner struct frequently
    for (int i = 0; i < 5; i++) {
        sum += access_inner(&outer->first);
    }
    
    // Access frequently_accessed multiple times
    for (int i = 0; i < 10; i++) {
        sum += outer->frequently_accessed;
    }
    
    // Access second inner struct and rarely_accessed only once
    sum += access_inner(&outer->second);
    sum += outer->rarely_accessed;
    
    return (int)sum;
}

int main() {
    struct Inner inner1 = {1, 2.0f, 'A'};
    struct Inner inner2 = {3, 4.0f, 'B'};
    struct Outer outer = {inner1, 5, inner2, 6.0f};
    
    int result = process_outer(&outer);
    
    // Verify the result is correct
    // Inner structs should have their fields reordered
    // Outer struct should have frequently accessed fields first
    return (result == 175) ? 0 : 1;
}

/* Expected transformations:

struct Inner {
    float frequently_used;  // Moved to beginning due to frequency
    int rarely_used;
    char very_rarely_used;
};

struct Outer {
    struct Inner first;          // Moved up due to frequency
    float frequently_accessed;   // Moved up due to frequency
    int rarely_accessed;
    struct Inner second;
};
*/
