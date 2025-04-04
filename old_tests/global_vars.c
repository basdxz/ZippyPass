#include <stdio.h>

typedef struct {
    int rarely_used1; // Cold field
    long frequently_used1; // Hot field
    int frequently_used2; // Hot field
    float rarely_used2; // Cold field
    int frequently_used3; // Hot field
    float rarely_used3; // Cold field
} HotColdStruct;

const HotColdStruct g_constVar = {0, 1, 2, 3, 4, 5};
HotColdStruct g_regularVar = {10, 20, 30, 40, 50, 60};

HotColdStruct g_zero_init = {0, 0, 0, 0, 0, 0};
HotColdStruct g_no_init;


void zap(HotColdStruct zamn) {
    zamn.rarely_used1 += 2;
    printf("%d\n", zamn.rarely_used1);
}

void zeet(HotColdStruct* zamn) {
    zamn->rarely_used1 = 2323;
}

void foop() {
    g_no_init.rarely_used2 = g_regularVar.rarely_used2;
    g_no_init.frequently_used2 = -1;
    zeet(&g_no_init);
    zap(g_no_init);
}

double getFooSum() {
    double foo_0 = g_constVar.rarely_used1 + g_regularVar.rarely_used1;
    double foo_1 = g_constVar.frequently_used1 + g_regularVar.frequently_used1;
    double foo_2 = g_constVar.frequently_used2 + g_regularVar.frequently_used2;
    double foo_3 = g_constVar.rarely_used2 + g_regularVar.rarely_used2;
    double foo_4 = g_constVar.frequently_used3 + g_regularVar.frequently_used3;
    double foo_5 = g_constVar.rarely_used3 + g_regularVar.rarely_used3;

    double foo = foo_0 + foo_1 + foo_2 + foo_3 + foo_4 + foo_5;
    return foo;
}

int main() {
    foop();

    double foo = getFooSum();
    printf("foo:%f\n", foo);

    const HotColdStruct l_constVar = {15, 0, 35, 45, 55, 65};
    HotColdStruct l_regularVar = {105, 0, 305, 405, 505, 605};

    double bar_0 = l_constVar.rarely_used1 + l_regularVar.rarely_used1;
    double bar_1 = l_constVar.frequently_used1 + l_regularVar.frequently_used1;
    double bar_2 = l_constVar.frequently_used2 + l_regularVar.frequently_used2;
    double bar_3 = l_constVar.rarely_used2 + l_regularVar.rarely_used2;
    double bar_4 = l_constVar.frequently_used3 + l_regularVar.frequently_used3;
    double bar_5 = l_constVar.rarely_used3 + l_regularVar.rarely_used3;

    double bar = bar_0 + bar_1 + bar_2 + bar_3 + bar_4 + bar_5;
    printf("bar:%f\n", bar);

    l_regularVar.frequently_used1 = 555;
    l_regularVar.frequently_used3 = 454;
    g_no_init = l_regularVar;

    l_regularVar.frequently_used1 = 454;
    l_regularVar.frequently_used3 = 555;
    g_zero_init = l_regularVar;
    double out = foo + bar + 1;

    g_no_init.frequently_used1 = 999;

    printf("%f\n", out);
    return out;
}
