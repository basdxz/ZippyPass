/**
 * test_basic_transform.c
 * 
 * Purpose: Verify basic struct reordering without breaking functionality
 * Expected transformation: Fields will be reordered based on access frequency
 */

typedef struct {
    int rarely_used;   // Should move to end
    int frequently_used1; // Should move to beginning
    long frequently_used2; // Should move to beginning
} BasicStruct;

int access_fields(BasicStruct* data) {
    int result = 0;
    
    // Access frequently used fields multiple times
    result += data->frequently_used1;
    result += data->frequently_used1;
    result += data->frequently_used1;
    
    result += data->frequently_used2;
    result += data->frequently_used2;
    result += data->frequently_used2;
    
    // Access rarely used field only once
    result += data->rarely_used;
    
    return result;
}

int main() {
    BasicStruct data = {1, 2, 3};
    return access_fields(&data) - 13; // Result should be 0
}

/* Expected transformed order:
typedef struct {
    int frequently_used1;
    long frequently_used2;
    int rarely_used;
} BasicStruct;
*/
