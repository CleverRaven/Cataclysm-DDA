// RUN: %check_clang_tidy %s cata-use-point-arithmetic %t -- -plugins=%cata_plugin -- -isystem %cata_include

#define CATA_NO_STL
#include "point.h"

point p0a, p0b;
point p0( p0a.x + p0b.x, p0a.y + p0b.y );
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: point p0( p0a + p0b );

tripoint p1a, p1b;
tripoint p1( p1a.x + p1b.x, p1a.y + p1b.y, p1a.z + p1b.z );
// CHECK-MESSAGES: warning: Construction of 'tripoint' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: point p1( p1a + p1b );

point p2a;
point p2( p2a.x + 1, p2a.y );
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: point p2( p2a + point( 1, 0 ) );

tripoint p3a, p3b;
tripoint p3( p3a.xy() + p3b.xy(), p3a.z + p3b.z );
// CHECK-MESSAGES: warning: Construction of 'tripoint' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: point p3( p3a + p3b );

point p4a, p4b;
point p4( p4a.x - p4b.x, p4a.y - p4b.y );
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: point p4( p4a - p4b );

tripoint p5a, p5b;
tripoint p5( p5a.xy() - p5b.xy(), p5a.z - p5b.z );
// CHECK-MESSAGES: warning: Construction of 'tripoint' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: point p5( p5a - p5b );

tripoint p6a, p6b;
tripoint p6( p6a.xy() - p6b.xy(), p6a.z + p6b.z );
// CHECK-MESSAGES: warning: Construction of 'tripoint' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: point p6( p6a + tripoint( -p6b.xy(), p6b.z ) );

point p7a, p7b, p7c;
point p7( p7a.x - ( p7b.x - p7c.x ), p7a.y - p7b.y + p7c.y );
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: point p7( p7a - p7b + p7c );

point p8a;
point p8( p8a.x - ( 3 && 4 ), p8a.y + 7 );
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: point p8( p8a + point( -( 3 && 4 ), 7 ) );

point p9a, p9b;
point p9( p9a.x + p9b.x + p9a.x, p9a.y + p9b.y + p9a.y );
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: point p9( 2 * p9a + p9b );

tripoint p10a;
point p10b;
tripoint p10( p10a.x + p10b.x, p10a.y + p10b.y, p10a.z );
// CHECK-MESSAGES: warning: Construction of 'tripoint' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: point p10( p10a + p10b );

tripoint p11a;
tripoint p11b;
tripoint p11( p11a.x + p11b.x, p11a.y + p11b.y, p11a.z );
// CHECK-MESSAGES: warning: Construction of 'tripoint' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: point p11( p11a + p11b.xy() );

point p12a;
point p12b;
point p12( 2 * p12a.x + p12b.x, 2 * p12a.y + p12b.y );
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: point p12( 2 * p12a + p12b );

point p13a;
point p13b;
point p13( p13a.x * 2 + p13b.x, p13a.y * 2 + p13b.y );
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: point p13( 2 * p13a + p13b );

tripoint p14a;
tripoint p14b;
tripoint p14( 2 * p14a.x + p14b.x, 2 * p14a.y + p14b.y, p14a.z + p14b.z );
// CHECK-MESSAGES: warning: Construction of 'tripoint' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: tripoint p14( tripoint( 2 * p14a.x, 2 * p14a.y, p14a.z ) + p14b );

tripoint p15a;
tripoint p15b;
tripoint p15( p15a.xy() * 2 + p15b.xy(), p15a.z + p15b.z );
// CHECK-MESSAGES: warning: Construction of 'tripoint' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: tripoint p15( tripoint( 2 * p15a.xy(), p15a.z ) + p15b );

tripoint p16a;
tripoint p16b;
tripoint p16( 2 * p16a.xy() + p16b.xy(), p16a.z + p16b.z );
// CHECK-MESSAGES: warning: Construction of 'tripoint' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: tripoint p16( tripoint( 2 * p16a.xy(), p16a.z ) + p16b );

int f17( point );
point p17a;
point p17b;
int i17 = f17( { p17a.x + p17b.x, p17a.y + p17b.y } );
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: int i17 = f17( p17a + p17b );

struct A18 {
    int x;
    int y;
};

point p18a;
A18 p18b;
point p18( p18a.x + p18b.x, p18a.y + p18b.y );
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: point p18( p18a + point( p18b.x, p18b.y ) );

struct A19 {
    int f19( const tripoint & );
};
tripoint p19a;
tripoint p19b;
int i19 = A19().f19( tripoint( p19a.x + p19b.x, p19a.y + p19b.y, p19a.z + p19b.z ) );
// CHECK-MESSAGES: warning: Construction of 'tripoint' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: int i19 = A19().f19( p19a + p19b );

point *p20a;
point *p20b;
point p20( p20a->x + p20b->x, p20a->y + p20b->y );
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: point p20( *p20a + *p20b );

tripoint *p21a;
tripoint *p21b;
tripoint p21( p21a->xy() + p21b->xy(), p21a->z + p21b->z );
// CHECK-MESSAGES: warning: Construction of 'tripoint' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: tripoint p21( *p21a + *p21b );

struct A22 {
    tripoint *operator->();
};

A22 p22a;
A22 p22b;
tripoint p22( p22a->xy() + p22b->xy(), p22a->z );
// CHECK-MESSAGES: warning: Construction of 'tripoint' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: tripoint p22( *p22a + p22b->xy() );

tripoint p23a;
tripoint p23( p23a.x + 1, p23a.y, p23a.z );
// CHECK-MESSAGES: warning: Construction of 'tripoint' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: tripoint p23( p23a + point( 1, 0 ) );

#define D24 ( 2 * 3 )
point p24a;
point p24( p24a.x, D24 - 1 + p24a.y );
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: point p24( p24a + point( 0, D24 - 1 ) );

#define D25 ( 2 * 3 )
point p25a;
point p25( p25a.x, D25 - 1 - p25a.y );
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: point p25( point( p25a.x, -p25a.y ) + point( 0, D25 - 1 ) );

point p26a;
point p26b;
point p26( -p26a.x + p26b.x, -p26a.y + p26b.y );
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: point p26( -p26a + p26b );

tripoint p27a;
tripoint p27b;
tripoint p27( -p27a.xy() + p27b.xy(), -p27a.z + p27b.z );
// CHECK-MESSAGES: warning: Construction of 'tripoint' can be simplified using overloaded arithmetic operators. [cata-use-point-arithmetic]
// CHECK-FIXES: point p27( -p27a + p27b );

#define D28 12
point p28a;
point p28b;
point p28( D28 * p28a.x + p28b.x, D28 * p28a.y + p28b.y );
// CHECK-MESSAGES: warning: Construction of 'point' where a coefficient computation involves multiplication by a macro constant.  Should you be using one of the coordinate transformation functions? [cata-use-point-arithmetic]
