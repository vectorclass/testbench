/****************************  testbench3.cpp   *******************************
* Author:        Agner Fog
* Date created:  2019-04-11
* Last modified: 2019-11-18
* Version:       2.00
* Project:       Testbench for vector class library, 3: mathematical functions
* Description:
* Compile and run this program to test mathematical functions in VCL
* This file contains test cases for mathematical functions with floating point 
* vector parameters.
* Each function is tested with many different combinations of input data.
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
* (c) Copyright Agner Fog 2019,
* Gnu general public license 3.0 https://www.gnu.org/licenses/gpl.html
******************************************************************************
Test cases:
1:   round
2:   truncate
3:   floor
4:   ceil
5:   square
6:   sqrt
7:   approx_recipr
8:   approx_rsqrt
9:   exponent
10:  fraction
11:  exp2
12:  mul_add
13:  mul_sub_x
20:  is_finite
21:  is_inf
22:  is_nan
23:  is_subnormal
24:  is_zero_or_subnormal
25:  nan_code, nan_vec
100: exp
101: expm1
102: exp2
103: exp10
104: log
105: log1p
106: log2
107: log10
108: cbrt
109: pow_ratio
110: pow_ratio
111: pow_ratio
112: pow_ratio
115: pow_const(vector, const int)
200: sin
201: cos
202: sincos
203: sincos
204: tan
205: asin
206: acos
207: atan
300: sinh
301: cosh
302: tanh
303: asinh
304: acosh
305: atanh
500: pow(vector,vector)
501: pow(vector,scalar)
502: pow(vector,int)
510: atan2(vector,vector)
511: maximum(vector,vector)
512: minimum(vector,vector)

*****************************************************************************/

#include <stdio.h>
#include <float.h>
#include <cmath>

#if (defined(__GNUC__) || defined(__clang__)) && !defined (__CYGWIN__)
#include <fpu_control.h>          // setting FP control word needed only in WSL
#endif

// Uncomment the next line if you want to test signed zeroes:
//#define SIGNED_ZERO             // pedantic about signed zero

#ifndef INSTRSET
#define INSTRSET 8
#endif

#include "vectorclass.h"          // vector class library
#if true                          // true if you want inline mathematical functions
                                  // false if you are using SVML library
#include "vectormath_exp.h"       // power, exponential, and logarithm functions
#include "vectormath_trig.h"      // trigonometric functions
#include "vectormath_hyp.h"       // hyperbolic functions
#else
#include "vectormath_lib.h"       // alternative for SVML library
#endif


#ifndef testcase
// ----------------------------------------------------------------------------
//            Specify input parameters here if running from an IDE
// ----------------------------------------------------------------------------


#define testcase 1

#define vtype Vec4d

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

#ifndef INSTRSET
#define INSTRSET 5
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
ST x0;                            // used with IGNORE_UNDERFLOW

long double pow_accurate(double x, double y); // reference function
uint32_t compare_sign(float a, float b);
uint32_t compare_sign(double a, double b);


/************************************************************************
*
*                          Test cases
*
************************************************************************/

#if   testcase == 1          // round
inline rtype testFunction(vtype const& a) { return round(a); }
RT referenceFunction(ST a) {
    //if (a == 0.f) return a;
    return (RT)rint(a);
}
#define FACCURACY 0          // must be precise
//#define IGNORE_SIGNED_ZERO
#define MAXD      1.E15      // max value for double parameter

#elif   testcase == 2        // truncate
inline rtype testFunction(vtype const& a) { return truncate(a); }
RT referenceFunction(ST a) { return (RT)trunc(a); }
#define FACCURACY 0          // must be precise
//#define IGNORE_SIGNED_ZERO
#define MAXD      1.E15      // max value for double parameter

#elif   testcase == 3        // floor
inline rtype testFunction(vtype const& a) { return floor(a); }
RT referenceFunction(ST a) { return (RT)floor(a); }
#define FACCURACY 0          // must be precise
//#define IGNORE_SIGNED_ZERO 
#define MAXD      1.E15      // max value for double parameter

#elif   testcase == 4        // ceil
inline rtype testFunction(vtype const& a) { return ceil(a); }
RT referenceFunction(ST a) { return (RT)ceil(a); }
#define FACCURACY 0          // must be precise
//#define IGNORE_SIGNED_ZERO 
#define MAXD      1.E15      // max value for double parameter

#elif   testcase == 5        // square
inline rtype testFunction(vtype const& a) { return a * a; }
long double referenceFunction(ST a) { return (long double)a* (long double)a; }

#elif   testcase == 6        // sqrt
inline rtype testFunction(vtype const& a) { return sqrt(a); }
long double referenceFunction(ST a) { 
    //return sqrtl(a); 
    return std::sqrt(a); 
}

