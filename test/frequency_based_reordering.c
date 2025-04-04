/**
 * test_frequency_based_reordering.c
 *
 * Purpose: Verify high-frequency fields are prioritized
 * Adapted from: hotcold.c
 */

typedef struct {
    int never_used;           // Not accessed, should move to end
    int rarely_used1;         // Cold field - accessed once
    long frequently_used1;    // Hot field - accessed 9 times
    int frequently_used2;     // Hot field - accessed 9 times
    float rarely_used2;       // Cold field - accessed once
    int frequently_used3;     // Hot field - accessed 9 times
    float rarely_used3;       // Cold field - accessed once
} HotColdStruct;

// This function creates a clear hot/cold access pattern
int process_struct(HotColdStruct *data) {
    int result = 0;

    // Access hot fields multiple times
    for (int i = 0; i < 3; i++) {
        result += data->frequently_used1;
        result += data->frequently_used2;
        result += data->frequently_used3;
    }

    // Access hot fields multiple more times
    for (int i = 0; i < 6; i++) {
        result += i * (
            data->frequently_used1 + 
            data->frequently_used2 + 
            data->frequently_used3
        );
    }

    // Access cold fields only once
    result += data->rarely_used1;
    result += data->rarely_used2;
    result += data->rarely_used3;

    return result;
}

int main() {
    HotColdStruct data = {0, 1, 2, 3, 4.0f, 5, 6.0f};
    int result = process_struct(&data);
    
    // The exact result value is complex but deterministic
    // This verification checks that the optimization doesn't change behavior
    return (result == 286) ? 0 : 1;
}

/* Expected transformed order based on access frequency:
typedef struct {
    long frequently_used1;
    int frequently_used2;
    int frequently_used3;
    int rarely_used1;
    float rarely_used2;
    float rarely_used3;
    int never_used;
} HotColdStruct;
*/
