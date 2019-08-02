/****************************  testbench1.cpp   *******************************
* Author:        Agner Fog
* Date created:  2019-04-09
* Last modified: 2019-08-02
* Version:       2.00
* Project:       Testbench for vector class library
* Description:
* Compile and run this program to test operators and functions in VCL
* This file contains test cases for general operators and functions.
* Each function or operator is tested with many different combinations 
* of input data.
*
* Instructions:
* The following parameters must be defined on the command line or added 
* in the top of this file:
*
* vtype:    Vector type to test
* rtype:    Vector type for result, if different from vtype
*           (If result is a scalar, specify a corresponding vector type)
* testcase: A number defining a function or operator to test. 
*           See the cases in this file.
* seed:     Seed for random number generator. May be any integer
* INSTRSET: Desired instruction set. Needs to be specified for MS compiler,
*           but determined automatically for other compilers. Values:
*           2:  SSE2
*           3:  SSE3
*           4:  SSSE3
*           5:  SSE4.1
*           6:  SSE4.2
*           7:  AVX
*           8:  AVX2
*           9:  AVX512F
*           10: AVX512BW/DQ/VL
*
* Compile with any compiler supported by VCL.
* Specify the desired instruction set and optimization options as parameters
* to the compiler.
*
* (c) Copyright 2019 Agner Fog.
* Gnu general public license 3.0 https://www.gnu.org/licenses/gpl.html
******************************************************************************
Test cases:
1:   operator +
2:   operator -
3:   operator *
4:   operator /            (floating point types)
5:   operator /            (integer types. divide by scalar)
6:   operator /            (signed integer types. divide by compile-time constant)
7:   operator /            (unsigned integer types. divide by compile-time constant)
8:   unary -
9:   max
10:  min
11:  maximum               (floating point types)
12:  minimum               (floating point types)
13:  abs
14:  if_add
15:  if_sub
16:  if_mul
17:  if_div                (floating point types)
100: operator <<           (integer types)
101: operator >>           (integer types)
102: rotate_left           (signed integers only)
103: add_saturated         (integer types)
104: sub_saturated         (integer types)
105: abs_saturated         (integer types)
106: operator &
107: operator |
108: operator ^
109: operator ~
110: andnot                (bool vectors only)
200: sign_combine          (float types)
201: square                (float types)
300: operator <
301: operator <=
302: operator ==
303: operator !=
304: operator >=
305: operator >
306: sign_bit              (float types)
400: horizontal_add
401: horizontal_add_x      (integer types)
402: horizontal_and        (bool vectors)
403: horizontal_or         (bool vectors)
404: horizontal_min
405: horizontal_max 
410: horizontal_find_first (bool vectors)
411: horizontal_count      (bool vectors)
412: to_bits               (bool vectors)
413: load_bits             (bool vectors)
500: conversion between integer vectors with same total number of bits
501: conversion between boolean vectors with same number of elements
502: reinterpret_i
503: reinterpret_f
504: reinterpret_d
505: roundi
506: truncatei
507: round_to_int32
508: truncate_to_int32
509: round_to_int32 with two parameters
510: truncate_to_int32 with two parameters
511: to_float              (integer to float)
512: to_double             (integer to double)
513: to_float              (double to float)
514: to_double             (float to double)
600: concatenate vectors (constructor with two parameters)
601: concatenate boolean vectors (constructor with two parameters)
602: get_low               (int or float vector)
603: get_high              (int or float vector)
604: get_low               (boolean vector)
605: get_high              (boolean vector)
606: extend                (integer. vector size doubled)
607: extend_low
608: extend_high
609: compress              (one integer parameter, vector size reduced)
610: compress              (two integer parameters)
611: compress_saturated    (two integer parameters, integer types)
612: insert(index, value)
613: extract(index)
614: cutoff(index)
615: load_partial(index, p)
616: store_partial(index, p)
650: constructor with all elements

*****************************************************************************/

#include <stdio.h>
#include <cmath>
#if defined (__linux__) && !defined(__LP64__)
#include <fpu_control.h>     // set floating point control word
#endif

// maximum vector size
#define MAX_VECTOR_SIZE 512


#ifndef INSTRSET
#define INSTRSET 8           // desired instruction set
#endif

#include <vectorclass.h>     // vector class library


#ifndef testcase 
// ----------------------------------------------------------------------------
//          Specify input parameters here if running from an IDE:
// ----------------------------------------------------------------------------

#define testcase 1

#define vtype Vec8f

#define rtype vtype

#define seed 1


#else
// ----------------------------------------------------------------------------
//            Default input parameters when compiling from a script
// ----------------------------------------------------------------------------

// input or index vector type to be tested
#ifndef vtype
#define vtype Vec2d
#endif

// return type or data vector type to be tested
#ifndef rtype
#define rtype vtype
#endif

// random number seed
#ifndef seed
#define seed 1
#endif

#endif  // testcase

// ----------------------------------------------------------------------------
//             Declarations
// ----------------------------------------------------------------------------


// dummy vectors used for getting element type
vtype dummy;
rtype dummyr;
typedef decltype(dummy[0]) ST;    // scalar type input vectors
typedef decltype(dummyr[0]) RT;   // scalar type for return vector

const int maxvectorsize = 64;     // max number of elements in a vector

uint64_t bitfield;                // integer for load_bits function


/************************************************************************
*
*                          Test cases
*
************************************************************************/

#if   testcase == 1    // +
inline rtype testFunction(vtype const& a, vtype const& b) { return a + b; }
RT referenceFunction(ST a, ST b) { return a + b; }

#elif testcase == 2    // - 
inline rtype testFunction(vtype const& a, vtype const& b) { return a - b; }
RT referenceFunction(ST a, ST b) { return a - b; }

#elif testcase == 3    // * 
inline rtype testFunction(vtype const& a, vtype const& b) { return a * b; }
RT referenceFunction(ST a, ST b) { return a * b; }

#elif testcase == 4    // /  (float types only)
inline rtype testFunction(vtype const& a, vtype const& b) { return a / b; }
RT referenceFunction(ST a, ST b) { return a / b; }

#elif testcase == 5    // /  int types: divide by scalar
#define WHOLE_VECTOR
ST b0;
inline rtype testFunction(vtype const& a, vtype const& b) { 
    int i = 0;
    while (b[i] == 0) i++;
    b0 = b[i];
    return a / b0; 
}
vtype referenceFunction(vtype a, vtype b) { 
    vtype r;
    for (int i = 0; i < a.size(); i++) {
        r.insert(i, a[i] / b0);
    }
    return r; } 

#elif testcase == 6    // /  int types: divide by compile-time constant
#if defined(indexes) && (indexes + 0 != 0) // if indexes is not blank, use it as divisor
const ST divisor = indexes ;
#else
const ST divisor = 27;
#endif
inline rtype testFunction(vtype const& a, vtype const& b) { 
    return a / (divisor); 
    //return a / const_int(divisor); 
}
RT referenceFunction(ST a, ST b) { 
    return a / divisor; }

