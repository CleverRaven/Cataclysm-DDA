// RUN: %check_clang_tidy %s cata-simplify-point-constructors %t -- -plugins=%cata_plugin -- -isystem %src_dir

// Can't include the real point header because this is compiled with
// -nostdinc++ and I couldn't see an easy way to change that.
struct point {
    constexpr point() : x( 0 ), y( 0 ) {}
    constexpr point( int x, int y );

    int x;
    int y;
};
struct tripoint {
    constexpr tripoint() : x( 0 ), y( 0 ), z( 0 ) {}
    constexpr tripoint( int x, int y, int z );

    int x;
    int y;
    int z;
};

point p0;
point p0a( p0.x, p0.y );
// CHECK-MESSAGES: warning: Construction of point can be simplified. [cata-simplify-point-constructors]
// CHECK-FIXES: point p0a( p0 );

point p1;
point p1a = point( p1.x, p1.y );
// CHECK-MESSAGES: warning: Construction of point can be simplified. [cata-simplify-point-constructors]
// CHECK-FIXES: point p1a = p1;

point p2;
tripoint p2a = tripoint( p2.x, p2.y, 0 );
// CHECK-MESSAGES: warning: Construction of point can be simplified. [cata-simplify-point-constructors]
// CHECK-FIXES: point p2a = tripoint( p2, 0 );
