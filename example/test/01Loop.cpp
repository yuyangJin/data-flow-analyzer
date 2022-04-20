#include <iostream>
#include <random>
#define N 10000
int main() {
  double X[N][N][N] = {0};
  int sum = 0;
  // for (int i = 0; i < N; i++) {
  //   X[i][i] = rand() % 100;
  // }
  int n = rand() % 100;
  int x = pow(2, n);
  for (int i = 0; i < N; i++) {
    x += 4;
    for (int j = 0; j < N; j++) {
      for (int k = 0; k < N; k++) {
        // x +=4;
        sum += X[(i + 2) / x][(j * 2 - 1) / 4][k + 3];
      }
    }
  }
  std::cout << "sum:" << sum << std::endl;
}