#elif testcase == 7    // /  int types: divide by compile-time unsigned constant
#if defined(indexes) && (indexes + 0 != 0) // if indexes is not blank, use it as divisor
const ST divisor = indexes ;
#else
const ST divisor = 27;
#endif
inline rtype testFunction(vtype const& a, vtype const& b) { 
    return a / const_uint(divisor); 
}
RT referenceFunction(ST a, ST b) { 
    return a / divisor; }

#elif testcase == 8    // unary - 
inline rtype testFunction(vtype const& a, vtype const& b) { return -b; }
RT referenceFunction(ST a, ST b) { return -b; }

#elif testcase == 9    // max
inline rtype testFunction(vtype const& a, vtype const& b) { return max(a, b); }
RT referenceFunction(ST a, ST b) { return a > b ? a : b; }
#define TESTNAN

#elif testcase == 10   // min
inline rtype testFunction(vtype const& a, vtype const& b) { return min(a, b); }
RT referenceFunction(ST a, ST b) { return a < b ? a : b; }
#define TESTNAN

#elif testcase == 11    // maximum
inline rtype testFunction(vtype const& a, vtype const& b) { return maximum(a, b); }
RT referenceFunction(ST a, ST b) {
    if (a!=a) return a;
    if (b!=b) return b;
    return a > b ? a : b; 
}
#define TESTNAN

#elif testcase == 12    // minimum
inline rtype testFunction(vtype const& a, vtype const& b) { return minimum(a, b); }
RT referenceFunction(ST a, ST b) {
    if (a!=a) return a;
    if (b!=b) return b;
    return a < b ? a : b; 
}
#define TESTNAN

#elif testcase == 13    // abs
inline rtype testFunction(vtype const& a, vtype const& b) { return abs(b); }
RT referenceFunction(ST a, ST b) { return b > 0 ? b : -b; }

#elif testcase == 14    // if_add
inline rtype testFunction(vtype const& f, vtype const& a, vtype const& b) { 
    return if_add(f != vtype(0), a, b); 
}
RT referenceFunction(ST f, ST a, ST b) { return f != 0 ? a+b : a; }
#define USE_FLAG

#elif testcase == 15    // if_sub
inline rtype testFunction(vtype const& f, vtype const& a, vtype const& b) { 
    return if_sub(f != vtype(0), a, b);
}
RT referenceFunction(ST f, ST a, ST b) { return f != 0 ? a-b : a; }
#define USE_FLAG

#elif testcase == 16    // if_mul
inline rtype testFunction(vtype const& f, vtype const& a, vtype const& b) { 
    return if_mul(f != vtype(0), a, b);
}
RT referenceFunction(ST f, ST a, ST b) { return f != 0 ? a*b : a; }
#define USE_FLAG

#elif testcase == 17    // if_div
inline rtype testFunction(vtype const& f, vtype const& a, vtype const& b) { 
    return if_div(f != vtype(0), a, b);
}
RT referenceFunction(ST f, ST a, ST b) { return f != 0 ? a/b : a; }
#define USE_FLAG


// ----------------------------------------------------------------------------
//                           integer only cases: 
// ----------------------------------------------------------------------------
#elif testcase == 100    // <<
ST b0;
inline rtype testFunction(vtype const& a, vtype const& b) {
    b0 = b[0];
    int c = b0 & (sizeof(ST) * 8 - 1);
    return a << c;
}
RT referenceFunction(ST a, ST b) {
    int c = b0 & (sizeof(ST) * 8 - 1);
    return a << c;
}

#elif testcase == 101    // >>
ST b0;
inline rtype testFunction(vtype const& a, vtype const& b) {
    b0 = b[0];
    int c = b0 & (sizeof(ST) * 8 - 1);
    return a >> c;
}
RT referenceFunction(ST a, ST b) {
    int c = b0 & (sizeof(ST) * 8 - 1);
    return a >> c;
}

#elif testcase == 102    // rotate_left (signed integers only)
#define WHOLE_VECTOR
ST b0;
inline rtype testFunction(vtype const& a, vtype const& b) {
    b0 = b[0];
    int s = sizeof(ST) * 8;  // size in bits
    int c = b0 & (s - 1);
    vtype r = rotate_left(a, c);
    return r;
}
// overloaded functions needed for converting all signed types to unsigned
uint8_t  rotleft(uint8_t a, uint8_t b) { return a << b | a >> (8 - b); }
uint16_t rotleft(uint16_t a, uint16_t b) { return a << b | a >> (16 - b); }
uint32_t rotleft(uint32_t a, uint32_t b) { return a << b | a >> (32 - b); }
uint64_t rotleft(uint64_t a, uint64_t b) { return a << b | a >> (64 - b); }
int8_t   rotleft(int8_t a, int8_t b) { return rotleft(uint8_t(a), uint8_t(b)); }
int16_t  rotleft(int16_t a, int16_t b) { return rotleft(uint16_t(a), uint16_t(b)); }
int32_t  rotleft(int32_t a, int32_t b) { return rotleft(uint32_t(a), uint32_t(b)); }
int64_t  rotleft(int64_t a, int64_t b) { return rotleft(uint64_t(a), uint64_t(b)); }

rtype referenceFunction(vtype const a, vtype const b) {
    b0 = b[0];
    int s = sizeof(ST) * 8;  // size in bits
    int c = b0 & (s - 1);
    ST res[64];
    for (int i = 0; i < a.size(); i++) {
        res[i] = rotleft(a[i], (ST)c);
    } 
    return vtype().load(res);
}

#elif testcase == 103    // add_saturated
inline rtype testFunction(vtype const& a, vtype const& b) {
    return add_saturated(a, b);
}
RT referenceFunction(ST a, ST b) {
    ST sum = a + b;
    if (ST(-1) < 0) { // signed type
        if (!((a < 0) ^ (b < 0)) && ((a < 0) ^ (sum < 0))) { // overflow
            sum = (1ull << (sizeof(ST) * 8 - 1)) - 1 + (sum >= 0);
        }
    }
    else {  // unsigned type
        if (sum < a) { // overflow
            sum = ST(-1);
        }
    }
    return sum;
}

#elif testcase == 104    // sub_saturated
inline rtype testFunction(vtype const& a, vtype const& b) { return sub_saturated(a, b); }
RT referenceFunction(ST a, ST b) {
    ST dif = a - b;
    if (ST(-1) < 0) { // signed type
        if (((a < 0) ^ (b < 0)) && ((a < 0) ^ (dif < 0))) { // overflow
            dif = ((uint64_t)1 << (sizeof(ST) * 8 - 1)) - 1 + (dif >= 0);
        }
    }
    else {  // unsigned type
        if (b > a) { // underflow
            dif = ST(0);
        }
    }
    return dif;
}

#elif testcase == 105    // abs_saturated
inline rtype testFunction(vtype const& a, vtype const& b) { return abs_saturated(b); }
RT referenceFunction(ST a, ST b) {
    volatile ST r = abs(b);   // volatile to prevent the compiler from optimizing away overflow check
    if (r < 0) return b - 1;
    return r;
}

