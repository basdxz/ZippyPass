/**
 * test_alignment_preservation.c
 *
 * Purpose: Verify field alignment requirements are preserved
 */

// Define a struct with fields requiring specific alignment
typedef struct {
    char c1;              // 1 byte, has no special alignment
    int i1;               // 4 bytes, typically aligned on 4-byte boundary
    double d1;            // 8 bytes, typically aligned on 8-byte boundary
    char c2;              // 1 byte, has no special alignment
    long long l1;         // 8 bytes, typically aligned on 8-byte boundary
    char c3;              // 1 byte, has no special alignment
    float f1;             // 4 bytes, typically aligned on 4-byte boundary
} AlignmentStruct;

// Function to access fields with a specific pattern
// that should trigger reordering
double access_fields(AlignmentStruct* s) {
    double result = 0.0;

    // Access c1, c2, c3 frequently
    for (int i = 0; i < 10; i++) {
        result += s->c1;
        result += s->c2;
        result += s->c3;
    }

    // Access i1 and f1 moderately
    for (int i = 0; i < 5; i++) {
        result += s->i1;
        result += s->f1;
    }

    // Access d1 and l1 rarely
    result += s->d1;
    result += s->l1;

    return result;
}

// Test for misaligned access to fields
int test_alignment(AlignmentStruct* s) {
    // Do some operations that would cause problems if alignment
    // requirements were not preserved
    s->d1 += 1.0;
    s->l1 += 1;

    return 0;  // Return 0 if no crash due to misalignment
}

void biasLastElement(AlignmentStruct* s) {
   s->f1 += s->c1;
   s->f1 += s->i1;
   s->f1 += s->d1;
   s->f1 += s->c2;
   s->f1 += s->l1;
   s->f1 += s->c3;
}

int main() {
    // Initialize structure with known values
    AlignmentStruct data = {'A', 10, 20.0, 'B', 30, 'C', 40.0f};
    
    // Access fields to trigger optimization
    double result = access_fields(&data);

    biasLastElement(&data);
    
    // Test if alignment is preserved after optimization
    int alignment_result = test_alignment(&data);
    
    // The exact result value depends on the field values and operations
    return (result == 375.0 && alignment_result == 0) ? 0 : 1;
}

/* Expected transformation:
The optimizer should reorder fields based on access frequency while
preserving alignment requirements:

typedef struct {
    // Char fields accessed most frequently
    char c1;
    char c2;
    char c3;
    // Padding here (typically 1 byte) to align i1
    
    // Medium frequency fields
    int i1;
    float f1;
    
    // Rarely accessed fields requiring 8-byte alignment
    double d1;
    long long l1;
} AlignmentStruct;

Note: The actual padding may vary by platform, but alignment constraints
for the double and long long fields must be preserved to avoid crashes.
*/
