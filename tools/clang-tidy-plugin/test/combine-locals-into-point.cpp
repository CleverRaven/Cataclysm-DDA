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
    const int x_fall = 7;
    // CHECK-MESSAGES: warning: Variables 'x_fall' and 'y_fall' could be combined into a single 'point' variable. [cata-combine-locals-into-point]
    // CHECK-FIXES: const point fall( 7, 14 );
    const int y_fall = 14;
    f0( point( x_fall, y_fall ) );
    // CHECK-MESSAGES: note: Update 'x_fall' to 'fall.x'.
    // CHECK-MESSAGES: note: Update 'y_fall' to 'fall.y'.
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

void g4()
{
    const int fall_x = 7;
    // CHECK-MESSAGES: warning: Variables 'fall_x' and 'fall_y' could be combined into a single 'point' variable. [cata-combine-locals-into-point]
    // CHECK-FIXES: const point fall( 7, 14 );
    const int fall_y = 14;
    f0( point( fall_x, fall_y ) );
    // CHECK-MESSAGES: note: Update 'fall_x' to 'fall.x'.
    // CHECK-MESSAGES: note: Update 'fall_y' to 'fall.y'.
    // CHECK-FIXES: f0( point( fall.x, fall.y ) );
}

void g5()
{
    int x_ = 7;
    // CHECK-MESSAGES: warning: Variables 'x_' and 'y_' could be combined into a single 'point' variable. [cata-combine-locals-into-point]
    // CHECK-FIXES: point p( 7, 14 );
    int y_ = 14;
    /*INDENT-OFF*/
    auto f = [&]() { x_ = 1; };
    /*INDENT-ON*/
    // CHECK-MESSAGES: note: Update 'x_' to 'p.x'.
    // CHECK-FIXES: auto f = [&]() { p.x = 1; };
}

void g5( const tripoint &p )
{
    int x = p.x;
    // CHECK-MESSAGES: warning: Variables 'x' and 'y' could be combined into a single 'point' variable. [cata-combine-locals-into-point]
    // CHECK-FIXES: point p2( p.x, p.y );
    int y = p.y;
    x = 1;
    // CHECK-MESSAGES: note: Update 'x' to 'p2.x'.
    // CHECK-FIXES: p2.x = 1;
}

void g6()
{
    int new_x = 0;
    // CHECK-MESSAGES: warning: Variables 'new_x' and 'new_y' could be combined into a single 'point' variable. [cata-combine-locals-into-point]
    // CHECK-FIXES: point new_( 0, 1 );
    int new_y = 1;
    new_x = 1;
    // CHECK-MESSAGES: note: Update 'new_x' to 'new_.x'.
    // CHECK-FIXES: new_.x = 1;
}

void g7()
{
    int x = 0;
    // CHECK-MESSAGES: warning: Variables 'x', 'y', and 'z' could be combined into a single 'tripoint' variable. [cata-combine-locals-into-point]
    // CHECK-FIXES: tripoint p( 0, 1, 2 );
    int y = 1;
    int z = 2;
    x = 1;
    // CHECK-MESSAGES: note: Update 'x' to 'p.x'.
    // CHECK-FIXES: p.x = 1;
}

void g8( const tripoint &p1 )
{
    int x1 = p1.x;
    // CHECK-MESSAGES: warning: Variables 'x1' and 'y1' could be combined into a single 'point' variable. [cata-combine-locals-into-point]
    // CHECK-FIXES: point p12( p1.x, p1.y );
    int y1 = p1.y;
    x1 = 1;
    // CHECK-MESSAGES: note: Update 'x1' to 'p12.x'.
    // CHECK-FIXES: p12.x = 1;
}

void g9()
{
    // Don't mess with bools
    bool x = false;
    bool y = true;
    const bool x1 = false;
    const bool y1 = true;
}

void g10()
{
    // When multiple changes to be done in one function, only perform one (to
    // avoid overlapping replacements)
    int x = 0;
    // CHECK-MESSAGES: warning: Variables 'x' and 'y' could be combined into a single 'point' variable. [cata-combine-locals-into-point]
    // CHECK-FIXES: point p( 0, 1 );
    int y = 1;
    int x1 = x;
    int y1 = y;
}

void g11()
{
    static constexpr int x = 0;
    // CHECK-MESSAGES: warning: Variables 'x' and 'y' could be combined into a single 'point' variable. [cata-combine-locals-into-point]
    // CHECK-FIXES: static constexpr point p( 0, 1 );
    static constexpr int y = 1;
}

void g12()
{
    // Don't mess with unsigned things
    unsigned x = false;
    unsigned y = true;
    const unsigned x1 = false;
    const unsigned y1 = true;
}

void g13()
{
    // Check that suppressing the first warning suppresses the rest.
    // NOLINTNEXTLINE(cata-combine-locals-into-point)
    int rx = 7;
    int ry = 14;
    f0( point( rx, ry ) );
}
