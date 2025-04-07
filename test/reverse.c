typedef struct {
    long h;
    int g;
    int f;
    int e;
    int d;
    int c;
    int b;
    int a;
} BackwardsStruct;

int funcA(BackwardsStruct v) {
    return v.a + v.b + v.c + v.d + v.e + v.f + v.g + v.h;
}

int main() {
    return 0;
}
