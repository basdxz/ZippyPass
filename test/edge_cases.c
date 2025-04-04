/**
 * test_edge_cases.c
 *
 * Purpose: Verify behavior with unusual struct patterns
 * Tests various edge cases for struct field reordering
 */

// Empty struct (should be left alone)
typedef struct {
    // Empty
} EmptyStruct;

// Single-field struct (no reordering possible)
typedef struct {
    int only_field;
} SingleFieldStruct;

// Struct with only pointer fields
typedef struct {
    void* ptr1;      // Accessed frequently
    char* ptr2;      // Rarely accessed
    int*  ptr3;      // Medium access
} PointerStruct;

// Very large struct with many fields
typedef struct {
    int field1;      // Frequently accessed
    int field2;      // Rarely accessed
    int field3;      // Frequently accessed
    int field4;      // Rarely accessed
    int field5;      // Frequently accessed
    int field6;      // Rarely accessed
    int field7;      // Frequently accessed
    int field8;      // Rarely accessed
    int field9;      // Frequently accessed
    int field10;     // Rarely accessed
    int field11;     // Frequently accessed
    int field12;     // Rarely accessed
    int field13;     // Frequently accessed
    int field14;     // Rarely accessed
    int field15;     // Frequently accessed
    int field16;     // Rarely accessed
    int field17;     // Frequently accessed
    int field18;     // Rarely accessed
    int field19;     // Frequently accessed
    int field20;     // Rarely accessed
} LargeStruct;

// Access functions for each type

void access_empty(EmptyStruct* s) {
    // Nothing to access
}

int access_single(SingleFieldStruct* s) {
    // Access the only field multiple times
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        sum += s->only_field;
    }
    return sum;
}

int access_pointers(PointerStruct* s, int* values) {
    int sum = 0;
    
    // Access ptr1 frequently
    for (int i = 0; i < 10; i++) {
        sum += *(int*)s->ptr1;
    }
    
    // Access ptr3 a few times
    for (int i = 0; i < 3; i++) {
        sum += s->ptr3[i];
    }
    
    // Access ptr2 once
    sum += *s->ptr2;
    
    return sum;
}

int access_large(LargeStruct* s) {
    int sum = 0;
    
    // Access odd-numbered fields frequently
    for (int i = 0; i < 5; i++) {
        sum += s->field1;
        sum += s->field3;
        sum += s->field5;
        sum += s->field7;
        sum += s->field9;
        sum += s->field11;
        sum += s->field13;
        sum += s->field15;
        sum += s->field17;
        sum += s->field19;
    }
    
    // Access even-numbered fields rarely
    sum += s->field2;
    sum += s->field4;
    sum += s->field6;
    sum += s->field8;
    sum += s->field10;
    sum += s->field12;
    sum += s->field14;
    sum += s->field16;
    sum += s->field18;
    sum += s->field20;
    
    return sum;
}

int main() {
    /* Expected transformations:
    
    EmptyStruct - no change possible
    
    SingleFieldStruct - no change possible
    
    PointerStruct should be reordered to:
    typedef struct {
        void* ptr1;      // Most frequent
        int*  ptr3;      // Medium access
        char* ptr2;      // Least frequent
    } PointerStruct;
    
    LargeStruct should group all odd-numbered fields first:
    typedef struct {
        int field1;      // Frequently accessed
        int field3;      // Frequently accessed
        int field5;      // Frequently accessed
        int field7;      // Frequently accessed
        int field9;      // Frequently accessed
        int field11;     // Frequently accessed
        int field13;     // Frequently accessed
        int field15;     // Frequently accessed
        int field17;     // Frequently accessed
        int field19;     // Frequently accessed
        
        int field2;      // Rarely accessed
        int field4;      // Rarely accessed
        int field6;      // Rarely accessed
        int field8;      // Rarely accessed
        int field10;     // Rarely accessed
        int field12;     // Rarely accessed
        int field14;     // Rarely accessed
        int field16;     // Rarely accessed
        int field18;     // Rarely accessed
        int field20;     // Rarely accessed
    } LargeStruct;
    */
    // Test empty struct
    EmptyStruct empty;
    access_empty(&empty);
    
    // Test single field struct
    SingleFieldStruct single = {42};
    int single_result = access_single(&single);

    // Test pointer struct
    int values[3] = {10, 20, 30};
    char c_value = 'A';
    int i_value = 100;
    PointerStruct ptrs = {&i_value, &c_value, values};
    int ptr_result = access_pointers(&ptrs, values);

    // Test large struct
    LargeStruct large = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20
    };
    int large_result = access_large(&large);

    // Verify all accesses worked correctly
    // These particular values depend on the exact implementation
    return (single_result == 210 && ptr_result == 1165 && large_result == 1050) ? 0 : 1;
}