/**********************  get_instruction_set.cpp   ****************************
* Author:        Agner Fog
* Date created:  2019-05-08
* Last modified: 2022-07-20
* Version:       2.02.00
* Project:       vector class library
* Description:
* This program outputs the largest instruction set supported on the CPU.
* Used in the test script runtest.sh

Output value:
 0           = 80386 instruction set
 1  or above = SSE (XMM) supported by CPU (not testing for O.S. support)
 2  or above = SSE2
 3  or above = SSE3
 4  or above = Supplementary SSE3 (SSSE3)
 5  or above = SSE4.1
 6  or above = SSE4.2
 7  or above = AVX supported by CPU and operating system
 8  or above = AVX2
 9  or above = AVX512F
10  or above = AVX512VL, AVX512BW, and AVX512DQ
11  or above = AVX512VBMI and AVX512VBMI2
12  or above = AVX512_FP16

 (c) Copyright 2019-2022 GNU General Public License 3.0 or later www.gnu.org/licenses
******************************************************************************/


#include <stdio.h>

#define VCL_NAMESPACE

// include the function instrset_detect
#include "instrset_detect.cpp"

int main() {
    int iset = instrset_detect();
    if (iset == 10 && hasAVX512VBMI2()) iset = 11;
    if (iset == 11 && hasAVX512FP16())  iset = 12;
    printf("%i\n", iset);
    return 0;
}
