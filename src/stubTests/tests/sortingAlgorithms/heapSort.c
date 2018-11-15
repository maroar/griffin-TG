
void heapsort(int *arr, unsigned int N) 
{
    if(N==0)
      return;

    int t; 
    unsigned int n = N, parent = N/2, index, child;

    while (1) { 
        if (parent > 0) {
            t = arr[--parent];
        } else {
            n--;
            if (n == 0) {
                return;
            }
            t = arr[n];
            arr[n] = arr[0];
        }
        index = parent; 
        child = index * 2 + 1; 
        while (child < n) {
            if (child + 1 < n  &&  arr[child + 1] > arr[child]) {
                child++; 
            }
            if (arr[child] > t) {
                arr[index] = arr[child]; 
                index = child;
                child = index * 2 + 1;
            } else {
                break; 
            }
        }
        arr[index] = t; 
    }
}
