/****************************  testbench4.cpp   *******************************
* Author:        Agner Fog
* Date created:  2019-04-09
* Last modified: 2022-07-20
* Version:       2.02.00
* Project:       Testbench for vector class library
* Description:
* Compile and run this program to test operators and functions in VCL
* This file contains test cases for general operators and functions
* on half-precision floating point vectors.
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
* (c) Copyright 2019 - 2022 Agner Fog.
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
9:   max                   // fails in emulator for Vec16h unless -ffp-model=strict
10:  min
11:  maximum               (floating point types)
12:  minimum               (floating point types)
13:  abs
14:  if_add                // clang compilation fails if  -ffp-model=strict
15:  if_sub
16:  if_mul
17:  if_div                (floating point types)
18:  select
20:  store_a

106: operator &
107: operator |
108: operator ^
109: operator !

200: sign_combine          (float types)
201: change_sign
202: square                (float types)
210: is_finite
211: is_inf
212: is_nan
213: is_subnormal
214: is_zero_or_subnormal
220: infinite8h
221: nan_vec
222: nan_code

300: operator <
301: operator <=
302: operator ==
303: operator !=
304: operator >=
305: operator >
306: sign_bit              (float types)

400: horizontal_add
401: horizontal_add_x      (integer types and half precision)
404: horizontal_min
405: horizontal_max

501: reinterpret_h
502: reinterpret_i
503: reinterpret_f
504: reinterpret_d
505: roundi
506: truncatei
507: round
508: truncate
509: floor
510: ceil
519: sqrt
520: approx_recipr
521: approx_rsqrt
530: mul_add
531: mul_sub
532: nmul_add
540: exponent
541: fraction
542: pow_n
543: exp2
550: exp
551: exp2
552: exp10
553: expm1
560: sin
561: cos
562: tan
563: sincos
570: sinpi
571: cospi
572: tanpi
573: sincospi

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
620: to_float16            (int16_t to float16)
621: to_float16            (uint16_t to float16)
622: Vec8h -> Vec8f
623: Vec8f -> Vec8h
624: Vec8h -> Vec8f -> Vec8h 
650: constructor with all elements

900: test supported instruction sets

*****************************************************************************/

#include <stdio.h>
#include <cmath>
#if defined (__linux__) && !defined(__LP64__)
#include <fpu_control.h>     // set floating point control word
#endif

// maximum vector size
#define MAX_VECTOR_SIZE 512


#ifndef INSTRSET
#define INSTRSET 10          // desired instruction set
#endif

#if INSTRSET >= 9 && !defined(__F16C__)
#define __F16C__
#endif

#ifndef __AVX512FP16__
//#define __AVX512FP16__
#endif
//#define __AVX512VBMI2__

#include <vectorclass.h>     // vector class library
#include <vectorfp16.h>      // half precision vectors


#ifndef testcase 
// ----------------------------------------------------------------------------
//          Specify input parameters here if running from an IDE:
// ----------------------------------------------------------------------------

#define testcase 1

#define vtype Vec16h

#define rtype vtype
//#define rtype Vec16s

#define seed 1

#define EXHAUSTIVE_TEST

#define TESTNAN


#else
// ----------------------------------------------------------------------------
//            Default input parameters when compiling from a script
// ----------------------------------------------------------------------------

// input or index vector type to be tested
#ifndef vtype
#define vtype Vec8h
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

#if testcase == 900
#include "instrset_detect.cpp"
#endif

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
    x = false;
    for (int i = 0; i < x.size(); i++) {
        x.insert(i, p[i]);  // bool vectors have no load function
    }
}

// ----------------------------------------------------------------------------
//                      conversions Float16 <-> float
// ----------------------------------------------------------------------------

// type Float16 emulates _Float16 if _Float16 not defined (vectorfp16e.h)

// convert float to user-defined type Float16
#ifdef __AVX512FP16__
uint16_t to_float16(float x, bool supportSubnormal = true) {
    return _mm_cvtsi128_si32(_mm_castph_si128(_mm_cvtss_sh(_mm_setzero_ph(), _mm_load_ss(&x))));
}

float to_float(uint32_t half, bool supportSubnormal = true) {
    return _mm_cvtss_f32(_mm_cvtsh_ss (_mm_setzero_ps(),_mm_castsi128_ph(_mm_cvtsi32_si128(half))));
}

// Float16 is defined. Define bit-casting between uint16_t <-> Float16
static inline uint16_t castfp162s(Float16 x) {
    union {
        Float16 f;
        uint16_t i;
    } u;
    u.f = x;
    return u.i;
}
static inline Float16 casts2fp16(uint16_t x) {
    union {
        uint16_t i;
        Float16 f;
    } u;
    u.i = x;
    return u.f;
}

#else

// half to float conversion functions
// Convert half precision floating point number to single precision
// Optional support for subnormals
// NAN payload is right-justified for ForwardCom
float to_float(uint32_t half, bool supportSubnormal = true) {
    union {
        uint32_t hhh;
        float fff;
        struct {
            uint32_t mant: 23;
            uint32_t expo:  8;
            uint32_t sign:  1;
        };
    } u;

    u.hhh  = (half & 0x7fff) << 13;              // Exponent and mantissa
    u.hhh += 0x38000000;                         // Adjust exponent bias
    if ((half & 0x7C00) == 0) {// Subnormal
        if (supportSubnormal) {
            u.hhh = 0x3F800000 - (24 << 23);     // 2^-24
            u.fff *= int(half & 0x3FF);          // subnormal value = mantissa * 2^-24
        }
        else {        
            u.hhh = 0;                           // make zero
        }
    }
    if ((half & 0x7C00) == 0x7C00) {             // infinity or nan
        u.expo = 0xFF;
        if (half & 0x3FF) {  // nan
            u.mant = 1 << 22 | (half & 0x1FF);   // NAN payload is right-justified only in ForwardCom
        }
    }
    u.hhh |= (half & 0x8000) << 16;              // sign bit
    return u.fff;
}


