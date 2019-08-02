/****************************  testbench2.cpp   *******************************
* Author:        Agner Fog
* Date created:  2019-04-15
* Last modified: 2019-08-02
* Version:       2.00
* Project:       Testbench for vector class library, 2: permute functions etc.
* Description:
* Compile and run this program to test permute functions etc. in VCL
* This file contains test cases for permute, blend, lookup, gather and
* scatter functions, and other functions that need template parameters
*
* Instructions:
* The following parameters must be defined on the command line or added 
* in the top of this file:
*
* vtype:     Vector type to test
*
* vtypei:    Vector type for indexes, if different from vtype.
*
* testcase:  A number defining a function to test. 
*            See the cases in this file.
*
* funcname:  Name of function to test
*
* indexes:   A comma-separated list of indexes to be used as template parameters in
*            permute, blend, gather, and scatter functions, or indexes in lookup functions
*
* seed:      Seed for random number generator. May be any integer
*
* INSTRSET:  Desired instruction set. Needs to be specified for MS compiler,
*            but determined automatically for other compilers. Values:
*            2:  SSE2
*            3:  SSE3
*            4:  SSSE3
*            5:  SSE4.1
*            6:  SSE4.2
*            7:  AVX
*            8:  AVX2
*            9:  AVX512F
*            10: AVX512BW/DQ/VL
*
* testcase: A number defining the type of function to test
*
*
* Compile with any compiler supported by VCL.
* Specify the desired instruction set and optimization options as parameters
* to the compiler.
*
* (c) Copyright Agner Fog 2019,
* Gnu general public license 3.0 https://www.gnu.org/licenses/gpl.html
******************************************************************************
Test cases:
1:  permute
2:  blend
3:  lookup with one index vector and one data vector
4:  lookup with one index vector and two data vectors
5:  lookup with one index vector and four data vectors
6:  lookup with table
7:  shift_bytes_up
8:  shift_bytes_down
9:  change_sign
10: gather
11: scatter with fixed indexes
12: scatter with variable indexes
*****************************************************************************/

#include <stdio.h>


//#define __AVX512VBMI__
//#define __AVX512VBMI2__

#ifndef INSTRSET
#define INSTRSET      8      // desired instruction set
#endif


#include <vectorclass.h>

#ifndef testcase
// ----------------------------------------------------------------------------
//            Specify input parameters here if running from an IDE
// ----------------------------------------------------------------------------

#define testcase 1

#define vtype Vec4i

#define rtype vtype

#define funcname permute4

#define indexes  2,2,0,1

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

// function name
#ifndef funcname
#define funcname permute2
#endif

// indexes
#ifndef indexes
#define indexes 0,0
#endif

// random number seed
#ifndef seed
#define seed 1
#endif


#endif  // testcase
// ----------------------------------------------------------------------------
//             Definitions
// ----------------------------------------------------------------------------

#ifndef vtypei
#define vtypei vtype
#endif

// dummy vector used for getting element type
vtype dummy;
typedef decltype(dummy[0]) ST;                   // scalar type for data vectors

rtype dummyr;
typedef decltype(dummyr[0]) RT;                  // scalar type for return type data vectors

vtypei dummyi;
typedef decltype(dummyi[0]) STI;                 // scalar type for index vectors

const int vectorsize = sizeof(vtype) / sizeof(ST); // size of vector

int indexlist[vectorsize] = { indexes };         // make list of indexes
const int indexnum = sizeof(indexlist) / sizeof(int);  // number of indexes

const int tablesize = 1024;                      // size of index table
STI indextable[tablesize];                       // index table for lookup, gather, scatter
RT  datatable[tablesize];                        // data table for lookup<>


const int maxerrors = 10;                        // maximum errors to report
int numerr = 0;                                  // count errors
int error = 0;                                   // position of error + 1, -1 = index out of range


/************************************************************************
*
*                          Test cases
*
************************************************************************/

#if   testcase == 1    // permute
inline vtype testFunction(vtype const& a, vtype const&) {
    vtype r = funcname<indexes>(a);                // call permute function
    return r;
}