#elif testcase == 106    // & 
inline rtype testFunction(vtype const& a, vtype const& b) { return a & b; }
RT referenceFunction(ST a, ST b) { return a & b; }

#elif testcase == 107    // | 
inline rtype testFunction(vtype const& a, vtype const& b) { return a | b;}
RT referenceFunction(ST a, ST b) { return a | b; }

#elif testcase == 108    // ^
inline rtype testFunction(vtype const& a, vtype const& b) { return a ^ b; }
RT referenceFunction(ST a, ST b) { return a ^ b; }

#elif testcase == 109    // ~
inline rtype testFunction(vtype const& a, vtype const& b) { return a & ~b; }
RT referenceFunction(ST a, ST b) { return a & ~b; }

#elif testcase == 110    // andnot (bool vectors only)
inline rtype testFunction(vtype const& a, vtype const& b) { return andnot(a, b); }
RT referenceFunction(ST a, ST b) { return a & ~b; }


// ----------------------------------------------------------------------------
//                           floating point only cases
//               (round, sqrt, pow, etc. are in the math testbench)
// ----------------------------------------------------------------------------

#elif testcase == 200    // sign_combine
inline rtype testFunction(vtype const& a, vtype const& b) { return sign_combine(a, b); }
// define signbit function because Visual studio is missing it
bool signbit_(float x) {
    union { float f; uint32_t i; } u;
    u.f = x; return u.i >> 31 != 0;
}
bool signbit_(double x) {
    union { double f; uint64_t i; } u;
    u.f = x; return u.i >> 63 != 0;
}
RT referenceFunction(ST a, ST b) {
    return signbit_(b) ? -a : a;
}

#elif testcase == 201    // square
inline rtype testFunction(vtype const& a, vtype const& b) { return square(b); }
RT referenceFunction(ST a, ST b) {
    return b * b;
}


// ----------------------------------------------------------------------------
//                           boolean result cases
// ----------------------------------------------------------------------------

#elif testcase == 300    // <
inline rtype testFunction(vtype const& a, vtype const& b) { return a < b; }
RT referenceFunction(ST a, ST b) { return a < b; }

#elif testcase == 301    // <=
inline rtype testFunction(vtype const& a, vtype const& b) { return a <= b; }
RT referenceFunction(ST a, ST b) { return a <= b; }

#elif testcase == 302    // ==
inline rtype testFunction(vtype const& a, vtype const& b) { return a == b; }
RT referenceFunction(ST a, ST b) { return a == b; }

#elif testcase == 303    // !=
inline rtype testFunction(vtype const& a, vtype const& b) { return a != b; }
RT referenceFunction(ST a, ST b) { return a != b; }

#elif testcase == 304    // >=
inline rtype testFunction(vtype const& a, vtype const& b) { return a >= b; }
RT referenceFunction(ST a, ST b) { return a >= b; }

#elif testcase == 305    // >
inline rtype testFunction(vtype const& a, vtype const& b) { return a > b; }
RT referenceFunction(ST a, ST b) { return a > b; }

#elif testcase == 306    // sign_bit (float types only)
inline rtype testFunction(vtype const& a, vtype const& b) { return sign_bit(b); }
// define signbit function because Visual studio is missing it
bool signbit_(float x) {
    union { float f; uint32_t i; } u;
    u.f = x; return u.i >> 31 != 0;
}
bool signbit_(double x) {
    union { double f; uint64_t i; } u;
    u.f = x; return u.i >> 63 != 0;
}
RT referenceFunction(ST a, ST b) {
    return signbit_(b);
}


// ----------------------------------------------------------------------------
//                           scalar result cases
// ----------------------------------------------------------------------------

#elif testcase == 400    // horizontal_add
#define SCALAR_RESULT
#define FACCURACY 4      // accept accumulating rounding errors
inline rtype testFunction(vtype const& a, vtype const& b) { return horizontal_add(b); }
RT referenceFunction(vtype const& a, vtype const& b) {
    ST sum = 0;
    for (int i = 0; i < b.size(); i++) sum += b[i];
    return sum;
}

#elif testcase == 401    // horizontal_add_x
#define SCALAR_RESULT
#define FACCURACY 4      // accept accumulating rounding errors
inline rtype testFunction(vtype const& a, vtype const& b) { return horizontal_add_x(b); }
RT referenceFunction(vtype const& a, vtype const& b) {
    RT sum = 0;
    for (int i = 0; i < b.size(); i++) sum += (RT)b[i];
    return sum;
}

#elif testcase == 402    // horizontal_and
#define SCALAR_RESULT
inline rtype testFunction(vtype const& a, vtype const& b) { return rtype(horizontal_and(b)); }
RT referenceFunction(vtype const& a, vtype const& b) {
    RT sum = true;
    for (int i = 0; i < b.size(); i++) sum = sum && b[i];
    return sum;
}

#elif testcase == 403    // horizontal_or
#define SCALAR_RESULT
inline rtype testFunction(vtype const& a, vtype const& b) { return rtype(horizontal_or(b)); }
RT referenceFunction(vtype const& a, vtype const& b) {
    RT sum = false;
    for (int i = 0; i < b.size(); i++) sum = sum || b[i];
    return sum;
}

#elif testcase == 404    // horizontal_min
#define SCALAR_RESULT
inline rtype testFunction(vtype const& a, vtype const& b) { return rtype(horizontal_min(b)); }
RT referenceFunction(vtype const& a, vtype const& b) {
    ST m = b[0];
    for (int i = 0; i < b.size(); i++) {
        if (b[i] != b[i]) return b[i]; // NAN
        if (b[i] < m) m = b[i];
    }
    return m;
}

#elif testcase == 405    // horizontal_max
#define SCALAR_RESULT
inline rtype testFunction(vtype const& a, vtype const& b) { return rtype(horizontal_max(b)); }
RT referenceFunction(vtype const& a, vtype const& b) {
    ST m = b[0];
    for (int i = 0; i < b.size(); i++) {
        if (b[i] != b[i]) return b[i]; // NAN
        if (b[i] > m) m = b[i];
    }
    return m;
}

#elif testcase == 410    // horizontal_find_first
#define SCALAR_RESULT    // integer result
inline rtype testFunction(vtype const& a, vtype const& b) {return rtype(horizontal_find_first(vtype(b))); }
RT referenceFunction(vtype const& a, vtype const& b) {
    RT sum = -1;
    for (int i = 0; i < b.size(); i++) {
        if (b[i] && sum < 0) sum = i;
    }
    return sum;
}

#elif testcase == 411    // horizontal_count
#define SCALAR_RESULT
inline rtype testFunction(vtype const& a, vtype const& b) { return horizontal_count(b); }
RT referenceFunction(vtype const& a, vtype const& b) {
    RT sum = 0;
    for (int i = 0; i < b.size(); i++) {
        if (b[i]) sum++;
    }
    return sum;
}

#elif testcase == 412    // to_bits
#define SCALAR_RESULT
inline rtype testFunction(vtype const& a, vtype const& b) { return to_bits(b); }
RT referenceFunction(vtype const& a, vtype const& b) {
    RT r = 0;
    for (int i = 0; i < b.size(); i++) {
        r |= RT(b[i]) << i;
    }
    return r;
}

