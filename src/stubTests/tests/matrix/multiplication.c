void multiply(int M, int N, int K, int **matA, int **matB, int **matC) 
{
  int i, j, k;

  for (i = 0; i < M; i++) {
    for (j = 0; j < K; j++) {
      matC[i][j] = 0;
      for (k = 0; k < N; k++) {
        matC[i][j] += matA[i][k] * matB[k][j];
      }
    }
  }
}

