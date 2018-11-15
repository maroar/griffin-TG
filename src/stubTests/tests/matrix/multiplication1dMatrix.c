void multiplyMatrices(int* matA, int rA, int cA, int* matB,
        int rB, int cB, int* matC, int rC, int cC) {

  int i, j, k, sum;

  for (i = 0; i <= rA; i++) 
  {
    for (j = 0; j <= cB; j++) 
    {
      sum = 0;
      for (k = 0; k <= rB; k++)
      {
        sum = sum + matA[i * cA + k] * matB[k * cB + j];
      }
      matC[i * cC + j] = sum;
    }
  }
}