#elif testcase == 413    // load_bits. (vtype is the type to test, rtype is the same)

inline rtype testFunction(vtype const& f, vtype const& a, vtype const& b) { 
    rtype r;  
    if (vtype::size() <= 8) {
        r.load_bits((uint8_t)bitfield);
    }
    else if (vtype::size() <= 16) {        
        r.load_bits((uint16_t)bitfield);
    }
    else if (vtype::size() <= 32) {        
        r.load_bits((uint32_t)bitfield);
    }
    else {
        r.load_bits(bitfield);  // ignore warnings here
    }
    return r;
}
rtype referenceFunction(vtype const& a, vtype const& b) {
    rtype r(false);
    for (int i=0; i < r.size(); i++) {
        r.insert(i, (bitfield >> i) & 1);
    }
    return r;
}
#define WHOLE_VECTOR   // test whole vector, not individual elements
#define USE_FLAG       // use bitfield



// ----------------------------------------------------------------------------
//                           type conversion functions
// ----------------------------------------------------------------------------

#elif testcase == 500    // direct conversion between integer vectors with same total number of bits
inline rtype testFunction(vtype const& a, vtype const& b) { return rtype(b); }
rtype referenceFunction(vtype const& a, vtype const& b) {
    int8_t temp[maxvectorsize];
    rtype r;
    b.store(temp);
    r.load(temp);
    return r;
}
#define WHOLE_VECTOR   // test whole vector, not individual elements


#elif testcase == 501    // direct conversion between boolean vectors with same number of elements
// possible for compact boolean vectors. not always possible for big boolean vectors
inline rtype testFunction(vtype const& a, vtype const& b) { return rtype(b); }
rtype referenceFunction(vtype const& a, vtype const& b) {    
    uint64_t temp = to_bits(b);
    rtype r;  
    if (vtype::size() <= 8) {
        r.load_bits((uint8_t)temp);
    }
    else if (vtype::size() <= 16) {        
        r.load_bits((uint16_t)temp);
    }
    else if (vtype::size() <= 32) {        
        r.load_bits((uint32_t)temp);
    }
    else {
        r.load_bits(temp);  // ignore warnings here
    }
    return r;
}
#define WHOLE_VECTOR   // test whole vector, not individual elements


#elif testcase == 502    // reinterpret_i
// float or double to int
inline rtype testFunction(vtype const& a, vtype const& b) { return reinterpret_i(b); }
rtype referenceFunction(vtype a, vtype b) {
    int8_t temp[maxvectorsize];
    rtype r;
    b.store((ST*)temp);
    r.load(temp);
    return r;
}
#define WHOLE_VECTOR   // test whole vector, not individual elements


#elif testcase == 503    // reinterpret_f
// int to float
inline rtype testFunction(vtype const& a, vtype const& b) { return reinterpret_f(b); }
rtype referenceFunction(vtype a, vtype b) {
    int8_t temp[maxvectorsize];
    rtype r;
    b.store((ST*)temp);
    r.load((RT*)temp);
    return r;
}
#define WHOLE_VECTOR   // test whole vector, not individual elements


#elif testcase == 504    // reinterpret_d
// int to double
inline rtype testFunction(vtype const& a, vtype const& b) { return reinterpret_d(b); }
rtype referenceFunction(vtype a, vtype b) {
    int8_t temp[maxvectorsize];
    rtype r;
    b.store((ST*)temp);
    r.load((RT*)temp);
    return r;
}
#define WHOLE_VECTOR   // test whole vector, not individual elements


#elif testcase == 505    // roundi
inline rtype testFunction(vtype const& a, vtype const& b) { return roundi(b); }
RT referenceFunction(ST a, ST b) {
    RT r;
    if (b >= 0) {
        r = (RT)(b + 0.5);                 // 0.5 rounds up
        if ((r & 1) && r - b == 0.5) r--;  // round down if result is odd
    }
    else {
        r = (RT)(b - 0.5);                 // 0.5 rounds up
        if ((r & 1) && b - r == 0.5) r++;  // round up if result is odd
    }
    return r;
}

#elif testcase == 506    // truncatei
inline rtype testFunction(vtype const& a, vtype const& b) { return truncatei(b); }
RT referenceFunction(ST a, ST b) {
    RT r;
    if (b >= 0) {
        r = (RT)(b);                 // truncate towards zero
    }
    else {
        r = -(RT)(-b);
    }
    return r;
}

#elif testcase == 507    // round_to_int32
inline rtype testFunction(vtype const& a, vtype const& b) { return round_to_int32(b); }
rtype referenceFunction(vtype a, vtype b) {
    ST x; RT r; rtype y(0);
    int vsize = vtype::size();  // size of input vector
    int rsize = rtype::size();  // size of output vector
    int i;
    for (i = 0; i < vsize && i < rsize; i++) {
        x = b[i];
        if (x >= 0) {
            r = (RT)(x + 0.5);                 // 0.5 rounds up
            if ((r & 1) && r - x == 0.5) r--;  // round down if result is odd
        }
        else {
            r = (RT)(x - 0.5);                 // 0.5 rounds up
            if ((r & 1) && x - r == 0.5) r++;  // round up if result is odd
        }
        y.insert(i, r);
    }
    return y;
}
#define WHOLE_VECTOR   // test whole vector, not individual elements


#elif testcase == 508    // truncate_to_int32
inline rtype testFunction(vtype const& a, vtype const& b) { return truncate_to_int32(b); }
rtype referenceFunction(vtype a, vtype b) {
    ST x; RT r; rtype y(0);
    int vsize = vtype::size();  // size of input vector
    int rsize = rtype::size();  // size of output vector
    int i;
    for (i = 0; i < vsize && i < rsize; i++) {
        x = b[i];
        if (x >= 0) {
            r = (RT)(x);                 // truncate towards zero
        }
        else {
            r = -(RT)(-x);
        }
        y.insert(i, r);
    }
    return y;
}
#define WHOLE_VECTOR   // test whole vector, not individual elements

#elif testcase == 509    // round_to_int32 with two parameters
inline rtype testFunction(vtype const& a, vtype const& b) { return round_to_int32(a,b); }
rtype referenceFunction(vtype a, vtype b) {
    ST x; RT r; rtype y(0);
    int vsize = vtype::size();  // size of input vector
    int rsize = rtype::size();  // size of output vector
    int i;
    for (i = 0; i < rsize; i++) {
        if (i < vsize) {         
            x = a[i];
        }
        else {
            x = b[i-vsize];
        }
        if (x >= 0) {
            r = (RT)(x + 0.5);                 // 0.5 rounds up
            if ((r & 1) && r - x == 0.5) r--;  // round down if result is odd
        }
        else {
            r = (RT)(x - 0.5);                 // 0.5 rounds up
            if ((r & 1) && x - r == 0.5) r++;  // round up if result is odd
        }
        y.insert(i, r);
    }
    return y;
}
#define WHOLE_VECTOR   // test whole vector, not individual elements

