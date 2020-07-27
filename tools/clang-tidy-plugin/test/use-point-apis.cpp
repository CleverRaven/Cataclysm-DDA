// RUN: %check_clang_tidy %s cata-use-point-apis %t -- -plugins=%cata_plugin -- -isystem %cata_include

#define CATA_NO_STL
#include "point.h"

int f0( int x, int y );
int f0( const point &p );

int g0()
{
    return f0( 0, 1 );
    // CHECK-MESSAGES: warning: Call to 'f0' could instead call overload using a point parameter. [cata-use-point-apis]
    // CHECK-FIXES: return f0( point( 0, 1 ) );
}

// Check that it can swap arguments
int f1( int y, int x );
int f1( const point &p );

int g1()
{
    return f1( 0, 1 );
    // CHECK-MESSAGES: warning: Call to 'f1' could instead call overload using a point parameter. [cata-use-point-apis]
    // CHECK-FIXES: return f1( point( 1, 0 ) );
}

// Check that it verifies the correct overload
int f2( int y, int x );
int f2( const tripoint &p );
int f2( const long &p );

int g2()
{
    return f2( 0, 1 );
}

// Check that it works for tripoints
int f3( int y, int x, int z );
int f3( const tripoint &p );

int g3()
{
    return f3( 0, 1, 2 );
    // CHECK-MESSAGES: warning: Call to 'f3' could instead call overload using a tripoint parameter. [cata-use-point-apis]
    // CHECK-FIXES: return f3( tripoint( 1, 0, 2 ) );
}

// Check that it works amongst other arguments
int f4( float f, int x, int y, int z, int w );
int f4( float f, const tripoint &p, int w );

int g4()
{
    return f4( 0, 1, 2, 3, 4 );
    // CHECK-MESSAGES: warning: Call to 'f4' could instead call overload using a tripoint parameter. [cata-use-point-apis]
    // CHECK-FIXES: return f4( 0, tripoint( 1, 2, 3 ), 4 );
}

// Avoid generating infinite recursion
int f5( int x, int y );
int f5( const point &p )
{
    return f5( p.x, p.y );
}

// Check handling of default parameters
int f6( int x, int y, int z = 0 );
int f6( const tripoint &p );

int g6()
{
    return f6( 1, 2 );
    // CHECK-MESSAGES: warning: Call to 'f6' could instead call overload using a tripoint parameter. [cata-use-point-apis]
    // CHECK-FIXES: return f6( tripoint( 1, 2, 0 ) );
}

int g7()
{
    auto lambda = []( int x, int y ) {
        return x;
    };
    return lambda( 0, 1 );
}

struct A8 {
    int operator()( int x, int y );
    int operator()( const point &p );
};

int g8()
{
    A8 a;
    return a( 0, 1 );
    // CHECK-MESSAGES: warning: Call to 'operator()' could instead call overload using a point parameter. [cata-use-point-apis]
    // CHECK-FIXES: return a( point( 0, 1 ) );
}

struct A9 {
    int f( int x, int y );
    int f( const point &p );
};

int g9()
{
    A9 a;
    return a.f( 0, 1 );
    // CHECK-MESSAGES: warning: Call to 'f' could instead call overload using a point parameter. [cata-use-point-apis]
    // CHECK-FIXES: return a.f( point( 0, 1 ) );
}

// Check function templates
template<typename T>
int f10( T t, int x, int y );
template<typename T>
int f10( T t, const point &p );

int g10()
{
    return f10( "foo", 0, 1 );
    // CHECK-MESSAGES: warning: Call to 'f10<const char *>' could instead call overload using a point parameter. [cata-use-point-apis]
    // CHECK-FIXES: return f10( "foo", point( 0, 1 ) );
}

template<typename... Args>
int f11( int, int x, int y, Args &&... );
template<typename... Args>
int f11( int, const point &p, Args &&... );

int g11()
{
    return f11( 7, 0, 1, "foo", 3.5f );
    // CHECK-MESSAGES: warning: Call to 'f11<char const (&)[4], float>' could instead call overload using a point parameter. [cata-use-point-apis]
    // CHECK-FIXES: return f11( 7, point( 0, 1 ), "foo", 3.5f );
}

// Check const-qualified int args
int f12( const int x, const int y );
int f12( const point &p );

int g12()
{
    return f12( 0, 1 );
    // CHECK-MESSAGES: warning: Call to 'f12' could instead call overload using a point parameter. [cata-use-point-apis]
    // CHECK-FIXES: return f12( point( 0, 1 ) );
}

int f13( int X, int Y );
int f13( const point &p );

int g13()
{
    return f13( 0, 1 );
    // CHECK-MESSAGES: warning: Call to 'f13' could instead call overload using a point parameter. [cata-use-point-apis]
    // CHECK-FIXES: return f13( point( 0, 1 ) );
}

int f14( int x1, int y1 );
int f14( const point &p );

int g14()
{
    return f14( 0, 1 );
    // CHECK-MESSAGES: warning: Call to 'f14' could instead call overload using a point parameter. [cata-use-point-apis]
    // CHECK-FIXES: return f14( point( 0, 1 ) );
}

int f15( int foo_x, int foo_y );
int f15( const point &p );

int g15()
{
    return f15( 0, 1 );
    // CHECK-MESSAGES: warning: Call to 'f15' could instead call overload using a point parameter. [cata-use-point-apis]
    // CHECK-FIXES: return f15( point( 0, 1 ) );
}

int f16( int x = 0, int y = 0 );
int f16( const point &p );

int g16()
{
    return f16();
}

struct A17 {
    A17( int x, int y );
    A17( const point &p );
};

A17 g17()
{
    return A17( 0, 1 );
    // CHECK-MESSAGES: warning: Call to 'A17' could instead call overload using a point parameter. [cata-use-point-apis]
    // CHECK-FIXES: return A17( point( 0, 1 ) );
}

point g18()
{
    return point( 0, 1 );
}

// Check const-qualification is ignored on extra params
int f19( const int x, const int y, const int i );
int f19( const point &p, int i );

int g19()
{
    return f19( 0, 1, 2 );
    // CHECK-MESSAGES: warning: Call to 'f19' could instead call overload using a point parameter. [cata-use-point-apis]
    // CHECK-FIXES: return f19( point( 0, 1 ), 2 );
}

struct A20 {
    // Regression test for calling operator= taking an integral arg
    A20 &operator=( int x );
};

void g20( A20 &a, bool b )
{
    a = b;
}