vtype compareFunction(vtype const& r, vtype const& a, vtype const&) {
    // compare result r with expected value of permute<indexes>(a)
    ST result[vectorsize];                       // array for result
    error = 0;                                   // no error yet
    for (int j = 0; j < vectorsize; j++) {       // loop through result vector
        int i = indexlist[j];                    // corresponding index
        if (i == -1) {
            result[j] = 0;                       // -1 means set to zero
        }
        else if (i == V_DC) {                    // don't care
            result[j] = -1;
            continue;
        }
        else if ((uint32_t)i >= vectorsize) {
            result[j] = -1;
            error = -1;                          // index out of range          
        }
        else {
            result[j] = a[i];                    // get permuted value
        }
        if (r[j] != result[j]) {                 // error
            error = j + 1;                         // error position
        }
    }
    return vtype().load(result);                 // return expected vector
}


#elif   testcase == 2    // blend
inline vtype testFunction(vtype const& a, vtype const& b) {
    vtype r = funcname<indexes>(a, b);          // call blend function
    // r.insert(0, 99);
    return r;
}

vtype compareFunction(vtype const& r, vtype const& a, vtype const& b) {
    // compare result r with expected value of permute<indexes>(a)
    ST result[vectorsize];                       // array for result
    error = 0;                                   // no error yet
    for (int j = 0; j < vectorsize; j++) {       // loop through result vector
        int i = indexlist[j];                    // corresponding index
        if (i == -1) {
            result[j] = 0;                       // -1 means set to zero
        }
        else if (i == V_DC) {                    // don't care
            result[j] = -1;
            continue;
        }
        else if ((uint32_t)i >= vectorsize * 2) {
            result[j] = -1;
            error = -1;                          // index out of range          
        }
        else if (i >= vectorsize) {
            result[j] = b[i - vectorsize];       // get permuted value from b
        }
        else {
            result[j] = a[i];                    // get permuted value from a
        }
        if (r[j] != result[j]) {                 // error
            error = j + 1;                         // error position
        }
    }
    return vtype().load(result);                 // return expected vector
}

#elif   testcase == 3    // lookup function with one index vector and one data vector

inline rtype testFunction(vtypei const& i, rtype const& a) {
    rtype r = funcname(i, a);                    // call lookup function
    return r;
}

rtype compareFunction(rtype const& r, vtypei const& ix, rtype const& a) {
    // compare result r with expected value
    RT result[vectorsize];                       // array for result
    error = 0;                                   // no error yet
    for (int j = 0; j < vectorsize; j++) {       // loop through result vector
        int i = (int)ix[j];                      // corresponding index
        if ((uint32_t)i >= (uint32_t)a.size()) {
            result[j] = -1;
            error = -1;                          // index out of range          
        }
        else {
            result[j] = a[i];                    // get permuted value
        }
        if (r[j] != result[j]) {                 // error
            error = j + 1;                       // error position
        }
    }
    return rtype().load(result);                 // return expected vector
}

#elif   testcase == 4    // lookup function with one index vector and two data vectors

inline rtype testFunction(vtypei const& i, rtype const& a, rtype const& b) {
    rtype r = funcname(i, a, b);                    // call lookup function
    return r;
}

rtype compareFunction(rtype const& r, vtypei const& ix, rtype const& a, rtype const& b) {
    // compare result r with expected value
    RT result[vectorsize];                       // array for result
    error = 0;                                   // no error yet
    for (int j = 0; j < vectorsize; j++) {       // loop through result vector
        int i = ix[j];                           // corresponding index
        if ((uint32_t)i >= (uint32_t)a.size() * 2) {
            result[j] = -1;
            error = -1;                          // index out of range          
        }
        else if (i >= a.size()) {
            result[j] = b[i - a.size()];           // get permuted value from b
        }
        else {
            result[j] = a[i];                    // get permuted value from a
        }
        if (r[j] != result[j]) {                 // error
            error = j + 1;                         // error position
        }
    }
    return rtype().load(result);                 // return expected vector
}

#elif   testcase == 5    // lookup function with one index vector and four data vectors


inline vtype testFunction(vtypei const& i, vtype const& a, vtype const& b, vtype const& c, vtype const& d) {
    vtype r = funcname(i, a, b, c, d);           // call lookup function
     //  r.insert(1, 99); 
    return r;
}

