// RUN: %check_clang_tidy -allow-stdinc %s cata-use-named-point-constants %t -- --load=%cata_plugin -- -isystem %cata_include

#include "point.h"

point p0( 0, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point::zero' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: point p0( point::zero );

tripoint p1( 0, 0, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint::zero' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p1( tripoint::zero );

point p2( 3, 3 );

point p3( 0, -1 );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point::north' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: point p3( point::north );

point p4( 1, -1 );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point::north_east' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: point p4( point::north_east );

point p5( +1, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point::east' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: point p5( point::east );

point p6( 1, 1 );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point::south_east' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: point p6( point::south_east );

point p7( 0, 1 );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point::south' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: point p7( point::south );

point p8( -1, 1 );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point::south_west' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: point p8( point::south_west );

point p9( -1, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point::west' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: point p9( point::west );

point p10( -1, -1 );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point::north_west' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: point p10( point::north_west );

tripoint p11( 0, 0, 1 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint::above' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p11( tripoint::above );

tripoint p12( 0, 0, -1 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint::below' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p12( tripoint::below );

tripoint p13( 0, -1, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint::north' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p13( tripoint::north );

tripoint p14( 1, -1, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint::north_east' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p14( tripoint::north_east );

tripoint p15( 1, 0, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint::east' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p15( tripoint::east );

tripoint p16( 1, 1, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint::south_east' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p16( tripoint::south_east );

tripoint p17( 0, 1, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint::south' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p17( tripoint::south );

tripoint p18( -1, 1, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint::south_west' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p18( tripoint::south_west );

tripoint p19( -1, 0, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint::west' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p19( tripoint::west );

tripoint p20( -1, -1, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint::north_west' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p20( tripoint::north_west );

int f21( const point & );
point p21;
int i21 = f21( p21 + point( 1, 0 ) );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point::east' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: int i21 = f21( p21 + point::east );

int f22( const point & = point( 0, 0 ) );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point::zero' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: int f22( const point & = point::zero );

int f23( const point & = { 0, 0 } );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point::zero' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: int f23( const point & = point::zero );

point p24[1] = { { 0, 0 } };
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point::zero' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: point p24[1] = { point::zero };
