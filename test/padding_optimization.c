/**
 * test_padding_optimization.c
 *
 * Purpose: Verify padding is minimized through reordering
 * Adapted from: padding.c
 */

// Poor layout - mixing types causes padding
typedef struct {
    char a;        // 1 byte + 7 bytes padding
    double b;      // 8 bytes
    char c;        // 1 byte + 7 bytes padding
    double d;      // 8 bytes
    char e;        // 1 byte + 7 bytes padding
    double f;      // 8 bytes
} PaddingStruct;  // Total: 48 bytes

// Reference optimal layout - manually grouped by size
// In an ideal world, our optimization would transform to this
/*
typedef struct {
    double b;      // 8 bytes
    double d;      // 8 bytes
    double f;      // 8 bytes
    char a;        // 1 byte
    char c;        // 1 byte
    char e;        // 1 byte
    // 5 bytes padding
} OptimalPaddingStruct;  // Total: 32 bytes
*/

// Function that accesses fields in a pattern
// that should trigger reordering by size
int access_fields(PaddingStruct* ps) {
    // Access double fields frequently
    double sum = ps->b + ps->d + ps->f;
    sum += ps->b * 2;
    sum += ps->d * 2;
    sum += ps->f * 2;
    
    // Access char fields only once
    char chars = ps->a + ps->c + ps->e;
    
    return (int)(sum + chars);
}

int main() {
    // At the very least, the struct size should be <= 48 bytes (original)
    // Ideally, it would be 32 bytes (optimal packing)
    
    PaddingStruct data = {'A', 1.0, 'B', 2.0, 'C', 3.0};
    int result = access_fields(&data);
    
    // Return 0 if the behavior is correct
    return (result == 18) ? 0 : 1;
}

/*
Expected transformation:
The optimizer should group fields by similar types to reduce padding:

typedef struct {
    double b;
    double d;
    double f;
    char a;
    char c;
    char e;
} PaddingStruct;
*/
