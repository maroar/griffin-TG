void kernel_trmm(int m, int n,
		 float alpha,
		 float **A,
		 float **B)
{
  int i, j, k;
  float temp;

  for (i = 0; i < m; i++)
     for (j = 0; j < n; j++) {
        for (k = i+1; k < m; k++) 
           B[i][j] += A[k][i] * B[k][j];
        B[i][j] = alpha * B[i][j];
     }

}
