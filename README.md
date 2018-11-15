# Griffin-TG

Griffin-TG it's a tool that allows its users to run isolated functions in C automatically. The kind of function accepted by the tool are the ones that receive arrays by parameters. The main goal of the tool is to generate data to execute the target function that don't lead to a bad memory access. By a bad memory access, we mean an access out of the limits of the memory allocated to the array. Considering the goal stated before, this is made with random generated inputs that are passed to the function trough its parameters. This tool can be used together with other programs for dynamically analyse functions written in C.

## Prerequisites

* Cmake
* C++14 compiler
* xdot 
* valgrind (optional: if you want to check that the function will run without memory errors caused by bad memory access.)

## Installing 

    cd src
    cmake CMakeLists.txt
    make

## Running

1) the target file must be in the folder "griffin-TG/src/stubTests/";
2) in the folder "griffin-TG/src/" there is a script, "testFile.sh", that can be used to run the tool for the target function, ex.: "./testFile.sh stubTests/targetFile.c"
3) there are some examples of inputs to the griffin-TG in the folders in "griffin-TG/src/stubTests/tests/". To try them, you need just to copy them to the folder "griffin-TG/src/stubTests/" and use the script "griffin-TG/src/testFile.sh" in each file that you want;

## Warning

We performed the experiments in a machine with Ubuntu 16.04 LTS (kernel 4.15.0-36-generic). 

## Outputs

The output of the tool is a main file that is able to execute the function passed as parameter without causing errors related to bad memory access. Once the tool is used, this main file can be found in "griffin-TG/src/stubTests/mains/". Besides the previous file, the tool creates the following files in the process:
1) griffin-TG/src/initGraph.dot: debug information showing the order of the declarations of each used variable used to build the main file;
2) griffin-TG/src/stubTests/*.dot: debug information about the way as the arrays are accessed and how the information flows during the process of finding the input data to the file;
3) griffin-TG/src/stubTests/graphs/*.pdf: same information as in the item before, 2), but the files in this folder are images in pdf;

## Limitations

1) we analyze access made by the syntax '[]', eg: "v [a + 1]";
2) the tool can analyze only one function in plain C (no macros);
3) the code given as input to the tool must be something compilable;
4) the current state of Griffin-TG can not handle structs;
5) in loops, avoid using subtraction or operations that lead to expression with negative coefficients;
6) variables used to guide the loop should be declared before the 'for' or 'while' statement.

## About

This is a research project. While we strive to keep the code clean and to make the tool easily accessible, this is an academic effort. Neverless, feel free to provide feedback or to report bugs.

## Contact

Marcus Rodrigues de Araújo (demaroar@gmail.com)

