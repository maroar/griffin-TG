void kernel_syr2k(int n, int m,
		  float alpha,
		  float beta,
		  float **C,
		  float **A,
		  float **B)
{
  int i, j, k;


  for (i = 0; i < n; i++)
    for (j = 0; j < n; j++)
      C[i][j] *= beta;
  for (i = 0; i < n; i++)
    for (k = 0; k < m; k++) {
      for (j = 0; j < n; j++)
	{
	  C[i][j] += A[j][k] * alpha*B[i][k] + B[j][k] * alpha*A[i][k];
	}
     }

}