// Convert floating point number to half precision.
// Round to nearest or even. 
// Optional support for subnormals
// NAN payload is right-justified
uint16_t to_float16(float x, bool supportSubnormal = true) {
    union {                                      // single precision float
        float f;
        struct {
            uint32_t mant: 23;
            uint32_t expo:  8;
            uint32_t sign:  1;
        };
    } u;
    union {                                      // half precision float
        uint16_t h;
        struct {
            uint16_t mant: 10;
            uint16_t expo:  5;
            uint16_t sign:  1;
        };
    } v;
    u.f = x;
    v.expo = u.expo - 0x70;                      // adjust exponent bias
    v.mant = u.mant >> 13;                       // get upper part of mantissa
    if (u.mant & (1 << 12)) {                    // round to nearest or even
        if ((u.mant & ((1 << 12) - 1)) || (v.mant & 1)) { // round up if odd or remaining bits are nonzero
            v.h++;                               // overflow will carry into exponent and sign
        }
    }
    v.sign = u.sign;
    if (u.expo == 0xFF) {                        // infinity or nan
        v.expo = 0x1F;
        if (u.mant != 0) {                       // Nan
            v.mant = (u.mant & 0x1FF) | 0x200;   // NAN payload is right-justified only in ForwardCom        
        }
    }
    else if (u.expo > 0x8E) {
        v.expo = 0x1F;  v.mant = 0;              // overflow -> inf
    }
    else if (u.expo < 0x71) {
        v.expo = 0;
        if (supportSubnormal) {
            u.expo += 24;
            u.sign = 0;            
            uint16_t mants = _mm_cvt_ss2si(_mm_load_ss(&u.f));
            v.mant = mants & 0x3FF; // proper rounding of subnormal
            if (mants == 0x400) v.expo = 1; // round up to normal
        }
        else {        
            v.mant = 0;                          // underflow -> 0
        }
    }
    return v.h;   
}

#endif

// convert float to user-defined type Float16
Float16 float2fp16(float x) {
    return casts2fp16(to_float16(x));
}

// convert uint16_t to Float16 by bit casting
Float16 half2fp16 (uint16_t x) {
    return casts2fp16(x);
}

// convert uint16_t to Float16 by bit casting
uint16_t fp162half (Float16 x) {
    return castfp162s(x);
}


bool signbit_(float x) {
    union { float f; uint32_t i; } u;
    u.f = x; return u.i >> 31 != 0;
}
bool signbit_(double x) {
    union { double f; uint64_t i; } u;
    u.f = x; return u.i >> 63 != 0;
}
bool signbit_(Float16 x) {
    return (fp162half(x) & 0x8000) != 0;
}


/************************************************************************
*
*                          Test cases
*
************************************************************************/

#if   testcase == 1    // +
inline rtype testFunction(vtype const& a, vtype const& b) { return a + b; }
RT referenceFunction(ST a, ST b) { return a + b;}

#elif testcase == 2    // - 
inline rtype testFunction(vtype const& a, vtype const& b) { return a - b; }
RT referenceFunction(ST a, ST b) { return a - b; }

#elif testcase == 3    // * 
inline rtype testFunction(vtype const& a, vtype const& b) { 
    return a * b; 
}
RT referenceFunction(ST a, ST b) {
    return a * b; 
    /*
    float p = float(a) * float(b);
    uint32_t p1 = to_float16(p);
    union {
        uint16_t h;
        Float16 f;
    } u;
    u.h = p1;
    return u.f;
    */
}

#elif testcase == 4    // /  (float types only)
inline rtype testFunction(vtype const& a, vtype const& b) { return a / b; }
RT referenceFunction(ST a, ST b) { return a / b; }


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
inline rtype testFunction(vtype const& a, vtype const& b) { 
    return abs(b); 
}
RT referenceFunction(ST a, ST b) { return b > ST(0) ? b : -b; }

#elif testcase == 14    // if_add
inline rtype testFunction(vtype const& f, vtype const& a, vtype const& b) { 
    return if_add(f > vtype(ST(0)), a, b); 
}
RT referenceFunction(ST f, ST a, ST b) { return f > ST(0) ? a+b : a; }
#define USE_FLAG

#elif testcase == 15    // if_sub
inline rtype testFunction(vtype const& f, vtype const& a, vtype const& b) { 
    return if_sub(f < vtype(ST(1)), a, b);
}
RT referenceFunction(ST f, ST a, ST b) { return f < ST(1) ? a-b : a; }
#define USE_FLAG

#elif testcase == 16    // if_mul
inline rtype testFunction(vtype const& f, vtype const& a, vtype const& b) { 
    return if_mul(f > vtype(ST(0)), a, b);
}
RT referenceFunction(ST f, ST a, ST b) { 
    return f > ST(0) ? a*b : a; 
}
#define USE_FLAG

#elif testcase == 17    // if_div
inline rtype testFunction(vtype const& f, vtype const& a, vtype const& b) { 
    return if_div(f < vtype(ST(1)), a, b);
}
RT referenceFunction(ST f, ST a, ST b) { return f < ST(1) ? a/b : a; }
#define USE_FLAG

#elif testcase == 18    // select
inline rtype testFunction(vtype const& f, vtype const& a, vtype const& b) { 
    return select(f < vtype(ST(1)), a, b);
}
RT referenceFunction(ST f, ST a, ST b) { return f < ST(1) ? a : b; }
#define USE_FLAG

#elif testcase == 20    // store_a
inline rtype testFunction(vtype const& a, vtype const& b) { 
    union {             // make aligned array
        ST y[vtype::size()];
        vtype dummy;
    } aligned = {{0}};
    a.store_a(aligned.y);
    return vtype().load_a(aligned.y);
}
RT referenceFunction(ST a, ST b) { return  a; }

#elif testcase == 21    // store_nt
inline rtype testFunction(vtype const& a, vtype const& b) { 
    union {             // make aligned array
        ST y[vtype::size()];
        vtype dummy;
    } aligned = {{0}};
    a.store_nt(aligned.y);
    return vtype().load_a(aligned.y);
}
RT referenceFunction(ST a, ST b) { return  a; }