#elif testcase == 510    // truncate_to_int32 with two parameters
inline rtype testFunction(vtype const& a, vtype const& b) { return truncate_to_int32(a,b); }
rtype referenceFunction(vtype a, vtype b) {
    ST x; RT r; rtype y(0);
    int vsize = vtype::size();  // size of input vector
    int rsize = rtype::size();  // size of output vector
    int i;
    for (i = 0; i < rsize; i++) {
        if (i < vsize) {         
            x = a[i];
        }
        else {
            x = b[i-vsize];
        }
        if (x >= 0) {
            r = (RT)(x);                 // truncate towards zero
        }
        else {
            r = -(RT)(-x);
        }
        y.insert(i, r);
    }
    return y;
}
#define WHOLE_VECTOR   // test whole vector, not individual elements



#elif testcase == 511    // to_float (integer to float)
// test with signed and unsigned types
inline rtype testFunction(vtype const& a, vtype const& b) { return to_float(b); }
RT referenceFunction(ST a, ST b) {
    volatile ST b1 = b;  // prevent rounding error in VS 2019
    return RT(b1);
}

#elif testcase == 512    // to_double (integer to double)
// test with signed and unsigned types
inline rtype testFunction(vtype const& a, vtype const& b) { 
    return to_double(b); 
}
RT referenceFunction(ST a, ST b) {
    return RT(b);
}

#elif testcase == 513    // to_float (double to float)
inline rtype testFunction(vtype const& a, vtype const& b) { return to_float(b); }
rtype referenceFunction(vtype const& a, vtype const& b) {
    RT elements[maxvectorsize];
    int i = 0;
    for (i = 0; i < rtype::size() && i < vtype::size(); i++) {
        elements[i] = (RT)b[i];
    }
    for (; i < rtype::size(); i++) {
        elements[i] = 0;
    }
    return rtype().load(elements);
}
#define WHOLE_VECTOR    // test whole vector

#elif testcase == 514    // to_double (float to double)
inline rtype testFunction(vtype const& a, vtype const& b) { 
    return to_double(b); 
}
rtype referenceFunction(vtype const& a, vtype const& b) {
    RT elements[maxvectorsize];
    for (int i = 0; i < vtype::size(); i++) {
        elements[i] = (RT)b[i];
    }
    return rtype().load(elements);
}
#define WHOLE_VECTOR    // test whole vector


// ----------------------------------------------------------------------------
//             Functions without element-to-element correspondence
// ----------------------------------------------------------------------------

#elif testcase == 590  // shift_bytes_up. deprecated version
#define WHOLE_VECTOR   // test whole vector, not individual elements
ST b0;
inline rtype testFunction(vtype const& a, vtype const& b) {
    b0 = b[0];
    return shift_bytes_up(a, b[0]);
}
rtype referenceFunction(vtype const& a, vtype const& b) {
    if (unsigned(b0) > 15) return rtype(0);
    ST elements[sizeof(vtype) / sizeof(ST)];
    a.store(elements);
    int i;
    for (i = a.size() - 1; i >= b0; i--) elements[i] = elements[i - b0];
    for (i = 0; i < b0; i++) elements[i] = 0;
    return rtype().load(elements);
}

#elif testcase == 591   // shift_bytes_down. deprecated version
#define WHOLE_VECTOR    // test whole vector, not individual elements
ST b0;
inline rtype testFunction(vtype const& a, vtype const& b) {
    b0 = b[0];
    return shift_bytes_down(a, b[0]);
}
rtype referenceFunction(vtype const& a, vtype const& b) {
    if (unsigned(b0) > 15) return rtype(0);
    ST elements[sizeof(vtype) / sizeof(ST)];
    a.store(elements);
    int i;
    for (i = 0; i < a.size() - b0; i++) elements[i] = elements[i + b0];
    for (; i < a.size(); i++) elements[i] = 0;
    return rtype().load(elements);
}

#elif testcase == 600   // concatenate vectors
#define WHOLE_VECTOR    // test whole vector, not individual elements
inline rtype testFunction(vtype const& a, vtype const& b) { return rtype(a, b); }
rtype referenceFunction(vtype const& a, vtype const& b) {
    RT elements[sizeof(rtype) / sizeof(RT)];
    a.store(elements);
    b.store(elements + a.size());
    return rtype().load(elements);
}

#elif testcase == 601   // concatenate boolean vectors
#define WHOLE_VECTOR    // test whole vector, not individual elements
inline rtype testFunction(vtype const& a, vtype const& b) { return rtype(a, b); }
rtype referenceFunction(vtype const& a, vtype const& b) {
    uint64_t temp;
    temp = to_bits(a) | ((uint64_t)to_bits(b) << a.size());
    rtype r;
    if (rtype::size() <= 8) {
        r.load_bits((uint8_t)temp);
    }
    else if (rtype::size() <= 16) {        
        r.load_bits((uint16_t)temp);
    }
    else if (rtype::size() <= 32) {        
        r.load_bits((uint32_t)temp);
    }
    else {
        r.load_bits(temp);  // ignore warnings here
    }
    return r;
} 

#elif testcase == 602   // get_low
#define WHOLE_VECTOR    // test whole vector, not individual elements
inline rtype testFunction(vtype const& a, vtype const& b) { return b.get_low(); }
rtype referenceFunction(vtype const& a, vtype const& b) {
    RT elements[sizeof(vtype) / sizeof(ST)];
    b.store(elements);
    return rtype().load(elements);
}

#elif testcase == 603   // get_high
#define WHOLE_VECTOR    // test whole vector, not individual elements
inline rtype testFunction(vtype const& a, vtype const& b) { return b.get_high(); }
rtype referenceFunction(vtype const& a, vtype const& b) {
    RT elements[sizeof(vtype) / sizeof(ST)];
    b.store(elements);
    return rtype().load(elements + b.size() / 2);
}

#elif testcase == 604   // get_low (boolean vector)
#define WHOLE_VECTOR    // test whole vector, not individual elements
inline rtype testFunction(vtype const& a, vtype const& b) { return b.get_low(); }
rtype referenceFunction(vtype const& a, vtype const& b) {
    uint64_t temp;
    temp = to_bits(b);
    rtype r;
    if (rtype::size() <= 8) {
        r.load_bits((uint8_t)temp);
    }
    else if (rtype::size() <= 16) {        
        r.load_bits((uint16_t)temp);
    }
    else if (rtype::size() <= 32) {        
        r.load_bits((uint32_t)temp);
    }
    else {
        r.load_bits(temp);  // ignore warnings here
    }
    return r;
}

#elif testcase == 605   // get_high (boolean vector)
#define WHOLE_VECTOR    // test whole vector, not individual elements
inline rtype testFunction(vtype const& a, vtype const& b) { return b.get_high(); }
rtype referenceFunction(vtype const& a, vtype const& b) {
    uint64_t temp;
    temp = (uint64_t)to_bits(b) >> rtype::size();
    rtype r;
    if (rtype::size() <= 8) {
        r.load_bits((uint8_t)temp);
    }
    else if (rtype::size() <= 16) {        
        r.load_bits((uint16_t)temp);
    }
    else if (rtype::size() <= 32) {        
        r.load_bits((uint32_t)temp);
    }
    else {
        r.load_bits(temp);  // ignore warnings here
    }
    return r;
}

