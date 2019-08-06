// RUN: %check_clang_tidy %s cata-use-point-apis %t -- -plugins=%cata_plugin --

// Can't include the real point header because this is compiled with
// -nostdinc++ and I couldn't see an easy way to change that.
struct point {
    constexpr point() : x( 0 ), y( 0 ) {}
    constexpr point( int x, int y );
    int x;
    int y;
};
struct tripoint {
    constexpr tripoint() = default;
    constexpr tripoint( int x, int y, int z );
};

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