#elif testcase == 106  // operator &
inline rtype testFunction(vtype const& a, vtype const& b) { return a & b; }
RT referenceFunction(ST a, ST b) {
    return casts2fp16(castfp162s(a) & castfp162s(b));
}

#elif testcase == 107  // operator |
inline rtype testFunction(vtype const& a, vtype const& b) { return a | b; }
RT referenceFunction(ST a, ST b) {
    return casts2fp16(castfp162s(a) | castfp162s(b));
}

#elif testcase == 108  // operator ^
inline rtype testFunction(vtype const& a, vtype const& b) { return a ^ b; }
RT referenceFunction(ST a, ST b) {
    return casts2fp16(castfp162s(a) ^ castfp162s(b));
}

#elif testcase == 109  // operator !
inline rtype testFunction(vtype const& a, vtype const& b) { return !b; }
RT referenceFunction(ST a, ST b) {
    return b == ST(0.);
} 


// ----------------------------------------------------------------------------
//                           floating point only cases
//               (round, sqrt, pow, etc. are in the math testbench)
// ----------------------------------------------------------------------------

#elif testcase == 200    // sign_combine
inline rtype testFunction(vtype const& a, vtype const& b) { return sign_combine(a, b); }
// define signbit function because Visual studio is missing it

RT referenceFunction(ST a, ST b) {
    return signbit_(b) ? -a : a;
}

#elif testcase == 201    // change_sign
static int aindex = 0;   // index to vector element

/*  works with clang, not with g++:
template <int n> rtype testFunction1(vtype const& a) {
    aindex = 0;
    rtype y = 0;
    if constexpr (n == 8)  y = change_sign<1,0,0,0,0,1,0,0>(a); 
    if constexpr (n == 16) y = change_sign<1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1>(a); 
    if constexpr (n == 32) y = change_sign<1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0>(a); 
    return y;
} */

inline Vec8h testFunction(Vec8h const& a, Vec8h const& b) { 
    aindex = 0;
    return change_sign<1,0,0,0,0,1,0,0>(a); 
}

inline Vec16h testFunction(Vec16h const& a, Vec16h const& b) { 
    aindex = 0;
    return change_sign<1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1>(a); 
}

inline Vec32h testFunction(Vec32h const& a, Vec32h const& b) { 
    aindex = 0;
    return change_sign<1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0>(a); 
}


RT referenceFunction(ST a, ST b) {
    if (aindex++ % 5 == 0) return -a; else return a;
}

#elif testcase == 202    // square
inline rtype testFunction(vtype const& a, vtype const& b) { return square(b); }
RT referenceFunction(ST a, ST b) {
    return b * b;
}

#elif testcase == 210    // is_finite
inline rtype testFunction(vtype const& a, vtype const& b) { return is_finite(a); }
RT referenceFunction(ST a, ST b) {
    uint16_t ai = fp162half(a);
    if ((ai & 0x7C00) != 0x7C00) return 1; else return 0;
}

#elif testcase == 211    // is_inf
inline rtype testFunction(vtype const& a, vtype const& b) { return is_inf(a); }
RT referenceFunction(ST a, ST b) {
    uint16_t ai = fp162half(a);
    if ((ai & 0x7C00) == 0x7C00 && (ai & 0x03FF) == 0) return 1; else return 0;
}

#elif testcase == 212    // is_nan
inline rtype testFunction(vtype const& a, vtype const& b) { return is_nan(a); }
RT referenceFunction(ST a, ST b) {
    uint16_t ai = fp162half(a);
    if ((ai & 0x7C00) == 0x7C00 && (ai & 0x03FF) != 0) return 1; else return 0;
}

#elif testcase == 213    // is_subnormal
inline rtype testFunction(vtype const& a, vtype const& b) { return is_subnormal(a); }
RT referenceFunction(ST a, ST b) {
    uint16_t ai = fp162half(a);
    if ((ai & 0x7C00) == 0 && (ai & 0x03FF) != 0) return 1; else return 0;
}

#elif testcase == 214    // is_zero_or_subnormal
inline rtype testFunction(vtype const& a, vtype const& b) { return is_zero_or_subnormal(a); }
RT referenceFunction(ST a, ST b) {
    uint16_t ai = fp162half(a);
    if ((ai & 0x7C00) == 0) return 1; else return 0;
}

#elif testcase == 220    // infinite8h
template <int n> auto testFunction1(vtype const& a) {
    if constexpr (n == 8)  return infinite8h();
    if constexpr (n == 16) return infinite16h();
    if constexpr (n == 32) return infinite32h();
}
inline rtype testFunction(vtype const& a, vtype const& b) { 
    return testFunction1<vtype::size()>(a); 
}
RT referenceFunction(ST a, ST b) {
    return half2fp16(0x7C00);
}
  
#elif testcase == 221    // nan_vec
inline rtype testFunction(vtype const& a, vtype const& b) { return nan_vec<vtype>(5); }
RT referenceFunction(ST a, ST b) {
    return half2fp16(0x7E05);
}

#elif testcase == 222    // nan_code
inline rtype testFunction(vtype const& a, vtype const& b) { 
    return nan_code(a); 
}
RT referenceFunction(ST a, ST b) {
    uint16_t k = fp162half(a) ;
    if ((k & 0x7C00) != 0x7C00) return 0;  // not NAN
    return k & 0x3FF;
}

// ----------------------------------------------------------------------------
//                           boolean result cases
// ----------------------------------------------------------------------------

#elif testcase == 300    // <
inline rtype testFunction(vtype const& a, vtype const& b) { return a < b; }
RT referenceFunction(ST a, ST b) { return float(a) < float(b); }

#elif testcase == 301    // <=
inline rtype testFunction(vtype const& a, vtype const& b) { return a <= b; }
RT referenceFunction(ST a, ST b) { return float(a) <= float(b); }

#elif testcase == 302    // ==
inline rtype testFunction(vtype const& a, vtype const& b) { return a == b; }
RT referenceFunction(ST a, ST b) { return float(a) == float(b); }

#elif testcase == 303    // !=
inline rtype testFunction(vtype const& a, vtype const& b) { return a != b; }
RT referenceFunction(ST a, ST b) { return float(a) != float(b); }

