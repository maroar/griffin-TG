void kernel_floyd_warshall(int **path, int n) {
  int i, j, k;
  for (k = 0; k < n; k++) {                                                                            
    for(i = 0; i < n; i++)                                                 
      for (j = 0; j < n; j++)                                                  
        path[i][j] = path[i][j] < path[i][k] + path[k][j] ?                        
          path[i][j] : path[i][k] + path[k][j];                                    
  }                                                                                                                                                             
}
