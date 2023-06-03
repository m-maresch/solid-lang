#include <iostream>

extern "C" {

double avg(double, double);

}

/*
 * 1) Create output.o file of:
 * func avg(x y) (x + y) * 0.5;
 *
 * 2) Link this program to output.o:
 * clang++ link.cpp output.o -o main
 *
 * 3) Run it:
 * ./main
*/
int main() {
    std::cout << "avg of 3 and 4: " << avg(3.0, 4.0) << std::endl;
}