vtype compareFunction(vtype const& r, vtypei const& ix, vtype const& a, vtype const& b, vtype const& c, vtype const& d) {
    // compare result r with expected value
    ST result[vectorsize];                       // array for result
    error = 0;                                   // no error yet
    for (int j = 0; j < vectorsize; j++) {       // loop through result vector
        uint32_t i = (uint32_t)ix[j];            // corresponding index
        if (sizeof(ST) == 1) i &= 0xFF;          // index may be unsigned
        if (i >= (uint32_t)a.size() * 4) {
            result[j] = -1;
            error = -1;                          // index out of range          
        }
        else if (i >= (uint32_t)a.size() * 3) {
            result[j] = d[i - a.size() * 3];     // get permuted value from d
        }
        else if (i >= (uint32_t)a.size() * 2) {
            result[j] = c[i - a.size() * 2];     // get permuted value from c
        }
        else if (i >= (uint32_t)a.size()) {
            result[j] = b[i - a.size()];         // get permuted value from b
        }
        else {
            result[j] = a[i];                    // get permuted value from a
        }
        if (r[j] != result[j]) {                 // error
            error = j + 1;                       // error position
        }
    }
    return vtype().load(result);                 // return expected vector
}

#elif   testcase == 6    // lookup function with table

inline rtype testFunction1(vtypei const & ix, RT const * table) {
    rtype r = lookup<tablesize>(ix, table);         // call lookup function
    return r;
}

inline rtype testFunction2(vtypei const & ix, RT const * table) {
    rtype r = lookup<vectorsize>(ix, table);      // call lookup function
    return r;
}

inline rtype testFunction3(vtypei const & ix, RT const * table) {
    rtype r = lookup<vectorsize * 2 - 1>(ix, table);      // call lookup function
    return r;
}

rtype compareFunction(uint32_t imax, rtype const & r, vtypei const & ix) {
    // compare result r with expected value
    RT result[vectorsize];                          // array for result
    error = 0;                                      // no error yet
    for (int j = 0; j < vectorsize; j++) {          // loop through result vector
        uint32_t i = (uint32_t)ix[j];               // corresponding index
        if (sizeof(ix[0]) == 1) i &= 0xFF; 
        if ((uint32_t)i >= (uint32_t)imax) {
            result[j] = -1;
            error = -1;                             // index out of range          
        }
        else {
            result[j] = datatable[i];               // get permuted value from datatable
        }
        if (r[j] != result[j]) {                    // error
            error = j + 1;                          // error position
        }
    }
    return rtype().load(result);                    // return expected vector
}

#elif   testcase == 7    // shift_bytes_up

inline vtype testFunction(vtypei const& a) {
    vtype r = funcname<indexes>(a);           // call shift_bytes_up function
    return r;
}

vtype compareFunction(vtype const& r, vtypei const& a) {
    ST result[vectorsize*2] = {0};
    const int count = indexes;
    if (count >= vectorsize) return 0;
    int ct = count % vectorsize;
    a.store(result+ct);
    error = 0;                                      // no error yet
    for (int j = 0; j < vectorsize; j++) {          // loop through result vector
        if (r[j] != result[j]) {                    // error
            error = j + 1;                          // error position
        }
    }
    return vtype().load(result);                    // return expected vector
}

#elif   testcase == 8    // shift_bytes_down

inline vtype testFunction(vtypei const& a) {
    vtype r = funcname<indexes>(a);           // call shift_bytes_up function
    return r;
}

vtype compareFunction(vtype const& r, vtypei const& a) {
    ST result[vectorsize*4];
    a.store(result);
    vtype(0).store(result+vectorsize);
    error = 0;                                      // no error yet
    const int count = indexes;
    if (count >= a.size()) return 0;
    const int ct = count % a.size();

    for (int j = 0; j < a.size(); j++) {          // loop through result vector
        if (r[j] != result[j+ct]) {                    // error
            error = j + 1;                          // error position
        }
    }
    return vtype().load(result+ct);                    // return expected vector
}

#elif  testcase == 9    // change_sign

inline vtype testFunction(vtypei const& a) {
    vtype r = funcname<indexes>(a);           // call change_sign function
    return r;
}

