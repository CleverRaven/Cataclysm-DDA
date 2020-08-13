// RUN: %check_clang_tidy %s cata-use-named-point-constants %t -- -plugins=%cata_plugin -- -isystem %cata_include

#define CATA_NO_STL
#include "point.h"

point p0( 0, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point_zero' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: point p0( point_zero );

tripoint p1( 0, 0, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint_zero' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p1( tripoint_zero );

point p2( 3, 3 );

point p3( 0, -1 );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point_north' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: point p3( point_north );

point p4( 1, -1 );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point_north_east' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: point p4( point_north_east );

point p5( +1, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point_east' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: point p5( point_east );

point p6( 1, 1 );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point_south_east' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: point p6( point_south_east );

point p7( 0, 1 );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point_south' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: point p7( point_south );

point p8( -1, 1 );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point_south_west' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: point p8( point_south_west );

point p9( -1, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point_west' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: point p9( point_west );

point p10( -1, -1 );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point_north_west' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: point p10( point_north_west );

tripoint p11( 0, 0, 1 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint_above' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p11( tripoint_above );

tripoint p12( 0, 0, -1 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint_below' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p12( tripoint_below );

tripoint p13( 0, -1, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint_north' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p13( tripoint_north );

tripoint p14( 1, -1, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint_north_east' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p14( tripoint_north_east );

tripoint p15( 1, 0, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint_east' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p15( tripoint_east );

tripoint p16( 1, 1, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint_south_east' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p16( tripoint_south_east );

tripoint p17( 0, 1, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint_south' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p17( tripoint_south );

tripoint p18( -1, 1, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint_south_west' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p18( tripoint_south_west );

tripoint p19( -1, 0, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint_west' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p19( tripoint_west );

tripoint p20( -1, -1, 0 );
// CHECK-MESSAGES: warning: Prefer constructing 'tripoint' from named constant 'tripoint_north_west' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: tripoint p20( tripoint_north_west );

int f21( const point & );
point p21;
int i21 = f21( p21 + point( 1, 0 ) );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point_east' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: int i21 = f21( p21 + point_east );

int f22( const point & = point( 0, 0 ) );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point_zero' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: int f22( const point & = point_zero );

int f23( const point & = { 0, 0 } );
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point_zero' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: int f23( const point & = point_zero );

point p24[1] = { { 0, 0 } };
// CHECK-MESSAGES: warning: Prefer constructing 'point' from named constant 'point_zero' rather than explicit integer arguments. [cata-use-named-point-constants]
// CHECK-FIXES: point p24[1] = { point_zero };
