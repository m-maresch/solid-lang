#include "BuiltIns.h"
#include <cstdio>

double print(double num) {
    fprintf(stderr, "%f\n", num);
    return 0;
}