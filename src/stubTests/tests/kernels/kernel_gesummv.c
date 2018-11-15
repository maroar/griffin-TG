void kernel_gesummv(int n,
		    int alpha,
		    int beta,
		    float **A,
		    float **B,
		    float *tmp,
		    float *x,
		    float *y)
{
  int i, j;

  for (i = 0; i < n; i++)
    {
      tmp[i] = 0.0;
      y[i] = 0.0;
      for (j = 0; j < n; j++)
	{
	  tmp[i] = A[i][j] * x[j] + tmp[i];
	  y[i] = B[i][j] * x[j] + y[i];
	}
      y[i] = alpha * tmp[i] + beta * y[i];
    }
}