#elif testcase == 304    // >=
inline rtype testFunction(vtype const& a, vtype const& b) { return a >= b; }
RT referenceFunction(ST a, ST b) { return float(a) >= float(b); }

#elif testcase == 305    // >
inline rtype testFunction(vtype const& a, vtype const& b) { return a > b; }
RT referenceFunction(ST a, ST b) { return float(a) > float(b); }

#elif testcase == 306    // sign_bit (float types only)
inline rtype testFunction(vtype const& a, vtype const& b) { return sign_bit(b); }

RT referenceFunction(ST a, ST b) {
    return signbit_(b);
}


// ----------------------------------------------------------------------------
//                           scalar result cases
// ----------------------------------------------------------------------------

#elif testcase == 400    // horizontal_add
#define SCALAR_RESULT
#define FACCURACY 8      // accept accumulating rounding errors
inline rtype testFunction(vtype const& a, vtype const& b) { 
    return rtype(horizontal_add(a)); 
}
RT referenceFunction(vtype const& a, vtype const& b) {
    float sum = 0;
    for (int i = 0; i < a.size(); i++) sum += float(a[i]);
    return sum;

    /*
    RT sss[vtype::size()];
    int i;
    for (i = 0; i < vtype::size(); i++) sss[i] = a[i];
    if constexpr (vtype::size() >= 32) {
        for (i = 0; i < 16; i++) sss[i] = sss[i] + sss[i+16];
    }
    if constexpr (vtype::size() >= 16) {
        for (i = 0; i < 8; i++) sss[i] = sss[i] + sss[i+8];
    }
    for (i = 0; i < 4; i++) sss[i] = sss[i] + sss[i+4];
    for (i = 0; i < 2; i++) sss[i] = sss[i] + sss[i+2];
    sss[0] = sss[0] + sss[1];
    return sss[0];*/
}