#elif testcase == 606   // extend (vector size doubled)
inline rtype testFunction(vtype const& a, vtype const& b) { return extend(b); }
rtype referenceFunction(vtype const& a, vtype const& b) {
    RT elements[vtype::size()];
    for (int i = 0; i < vtype::size(); i++) {
        elements[i] = (RT)b[i];
    }
    return rtype().load(elements);
}
#define WHOLE_VECTOR    // test whole vector

#elif testcase == 607   // extend_low (signed, unsigned, and float)
#define WHOLE_VECTOR    // test whole vector, not individual elements
inline rtype testFunction(vtype const& a, vtype const& b) { return extend_low(b); }
rtype referenceFunction(vtype const& a, vtype const& b) {
    RT elements[sizeof(vtype) / sizeof(ST)];
    for (int i = 0; i < b.size() / 2; i++) elements[i] = b[i];
    return rtype().load(elements);
}

#elif testcase == 608   // extend_high (signed, unsigned, and float)
#define WHOLE_VECTOR    // test whole vector, not individual elements
inline rtype testFunction(vtype const& a, vtype const& b) { return extend_high(b); }
rtype referenceFunction(vtype const& a, vtype const& b) {
    RT elements[sizeof(vtype) / sizeof(ST)];
    for (int i = 0; i < b.size() / 2; i++) elements[i] = b[i + b.size() / 2];
    return rtype().load(elements);
}

#elif testcase == 609   // compress (vector size halved
inline rtype testFunction(vtype const& a, vtype const& b) { return compress(b); }
rtype referenceFunction(vtype const& a, vtype const& b) {
    RT elements[vtype::size()];
    for (int i = 0; i < vtype::size(); i++) {
        elements[i] = (RT)b[i];
    }
    return rtype().load(elements);
}
#define WHOLE_VECTOR    // test whole vector

#elif testcase == 610   // compress (signed, unsigned, and float)
#define WHOLE_VECTOR    // test whole vector, not individual elements
inline rtype testFunction(vtype const& a, vtype const& b) { return compress(a, b); }
rtype referenceFunction(vtype const& a, vtype const& b) {
    RT elements[sizeof(rtype) / sizeof(RT)];  int i;
    for (i = 0; i < a.size(); i++) elements[i] = RT(a[i]);
    for (; i < a.size() * 2; i++) elements[i] = RT(b[i - a.size()]);
    return rtype().load(elements);
}

#elif testcase == 611   // compress_saturated (signed, unsigned)
#define WHOLE_VECTOR    // test whole vector, not individual elements
inline rtype testFunction(vtype const& a, vtype const& b) { return compress_saturated(a, b); }
rtype referenceFunction(vtype const& a, vtype const& b) {
    ST elements1[sizeof(rtype) / sizeof(RT)];
    RT elements2[sizeof(rtype) / sizeof(RT)];
    a.store(elements1); b.store(elements1 + a.size());
    for (int i = 0; i < rtype::size(); i++) {
        elements2[i] = (RT)elements1[i];
        if (elements2[i] != elements1[i]) { // overflow. make saturation
            if (RT(-1) < 0) { // signed type
                elements2[i] = (RT(1) << (sizeof(RT) * 8 - 1)) - (elements1[i] > 0);
            }
            else { // unsigned type
                elements2[i] = RT(-1);
            }
        }
    }
    return rtype().load(elements2);
}

#elif testcase == 612   // insert (index, value)
#define WHOLE_VECTOR    // test whole vector, not individual elements

inline rtype testFunction(vtype const& a, vtype const& b) { 
    int index = int(b[0]) & (a.size() - 1);
    vtype aa = a;
    aa.insert(index, b[1]);
    return aa;
}
rtype referenceFunction(vtype const& a, vtype const& b) {
    ST aa[maxvectorsize];
    a.store(aa);
    int index = int(b[0]) & (a.size() - 1);
    aa[index] = b[1];
    return rtype().load(aa);
}

#elif testcase == 613   // extract(index)
#define WHOLE_VECTOR    // test whole vector, not individual elements

inline rtype testFunction(vtype const& a, vtype const& b) { 
    int index = int(b[0]) & (a.size() - 1);
    RT aa = a[index];
    return rtype(aa);
}
rtype referenceFunction(vtype const& a, vtype const& b) {
    ST aa[maxvectorsize];
    a.store(aa);
    int index = int(b[0]) & (a.size() - 1);
    return rtype(aa[index]);
} 

#elif testcase == 614   // cutoff(index)
#define WHOLE_VECTOR    // test whole vector, not individual elements

inline rtype testFunction(vtype const& a, vtype const& b) { 
    int index = uint32_t(b[0]) % uint32_t(a.size() + 1);
    vtype aa = a;
    return aa.cutoff(index);
}
rtype referenceFunction(vtype const& a, vtype const& b) {
    ST aa[maxvectorsize];
    a.store(aa);
    int index = uint32_t(b[0]) % uint32_t(a.size() + 1);
    for (int i = index; i < a.size(); i++) aa[i] = 0;
    return rtype().load(aa);
} 

#elif testcase == 615   // load_partial(index, p)
#define WHOLE_VECTOR    // test whole vector, not individual elements

inline rtype testFunction(vtype const& a, vtype const& b) { 
    int index = uint32_t(b[0]) % uint32_t(a.size() + 1);
    ST aa[maxvectorsize];
    a.store(aa);
    vtype y;
    return y.load_partial(index, aa);
}
rtype referenceFunction(vtype const& a, vtype const& b) {
    int index = uint32_t(b[0]) % uint32_t(a.size() + 1);
    ST aa[maxvectorsize];
    a.store(aa);
    vtype y;
    y.load(aa);
    return y.cutoff(index);
} 

#elif testcase == 616   // store_partial(index, p)
#define WHOLE_VECTOR    // test whole vector, not individual elements

inline rtype testFunction(vtype const& a, vtype const& b) { 
    int index = uint32_t(b[0]) % uint32_t(a.size() + 1);
    ST aa[maxvectorsize];
    for (int i=0; i<maxvectorsize; i++) aa[i] = ST(99);
    a.store_partial(index, aa);
    return vtype().load(aa);
}
rtype referenceFunction(vtype const& a, vtype const& b) {
    int index = uint32_t(b[0]) % uint32_t(a.size() + 1);
    ST aa[maxvectorsize];
    a.store(aa);
    for (int i = index; i < maxvectorsize; i++) aa[i] = ST(99);
    return vtype().load(aa);
} 

// Not in version 1.xx:
#elif testcase == 650 && VECTORCLASS_H >= 20000  // constructor with all elements
#define WHOLE_VECTOR    // test whole vector, not individual elements

