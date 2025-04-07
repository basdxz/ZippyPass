// This test demonstrates a struct with fields that have different usage patterns
// Some fields are accessed frequently (hot) while others are rarely accessed (cold)
// The optimization should reorder fields to prioritize frequently accessed ones

typedef struct {
    int never_used;
    int rarely_used1; // Cold field
    long frequently_used1; // Hot field
    int frequently_used2; // Hot field
    float rarely_used2; // Cold field
    int frequently_used3; // Hot field
    float rarely_used3; // Cold field
} HotColdStruct;

// This function accesses the "hot" fields multiple times
// but the "cold" fields only once, creating a clear usage pattern
int process_struct(HotColdStruct *data) {
    int result = 0;

    // Access hot fields multiple times
    result += data->frequently_used1;
    result += data->frequently_used2;
    result += data->frequently_used3;

    result += data->frequently_used1 * 2;
    result += data->frequently_used2 * 2;
    result += data->frequently_used3 * 2;

    result += data->frequently_used1 * 3;
    result += data->frequently_used2 * 3;
    result += data->frequently_used3 * 3;

    // Access cold fields only once
    result += data->rarely_used1;
    result += data->rarely_used2;
    result += data->rarely_used3;

    return result;
}

int main() {
    HotColdStruct data = {0, 1, 2, 3, 4, 5, 6};
    return process_struct(&data);
}
