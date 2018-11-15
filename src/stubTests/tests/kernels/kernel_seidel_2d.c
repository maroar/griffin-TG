void kernel_seidel_2d(int n, int tsteps, float **A) {
  int i, j, t;
  for (t = 0; t <= tsteps - 1; t++)
    for (i = 1; i <= n - 2; i++)
      for (j = 1; j <= n - 2; j++)
        A[i][j] = (A[i-1][j-1] + A[i-1][j] + A[i-1][j+1]
           + A[i][j-1] + A[i][j] + A[i][j+1]
           + A[i+1][j-1] + A[i+1][j] + A[i+1][j+1])/9.0;
}
