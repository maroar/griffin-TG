void kernel_symm(int m, int n,
		 float alpha,
		 float beta,
		 float **C,
		 float **A,
		 float **B)
{
  int i, j, k;
  float temp2;

   for (i = 0; i < m; i++)
      for (j = 0; j < n; j++ )
      {
        temp2 = 0;
        for (k = 0; k < i; k++) {
           C[k][j] += alpha*B[i][j] * A[i][k];
           temp2 += B[k][j] * A[i][k];
        }
        C[i][j] = beta * C[i][j] + alpha*B[i][j] * A[i][i] + alpha * temp2;
     }

}