vtype compareFunction(vtype const& r, vtypei const& a) {
    ST result[vectorsize];
    a.store(result);
    for (int j = 0; j < a.size(); j++) {          // loop through result vector
        if (indexlist[j] != 0) {                  // index = 1
            result[j] = -result[j];               // change sign
        }
    }
    return rtype().load(result);                  // return expected vector
}

#elif   testcase == 10    // gather function

// (remember to test with indexes that cover all special cases:
// fitting into one block, fitting into two separate blocks, or sparce)

inline rtype testFunction(vtypei const & ix, RT const* table) {
    rtype r = funcname<indexes>(table);         // call lookup function
    return r;
}

rtype compareFunction(rtype const& r, vtypei const& ix) {
    // compare result r with expected value
    RT result[vectorsize];                          // array for result
    error = 0;                                      // no error yet
    for (int j = 0; j < vectorsize; j++) {          // loop through result vector
        int i = ix[j];                              // corresponding index
        result[j] = datatable[i];               // get permuted value from datatable
        if (r[j] != result[j]) {                    // error
            error = j + 1;                          // error position
        }
    }
    return rtype().load(result);                    // return expected vector
}

#elif   testcase == 11    // scatter function with fixed indexes

inline void testFunction(rtype const & a, RT * table) {
    scatter<indexes>(a, table);         // call lookup function
}

void compareFunction(rtype const & a, RT * table) {
    // compare result r with expected value
    error = 0;                                      // no error yet
    for (int j = 0; j < vectorsize; j++) {          // loop through result vector
        int i = indexlist[j];                       // corresponding index
        if (i >= 0) {
            table[i + tablesize / 2] = a[j];
        }
    }
    for (int j = 0; j < tablesize / 2; j++) {       // loop to compare outputs
        if (table[j] != table[j + tablesize / 2]) {
            error = j;
            numerr++;
        }
    }
}

#elif   testcase == 12    // scatter function with variable indexes

inline void testFunction(vtypei const & ix, int limit, rtype const & a, RT * table) {
    funcname(ix, limit, a, table);         // call lookup function
    //scatter16f(Vec16i index, uint32_t limit, Vec16f data, float * destination) {
}

void compareFunction(vtypei const & ix, int limit, rtype const & a, RT * table) {
    // compare result r with expected value
    error = 0;                                      // no error yet
    for (int j = 0; j < vectorsize; j++) {          // loop through result vector
        int i = (int)ix[j];                         // corresponding index
        if (i >= 0 && i < limit) {
            table[i + tablesize / 2] = a[j];
        }
    }
    for (int j = 0; j < tablesize / 2; j++) {       // loop to compare outputs
        if (table[j] != table[j + tablesize / 2]) {
            error = j;
            numerr++;
        }
    }
}

#else
// End of test cases
#error unknown test case

#endif



// ----------------------------------------------------------------------------
//                           Overhead functions
// ----------------------------------------------------------------------------

// type-specific printing functions
void printReduced(int8_t x) {
    printf("%4i", x);
}

void printReduced(uint8_t x) {
    printf("%4u", x);
}

void printReduced(int16_t x) {
    printf("%4i", x);
}

void printReduced(uint16_t x) {
    printf("%4u", x);
}

void printReduced(int32_t x) {
    printf("%4i", x);
}

void printReduced(uint32_t x) {
    printf("%4u", x);
}

void printReduced(uint64_t x) {
    if (x >> 32 != 0) {
        printf("0x%X%08X", uint32_t(x >> 32), uint32_t(x));
    }
    else {    
        printf("%4u", (uint32_t)x);
    }
}

void printReduced(int64_t x) {
    printf("%8X", int32_t(x));
}

void printReduced(float x) {
    printf("%6.3G", x);
}

void printReduced(double x) {
    printf("%6.3G", x);
}