#elif   testcase == 7        // approx_recipr
inline rtype testFunction(vtype const& a) { return approx_recipr(a); }
long double referenceFunction(ST a) { return (long double)1. / a; }
#define FACCURACY 8192       // imprecision expected

#elif   testcase == 8        // approx_rsqrt
inline rtype testFunction(vtype const& a) { return approx_rsqrt(a); }
long double referenceFunction(ST a) { 
    return 1. / sqrt(a); 
}
#define FACCURACY 8192       // imprecision expected

#elif   testcase == 9        // exponent
inline rtype testFunction(vtype const& a) { return exponent(a); }

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__)
__attribute__ ((optimize("-fno-unsafe-math-optimizations"))) 
#elif defined(__clang__)
__attribute__((optnone))
#endif

RT referenceFunction(float a) {
    if (a == 0) return -0x7F;
    if (std::isnan(a) || std::isinf(a)) return 0x80; // does not work with -ffast-math
    if (!is_finite(vtype(a))[0]) return 0x80;
    int n;
    frexp(a, &n);
    return n - 1;
}

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__)
__attribute__ ((optimize("-fno-unsafe-math-optimizations"))) 
#elif defined(__clang__)
__attribute__((optnone))
#endif

RT referenceFunction(double a) {
    if (a == 0) return -0x3FF;
    if (std::isnan(a) || std::isinf(a)) return 0x400;
    if (!(is_finite(vtype(a)))[0]) return 0x400;
    int n;
    frexp(a, &n);
    return n - 1;
}
#define FACCURACY 0          // precision required

#elif   testcase == 10       // fraction
inline rtype testFunction(vtype const& a) { return fraction(a); }
long double referenceFunction(ST a) {
    if (a == 0) return 1.;
    if (is_inf(vtype(a))[0]) return 1.;
    int n;
    return std::fabs(2. * frexp(a, &n));
}
#define FACCURACY 0          // precision required
#define IGNORE_NAN           // result for NAN input is implementation dependent

#elif   testcase == 11       // exp2
inline rtype testFunction(vtype const& a) { return exp2(a); }
long double referenceFunction(ST a) {
    return powl(2., a);
}
#define FACCURACY 0          // precision required

#elif   testcase == 12       // mul_add
inline rtype testFunction(vtype const& a) { return mul_add(a, a, a); }
long double referenceFunction(ST a) {
    return (long double)a* a + a;
}
#define FACCURACY 8          // expected precision

#elif   testcase == 13       // mul_sub_x
inline rtype testFunction(vtype const& a) { return mul_sub_x(a, a, a); }
long double referenceFunction(ST a) {
    return (long double)a* a - a;
}
#define FACCURACY 60         // precision may be bad in some cases!
#define IGNORE_NAN           // INF input may give INF or NAN output

#elif   testcase == 20       // is_finite

inline rtype testFunction(vtype const& a) { return is_finite(a); }

long double referenceFunction(ST a) {
    return std::isfinite(a);
}
#define FACCURACY 0          // expected precision

#elif   testcase == 21       // is_inf
inline rtype testFunction(vtype const& a) { return is_inf(a); }
long double referenceFunction(ST a) {
    if (a != a) return 0.;   // nan
    return !std::isfinite(a);
}
#define FACCURACY 0          // expected precision

#elif   testcase == 22       // is_nan
inline rtype testFunction(vtype const& a) { return is_nan(a); }
long double referenceFunction(ST a) {
    if (a != a) return 1.;   // nan
    return 0.;
}
#define FACCURACY 0          // expected precision

#elif   testcase == 23       // is_subnormal
inline rtype testFunction(vtype const& a) { return is_subnormal(a * (ST)0.125); }
long double referenceFunction(ST a) {
    return (std::fpclassify(a * (ST)0.125) == FP_SUBNORMAL);
}
#define FACCURACY 0          // expected precision

#elif   testcase == 24       // is_zero_or_subnormal
inline rtype testFunction(vtype const& a) { return is_zero_or_subnormal(a * (ST)0.125); }
long double referenceFunction(ST a) {
    ST b = a * (ST)0.125;
    if (b == 0) return 1.;
    return (std::fpclassify(b) == FP_SUBNORMAL);
}
#define FACCURACY 0          // expected precision

#elif   testcase == 25       // nan_code, nan_vec

uint32_t a0i;
inline rtype testFunction(vtype const& a) {
    auto ai = roundi(a);
    a0i = ai[0] & 0x003FFFFF;
    vtype b = nan_vec<vtype>(a0i);
    rtype c = nan_code(b);
    return c;
}
long double referenceFunction(ST a) {
    return a0i | 0x00400000;
}
#define FACCURACY 0          // expected precision


// ----------------------------------------------------------------------------
//             Powers, exponential functions and logarithms: vectormath_exp.h
// ----------------------------------------------------------------------------

