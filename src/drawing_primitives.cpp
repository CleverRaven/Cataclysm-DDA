#include "drawing_primitives.h"

#include <functional>
#include <vector>

#include "enums.h"
#include "line.h"
#include "rng.h"

void draw_line( std::function<void( const int, const int )>set, int x1, int y1, int x2, int y2 )
{
    std::vector<point> line = line_to( x1, y1, x2, y2, 0 );
    for( auto &i : line ) {
        set( i.x, i.y );
    }
    set( x1, y1 );
}

void draw_square( std::function<void( const int, const int )>set, int x1, int y1, int x2, int y2 )
{
    if( x1 > x2 ) {
        std::swap( x1, x2 );
    }
    if( y1 > y2 ) {
        std::swap( y1, y2 );
    }
    for( int x = x1; x <= x2; x++ ) {
        for( int y = y1; y <= y2; y++ ) {
            set( x, y );
        }
    }
}

void draw_rough_circle( std::function<void( const int, const int )>set, int x, int y, int rad )
{
    for( int i = x - rad; i <= x + rad; i++ ) {
        for( int j = y - rad; j <= y + rad; j++ ) {
            if( trig_dist( x, y, i, j ) + rng( 0, 3 ) <= rad ) {
                set( i, j );
            }
        }
    }
}

void draw_circle( std::function<void( const int, const int )>set, double x, double y, double rad )
{
    for( int i = x - rad - 1; i <= x + rad + 1; i++ ) {
        for( int j = y - rad - 1; j <= y + rad + 1; j++ ) {
            if( ( x - i ) * ( x - i ) + ( y - j ) * ( y - j ) <= rad * rad ) {
                set( i, j );
            }
        }
    }
}

void draw_circle( std::function<void( const int, const int )>set, int x, int y, int rad )
{
    for( int i = x - rad; i <= x + rad; i++ ) {
        for( int j = y - rad; j <= y + rad; j++ ) {
            if( trig_dist( x, y, i, j ) <= rad ) {
                set( i, j );
            }
        }
    }
}