#elif testcase == 401    // horizontal_add_x
#define SCALAR_RESULT
#define FACCURACY 30     // accept accumulating rounding errors
inline rtype testFunction(vtype const& a, vtype const& b) { return horizontal_add_x(b); }
RT referenceFunction(vtype const& a, vtype const& b) {
    double sum = 0;
    for (int i = 0; i < b.size(); i++) sum += double(float(b[i]));
    return RT(sum);
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
inline rtype testFunction(vtype const& a, vtype const& b) { 
    return rtype(horizontal_min(b)); 
}
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


// ----------------------------------------------------------------------------
//                           type conversion functions
// ----------------------------------------------------------------------------

#elif testcase == 501    // reinterpret_h
// int to float16
inline rtype testFunction(vtype const& a, vtype const& b) { return reinterpret_h(b); }
rtype referenceFunction(vtype a, vtype b) {
    int8_t temp[maxvectorsize];
    rtype r;
    b.store((ST*)temp);
    r.load(temp);
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
inline rtype testFunction(vtype & a, vtype const& b) { 
    rtype r = roundi(a);
    // work around problem with VCVTPH2W(INF) gives 0x8000
    rtype r2 = select(r == rtype(-0x8000) && a > vtype(ST(0)), rtype(0x7FFF), r);
    return r2;
}
RT referenceFunction(ST a, ST b) {
    RT r;
    uint16_t ai = fp162half(a);
    float af = a;
    if (af >= float( 0x7FFF)) return 0x7FFF;     // overflow saturates
    if (af <  float(-0x8000)) return 0x8000;     // negative overflow saturates
    if ((ai & 0x7C00) == 0x7C00) {               // infinity or nan
        return 0x8000;                           // NAN
    }
    if (af >= 0) {
        r = (RT)(af + 0.5);                      // 0.5 rounds up
        if ((r & 1) && r - af == 0.5) r--;       // round down if result is odd
    }
    else {
        r = (RT)(af - 0.5);                      // 0.5 rounds up
        if ((r & 1) && af - r == 0.5) r++;       // round up if result is odd
    }
    return r;
}

#elif testcase == 506    // truncatei
inline rtype testFunction(vtype const& a, vtype const& b) { 
    rtype r = truncatei(a);
    // work around problem with vcvttps2dq VCVTPH2W(INF) gives 0x8000
    rtype r2 = select(r == rtype(-0x8000) && a > vtype(ST(0)), rtype(0x7FFF), r);
    return r2;
}
RT referenceFunction(ST a, ST b) {
    RT r;
    uint16_t ai = fp162half(a);         
    float af = a;
    if (af >= float( 0x7FFF)) return 0x7FFF;     // overflow saturates
    if (af <  float(-0x8000)) return 0x8000;     // negative overflow saturates
    if ((ai & 0x7C00) == 0x7C00) {               // infinity or nan
        return 0x8000;                           // NAN
    }
    if (af >= 0) {
        r = (RT)(af);                 // truncate towards zero
    }
    else {
        r = -(RT)(-af);
    }
    return r;
}

#elif testcase == 507    // round
inline rtype testFunction(vtype const& a, vtype const& b) { return round(a); }
RT referenceFunction(ST a, ST b) {
    int r;
    uint16_t ai = fp162half(a);         
    float af = float(a);
    if ((ai & 0x7C00) == 0x7C00) {               // infinity or nan
        return a;
    }
    if (ai == 0x8000) return a;                  // -0
    if (af >= 0) {
        r = int(af + 0.5f);                      // 0.5 rounds up
        if ((r & 1) && r - af == 0.5f) r--;      // round down if result is odd
    }
    else {
        r = int(af - 0.5f);                      // 0.5 rounds up
        if ((r & 1) && af - r == 0.5) r++;       // round up if result is odd
    }
    return RT(float(r));
}

#elif testcase == 508    // truncate
inline rtype testFunction(vtype const& a, vtype const& b) { return truncate(a); }
RT referenceFunction(ST a, ST b) {
    int r;
    uint16_t ai = fp162half(a); 
    float af = a;
    if ((ai & 0x7C00) == 0x7C00) {               // infinity or nan
        return a;
    }
    if (ai == 0x8000) return a;                  // -0
    r = int(af);                                 // truncate towards zero
    return (RT)float(r);
}

#elif testcase == 509    // floor
inline rtype testFunction(vtype const& a, vtype const& b) { return floor(a); }
RT referenceFunction(ST a, ST b) {
    int r;
    uint16_t ai = fp162half(a);         
    float af = a;
    if ((ai & 0x7C00) == 0x7C00) {               // infinity or nan
        return a;
    }
    if (ai == 0x8000) return a;                  // -0
    r = int(af);                                 // truncate towards zero
    if (r > af) r--;                             // round down
    return (RT)float(r);
}

#elif testcase == 510    // ceil
inline rtype testFunction(vtype const& a, vtype const& b) { return ceil(a); }
RT referenceFunction(ST a, ST b) {
    int r;
    uint16_t ai = fp162half(a);         
    float af = a;
    if ((ai & 0x7C00) == 0x7C00) {               // infinity or nan
        return a;
    }
    if (ai == 0x8000) return a;                  // -0
    r = int(af);                                 // truncate towards zero
    if (r < af) r++;                             // round up
    return (RT)float(r);
}


#elif testcase == 519    // sqrt
inline rtype testFunction(vtype const& a, vtype const& b) { return sqrt(a); }
RT referenceFunction(ST a, ST b) {
    float c = sqrtf(float(a));
    return RT(c);
}

#elif testcase == 520    // approx_recipr
inline rtype testFunction(vtype const& a, vtype const& b) { return approx_recipr(a); }
RT referenceFunction(ST a, ST b) {
    float c = 1.0f / float(a);
    return RT(c);
}
#define FACCURACY 1      // accept  rounding errors

#elif testcase == 521    // approx_rsqrt
inline rtype testFunction(vtype const& a, vtype const& b) { return approx_rsqrt(a); }
RT referenceFunction(ST a, ST b) {
    float c = 1.0 / sqrt(double(a));
    return RT(c);
}
#define FACCURACY 1      // accept  rounding errors

#elif testcase == 530    // mul_add
inline rtype testFunction(vtype const& a, vtype const& b) { return mul_add(a, b, vtype(1.f)); }
RT referenceFunction(ST a, ST b) {
    float c = float(a) * float(b) + 1.0f;
    return RT(c);
}
#define FACCURACY 2      // accept  rounding errors

#elif testcase == 531    // mul_sub
inline rtype testFunction(vtype const& a, vtype const& b) { return mul_sub(a, vtype(1.25f), b); }
RT referenceFunction(ST a, ST b) {
    float c = float(a) * 1.25f - float(b);
    return RT(c);
}
#define FACCURACY 2      // accept  rounding errors

#elif testcase == 532    // nmul_add
inline rtype testFunction(vtype const& a, vtype const& b) { return nmul_add(vtype(2.25f), a, b); }
RT referenceFunction(ST a, ST b) {
    float c = -2.25f * float(a) + float(b);
    return RT(c);
}
#define FACCURACY 2      // accept  rounding errors

#elif testcase == 540    // exponent
inline rtype testFunction(vtype const& a, vtype const& b) { return exponent(a); }
RT referenceFunction(ST a, ST b) {
    uint16_t c = fp162half(a);
    uint16_t d = (c >> 10 & 0x1F) - 0x0F;
    return RT(d);
}

#elif testcase == 541    // fraction
inline rtype testFunction(vtype const& a, vtype const& b) { 
    rtype r = select(is_zero_or_subnormal(a),rtype(ST(0)), fraction(a));
    return r;
}
RT referenceFunction(ST a, ST b) {
    uint16_t c = fp162half(a);
    if ((c & 0x7C00) == 0) return 0;
    uint16_t d = (c & 0x03FF) | 0x3C00;
    return half2fp16(d);
}
#define IGNORE_NAN  // only some versions propagate NANs
#define IGNORE_SUBNORMAL

#elif testcase == 542    // pow_n
inline rtype testFunction(vtype const& a, vtype const& b) { 
    return pow(a, 6);               // 3 different versions
    //return pow(a, 6u);          
    //return pow(a, const_int(6));
} 
RT referenceFunction(ST a, ST b) {
    RT a2 = a*a;
    RT a4 = a2*a2;
    return a2*a4;
}
#define FACCURACY 5      // accept accumulating rounding errors


#elif testcase == 543    // exp2
inline rtype testFunction(vtype const& a, vtype const& b) { return exp2(a); }
RT referenceFunction(ST a, ST b) {
    if (a > 15) return half2fp16(0x7C00); // INF
    if (a <= -15) return 0;
    if (a >= 0) return RT(float(1 << a));
    else return RT(float(1./(1 << -a)));
}

#elif testcase == 550    // exp
inline rtype testFunction(vtype const& a, vtype const& b) { return exp(b); }
RT referenceFunction(ST a, ST b) {
#ifdef __AVX512FP16__    // intermediate calculations with half precision
    float maxexp = 10.75f;
#else
    float maxexp = 1000.f;
#endif
    if (float(b) >= maxexp) return half2fp16(0x7C00); // INF
    float bf = float(b);
    float y = expf(bf);
    return y;
}
#ifdef __AVX512FP16__    // intermediate calculations with half precision
#define FACCURACY 7      // accept accumulating rounding errors
#else                    // intermediate calculations with float precision
#define FACCURACY 1      // accept accumulating rounding errors
#endif
#define IGNORE_SUBNORMAL

#elif testcase == 551    // exp2

inline rtype testFunction(vtype const& a, vtype const& b) { return exp2(b); }
RT referenceFunction(ST a, ST b) {
#ifdef __AVX512FP16__    // intermediate calculations with half precision
    float maxexp = 15.5f;
#else
    float maxexp = 1000.f;
#endif
    if (float(b) >= maxexp) return half2fp16(0x7C00); // INF

    return exp2f(float(b));
}
#ifdef __AVX512FP16__    // intermediate calculations with half precision
#define FACCURACY 6      // accept accumulating rounding errors
#else                    // intermediate calculations with float precision
#define FACCURACY 1      // accept accumulating rounding errors
#endif
#define IGNORE_SUBNORMAL

#elif testcase == 552    // exp10
inline rtype testFunction(vtype const& a, vtype const& b) { return exp10(b); }
RT referenceFunction(ST a, ST b) {
#ifdef __AVX512FP16__    // intermediate calculations with half precision
    float maxexp = 4.667f;
#else
    float maxexp = 1000.f;
#endif
    if (float(b) >= maxexp) return half2fp16(0x7C00); // INF
    return powf(10.f, float(b));
}
#ifdef __AVX512FP16__    // intermediate calculations with half precision
#define FACCURACY 6      // accept accumulating rounding errors
#else                    // intermediate calculations with float precision
#define FACCURACY 1      // accept accumulating rounding errors
#endif
#define IGNORE_SUBNORMAL

#elif testcase == 553    // expm1

inline rtype testFunction(vtype const& a, vtype const& b) { return expm1(b); }
RT referenceFunction(ST a, ST b) {
#ifdef __AVX512FP16__    // intermediate calculations with half precision
    float maxexp = 10.75f;
#else
    float maxexp = 1000.f;
#endif
    if (float(b) >= maxexp) return half2fp16(0x7C00); // INF
    return expm1f(float(b));
}
#ifdef __AVX512FP16__    // intermediate calculations with half precision
#define FACCURACY 7      // accept accumulating rounding errors
#else                    // intermediate calculations with float precision
#define FACCURACY 1      // accept accumulating rounding errors
#endif
#define IGNORE_SUBNORMAL

#elif testcase == 560   //  sin
inline rtype testFunction(vtype const& a, vtype const& b) { return sin(b); }
RT referenceFunction(ST a, ST b) {
    // limit
    float sinlimit = 314.25;
    //if ((fp162half(b) & 0x7FFF) == 0x7C00) return half2fp16(0x7E00); // INF -> NAN
    if (abs(float(b)) > sinlimit) return 0;
    return sinf(float(b));
}
#define FACCURACY 6      // accept accumulating rounding errors
#define IGNORE_SUBNORMAL

#elif testcase == 561   //  cos
inline rtype testFunction(vtype const& a, vtype const& b) { return cos(b); }
RT referenceFunction(ST a, ST b) {
    // limit
    float sinlimit = 314.25;
    //if ((fp162half(b) & 0x7FFF) == 0x7C00) return half2fp16(0x7E00); // INF -> NAN
    if (abs(float(b)) > sinlimit) return 1.;
    return cosf(float(b));
}
#define FACCURACY 12      // accept accumulating rounding errors
#define IGNORE_SUBNORMAL

#elif testcase == 562   //  tan
inline rtype testFunction(vtype const& a, vtype const& b) { 
    return tan(b); 
}
RT referenceFunction(ST a, ST b) {
    // limit
    float sinlimit = 314.25;
    //if ((fp162half(b) & 0x7FFF) == 0x7C00) return half2fp16(0x7E00); // INF -> NAN
    if (abs(float(b)) > sinlimit) return 0.;
    return tanf(float(b));
}
#define FACCURACY 1000      // rounding errors are high for high x
#define IGNORE_SUBNORMAL

#elif testcase == 563   //  sincos
inline rtype testFunction(vtype const& a, vtype const& b) { 
    vtype s, c;
    s = sincos(&c, b);
    return select(b >= vtype(ST(0)), s, c);
}
RT referenceFunction(ST a, ST b) {
    float s = sinf(b);
    float c = cosf(b);
    // limit
    float sinlimit = 314.25;
    /*if ((fp162half(b) & 0x7FFF) == 0x7C00) {
        //c = s = half2fp16(0x7E00); // INF -> NAN
    }
    else */
    if (abs(float(b)) > sinlimit) {
        s = 0; c = 1;
    }  
    return b >= ST(0) ? s : c;
}
#define FACCURACY 12      // accept accumulating rounding errors
#define IGNORE_SUBNORMAL


#elif testcase == 570   // sinpi
inline rtype testFunction(vtype const& a, vtype const& b) { return sinpi(b); }
RT referenceFunction(ST a, ST b) {
    // limit
#ifdef __AVX512FP16__
    if (fabs(b) > 32000) return 0; // overflow -> 0
#else
    if ((fp162half(b) & 0x7FFF) == 0x7C00) return half2fp16(0x7E00); // INF -> NAN
#endif
    return (float)sinl(float(b)*3.14159265358979323846);
}
#define FACCURACY 2       // accept accumulating rounding errors
#define IGNORE_SUBNORMAL

#elif testcase == 571   // cospi
inline rtype testFunction(vtype const& a, vtype const& b) { return cospi(b); }
RT referenceFunction(ST a, ST b) {
    // limit
#ifdef __AVX512FP16__
    if (fabs(b) > 32000) return 1.; // overflow -> 1
#else
    if ((fp162half(b) & 0x7FFF) == 0x7C00) return half2fp16(0x7E00); // INF -> NAN
#endif
    return (float)cosl(float(b)*3.14159265358979323846);
}
#define FACCURACY 2       // accept accumulating rounding errors
#define IGNORE_SUBNORMAL

#elif testcase == 572   // tanpi
inline rtype testFunction(vtype const& a, vtype const& b) { return tanpi(b); }
RT referenceFunction(ST a, ST b) {
    // limit
#ifdef __AVX512FP16__
    if (fabs(b) > 32000) return 0;; // overflow -> 0
#else
    if ((fp162half(b) & 0x7FFF) == 0x7C00) return half2fp16(0x7E00); // INF -> NAN
#endif

    int64_t ia2 = int64_t(float(b) * 2.);
    if (ia2 == float(b) * 2.) {  // sign of INF result should alternate according to IEEE 754-2019
        if ((ia2 & 3) == 1) return half2fp16(0x7C00); //  INF
        if ((ia2 & 3) == 3) return half2fp16(0xFC00); // -INF
    } 
    return (float)tanl(float(b)*3.14159265358979323846);
}
#define FACCURACY 4       // accept accumulating rounding errors
#define IGNORE_SUBNORMAL

#elif testcase == 573   // sincospi
inline rtype testFunction(vtype const& a, vtype const& b) { 
    vtype s, c;
    s = sincospi(&c, b);
    return select(b >= vtype(ST(0)), s, c);
}
RT referenceFunction(ST a, ST b) {
    float s = (float)sin(float(b)*3.14159265358979323846);
    float c = (float)cos(float(b)*3.14159265358979323846);
    // limit
#ifdef __AVX512FP16__
    if (fabs(b) > 32000) {
        s = 0; c = 1;
    }
#else
    if ((fp162half(b) & 0x7FFF) == 0x7C00) {
        c = s = half2fp16(0x7E00); // INF -> NAN
    }
#endif
    return b >= ST(0) ? s : c;
}
#define FACCURACY 2      // accept accumulating rounding errors
#define IGNORE_SUBNORMAL



// ----------------------------------------------------------------------------
//             Functions without element-to-element correspondence
// ----------------------------------------------------------------------------

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

#elif testcase == 620    // to_float16: int16_t to float16
// test with signed and unsigned types
inline rtype testFunction(vtype const& a, vtype const& b) { return to_float16(b); }
RT referenceFunction(ST a, ST b) {
    return float(b);
}

#elif testcase == 621    // to_float16: uint16_t to float16
inline rtype testFunction(vtype const& a, vtype const& b) { return to_float16(b); }
RT referenceFunction(ST a, ST b) {
    return float(b);
}

#elif testcase == 622   // to_float: Float16 -> float
#define TESTNAN

// test with signed and unsigned types
inline rtype testFunction(vtype const& a, vtype const& b) { 
    return to_float(b); 
}
float referenceFunction(Float16 a, Float16 b) {
    return float(b);
}

#elif testcase == 623   // to_float16: float -> Float16
#define TESTNAN

inline rtype testFunction(vtype const& a, vtype const& b) {
    return to_float16(b); 
}
RT referenceFunction(ST a, ST b) {
    return Float16(b);
     //return casts2fp16(to_float16(b)); 
     //return casts2fp16(_mm_cvtsi128_si32((_mm_cvtps_ph(Vec4f(b), _MM_FROUND_NO_EXC*0))));
}

#elif testcase == 624   // to_float: Vec8h -> Vec8f -> Vec8h
#define TESTNAN

inline rtype testFunction(vtype const& a, vtype const& b) {
    return to_float16(to_float(a)); 
}
RT referenceFunction(ST a, ST b) {
     return a; 
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

#elif testcase == 900   // test supported instruction sets

inline rtype testFunction(vtype const& a, vtype const& b) { return a + b; }  // dummy test case
RT referenceFunction(ST a, ST b) { return a + b;}

#else
// End of test cases
#error unknown test case
#endif

///////////////////////////////////////////
// type-specific printing functions
///////////////////////////////////////////

void printVal(int8_t const x) {
    if (x < 0) {
        printf("-%X", -x);
    }
    else {
        printf("%X", x);
    }
}

void printVal(uint8_t const x) {
    printf("%X", x);
}

void printVal(int16_t const x) {
    if (x < 0) {
        printf("-%X", -x);
    }
    else {
        printf("%X", x);
    }
}

void printVal(uint16_t const x) {
    printf("%X", x);
}

void printVal(int32_t const x) {
    if (x < 0) {
        printf("-%X", -x);
    }
    else {
        printf("%X", x);
    }
}

void printVal(uint32_t const x) {
    printf("%X", x);
}

void printVal(int64_t const x) {
    if (x < 0) {
        printf("-%llX", -(long long)x);
    }
    else {
        printf("%llX", (long long)x);
    }
}

void printVal(uint64_t const x) {
    printf("%llX", (unsigned long long)x);
}

void printVal(float const x) {
    printf("%10.7G", x);
}

void printVal(double const x) {
    printf("%10.7G", x);
}


void printVal(Float16 const x) {
    float y = float(x);
    printVal(y);
}

void printVal(bool const x) {
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

template <>  // special case double
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

template <>  // special case Float16
Float16 get_random<Float16>(ranGen & rangen) {
    return to_float16( (get_random<float>(rangen)- 0.4f) * 70000.f);
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
#ifdef EXHAUSTIVE_TEST
        listsize = (1 << 16)                     // run through all possible values of half precision
#else 
        listsize = 1024
#endif
    };
    TestData() {                                 // constructor
        int i;                                   // loop counter
#ifdef EXHAUSTIVE_TEST
        // loop through all possible half precision values. a and b have the same value            
        for (i = 0; i < listsize; i++) {
            list[i] = half2fp16(i);   // nextafter 1.0
        }
#else
        if (T(1.1f) != T(1)) {
            // floating point type

            // fill boundary data into array
            for (i = 0; i < 20; i++) {
                //list[i] = T((i - 4) * T(0.25));
                list[i] = T((i - 4) * (0.25f));
            }
            // additional special values, float:
            if constexpr (sizeof(T) == 2) {
                // emulated half precision
                // fill boundary data into array
                for (i = 0; i < 20; i++) {
                    list[i] = float2fp16(float((i - 4) * (0.25f)));
                }
#ifdef TESTNAN   // test also with NAN, INF, and other special data
                list[i++] = half2fp16(0x8000);   // -0
                list[i++] = half2fp16(0x0400);   // smallest positive normal number
                list[i++] = half2fp16(0x8400);   // largest negative normal number
                list[i++] = half2fp16(0x3C01);   // nextafter 1.0
                list[i++] = half2fp16(0x3BFF);   // nextbefore 1.0
                list[i++] = half2fp16(0x7C00);   // inf
                list[i++] = half2fp16(0xFC00);   // -inf
                list[i++] = half2fp16(0x7F00);   // nan
#endif
                // fill random data into rest of array
                for (; i < 100; i++) {
                    list[i] = get_random<Float16>(ran);
                }
                for (; i < listsize; i++) {
                    list[i] = half2fp16(get_random<uint16_t>(ran));
                }
            }
            if constexpr (sizeof(ST) == 4) {  // float
#ifdef TESTNAN   // test also with NAN, INF, and other special data
                list[i++] = (T)bit_castf(0x80000000);   // -0
                list[i++] = (T)bit_castf(0x00800000);   // smallest positive normal number
                list[i++] = (T)bit_castf(0x80800000);   // largest negative normal number
                list[i++] = (T)bit_castf(0x3F7FFFFF);   // nextafter 1.0, 0
                list[i++] = (T)bit_castf(0x3F800001);   // nextafter 1.0, 2
                list[i++] = (T)bit_castf(0x7F800000);   // inf
                list[i++] = (T)bit_castf(0xFF800000);   // -inf
                list[i++] = (T)bit_castf(0x7FF00000);   // nan
                list[i++] = -65512.3058f;               // overflow when converted to float16
                list[i++] = 167890.12345f;              // overflow when converted to float16
                list[i++] = 0.0000517f;                 // subnormal when converted to float16
                list[i++] = 0.0000000594303f;           // underflow when converted to float16
                list[i++] = 1.234E-22f;                 // underflow when exponent is converted
#endif
                // fill random data into rest of array
                for (; i < listsize; i++) {
                    list[i] = (get_random<T>(ran) - 0.4f) * (T)70000.;

                }
            }
            else if constexpr (sizeof(ST) == 8) {  // double
#ifdef TESTNAN   // test also with NAN, INF, and other special data
                list[i++] = (T)bit_castd(0x8000000000000000);   // -0
                list[i++] = (T)bit_castd(0x0010000000000000);   // smallest positive normal number
                list[i++] = (T)bit_castd(0x8010000000000000);   // largest negative normal number
                list[i++] = (T)bit_castd(0x3FEFFFFFFFFFFFFF);   // nextafter 1.0, 0
                list[i++] = (T)bit_castd(0x3FF0000000000001);   // nextafter 1.0, 2
                list[i++] = (T)bit_castd(0x7FF0000000000000);   // inf
                list[i++] = (T)bit_castd(0xFFF0000000000000);   // -inf
                list[i++] = (T)bit_castd(0x7FFC000000000000);   // nan
#endif
                // fill random data into rest of array
                for (; i < listsize; i++) {
                    list[i] = (get_random<T>(ran) - 0.4f) * (T)70000.;

                }
            }
        }
        else {
            if constexpr (vtype::elementtype() < 15) {
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
#endif   // EXHAUSTIVE_TEST
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

// special cass for Float16:
template <>
inline bool compare_scalars<Float16>(Float16 const a, Float16 const b) {
    uint16_t aa = castfp162s(a);
    uint16_t bb = castfp162s(b);
    if ((aa & 0x7FFF) == 0 && (bb & 0x7FFF) == 0) return true;  // both are zero except for sign bit
    if ((aa & 0x7FFF) > 0x7C00 && (bb & 0x7FFF) > 0x7C00) return true;  // both are NAN

    if (aa == bb) return true;
#ifdef IGNORE_SUBNORMAL
    if ((aa & 0x7C00) == 0 && (bb & 0x7C00) == 0) return true;  // both are zero or subnormal
#endif
#ifdef IGNORE_NAN
    if ((aa & 0x7FFF) > 0x7C00 || (bb & 0x7FFF) > 0x7C00) return true;  // ignore NAN results
#endif

#ifdef FACCURACY     // accept minor difference
    int dif = abs(aa - bb);
    if (dif <= FACCURACY) return true;
    printf("\n%i ULP ", dif);
#endif
    return false;
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
    //printf("\na = %G, b=%G, delta = %G, dif=%G ", a, b, delta_unit(a), dif);
    //printf("\n%.0G ULP ", dif);
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
int main () {
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

#if defined(SCALAR_RESULT) || defined(WHOLE_VECTOR)   // result is scalar || test whole vector
#else
    // list for expected results
    RT expectedList[maxvectorsize];
#endif

    int i, j;   // loop counters

    for (i = 0; i < adata.size(); i += vectorsize) {

        //a.load(adata.list + i);
        loadData(a, adata.list + i);

#ifdef EXHAUSTIVE_TEST
        j = i;
        b = a;
        {
#else
        for (j = 0; j < bdata.size(); j += vectorsize) {
            loadData(b, bdata.list + j);
#endif // EXHAUSTIVE_TEST

#if defined(USE_FLAG)
            vtype f(ST(0));
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
#ifdef SCALAR_RESULT
            bool same = compare_scalars(result[0], expected[0]);
            int numresult = 1;
            /*
            if (!same) {
                printf("\nscalar error i=%i j=%i, a=", i, j);
                 //printVal(a[0]); printf(", b="); printVal(b[0]);
                 for (int k=0; k<a.size(); k++) {
                     printf(" %8X ", fp162half(a[k]));
                     printVal(a[k]);
                 }
                 printf(", sum = %8X ", fp162half(result[0])); printVal(result[0]);
                 printf(" expected = %8X ", fp162half(expected[0])); printVal(expected[0]);                 
            }*/

#else
            bool same = compare_vectors(result, expected);
            int numresult = result.size();     // number of elements in result vector

            /*
            if (!same) {
                printf("\nvector error i=%i j=%i, a=", i, j);
                 for (int k=0; k<a.size(); k++) {
                     printf(" %8X ", fp162half(a[k]));
                     printVal(a[k]);
                 }
                 printf("\nr=");
                 for (int k=0; k<a.size(); k++) {
                     printf(" %8X ", fp162half(result[k]));
                     printVal(result[k]);
                 }
            }*/                 

#endif
            if (!same) {
                // values are different. report error 
                if (++numerr == 1) {
                    printf("\ntest case %i:", testcase);  // print test case first time
                }
                printf("\nError at %i, %i:", i, j);
                for (int n = 0; n < numresult; n++) {
                    printf("\n"); printVal(a[n]); 
                    printf(", "); printVal(b[n]); printf(": "); 
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

#if testcase == 900
    printf("instrset_detect = %i\nhasAVX512ER %i\nhasAVX512VBMI %i\nhasAVX512VBMI2 %i\nhasF16C %i\nhasAVX512FP16 %i\n",
        instrset_detect(),
        hasAVX512ER(),
        hasAVX512VBMI(),
        hasAVX512VBMI2(),
        hasF16C(),
        hasAVX512FP16());
#endif

    return numerr;
}