#elif   testcase == 100      // exp
inline rtype testFunction(vtype const& a) { return exp(a); }
long double referenceFunction(ST a) { return expl(a); }
#ifdef VECTORMATH_LIB_H
#define FACCURACY 4          // expected precision
#else
#define FACCURACY 2          // expected precision
#endif
#define MAXF      87.f       // max value for float parameter
#define MAXD      708.       // max value for double parameter


#elif   testcase == 101      // expm1
inline rtype testFunction(vtype const& a) { return expm1(a); }
long double referenceFunction(ST a) { 
    return expm1l(a); 
}
#define FACCURACY 3          // expected precision
#define MAXF      87.f       // max value for float parameter
#define MAXD      708.       // max value for double parameter

#elif   testcase == 102      // exp2
inline rtype testFunction(vtype const& a) { return exp2(a); }
long double referenceFunction(ST a) { 
    return exp2l(a); 
}
#define FACCURACY 3          // expected precision
#define MAXF      27.f       // max value for float parameter
#define MAXD      1020.      // max value for double parameter

#elif   testcase == 103      // exp10
inline rtype testFunction(vtype const& a) { return exp10(a); }
long double referenceFunction(ST a) {
#define powLL pow_accurate
    long double y = 1.;
    if (a > 2.) {            // loop calculation for better precision
        int cnt = (int)a;
        for (int i=0; i < cnt; i++) {
            y *= 10.;
        }
        return y * powLL(10.,a-(double)cnt);    
    }
    else if (a < -2.) {
        int cnt = (int)(-a);
        for (int i=0; i < cnt; i++) {
            y *= 10.;
        }
        return powLL(10., a+(double)cnt) / y;
    }
    return powLL(10.f, a); 
}
#define FACCURACY 10         // poor precision in library version
#define YACCURACY 0
#define MAXF      36.f       // max value for float parameter
#define MAXD      307.       // max value for double parameter

#elif   testcase == 104      // log
inline rtype testFunction(vtype const& a) { return log(a); }
long double referenceFunction(ST a) { return logl(a); }
#define FACCURACY 3          // expected precision

#elif   testcase == 105      // log1p
inline rtype testFunction(vtype const& a) { return log1p(a); }
long double referenceFunction(ST a) { return log1pl(a); }
#define FACCURACY 2          // expected precision

#elif   testcase == 106      // log2
inline rtype testFunction(vtype const& a) { return log2(a); }
long double referenceFunction(ST a) { return log2l(a); }
#define FACCURACY 2          // expected precision

#elif   testcase == 107      // log10
inline rtype testFunction(vtype const& a) { return log10(a); }
long double referenceFunction(ST a) { return log10l(a); }
#define FACCURACY 3          // expected precision

#elif   testcase == 108      // cube root
inline rtype testFunction(vtype const& a) { return cbrt(a); }
long double referenceFunction(ST a) { return cbrtl(a); }

#define FACCURACY 5          // expected precision
#define MAXF      1.E29      // max value for float parameter
#define MAXD      1.E200     // max value for double parameter
#define USE_ABSOLUTE_ERROR   // ignore extremely small values

#elif   testcase >= 109 && testcase < 115  // pow_ratio
    // define subcases
    #if   testcase == 109
    constexpr int A = -2;
    constexpr int B =  3;
    #elif testcase == 110
    constexpr int A = -1;
    constexpr int B =  4;
    #elif testcase == 111
    constexpr int A =  5;
    constexpr int B =  6;
    #elif testcase == 112
    constexpr int A =  4;
    constexpr int B =  7;
    #endif 

inline rtype testFunction(vtype const& x) { return pow_ratio(x, A, B); }
long double referenceFunction(ST x) {
    x0 = (ST)x;
    ST xx = x;
    if ((B & 1) && xx < 0) xx = -xx; // negative x allowed when B is odd
    long double y = powl(xx, (long double)A / (long double)B); 
    if ((B & 1) != 0 && (A & 1) != 0 && compare_sign(x0, 0)) y = -y; // get sign of x if A and B odd
    return y;
}
#define FACCURACY 300        // poor precision in extreme cases
#define MAXF      1.E20      // max value for float parameter
#define MAXD      1.E100     // max value for double parameter
#define USE_ABSOLUTE_ERROR   // ignore extremely small values
#define IGNORE_OVERFLOW
#define IGNORE_UNDERFLOW


#elif   testcase == 115      // pow_const(vector, const int)
inline rtype testFunction(vtype const& a) { return pow_const(a, -3); }
long double referenceFunction(ST a) { return powl(a, -3); }
#define FACCURACY 3 //5


// ----------------------------------------------------------------------------
//             Trigonometric functions: vectormath_trig.h
// ----------------------------------------------------------------------------
#elif   testcase == 200      // sin
inline rtype testFunction(vtype const& a) { return sin(a); }
long double referenceFunction(ST a) { 
    return sinl(a); }
