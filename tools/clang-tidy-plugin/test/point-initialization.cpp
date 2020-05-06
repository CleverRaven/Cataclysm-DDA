// RUN: %check_clang_tidy %s cata-point-initialization %t -- -plugins=%cata_plugin -- -isystem %cata_include

#define CATA_NO_STL
#include "point.h"

struct non_point {
    constexpr non_point() = default;
    constexpr non_point( int, int );
};

point p0 = point_zero;
// CHECK-MESSAGES: warning: Unnecessary initialization of 'p0'. 'point' is zero-initialized by default. [cata-point-initialization]
point p1( 0, 0 );
// CHECK-MESSAGES: warning: Unnecessary initialization of 'p1'. 'point' is zero-initialized by default. [cata-point-initialization]

tripoint t0 = tripoint_zero;
// CHECK-MESSAGES: warning: Unnecessary initialization of 't0'. 'tripoint' is zero-initialized by default. [cata-point-initialization]
tripoint t1( 0, 0, 0 );
// CHECK-MESSAGES: warning: Unnecessary initialization of 't1'. 'tripoint' is zero-initialized by default. [cata-point-initialization]

// Verify that it doesn't trigger on only some zero args
tripoint t1a( 0, 0, 1 );

// Verify that it doesn't trigger on other types
non_point n0( 0, 0 );

// Verify that it doesn't trigger on parameter default arguments
void f( point p = point_zero );

struct A {
    A() : p2( point_zero ) {}
    // CHECK-MESSAGES: warning: Unnecessary initialization of 'p2'. 'point' is zero-initialized by default. [cata-point-initialization]
    A( int ) : p3( 0, 0 ) {}
    // CHECK-MESSAGES: warning: Unnecessary initialization of 'p3'. 'point' is zero-initialized by default. [cata-point-initialization]
    A( char ) : t2( tripoint_zero ) {}
    // CHECK-MESSAGES: warning: Unnecessary initialization of 't2'. 'tripoint' is zero-initialized by default. [cata-point-initialization]
    A( long ) : t3( 0, 0, 0 ) {}
    // CHECK-MESSAGES: warning: Unnecessary initialization of 't3'. 'tripoint' is zero-initialized by default. [cata-point-initialization]
    A( short ) : n2( 0, 0 ) {}

    point p2;
    point p3;
    tripoint t2;
    tripoint t3;
    non_point n2;
};
