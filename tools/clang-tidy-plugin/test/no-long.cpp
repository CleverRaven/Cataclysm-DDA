// RUN: %check_clang_tidy %s cata-no-long %t -- -plugins=%cata_plugin --

#include <limits.h>
#include <stdint.h>

long a1;
// CHECK-MESSAGES: warning: Variable 'a1' declared as 'long'. Prefer int or int64_t to long. [cata-no-long]

unsigned long a2;
// CHECK-MESSAGES: warning: Variable 'a2' declared as 'unsigned long'. Prefer unsigned int or uint64_t to unsigned long. [cata-no-long]

const long a3 = 0;
// CHECK-MESSAGES: warning: Variable 'a3' declared as 'const long'. Prefer int or int64_t to long. [cata-no-long]

long &a4 = a1;
// CHECK-MESSAGES: warning: Variable 'a4' declared as 'long &'. Prefer int or int64_t to long. [cata-no-long]

int64_t c;
uint64_t d;

void f1( long e );
// CHECK-MESSAGES: warning: Variable 'e' declared as 'long'. Prefer int or int64_t to long. [cata-no-long]

long f2();
// CHECK-MESSAGES: warning: Function 'f2' declared as returning 'long'. Prefer int or int64_t to long. [cata-no-long]

int64_t f3();
auto f4() -> decltype( 0L );

int i1 = static_cast<long>( 0 );
// CHECK-MESSAGES: warning: Static cast to 'long'.  Prefer int or int64_t to long. [cata-no-long]
int i2 = static_cast<int64_t>( 0 );

template<typename T>
void g( T gp0, long gp1 = 0 )
{
    // CHECK-MESSAGES: warning: Variable 'gp1' declared as 'long'.  Prefer int or int64_t to long. [cata-no-long]
    long gi0;
    // CHECK-MESSAGES: warning: Variable 'gi0' declared as 'long'.  Prefer int or int64_t to long. [cata-no-long]
    T gi1;
}

void h()
{
    g<long>( 0, 0 );
    // Would like to report an error here for the template argument, but have
    // not found a way to do so.

    g( LONG_MIN );
    // CHECK-MESSAGES: warning: Use of long-specific macro LONG_MIN [cata-no-long]
    g( LONG_MAX );
    // CHECK-MESSAGES: warning: Use of long-specific macro LONG_MAX [cata-no-long]
    g( ULONG_MAX );
    // CHECK-MESSAGES: warning: Use of long-specific macro ULONG_MAX [cata-no-long]
}

template<typename T>
struct A {
    T Af0();
    long Af1();
    // CHECK-MESSAGES: warning: Function 'Af1' declared as returning 'long'.  Prefer int or int64_t to long. [cata-no-long]
    T Af2( long Af2i );
    // CHECK-MESSAGES: warning: Variable 'Af2i' declared as 'long'.  Prefer int or int64_t to long. [cata-no-long]
};

A<long> a;

auto l0 = []( int64_t a )
{
    return a;
};
