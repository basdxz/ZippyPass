/**
 * test_struct_arrays.c
 *
 * Purpose: Verify arrays of structs are handled correctly
 * Adapted from: linked_list_search.c
 */

// Struct with mixed field access frequency
typedef struct {
    int key;            // Frequently accessed
    double payload;     // Rarely accessed
    float metadata;     // Medium frequency access
    char tag;           // Rarely accessed
} DataRecord;

#define ARRAY_SIZE 10

// Process array with specific access pattern
int process_array(DataRecord arr[], int size) {
    int sum = 0;
    
    // Scan through array, accessing fields with different frequencies
    for (int i = 0; i < size; i++) {
        // Access key in every iteration (highest frequency)
        sum += arr[i].key;
        
        // Access metadata every other iteration (medium frequency)
        if (i % 2 == 0) {
            sum += (int)arr[i].metadata;
        }
        
        // Access payload and tag rarely
        if (i == size / 2) {
            sum += (int)arr[i].payload;
            sum += arr[i].tag;
        }
    }
    
    return sum;
}

// Binary search using key field
int binary_search(DataRecord arr[], int size, int target) {
    int left = 0;
    int right = size - 1;
    
    while (left <= right) {
        int mid = left + (right - left) / 2;
        
        // Access key field for comparison (high frequency)
        if (arr[mid].key == target) {
            return mid;
        } else if (arr[mid].key < target) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    
    return -1;
}

int main() {
    // Initialize array of structs with increasing keys
    DataRecord records[ARRAY_SIZE];
    for (int i = 0; i < ARRAY_SIZE; i++) {
        records[i].key = i * 10;
        records[i].payload = i * 100.0;
        records[i].metadata = i * 1.5f;
        records[i].tag = 'A' + i;
    }
    
    // Process the array
    int process_result = process_array(records, ARRAY_SIZE);
    
    // Perform some searches
    int search1 = binary_search(records, ARRAY_SIZE, 30);
    int search2 = binary_search(records, ARRAY_SIZE, 70);
    int search3 = binary_search(records, ARRAY_SIZE, 15);
    
    // Verify expected results
    return (process_result == 489 && search1 == 3 && search2 == 7 && search3 == -1) ? 0 : 1;
}

/* Expected transformation:
Since 'key' is accessed most frequently, followed by 'metadata', 
the fields should be reordered to:

typedef struct {
    int key;            // Most frequently accessed
    float metadata;     // Medium frequency access
    double payload;     // Rarely accessed
    char tag;           // Rarely accessed
} DataRecord;
*/
