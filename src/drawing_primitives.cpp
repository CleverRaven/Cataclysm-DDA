#include "drawing_primitives.h"

#include <functional>
#include <utility>
#include <vector>

#include "coordinates.h"
#include "line.h"
#include "point.h"
#include "rng.h"

void draw_line( const std::function<void( const point_bub_ms & )> &set, const point_bub_ms &p1,
                const point_bub_ms &p2 )
{
    std::vector<point_bub_ms> line = line_to( p1, p2, 0 );
    for( point_bub_ms &i : line ) {
        set( i );
    }
    set( p1 );
}

void draw_square( const std::function<void( const point_bub_ms & )> &set, point_bub_ms p1,
                  point_bub_ms p2 )
{
    if( p1.x() > p2.x() ) {
        std::swap( p1.x(), p2.x() );
    }
    if( p1.y() > p2.y() ) {
        std::swap( p1.y(), p2.y() );
    }
    for( int x = p1.x(); x <= p2.x(); x++ ) {
        for( int y = p1.y(); y <= p2.y(); y++ ) {
            set( {x, y } );
        }
    }
}

void draw_rough_circle( const std::function<void( const point_bub_ms & )> &set,
                        const point_bub_ms &p, int rad )
{
    for( int i = p.x() - rad; i <= p.x() + rad; i++ ) {
        for( int j = p.y() - rad; j <= p.y() + rad; j++ ) {
            if( trig_dist( p, point_bub_ms( i, j ) ) + rng( 0, 3 ) <= rad ) {
                set( { i, j } );
            }
        }
    }
}

void draw_circle( const std::function<void( const point_bub_ms & )> &set, const rl_vec2d &p,
                  double rad )
{
    for( int i = p.x - rad - 1; i <= p.x + rad + 1; i++ ) {
        for( int j = p.y - rad - 1; j <= p.y + rad + 1; j++ ) {
            if( ( p.x - i ) * ( p.x - i ) + ( p.y - j ) * ( p.y - j ) <= rad * rad ) {
                set( { i, j } );
            }
        }
    }
}

void draw_circle( const std::function<void( const point_bub_ms & )> &set, const point_bub_ms &p,
                  int rad )
{
    for( int i = p.x() - rad; i <= p.x() + rad; i++ ) {
        for( int j = p.y() - rad; j <= p.y() + rad; j++ ) {
            if( trig_dist( p, point_bub_ms( i, j ) ) <= rad ) {
                set( { i, j } );
            }
        }
    }
}