#define FACCURACY 3          // desired accuracy
#define MAXF      1.E4       // max value for float parameter
#define MAXD      1.E4       // max value for double parameter
#define USE_ABSOLUTE_ERROR

#elif   testcase == 201      // cos
inline rtype testFunction(vtype const& a) { return cos(a); }
long double referenceFunction(ST a) { return cosl(a); }
#define FACCURACY 3          // desired accuracy
#define MAXF      1.E4       // max value for float parameter
#define MAXD      1.E4       // max value for double parameter
#define USE_ABSOLUTE_ERROR

#elif   testcase == 202      // sincos
inline rtype testFunction(vtype const& a) {
    vtype c;
    return sincos(&c, a);
}
long double referenceFunction(ST a) { return sinl(a); }
#define FACCURACY 3          // desired accuracy
#define MAXF      1.E4       // max value for float parameter
#define MAXD      1.E4       // max value for double parameter
#define USE_ABSOLUTE_ERROR

#elif   testcase == 203      // sincos
inline rtype testFunction(vtype const& a) {
    vtype c;
    sincos(&c, a);
    return c;
}
long double referenceFunction(ST a) { return cosl(a); }
#define FACCURACY 3          // desired accuracy
#define MAXF      1.E4       // max value for float parameter
#define MAXD      1.E4       // max value for double parameter
#define USE_ABSOLUTE_ERROR

#elif   testcase == 204      // tan
inline rtype testFunction(vtype const& a) { return tan(a); }
long double referenceFunction(ST a) { return std::tan((long double)a); }
#define FACCURACY 3 //!20    // desired accuracy. usually below 3. 
#define MAXF      1.E4       // max value for float parameter
#define MAXD      1.E4       // max value for double parameter

#elif   testcase == 205      // asin
inline rtype testFunction(vtype const& a) { return asin(a); }
long double referenceFunction(ST a) { return asinl(a); }
#define FACCURACY 3          // desired accuracy
#define MAXF      1.         // max value for float parameter
#define MAXD      1.         // max value for double parameter

#elif   testcase == 206      // acos
inline rtype testFunction(vtype const& a) { return acos(a); }
long double referenceFunction(ST a) { return acosl(a); }
#define FACCURACY 3          // desired accuracy
#define MAXF      1.         // max value for float parameter
#define MAXD      1.         // max value for double parameter

#elif   testcase == 207      // atan
inline rtype testFunction(vtype const& a) { return atan(a); }
long double referenceFunction(ST a) { return atanl(a); }
#define FACCURACY 3          // desired accuracy


// ----------------------------------------------------------------------------
//             Hyperbolic functions: vectormath_hyp.h
// ----------------------------------------------------------------------------

#elif   testcase == 300      // sinh
inline rtype testFunction(vtype const& a) { return sinh(a); }
long double referenceFunction(ST a) { return sinhl(a); }
#define FACCURACY 2          // desired accuracy
#define MAXF      88         // max value for float parameter
#define MAXD      709        // max value for double parameter

#elif   testcase == 301      // cosh
inline rtype testFunction(vtype const& a) { return cosh(a); }
long double referenceFunction(ST a) { 
    return coshl(a);         // avoid -INF return
}
//#define IGNORE_INF_SIGN    // why do I see coshl(-INF) = -INF??
#define FACCURACY 2          // desired accuracy
#define MAXF      88         // max value for float parameter
#define MAXD      709        // max value for double parameter

#elif   testcase == 302      // tanh
inline rtype testFunction(vtype const& a) { return tanh(a); }
long double referenceFunction(ST a) { return tanhl(a); }
#define FACCURACY 2          // desired accuracy

#elif   testcase == 303      // asinh
inline rtype testFunction(vtype const& a) { return asinh(a); }
long double referenceFunction(ST a) { return asinhl(a); }
#define FACCURACY 3          // desired accuracy

#elif   testcase == 304      // acosh
inline rtype testFunction(vtype const& a) { return acosh(a); }
long double referenceFunction(ST a) { return acoshl(a); }
#define FACCURACY 3          // desired accuracy

#elif   testcase == 305      // atanh
inline rtype testFunction(vtype const& a) { return atanh(a); }
long double referenceFunction(ST a) { return atanhl(a); }
#define FACCURACY 2          // desired accuracy
#define MAXF      1          // max value for float parameter
#define MAXD      1          // max value for double parameter


// ----------------------------------------------------------------------------
//             Functions with two parameters
// ----------------------------------------------------------------------------

#elif   testcase == 500      // pow(vector,vector)

inline rtype testFunction(vtype const& a, vtype const& b) { return pow(a, b); }
long double referenceFunction(ST a, ST b) { 
    return pow_accurate(a, b); }
