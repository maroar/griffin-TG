void kernel_gemm(int ni, int nj, int nk,
		 float alpha,
		 float beta,
		 float **C,
		 float **A,
		 float **B)
{
  int i, j, k;

  for (i = 0; i < ni; i++) {
    for (j = 0; j < nj; j++)
	C[i][j] *= beta;
    for (k = 0; k < nk; k++) {
       for (j = 0; j < nj; j++)
	  C[i][j] += alpha * A[i][k] * B[k][j];
    }
  }

}
