// Poor layout - mixing types causes padding
typedef struct {
    char a;        // 1 byte + 7 bytes padding
    double b;      // 8 bytes
    char c;        // 1 byte + 7 bytes padding
    double d;      // 8 bytes
    char e;        // 1 byte + 7 bytes padding
    double f;      // 8 bytes
} BadLayout;  // Total: 48 bytes

// Better layout - grouping by size
typedef struct {
    double b;      // 8 bytes
    double d;      // 8 bytes
    double f;      // 8 bytes
    char a;        // 1 byte
    char c;        // 1 byte
    char e;        // 1 byte
    // 5 bytes padding
} GoodLayout;  // Total: 32 bytes

volatile BadLayout bad;
volatile GoodLayout good;

int main() {

}