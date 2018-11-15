void kernel_trisolv(int n,
		    float **L,
		    float *x,
		    float *b)
{
  int i, j;

  for (i = 0; i < n; i++)
    {
      x[i] = b[i];
      for (j = 0; j <i; j++)
        x[i] -= L[i][j] * x[j];
      x[i] = x[i] / L[i][i];
    }

}
