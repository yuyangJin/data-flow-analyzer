#include <iostream>
#include <random>
#define N 10000
int main() {
    double X[N][N] = {0};
    int sum = 0;
    for (int i = 0; i < N; i++) {
        X[i][i] = rand() % 100;
    }
    int n = rand() % 100;
    int x = pow(2,n);
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            sum += X[(i+1)/x][(j*2-1)/3];
        }
    }
    std::cout << "sum:" << sum << std::endl; 
}