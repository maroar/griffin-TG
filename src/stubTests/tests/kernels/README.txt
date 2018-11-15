We use the test cases in this folder to validate the tool.
As stated in the paper, when reconstructed, the kernel LU 
need to be manually modified. The change is necessary to 
make sure that  division by zero will not occur during its 
execution, with the random data generated. In practice, the 
change must be made in the function that creates data for 
the elements of the matrix 'A'. One simple change as 
changing the range of data from [0, 100] to [1, 100] is 
enough to solve the problem, considering the current state 
of the tool.


