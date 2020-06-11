// RUN: %check_clang_tidy %s cata-combine-locals-into-point %t -- -plugins=%cata_plugin -- -isystem %cata_include

#define CATA_NO_STL
#include "point.h"

void f0( const point & );

void g0()
{
    int rx = 7;
    // CHECK-MESSAGES: warning: Variables 'rx' and 'ry' could be combined into a single 'point' variable. [cata-combine-locals-into-point]
    // CHECK-FIXES: point r( 7, 14 );
    int ry = 14;
    f0( point( rx, ry ) );
    // CHECK-MESSAGES: note: Update 'rx' to 'r.x'.
    // CHECK-MESSAGES: note: Update 'ry' to 'r.y'.
    // CHECK-FIXES: f0( point( r.x, r.y ) );
}

void g1()
{
    const int xfall = 7;
    // CHECK-MESSAGES: warning: Variables 'xfall' and 'yfall' could be combined into a single 'point' variable. [cata-combine-locals-into-point]
    // CHECK-FIXES: const point fall( 7, 14 );
    const int yfall = 14;
    f0( point( xfall, yfall ) );
    // CHECK-MESSAGES: note: Update 'xfall' to 'fall.x'.
    // CHECK-MESSAGES: note: Update 'yfall' to 'fall.y'.
    // CHECK-FIXES: f0( point( fall.x, fall.y ) );
}

void g2()
{
    int x = 7;
    // CHECK-MESSAGES: warning: Variables 'x' and 'y' could be combined into a single 'point' variable. [cata-combine-locals-into-point]
    // CHECK-FIXES: point p( 7, 14 );
    int y = 14;
    f0( point( x, y ) );
    // CHECK-MESSAGES: note: Update 'x' to 'p.x'.
    // CHECK-MESSAGES: note: Update 'y' to 'p.y'.
    // CHECK-FIXES: f0( point( p.x, p.y ) );
}

void g3()
{
    // Don't mess with variables when the second is in a for loop
    int x = 7;
    for( int y = 0; y < 10; ++y ) {
        f0( point( x, y ) );
    }
}
