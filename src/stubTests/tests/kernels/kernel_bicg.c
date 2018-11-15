void kernel_bicg(int m, int n,
		 float **A,
		 float *s,
		 float *q,
		 float *p,
		 float *r)
{
  int i, j;

  for (i = 0; i < m; i++)
    s[i] = 0;
  for (i = 0; i < n; i++)
    {
      q[i] = 0.0;
      for (j = 0; j < m; j++)
	{
	  s[j] = s[j] + r[i] * A[i][j];
	  q[i] = q[i] + A[i][j] * p[j];
	}
    }

}
