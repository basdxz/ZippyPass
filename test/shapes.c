#include <stdio.h>

struct Point {
    int x;
    int y;
};

struct Rectangle {
    struct Point top_left;
    struct Point bottom_right;
    char name[20];
};

int calculate_area(struct Rectangle* rect) {
    int width = rect->bottom_right.x - rect->top_left.x;
    int height = rect->bottom_right.y - rect->top_left.y;
    return width * height;
}

void print_rectangle(struct Rectangle* rect) {
    printf("Rectangle %s: (%d,%d) to (%d,%d)\n",
           rect->name,
           rect->top_left.x, rect->top_left.y,
           rect->bottom_right.x, rect->bottom_right.y);
}