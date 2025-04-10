typedef struct {
    int a;
    long b;
    char c;
} HotColdStruct;

HotColdStruct global_var = {1, 2, 3};
HotColdStruct global_var_2 = {4, 5, 6};

int main() {
    HotColdStruct local_var = {7, 8, 9};
    HotColdStruct local_var_2 = {10, 11, 12};

    // Add two locals
    long bar = local_var.b + local_var_2.a;
    // Add a local and a global
    long foo = local_var.b + global_var.b;
    // Add a global and a global
    long baz = global_var.b + global_var_2.a;

    return 0;
}
