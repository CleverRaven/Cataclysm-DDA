// RUN: %check_clang_tidy %s cata-simplify-point-constructors %t -- -plugins=%cata_plugin -- -isystem %cata_include

#define CATA_NO_STL
#include "point.h"

point p0;
point p0a( p0.x, p0.y );
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified. [cata-simplify-point-constructors]
// CHECK-FIXES: point p0a( p0 );

point p1;
point p1a = point( p1.x, p1.y );
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified. [cata-simplify-point-constructors]
// CHECK-FIXES: point p1a = p1;

point p2;
tripoint p2a = tripoint( p2.x, p2.y, 0 );
// CHECK-MESSAGES: warning: Construction of 'tripoint' can be simplified. [cata-simplify-point-constructors]
// CHECK-FIXES: point p2a = tripoint( p2, 0 );

tripoint p3;
tripoint p3a = tripoint( p3.x, p3.y, p3.z );
// CHECK-MESSAGES: warning: Construction of 'tripoint' can be simplified. [cata-simplify-point-constructors]
// CHECK-FIXES: point p3a = p3;

tripoint p4;
point p4a = point( p4.x, p4.y );
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified. [cata-simplify-point-constructors]
// CHECK-FIXES: point p4a = p4.xy();

point p5;
int f5( const point & );
int i5 = f5( { p5.x, p5.y } );
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified. [cata-simplify-point-constructors]
// CHECK-FIXES: int i5 = f5( p5 );

tripoint p6;
int f6( const tripoint & );
int i6 = f6( { p6.x, p6.y, p6.z } );
// CHECK-MESSAGES: warning: Construction of 'tripoint' can be simplified. [cata-simplify-point-constructors]
// CHECK-FIXES: int i6 = f6( p6 );

point p7;
point p7a = { p7.x, p7.y };
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified. [cata-simplify-point-constructors]
// CHECK-FIXES: point p7a = { p7 };

point p8;
point p8a{ p7.x, p7.y };
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified. [cata-simplify-point-constructors]
// CHECK-FIXES: point p8a{ p7 };

point p9;
int f9( const point & );
int i9 = f9( point( p9.x, p9.y ) );
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified. [cata-simplify-point-constructors]
// CHECK-FIXES: int i9 = f9( p9 );

struct A10 {
    point *operator->() const;
};
A10 a10;
int f10( const point & );
int i10 = f10( point( a10->x, a10->y ) );
// CHECK-MESSAGES: warning: Construction of 'point' can be simplified. [cata-simplify-point-constructors]
// CHECK-FIXES: int i10 = f10( *a10 );
