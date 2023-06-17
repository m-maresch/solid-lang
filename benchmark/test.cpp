#include <iostream>
#include <chrono>

extern "C" {

double testSolid(double);

}

double fac(double x) {
    if (x < 2)
        return 1;
    else
        return x * fac(x - 1);
}

double testCpp(double n) {
    int i = 0;

    do {
        fac(i) + fac(i + 1);
        i++;
    } while (i < n);

    return 0.0;
}

/*
 * 1) Create test.o file:
 * ./solid_lang test.solid
 *
 * 2) Link this program to test.o:
 * clang++ test.cpp test.o -o test
 *
 * 3) Run it:
 * ./test
*/
int main() {
    testCpp(10);
    testSolid(10);

    std::cout << "Test C++" << std::endl;
    auto startCpp = std::chrono::system_clock::now();
    testCpp(100000);
    auto endCpp = std::chrono::system_clock::now();

    std::cout << "Test Solid" << std::endl;
    auto startSolid = std::chrono::system_clock::now();
    testSolid(100000);
    auto endSolid = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsedCpp = endCpp - startCpp;
    std::chrono::duration<double> elapsedSolid = endSolid - startSolid;

    std::cout << "Elapsed C++: " << elapsedCpp.count() << "s" << std::endl;
    std::cout << "Elapsed Solid: " << elapsedSolid.count() << "s" << std::endl;
}
