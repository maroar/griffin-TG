
void quickSort(int *arr, int low, int high)
{
    if (low < high)
    {
        int temp;
        int pivot = arr[high];
        int i = (low - 1), j;
 
        for (j = low; j <= high- 1; j++)
        {
            if (arr[j] <= pivot)
            {
                i++;
                temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }

        temp = arr[i+1];
        arr[i+1] = arr[high];
        arr[high] = temp;

        int pi = i + 1;
        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}

