#include <iostream>
#include <random>

int main() {
    double X[10] = {0};
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        X[i] = rand() % 100;
    }
    for (int i = 0; i < 10; i++) {
        sum += X[i];
    }
    std::cout << "sum:" << sum << std::endl; 
}