/**
 * test_loop_access_patterns.c
 *
 * Purpose: Verify loop-based access patterns are correctly identified
 * Adapted from: for_loop.c
 */

typedef struct {
    int outside_loop;      // Accessed outside loop - low frequency
    float loop_usage1;     // Accessed in both loops - highest frequency
    long outside_loop2;    // Accessed outside loop - low frequency
    int loop_usage2;       // Accessed in inner loop only - medium frequency
    float rarely_accessed; // Accessed once - lowest frequency
} LoopStruct;

int process_struct(LoopStruct *data) {
    int result = 0;
    
    // Access outside loop only once
    result += data->outside_loop;
    result += data->outside_loop2;
    
    // Access loop_usage1 in outer loop (medium frequency)
    for (int i = 0; i < 5; i++) {
        result += data->loop_usage1;
        
        // Access loop_usage1 and loop_usage2 in nested loop (highest frequency)
        for (int j = 0; j < 3; j++) {
            result += data->loop_usage1 * j;
            result += data->loop_usage2 * j;
        }
    }
    
    // Access rarely_accessed once
    result += data->rarely_accessed;
    
    return result;
}

int main() {
    LoopStruct data = {1, 2.0f, 3, 4, 5.0f};
    int result = process_struct(&data);
    
    // Verification - doesn't matter what the exact value is as long as it's consistent
    return (result == 94) ? 0 : 1;
}

/* Expected transformed order based on loop access patterns:
typedef struct {
    float loop_usage1;     // Highest frequency (accessed in both loops)
    int loop_usage2;       // Medium frequency (accessed in inner loop)
    int outside_loop;      // Low frequency
    long outside_loop2;    // Low frequency
    float rarely_accessed; // Lowest frequency
} LoopStruct;
*/