#define FACCURACY 2
#define YACCURACY 0.6        // accuracy relative to second parameter
// The high error is in the reference library on Gnu and Clang in double precision. 
// MS shows no big error, and no big error in single precision
#define MAXF      1.E7       // max value for float parameter
#define MAXD      1.E10      // max value for double parameter
#define TWO_PARAMETERS       // has two parameters

#elif   testcase == 501      // pow(vector,scalar)
ST b0;
inline rtype testFunction(vtype const& a, vtype & b) { 
    b0 = b[0];
    b = b0;  // reset b for the sake of YACCURACY 
    return pow(a, b0); 
}
long double referenceFunction(ST a, ST b) { 
    return pow_accurate(a, b0); }
#define FACCURACY 2
#define YACCURACY 0.6        // accuracy relative to second parameter
#define MAXF      1.E7       // max value for float parameter
#define MAXD      1.E10      // max value for double parameter
#define TWO_PARAMETERS       // has two parameters

#elif   testcase == 502      // pow(vector,int)
int bi0;
inline rtype testFunction(vtype const& a, vtype & b) { 
    bi0 = int(b[0]);
    if (bi0 == 0x80000000) bi0--;  // avoid integer overflow
    b = ST(bi0);
    return pow(a, bi0); }
long double referenceFunction(ST a, ST b) { 
    return pow_accurate(a, (double)bi0); }
#define FACCURACY 2
#define YACCURACY 0.8        // accuracy relative to second parameter
#define MAXF      1.E6       // max value for float parameter
#define MAXD      1.E6       // max value for double parameter
#define TWO_PARAMETERS       // has two parameters

#elif   testcase == 510      // atan2

inline rtype testFunction(vtype const& a, vtype const& b) { return atan2(a, b); }
long double referenceFunction(ST a, ST b) { return atan2l(a, b); }
#define FACCURACY 3          // desired accuracy
#define TWO_PARAMETERS       // has two parameters

#elif   testcase == 511      // maximum
inline rtype testFunction(vtype const& a, vtype const& b) { return maximum(a, b); }
long double referenceFunction(ST a, ST b) {
    if (a != a || b != b) return a + b;  //propagate nan
    if (a > b) return a;
    if (a < b) return b;
    if (compare_sign(a,b)) return 0;   // +0 and -0
    return a;    
}
#define FACCURACY 0          // desired accuracy
#define TWO_PARAMETERS       // has two parameters

#elif   testcase == 512      // minimum
inline rtype testFunction(vtype const& a, vtype const& b) { return minimum(a, b); }
long double referenceFunction(ST a, ST b) {
    if (a != a || b != b) return a + b;  //propagate nan
    if (a < b) return a;
    if (a > b) return b;
    if (compare_sign(a,b)) return -0.f;   // +0 and -0
    return a;    
}
#define FACCURACY 0          // desired accuracy
#define TWO_PARAMETERS       // has two parameters

#else
// End of test cases
#error unknown test case
#endif

// bit_cast function to make special values
float bit_castf(uint32_t x){ // uint64_t -> double
    union {
        uint32_t i;
        float f;
    } u;
    u.i = x;
    return u.f;
}

double bit_castd(uint64_t x){// uint32_t -> float
    union {
        uint64_t i;
        double f;
    } u;
    u.i = x;
    return u.f;
}

// return 1 if a and b have different sign bit
uint32_t compare_sign(float a, float b) {
    union {
        float f;
        uint32_t i;
} u1, u2;
    u1.f = a;  u2.f = b;
    return (u1.i ^ u2.i) >> 31;
}

// return 1 if a and b have different sign bit
uint32_t compare_sign(double a, double b) {
    union {
        double f;
        uint64_t i;
    } u1, u2;
    u1.f = a;  u2.f = b;
    return uint32_t((u1.i ^ u2.i) >> 63);
}





#ifndef FACCURACY
#define FACCURACY 2          // Desired accuracy in ULP (unit in the last place)
#endif
#ifndef MAXF
#define MAXF      FLT_MAX    // max value for float parameter
#endif
#ifndef MAXD
#define MAXD      DBL_MAX    // max value for double parameter
#endif

#ifdef YACCURACY

// Accurate pow function.
// The powl function WSL is terribly inaccurate if you don't fix the FP control word
// The powl function under Mingw64 is also somewhat inaccurate.
// MS Visual Studio has no long double
// Make a more accurate version of powl if needed:

