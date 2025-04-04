/**
 * test_field_access_preservation.c
 *
 * Purpose: Verify field access remains semantically correct after reordering
 * Adapted from: shapes.c
 */

struct Point {
    int x;
    int y;
};

struct Rectangle {
    struct Point top_left;    // Rarely accessed
    char name[20];            // Frequently accessed
    struct Point bottom_right; // Rarely accessed
};

int calculate_area(struct Rectangle* rect) {
    int width = rect->bottom_right.x - rect->top_left.x;
    int height = rect->bottom_right.y - rect->top_left.y;
    return width * height;
}

int access_name(struct Rectangle* rect) {
    // Access name field multiple times to make it "hot"
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += rect->name[i];
    }
    return sum;
}

int main() {
    // Initialize rectangle with known values
    struct Rectangle rect = {{1, 2}, "ABCDEFGHIJ", {5, 6}};
    
    int area = calculate_area(&rect);
    int nameSum = access_name(&rect);
    
    // Verify results (area should be 16, nameSum should be 655)
    return (area == 16 && nameSum == 655) ? 0 : 1;
}

/* Expected transformation:
struct Rectangle {
    char name[20];        // Moved up due to frequent access
    struct Point top_left;
    struct Point bottom_right;
};
*/
