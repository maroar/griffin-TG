void kernel_lu(int **A, int n) {
  int k, j, i;
  for (k = 0; k < n; k++) {                                                                        
    for ( j = k + 1; j < n; j++)
      A[k][j] = A[k][j] / A[k][k];
      for( i = k + 1; i < n; i++)
    for ( j = k + 1; j < n; j++)
      A[i][j] = A[i][j] + A[i][k] * A[k][j];
  }
}