long double pow_accurate(double x, double y) {
#if false // (defined(__GNUC__) || defined(__clang__)) && !defined(__INTEL_COMPILER) 
    double ya = std::fabs(y);
    uint64_t yi = uint64_t(ya);   // integer part of y (not using round here because it may cause overflow of y1)
    // calculate pow(x,yi)
    long double y1 = 1.0;         // accumulator
    long double xp = std::fabs(x);
    uint64_t yj = yi;
    while (yj != 0) {             // loop for each bit in yi
        if (yj & 1) y1 *= xp;     // multiply if bit = 1
        xp *= xp;                 // xp = x^2, x^4, x^8, etc.
        yj >>= 1;                 // get next bit of yi
    }
    long double y2 = pow(std::fabs(x),ya-yi);   // remaining part of y
    y1 *= y2;
    bool yinteger = y == round(y);
    bool yodd = yinteger && (yi & 1) != 0;
    if (y < 0.) y1 = 1./y1;
    if (x < 0.) {
        if (!yinteger) return bit_castf(0x7FF00000); // nan
        if (yodd) y1 = - y1;      // negative if y is an odd integer
    }
    if (x == 0.) {
        bool minuszero = compare_sign(x,0.) != 0;
        if (y == 0.) return 1.;
        if (y > 0.) y1 = 0.;
        else y1 = bit_castf(0x7F800000); // inf
        if (minuszero && yodd) y1 = -y1;
    }
    if (x == bit_castf(0x7F800000)) { // x is inf
        if (y < 0.) y1 = 0.;
        else if (y == 0.) y1 = 1.;
        else y1 = x;
    }
    return y1;
}

#else  // MS or Intel compilers
    return powl(x,y);
}
#endif
#endif


// ----------------------------------------------------------------------------
//                           Overhead functions
// ----------------------------------------------------------------------------

const int maxerrors = 5;          // maximum errors to report
int numerr = 0;                   // count errors

// type-specific load function
template <typename T, typename E>
inline void loadData(T & x, E const* p) {
    x.load(p);
}

template <typename T>
inline void loadData(T & x, bool const* p) {
    for (int i = 0; i < x.size(); i++) {
        x.insert(i, p[i]);        // bool vectors have no load function
    }
}