template <typename V, typename E>
V construct_from_elements(E const * p) {
    if constexpr (V::size() == 2) {
        return V(p[0], p[1]);
    }
    else if constexpr (V::size() == 4) {
        return V(p[0], p[1], p[2], p[3]);
    }
    else if constexpr (V::size() == 8) {
        return V(p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
    }
    else if constexpr (V::size() == 16) {
        return V(p[0], p[1], p[2],  p[3],  p[4],  p[5],  p[6],  p[7], 
            p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
    }
    else if constexpr (V::size() == 32) {
        return V(p[0], p[1], p[2],  p[3],  p[4],  p[5],  p[6],  p[7], 
            p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15], 
            p[16], p[17], p[18], p[19], p[20], p[21], p[22], p[23], 
            p[24], p[25], p[26], p[27], p[28], p[29], p[30], p[31]);
    }
    else {
        static_assert(V::size() == 64); // no other sizes allowed
        return V(p[0], p[1], p[2],  p[3],  p[4],  p[5],  p[6],  p[7], 
            p[8],  p[9],  p[10], p[11], p[12], p[13], p[14], p[15], 
            p[16], p[17], p[18], p[19], p[20], p[21], p[22], p[23], 
            p[24], p[25], p[26], p[27], p[28], p[29], p[30], p[31],
            p[32], p[33], p[34], p[35], p[36], p[37], p[38], p[39], 
            p[40], p[41], p[42], p[43], p[44], p[45], p[46], p[47], 
            p[48], p[49], p[50], p[51], p[52], p[53], p[54], p[55], 
            p[56], p[57], p[58], p[59], p[60], p[61], p[62], p[63]); 
    }
}

inline rtype testFunction(vtype const a, vtype const b) { 
    ST aa[maxvectorsize];
    b.store(aa);
    return construct_from_elements<vtype,ST>(aa);
}

rtype referenceFunction(vtype const& a, vtype const& b) {
    return b;
} 


#else
// End of test cases
#error unknown test case
#endif


// ----------------------------------------------------------------------------
//                           Overhead functions
// ----------------------------------------------------------------------------

const int maxerrors = 10;      // maximum errors to report
int numerr = 0;                // count errors

// type-specific load function
template <typename T, typename E>
inline void loadData(T & x, E const* p) {
    x.load(p);
}

template <typename T>
inline void loadData(T & x, bool const* p) {
    for (int i = 0; i < x.size(); i++) {
        x.insert(i, p[i]);  // bool vectors have no load function
    }
}


// type-specific printing functions
void printVal(int8_t x) {
    if (x < 0) {
        printf("-%X", -x);
    }
    else {
        printf("%X", x);
    }
}

void printVal(uint8_t x) {
    printf("%X", x);
}

void printVal(int16_t x) {
    if (x < 0) {
        printf("-%X", -x);
    }
    else {
        printf("%X", x);
    }
}

void printVal(uint16_t x) {
    printf("%X", x);
}

void printVal(int32_t x) {
    if (x < 0) {
        printf("-%X", -x);
    }
    else {
        printf("%X", x);
    }
}

void printVal(uint32_t x) {
    printf("%X", x);
}

void printVal(int64_t x) {
    if (x < 0) {
        printf("-%llX", -(long long)x);
    }
    else {
        printf("%llX", (long long)x);
    }
}

void printVal(uint64_t x) {
    printf("%llX", (unsigned long long)x);
}

void printVal(float x) {
    printf("%10.7G", x);
}

void printVal(double x) {
    printf("%10.7G", x);
}

void printVal(bool x) {
    printf("%i", (int)x);
}

// Random number generator
class ranGen {
    // parameters for multiply-with-carry generator
    uint64_t x, carry;
public:
    ranGen(int Seed) {  // constructor
        x = Seed;  carry = 1765;  //initialize with seed
        next();  next();
    }
    uint32_t next() {  // get next random number, using multiply-with-carry method
        const uint32_t fac = 3947008974u;
        x = x * fac + carry;
        carry = x >> 32;
        x = uint32_t(x);
        return uint32_t(x);
    }
};

template <typename T>  // get random number of type T
T get_random(ranGen & rangen) {
    return (T)rangen.next();
}

template <>  // special case uint64_t
uint64_t get_random<uint64_t>(ranGen & rangen) {
    uint64_t xx;
    xx = (uint64_t)rangen.next() << 32;
    xx |= rangen.next();
    return xx;
}

template <>  // special case int64_t
int64_t get_random<int64_t>(ranGen & rangen) {
    return (int64_t)get_random<uint64_t>(rangen);
}

template <>  // special case float
float get_random<float>(ranGen & rangen) {
    union Uif {
        uint32_t i;
        float f;
    };
    Uif u1, u2;
    uint32_t r = rangen.next();                  // get 32 random bits
    // Insert exponent and random mantissa to get random number in the interval 1 <= x < 2
    // Subtract 1.0 if next bit is 0, or 1.0 - 2^-24 = 0.99999994f if next bit is 1
    u1.i = 0x3F800000 - ((r >> 8) & 1);          // bit 8
    u2.i = (r >> 9) | 0x3F800000;                // bit 9 - 31
    return u2.f - u1.f;
}

template <>  // special case float
double get_random<double>(ranGen & rangen) {
    union Uqd {
        uint64_t q;
        double d;
    };
    Uqd u1;
    uint64_t r = get_random<uint64_t>(rangen);   // get 64 random bits
    // Insert exponent and random mantissa to get random number in the interval 1 <= x < 2,
    // then subtract 1.0 to get the interval 0 <= x < 1.
    u1.q = (r >> 12) | 0x3FF0000000000000;       // bit 12 - 63
    return u1.d - 1.0;
}
template <>  // special case bool
bool get_random<bool>(ranGen & rangen) {
    return (rangen.next() & 1) != 0;
}


// make random number generator instance
ranGen ran(seed);

// bit_cast function to make special values
float bit_castf(uint32_t x) {  // uint64_t -> double
    union {
        uint32_t i;
        float f;
    } u;
    u.i = x;
    return u.f;
}

double bit_castd(uint64_t x) {  // uint32_t -> float
    union {
        uint64_t i;
        double f;
    } u;
    u.i = x;
    return u.f;
}


// template to generate list of testdata
template <typename T>
class TestData {
public:
    enum LS {
        // define array size. Must be a multiple of vector size:
        listsize = 1024
    };
    TestData() {                            // constructor
        int i;                              // loop counter
        if (T(1.1f) != 1) {
            // floating point type
            // fill boundary data into array
            for (i = 0; i < 20; i++) {
                list[i] = T((i - 4) * T(0.25));
            }
#ifdef TESTNAN   // test also with NAN, INF, and other special data
            // additional special values, float:
            if constexpr (sizeof(ST) == 4) {
                list[i++] = (T)bit_castf(0x80000000);   // -0
                list[i++] = (T)bit_castf(0x00800000);   // smallest positive normal number
                list[i++] = (T)bit_castf(0x80800000);   // largest negative normal number
                list[i++] = (T)bit_castf(0x3F7FFFFF);   // nextafter 1.0, 0
                list[i++] = (T)bit_castf(0x3F800001);   // nextafter 1.0, 2
                list[i++] = (T)bit_castf(0x7F800000);   // inf
                list[i++] = (T)bit_castf(0xFF800000);   // -inf
                list[i++] = (T)bit_castf(0x7FF00000);   // nan
            }
            else { // double
                list[i++] = (T)bit_castd(0x8000000000000000);   // -0
                list[i++] = (T)bit_castd(0x0010000000000000);   // smallest positive normal number
                list[i++] = (T)bit_castd(0x8010000000000000);   // largest negative normal number
                list[i++] = (T)bit_castd(0x3FEFFFFFFFFFFFFF);   // nextafter 1.0, 0
                list[i++] = (T)bit_castd(0x3FF0000000000001);   // nextafter 1.0, 2
                list[i++] = (T)bit_castd(0x7FF0000000000000);   // inf
                list[i++] = (T)bit_castd(0xFFF0000000000000);   // -inf
                list[i++] = (T)bit_castd(0x7FFC000000000000);   // nan
            }
#endif
            // fill random data into rest of array
            for (; i < listsize; i++) {
                if (vtype::elementtype() > 3) {                
                    list[i] = get_random<T>(ran) * (T)100;
                }
                else {  // bool
                    list[i] = get_random<T>(ran);
                }
            }
        }
        else {
            // integer type
            // fill boundary data into array
            for (i = 0; i < 6; i++) {
                list[i] = T(i - 2);
            }
            // data near mid-point of unsigned integers, or overflow point of signed integers:
            uint64_t m = (uint64_t(1) << (sizeof(T) * 8 - 1)) - 2;
            for (; i < 11; i++) {
                list[i] = T(m++);
            }
            // fill random data into rest of array
            for (; i < listsize; i++) {
                list[i] = get_random<T>(ran);
            }
        }
    }
    T list[listsize];                       // array of test data
    int size() {                            // get list size
        return listsize;
    }
};


#ifdef FACCURACY
// get value of least significant bit
float delta_unit(float x) {
    union {
        float f;
        uint32_t i;
    } u;
    x = std::fabs(x);
    Vec4f xv = Vec4f(x);
    if (!(is_finite(xv)[0])) return 1.f;
    if (x == 0.f || is_subnormal(xv)[0]) {
        u.i = 0x00800000;  // smallest positive normal number
        return u.f;
    }
    float x1 = x;
    u.f = x;
    u.i++;
    return u.f - x1;
}

double delta_unit(double x) {
    union {
        double f;
        uint64_t i;
    } u;
    x = std::fabs(x);
    Vec2d xv = Vec2d(x);
    if (!(is_finite(xv)[0])) return 1.;
    if (x == 0. || is_subnormal(xv)[0]) {
        u.i = 0x0010000000000000;  // smallest positive normal number
        return u.f;
    }
    double x1 = x;
    u.f = x;
    u.i++;
    return u.f - x1;
}
#endif


// compare two scalars. return true if different
template <typename T>
inline bool compare_scalars(T const a, T const b) {
    return a == b;
}

// special cases for float and double:
template <>
inline bool compare_scalars<float>(float const a, float const b) {
    if (a == b || (a != a && b != b)) return true; // return false if equal or both are NAN
#ifdef FACCURACY     // accept minor difference
    float dif = std::fabs(a - b) / delta_unit(a);
    if (dif <= FACCURACY) return true;
    printf("\n%.0f ULP ", dif);
#endif
    return false;
}

template <>
inline bool compare_scalars<double>(double const a, double const b) {
    if (a == b || (a != a && b != b)) return true; // return false if equal or both are NAN
#ifdef FACCURACY     // accept minor difference
    double dif = std::fabs(a - b) / delta_unit(a);
    if (dif <= FACCURACY) return true;
    printf("\n%.0f ULP ", dif);
#endif
    return false;
}

// compare two vectors. return true if different
template <typename T>
inline bool compare_vectors(T const& a, T const& b) {
    {
        for (int i = 0; i < a.size(); i++) {
            if (!compare_scalars(a[i], b[i])) return false;
        }
    }
    return true;
}


// program entry
int main() {
    vtype a, b;                   // operand vectors
    rtype result;                 // result vector
    rtype expected;               // expected result
    const int vectorsize = vtype::size();

#if defined (__linux__) && !defined(__LP64__)
    // Some 32-bit compilers use x87 calculations with long double precision for 
    // the reference function. This may give slightly different results because
    // the value is rounded twice. To get exactly the same value in the test function
    // and the reference function, we change the precision of x87 calculations.
    // (the fpu control function is different in Windows, but the precision is already
    // reduced in Windows anyway)
    fpu_control_t fpcw = 0x27f;
    _FPU_SETCW(fpcw);
#endif

    // make lists of test data
    TestData<ST> adata, bdata;

    // list for expected results
    RT expectedList[maxvectorsize];

    int i, j;   // loop counters

    for (i = 0; i < adata.size(); i += vectorsize) {
        //a.load(adata.list + i);
        loadData(a, adata.list + i);

        for (j = 0; j < bdata.size(); j += vectorsize) {
            loadData(b, bdata.list + j);

#if defined(USE_FLAG)
            vtype f;
            for (int k = 0; k < vectorsize; k++) {
                f.insert(k, ST(k%3 != 0));
            }
            bitfield = get_random<uint64_t>(ran);
            // function under test:
            result = testFunction(f, a, b);
#else
            // function under test:
            result = testFunction(a, b);
#endif

            // expected value to compare with
#if defined(SCALAR_RESULT) || defined(WHOLE_VECTOR)   // result is scalar || test whole vector
            expected = rtype(referenceFunction(a, b));
            expectedList[0] = 0;   // avoid warning for unused
#else       // result is vector
            for (int k = 0; k < vectorsize; k++) {
#if defined(USE_FLAG)
                expectedList[k] = (RT)(referenceFunction(f[k], adata.list[i + k], bdata.list[j + k]));
#else
                expectedList[k] = (RT)(referenceFunction(adata.list[i + k], bdata.list[j + k]));
#endif
            }
            loadData(expected, expectedList);
#endif
            // compare result with expected value
            if (!compare_vectors(result, expected)) {
                // values are different. report error 
                if (++numerr == 1) {
                    printf("\ntest case %i:", testcase);  // print test case first time
                }
                //int numresult = result.size();     // number of elements in result vector
#ifdef SCALAR_RESULT
                //numresult = 1;
#endif 
                printf("\nError at %i, %i:", i, j);
                for (int n = 0; n < result.size(); n++) {
                    printf("\n"); printVal(a[i]); 
                    printf(", "); printVal(b[i]); printf(": "); 
                    if (compare_scalars(result[n], expected[n])) {
                        printf("-> %2i:  ", n);   // elements are equal
                        printVal(result[n]);
                        printf(" == ");
                        printVal(expected[n]);
                    }
                    else {
                        printf("-> %2i:  ", n);   // elements are different
                        printVal(result[n]);
                        printf(" != ");
                        printVal(expected[n]);
                    }
                }
            }
            if (numerr > maxerrors) {
                exit(1);      // stop after maxerrors
            }
        }
    }
    if (numerr == 0) {
        printf("\nsuccess\n");
    }

    printf("\n");

    return numerr;
}
