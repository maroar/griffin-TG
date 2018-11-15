void kernel_syrk(int n, int m,
		 float alpha,
		 float beta,
		 float **C,
		 float **A)
{
  int i, j, k;

  for (i = 0; i < n; i++) {
    for (j = 0; j <= i; j++)
      C[i][j] *= beta;
    for (k = 0; k < m; k++) {
      for (j = 0; j <= i; j++)
        C[i][j] += alpha * A[i][k] * A[j][k];
    }
  }

}
