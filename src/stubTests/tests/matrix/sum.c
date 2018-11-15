void sum(int M, int N, int **matA, int **matB, int **matC)
{
  int i, j;

  for (i = 0; i < M; i++) {
    for (j = 0; j < N; j++) {
      matC[i][j] = matA[i][j] + matB[i][j];
    }
  }
}

