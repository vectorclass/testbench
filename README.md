# Testbench files for Vector Class Library

By Agner Fog, 2019-05-15

The testbench is a set of programs and scripts for testing the vector class library.
This is used during the development of VCL, but is not needed for programs that use VCL.

The testbench includes the following files:


* testbench1.cpp: C++ program for testing general functions and operators.
* testbench2.cpp: C++ program for testing permute, blend, lookup, gather, and scatter functions
* testbench3.cpp: C++ program for testing mathematical functions.
* runtest.sh:     Bash script for doing multiple tests, based on a list of test cases
* get_instruction_set.cpp: Used by runtest.sh for detecting the instruction set supported by the CPU
* test1.lst:      List of test cases for testbench1.cpp
* test2.lst:      List of test cases for testbench2.cpp
* test3.lst:      List of test cases for testbench3.cpp
* runcl32.bat:    Windows batch script for running runtest.sh under windows, using MS compiler, 32 bit mode
* runcl64.bat:    Windows batch script for running runtest.sh under windows, using MS compiler, 64 bit mode
* runicl64.bat:   Windows batch script for running runtest.sh under windows, using Intel compiler, 64 bit mode


The test programs can be run as stand-alone programs to test a single function, or they can
be run from the runtest.sh script to do a sequence of many tests, based on a list of test cases.


## To run as stand-alone program

Compile and run one of the testbenchx.cpp programs. 
Set the following defines to fit your purpose:

| variable | description |
|----------|-------------|
| testcase | A number. Each test case defines an operator, function, or group of functions to test. See the .cpp file for definition of the test cases. |
| vtype    | The vector type to use as input to the function under test. |
| rtype    | The vector type for the function return, if different from vtype. (If the return type is a scalar then use the smallest possible corresponding vector type) |
| funcname | Name of the function to test, if the testcase covers more than one function name. |
| indexes  | One or more template parameters, separated by commas, if the function under test needs template parameters. |
| seed     | Seed for random number generator. This generates random test data. Repeating a test with the same seed will generate the same results. |
| INSTRSET | Instruction set. See the description of instruction sets in vcl_manual.pdf |


## To run a series of tests with a script

runtest.sh is a bash script that will compile and run the testcasex.cpp programs multiple
times with different values for the parameters listed above. The list of test cases is
defined in a file with the following format:

General lines define a function or operator to test, and various parameters, separated by commas:
1. Test case (integer or range) indicating which function or operator to test
2. Vector type for parameters
3. Vector type for return value
4. Instruction set (2=SSE2, 3=SSE3, 4=SSSE3, 5=SSE4.1, 6=SSE4.2, 7=AVX, 8=AVX2+FMA, 
   9=AVX512F, 10=AVX512BW/DQ/VL
5. Function name (only for permute etc.)
6. List of indexes for permute, blend and gather functions. Separated by '+'
   (This will be converted to template parameters separated by ',')

Ranges can be specified for the following parameters:
1. Test case: a range specified as values separated by spaces
2. Vector type: Multiple vector types separted by spaces
3. The type for return value cannot specify a range.
   Make it blank to get the same as the vector type for parameters
4. Instruction set: a range specified as values separated by spaces

Special lines:

A line beginning with a dollar sign ($) specifies a parameter:
* $compiler= (1=Gnu, 2=Clang, 3=Intel for Linux or Mac, 10=Microsoft for Windows, 11=Intel for Windows)
* $mode= (32 for 32-bit mode, 64 for 64-bit mode)
* $testbench= (name of .cpp file)
* $outfile= (name of output file)
* $include= (directory where the .h include files of VCL can be found. May be relative path)
* $seed= (an integer for initializing the random number generator)

Comments begin with '#'. The file must end with a blank line.


## Running test scripts from Linux or MacOS

You can use the testx.lst files and modify them to fit your purpose.
Remember to set the variables in the beginning of the file to specify
the compiler, testbench cpp file, and include directory of your setup.

Run from a bash prompt:

./runtest.sh listfile.lst

The test will stop if it detects an error.


## Running test scripts from Windows

The Windows .bat file system is not well suited for complicated scripts. 
The bash shell and command language, which is common in Unix systems, is
much better suited for our purpose. There are several ways in which you
can run the bash script on a Windows computer:

1. Windows subsystem for Linux (WSL). Install WSL. Then install Gnu or 
   Clang compiler from the WSL command prompt. The compiler will generate 
   Linux executables and run them under WSL.

2. Mingw-w64 is a minimal Gnu system running under Windows. It includes 
   bash and a Gnu compiler. The functionality is similar to WSL.

3. Cygwin64. This is a shell and environment that emulates Linux commands 
   under Windows. Gnu and Clang C++ compilers are available from the Cygwin
   installation interface.
   Cygwin generates Windows executables, not Linux executables.
   Programs compiled under Cygwin can run in Windows outside the Cygwin shell,
   using a cygwin DLL.
   
4. Use the bash command interpreter from Cygwin with a Microsoft or Intel compiler.
   It is a little more tricky to combine cygwin bash with a non-cygwin compiler.
   Use runcl32.bat, runcl64.bat, or runicl64.bat. This will set
   the environment parameters for Microsoft Visual Studio C++ compiler and the Intel compiler and then
   invoke the bash shell and run runtest.sh. You need to edit the .bat file to
   specify the relevant paths and the name of the testcase list file.