void printReduced(bool x) {
    printf("%2i", (int)x);
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

// template to generate list of testdata
template <typename T>
class TestData {
public:
    enum LS {
        // define array size. Must be a multiple of vector size:
        listsize = 0x200
    };
    TestData(){};                                // constructor
    void makeRandom() {                          // fill random data into array
        for (int i = 0; i < listsize; i++) {
            list[i] = get_random<T>(ran);
        }
    }
    T list[listsize];                            // array of test data
    int size() {                                 // get list size
        return listsize;
    }
};

// make random indexes
vtypei makeIndexes(int n) {
    int i;                                       // loop counter
    // fill index table
    for (int i = 0; i < tablesize; i++) {
        uint32_t ix = get_random<uint32_t>(ran); // random index
        indextable[i] = STI(ix % uint32_t(n));    // modulo n
        datatable[i] = get_random<RT>(ran);
    }
    // make index vector
    vtypei vi;
    for (i = 0; i < vi.size(); i++) {
        vi.insert(i, STI(indextable[i]));
    }
    return vi;
}

// report if error
void errorreport(vtype const& a, vtype const& b, rtype const& r, rtype const& e) {
    if (numerr == 0) {
        printf("\ntest case %i:", testcase);
    }
    if (error == -1) {
        printf("\nindex out of range");
    }
    printf("\nindex, input, output, expected:");
    for (int j = 0; j < vectorsize; j++) {
        int i = indexlist[j];
        if (testcase >= 3) i = (int)indextable[j];
        if (error - 1 == j || (r[j] != e[j] && i != V_DC)) {
            printf("\n-> ");
        }
        else {
            printf("\n   ");
        }
        if (i == V_DC) printf("V_DC, ");
        else printf("%4i, ", i);
        printReduced(a[j]);  printf(",  ");
        if (testcase == 2) {
            printReduced(b[j]);  printf(",  ");
        }
        printReduced(r[j]);  printf(",  ");
        printReduced(e[j]);
    }
}


// program entry
int main(int argc, char* argv[]) {
    vtype a, b, c, d;             // operand vectors
    rtype ss;                     // data to scatter
    rtype result;                 // result vector
    rtype expected;               // expected result
    const int vectorsize = sizeof(vtype) / sizeof(decltype(a[0]));
    int ntest = 1;                // number of test runs

    // lists of test data
    TestData<ST> adata, bdata, cdata, ddata;

#if testcase < 3   // permute and blend
    // call function to test
    adata.makeRandom();
    a.load(adata.list);
    bdata.makeRandom();
    b.load(bdata.list);

    vtype r = testFunction(a, b);

    // compare with expected values
    vtype e = compareFunction(r, a, b);

    if (error) {
        errorreport(a, b, r, e);
        numerr++;
    }

#elif testcase == 3                              // lookup function with one data vector

    //adata.makeRandom();
    ntest = 20;                                  // number of test runs
    for (int t = 0; t < ntest; t++) {            // loop for test runs
        vtypei indx;                                 // index vector
        indx = makeIndexes(vectorsize);          // make random index vector
        ss.load(datatable);
        rtype r = testFunction(indx, ss);        // call function to test
        rtype e = compareFunction(r, indx, ss);  // compare with expected values
        if (error) {
            errorreport(a, a, r, e);
            numerr++;
        }
    }

#elif testcase == 4                                   // lookup function with two data vectors

    rtype ss2;
    vtypei indx;                                      // index vector
    ntest = 20;                                       // number of test runs
    for (int t = 0; t < ntest; t++) {                 // loop for test runs
        indx = makeIndexes(vectorsize * 2);           // make random index vector
        ss.load(datatable);
        ss2.load(datatable + ss.size());
        rtype r = testFunction(indx, ss, ss2);        // call function to test
        rtype e = compareFunction(r, indx, ss, ss2);  // compare with expected values
        if (error) {
            errorreport(a, b, r, e);
            numerr++;
        }
    }

#elif testcase == 5                              // lookup function with four data vectors

    // more test data
    adata.makeRandom();
    a.load(adata.list);
    bdata.makeRandom();
    b.load(bdata.list);
    cdata.makeRandom();
    c.load(cdata.list);
    ddata.makeRandom();
    d.load(ddata.list);

    vtypei indx;                                 // index vector
    ntest = 20;                                  // number of test runs
    for (int t = 0; t < ntest; t++) {            // loop for test runs
        indx = makeIndexes(vectorsize * 4);      // make random index vector
        vtype r = testFunction(indx, a, b, c, d);// call function to test
        vtype e = compareFunction(r, indx, a, b, c, d); // compare with expected values
        if (error) {
            errorreport(a, b, r, e);
            numerr++;
        }
    }

#elif   testcase == 6    // lookup function with table

    vtypei indx;                                 // index vector
    ntest = 20;                                  // number of test runs
    // test with large table (tablesize)
    for (int t = 0; t < ntest; t++) {            // loop for test runs
        indx = makeIndexes(tablesize);           // make random index vector
        rtype r = testFunction1(indx, datatable);// call function to test
        rtype e = compareFunction(tablesize, r, indx); // compare with expected values
        if (error) {
            errorreport(a, b, r, e);
            numerr++;
        }
    }
    // test with small table (vectorsize)
    for (int t = 0; t < ntest; t++) {            // loop for test runs
        indx = makeIndexes(vectorsize);          // make random index vector
        rtype r = testFunction2(indx, datatable);// call function to test
        rtype e = compareFunction(vectorsize, r, indx); // compare with expected values
        if (error) {
            errorreport(a, b, r, e);
            numerr++;
        }
    }

    // test with small table (vectorsize*2-1)
    for (int t = 0; t < ntest; t++) {            // loop for test runs
        indx = makeIndexes(vectorsize * 2 - 1);  // make random index vector
        rtype r = testFunction3(indx, datatable);// call function to test
        rtype e = compareFunction(vectorsize * 2, r, indx); // compare with expected values
        if (error) {
            errorreport(a, b, r, e);
            numerr++;
        }
    }

#elif   testcase == 7 || testcase == 8   // shift_bytes_up / shift_bytes_down
    // Note: 'indexes' is set to the shift count

    adata.makeRandom();
    a.load(adata.list);

    vtype r = testFunction(a);

    error = 0;

    // compare with expected values
    vtype e = compareFunction(r, a);

    if (error) {
        errorreport(a, b, r, e);
        numerr++;
    }

#elif   testcase == 9  // change_sign

    adata.makeRandom();
    a.load(adata.list);

    vtype r = testFunction(a);

    error = 0;

    // compare with expected values
    vtype e = compareFunction(r, a);

    if (horizontal_or(r != e)) {
        errorreport(a, b, r, e);
        numerr++;
    }


#elif   testcase == 10    // gather function with table


    STI indxlst[vectorsize] = { indexes };      // index vector
    vtypei indx = vtypei().load(indxlst);

    ntest = 10;                                  // number of test runs
    // test with large table (tablesize)
    for (int t = 0; t < ntest; t++) {            // loop for test runs
        rtype r = testFunction(indx, datatable); // call function to test
        rtype e = compareFunction(r, indx);      // compare with expected values
        if (error) {
            errorreport(a, b, r, e);
            numerr++;
        }
    }


#elif   testcase == 11    // scatter function with fixed indexes

    // make random data
    makeIndexes(vectorsize);
    ss.load(datatable);
    // zero table
    int j;
    for (j = 0; j < tablesize; j++) datatable[j] = 0;
    // test indexes
    for (j = 0; j < vectorsize; j++) {
        if (indexlist[j] >= tablesize / 2) {
            printf("\nError index out of range: %i", indexlist[j]);
            exit(1);
        }
    }
    // scatter
    testFunction(ss, datatable); 

    // compare
    compareFunction(ss, datatable);
    if (numerr) {
        printf("\nscatter error at position %i", error);
    }

#elif   testcase == 12    // scatter function with variable indexes

    // make random data
    makeIndexes(vectorsize);
    ss.load(datatable);
    // zero table
    int j;
    for (j = 0; j < tablesize; j++) datatable[j] = 0;

    // scatter
    vtypei ix = vtypei().load(indextable);

    ix.insert(3, 99999);  // test out of range

    testFunction(ix, tablesize / 2, ss, datatable);

    // compare
    compareFunction(ix, tablesize / 2, ss, datatable);
    if (numerr) {
        printf("\nscatter error at position %i", error);
    } 

#else

#error Unknown test case

#endif
  
    if (numerr == 0) {
        printf("\nsuccess\n");
    }
    printf("\n");

    return numerr;
}