template <typename T>
inline void loadData(T & x, long double const* p) {
    for (int i = 0; i < x.size(); i++) {
        x.insert(i, (decltype(x[0]))(p[i]));  // convert each element to float or double
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
    printf("%.7G", x);
}

void printVal(double x) {
    printf("%.7G", x);
}

void printVal(bool x) {
    printf("%i", (int)x);
}

// Random number generator
class ranGen {
    // parameters for multiply-with-carry generator
    uint64_t x, carry;
public:
    ranGen(int Seed) {            // constructor
        x = Seed;  carry = 1765;  //initialize with seed
        next();  next();
    }
    uint32_t next() {             // get next random number, using multiply-with-carry method
        const uint32_t fac = 3947008974u;
        x = x * fac + carry;
        carry = x >> 32;
        x = uint32_t(x);
        return uint32_t(x);
    }
};

template <typename T>             // get random number of type T
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
    uint32_t r = rangen.next();             // get 32 random bits
    // Insert exponent and random mantissa to get random number in the interval 1 <= x < 2
    // Subtract 1.0 if next bit is 0, or 1.0 - 2^-24 = 0.99999994f if next bit is 1
    u1.i = 0x3F800000 - ((r >> 8) & 1);     // bit 8
    u2.i = (r >> 9) | 0x3F800000;           // bit 9 - 31
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



// template to generate list of testdata
template <typename T>
class TestData {
public:
    enum LS {
        // define array size. Must be a multiple of vector size:
#ifdef TWO_PARAMETERS
        listsize = 0x100
#else
        listsize = 0x1000
#endif
    };
    TestData() {                                 // constructor
        int i, j;                                // loop counter
        // floating point type
        // fill sequential data into array
        for (i = 0; i < 20; i++) {
            list[i] = T(i - 4) * T(0.25);
        }
        // additional special values, float:
        if constexpr (sizeof(T) < 8) {
            list[i++] = bit_castf(0x80000000);   // -0
            list[i++] = bit_castf(0x00800000);   // smallest positive normal number
            list[i++] = bit_castf(0x80800000);   // largest negative normal number
            list[i++] = bit_castf(0x3F7FFFFF);   // nextafter 1.0, 0
            list[i++] = bit_castf(0x3F800001);   // nextafter 1.0, 2
            list[i++] = bit_castf(0x7F800000);   // inf
            list[i++] = bit_castf(0xFF800000);   // -inf
            list[i++] = bit_castf(0x7FF00000);   // nan
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

        // very big data
        T   f;  // scale factor
        T   m;  // maximum parameter
        if constexpr (sizeof(T) < 8) {
            f = (T)88.f;
            m = (T)MAXF;
        }
        else {
            f = (T)709.;
            m = (T)MAXD;
        }
        if (m > 100.) {
            // make very big data
            j = i;
            while (i < j + 20 && m > 1000) {
                T r = get_random<T>(ran);        // random
                T x = (T)exp(r * f);             // scale, make big
                if (x > m) continue;             // skip if too big
                list[i] = (i & 4) ? x : -x;
                i++;
            }
        }
        list[i++] = m;                           // test maximum value
        list[i++] = -m;                          // test -maximum value

        // fill random data into rest of array
        T scale = 100.;                          // scale factor
        if (scale > m) scale = m;
        for (; i < listsize; i++) {
            list[i] = get_random<T>(ran) * scale;
        }
    }
    T list[listsize];                            // array of test data
    int size() {                                 // get list size
        return listsize;
    }
};


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


// compare two scalars. return true if different
template <typename T>
inline double compare_scalars(T const a, T const b) { // default case for integers
    return std::fabs(a - b);
}

// special cases for float and double:
template <>
inline double compare_scalars<float>(float const a, float const b) {
    if ((a == b) || (a != a && b != b)) { // a and b equal or both are NAN
        return 0;
    }
    double dif = std::fabs(a - b) / delta_unit(a);
    return dif;
}

template <>
inline double compare_scalars<double>(double const a, double const b) {
    if ((a == b) || (a != a && b != b)) { // a and b equal or both are NAN
        return 0;
    }
    double dif = std::fabs(a - b) / delta_unit(a);
    return dif;
}

inline double compare_scalars(float const a, long double const b) {
#ifdef SIGNED_ZERO
    if (a == 0.f && compare_sign((double)a, (double)b)) {
        return -1.;  // difference in signed zero
    }
#endif
    if ((a == (float)b) || (a != a && b != b)) { // a and b equal or both are NAN
        return 0;
    }
    double dif = std::fabs((long double)a - b) / delta_unit(a);
    return dif;
}

inline double compare_scalars(double const a, long double const b) {
#ifdef SIGNED_ZERO
    if (a == 0. && compare_sign(a, (double)b)) {
        return -1.;  // difference in signed zero
    }
#endif
    if ((a == (double)b) || (a != a && b != b)) { // a and b equal or both are NAN
        return 0;
    }
    double aa = a;
#ifdef USE_ABSOLUTE_ERROR
    if (std::fabs(a) < .01) {
        aa = .01; // relative error becomes too big for periodic functions when result is near zero
    }
#endif
#ifdef IGNORE_OVERFLOW   // use this if test function and reference function have different overflow limits
    if (std::fabs(a) > 1.E306 && std::fabs(b) > 1.E306 && compare_sign(a, (double)b)== 0) {
        return 1;
    }
#endif
    double dif = std::fabs(double((long double)a - b)) / delta_unit(aa);
    return dif;
}

inline double compare_scalars(int32_t const a, long double const b) {
    if ((double)a == (double)b) { // a and b equal
        return 0;
    }
    double dif = std::fabs((double)a - b);
    return dif;
}

inline double compare_scalars(int64_t const a, long double const b) {
    if ((double)a == (double)b) { // a and b equal
        return 0;
    }
    double dif = std::fabs((double)a - b);
    return dif;
}

inline double compare_scalars(bool const a, long double const b) {
    if ((double)a == (double)b) { // a and b equal
        return 0.;
    }
    return 1.;
}


// compare two vectors. return true if different
template <typename T>
inline double compare_vectors(T const& a, T const& b) {
    double dif = 0;
    for (int i = 0; i < a.size(); i++) {
        double d = compare_scalars(a[i], b[i]);
        if (d > dif) dif = d;
    }
    return dif;
}

// report if error
template <typename T, typename R>
bool errorreport(int i, int k, T const a, R const r, long double const e) {
    bool ignore = false;  // error should not be reported as serious
    bool signerror;    // signed zero has wrong sign
    if (numerr == 0) {
        printf("\ntest case %i:", testcase);
    }
    printf("\nError at %i,%i: ", i, k);
    bool different = (double)r != (double)e;
    signerror = compare_sign((double)r, (double)e) != 0;
    different &= !(r != r && e != e); // both are NAN    
    if (different || signerror) {
        printVal(a);
        printf(" -> ");
        printVal(r);
        printf(" != ");
        printVal((R)e);
        printf(" diff = ");
        printf("%.2G", r!=e ? compare_scalars(r, e) : 0);
    }
#ifdef USE_ABSOLUTE_ERROR
    if constexpr (sizeof(T) < 8) {   // float
        if (std::fabs(a) < 5.E-38) ignore = true;
    }
    else {
        if (std::fabs(a) < 1.E-307) {        
            ignore = true;
        }
    }
#endif
#ifdef SIGNED_ZERO
    if (signerror) ignore = false;
#else
    if (signerror && ! different) ignore = true;
#endif
#ifdef IGNORE_UNDERFLOW
    if (x0 * x0 == 0 && (!signerror || e != e)) ignore = true;
#endif
#ifdef IGNORE_NAN
    if (r != r || e != e) ignore = true;
#endif
    if (ignore) printf(" ignored");
    return ignore;
}

// report if error. two parameters
template <typename T, typename R>
bool errorreport(int i, int j, int k, T const a, T const b, R const r, long double const e) {
    bool ignore = false;  // error should not be reported as serious
    if (numerr == 0) {
        printf("\ntest case %i:", testcase);
    }
    printf("\nError at %i,%i,%i: ", i, j, k);
    bool different = (double)r != (double)e;
#ifdef SIGNED_ZERO
    different |= compare_sign((double)r, (double)e) != 0;
#endif
    different &= !(r != r && e != e); // both are NAN    
    if (different) {
        printVal(a); printf(", "); printVal(b);
        printf(" -> ");
        printVal(r); printf(" != "); printVal((R)e);
        printf(" diff = ");
        printf("%.2G", r!=e ? compare_scalars(r, e) : 0);
    }
#ifdef USE_ABSOLUTE_ERROR
    if (sizeof(T) < 8) {   // float
        if (std::fabs(a) < 5.E-38) ignore = true;
    }
    else {                 // double
        if (std::fabs(a) < 1.E-307) ignore = true;
    }
#endif
    if (ignore) printf(" ignored");
    return ignore;
}



// program entry
int main(int argc, char* argv[]) {
    vtype a, b;                        // operand vectors
    rtype result;                      // result vector
    rtype expected;                    // expected result
    const int vectorsize = sizeof(vtype) / sizeof(decltype(a[0]));
    bool ignore;                       // ignore certain error conditions
    int sign_error = 0;                // results have different sign bit

#ifdef _FPU_CONTROL_H
    // The floating point control word is wrong in WSL. This gives poor 
    // precision in long double functions.
    // There is no way to know during compilation if running under WSL,
    // so we are setting the control word anyway to be safe.
    fpu_control_t cw;
    _FPU_GETCW(cw);
    //if (cw < 0X37F) printf("\nWarning FP control word was wrong: %X", cw);
    cw = 0X37F; 
    _FPU_SETCW(cw);
#endif

    // make lists of test data
    TestData<ST> adata;                // test data for first parameter

    // list for expected results
    // decltype(result[0]) expectedList[vectorsize];
    long double expectedList[vectorsize];

    int i, k;   // loop counters
    double dif = 0, maxdif = 0;        // maximum error found

#ifndef TWO_PARAMETERS                 // has one parameter

    for (i = 0; i < adata.size(); i += vectorsize) {
        a.load(adata.list + i);
        dif = 0;

        // function under test:
        result = testFunction(a);

        // expected value to compare with
        for (k = 0; k < vectorsize; k++) {
            expectedList[k] = referenceFunction(adata.list[i + k]);
            ignore = false;
            // compare result with expected value
            dif = compare_scalars(result[k], expectedList[k]);
            if (dif > FACCURACY || dif == -1. || dif != dif) {
                // report error if different
                ignore = errorreport(i, k, a[k], result[k], expectedList[k]);
                if (!ignore) {
                    numerr++;
                    sign_error += int(dif == -1.);
                }
                if (numerr > maxerrors) break;
            }
            if (dif > maxdif && !ignore && result[k] != expectedList[k]) maxdif = dif;
        }
        if (numerr > maxerrors) break;
    }
#else      // two parameters
    TestData<ST> bdata;                // test data for second parameter

    for (i = 0; i < adata.size(); i += vectorsize) {
        a.load(adata.list + i);

        for (int j = 0; j < bdata.size(); j += vectorsize) {
            b.load(bdata.list + j);
            dif = 0;

            // function under test:
            result = testFunction(a, b);

            // expected value to compare with
            for (k = 0; k < vectorsize; k++) {
                expectedList[k] = referenceFunction(a[k], b[k]);
                ignore = false;
                // compare result with expected value
                dif = compare_scalars(result[k], expectedList[k]);
                double accuracylimit = FACCURACY;
#ifdef YACCURACY
                accuracylimit += YACCURACY * std::fabs(b[k]);
#endif

                if (dif > accuracylimit || dif == -1.) {
                    // report error if different
                    ignore = errorreport(i, j, k, a[k], b[k], result[k], expectedList[k]);
                    if (!ignore) {
                        numerr++;
                        sign_error += int(dif == -1.);
                    }
                    if (numerr > maxerrors) break;
                }
                if (dif > maxdif && !ignore && result[k] != expectedList[k]) maxdif = dif;
            }
            if (numerr > maxerrors) {
                i = adata.size();
                break;
            }
        }
    }

#endif  // two parameters


    printf("\nAccuracy = %.2G ULP\n\n", maxdif);
    if (maxdif <= FACCURACY) {
        numerr = 0;         // report success if accuracy is as desirec
    }
#ifdef SIGNED_ZERO
    numerr += sign_error;   // count errors in signed zero
#endif

    return numerr;
}
