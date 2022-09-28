#include <iostream>

#define N 10

void init(double A[N][N]) {
    for (int i = 0; i < N; i++ ) {
        for (int j = 0; j < N; j++) {
            A[i][j] = (i + j)/2;
        }
    }
}
void seidel2d(double A[N][N]) {
    for (int i = 1; i < N-1; i++ ) {
        for (int j = 1; j < N-1; j++) {
            A[i][j] = (A[i-1][j] + A[i][j-1] 
              + A[i][j] + A[i][j+1] + A[i+1][j])/5;
        }
    }    
}

int main() {
    double A[N][N];
    init(A);
    seidel2d(A);
    std::cout << A[N/2][N/2] << std::endl;
}