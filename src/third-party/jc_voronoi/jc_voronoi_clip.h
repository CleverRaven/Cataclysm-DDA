// Copyright (c) 2019 Mathias Westerdahl
// For full LICENSE (MIT) or USAGE, see bottom of file

#ifndef JC_VORONOI_CLIP_H
#define JC_VORONOI_CLIP_H

#include "jc_voronoi.h"

#pragma pack(push, 1)

typedef struct jcv_clipping_polygon_ {
    jcv_point *points;
    int num_points;
} jcv_clipping_polygon;

#pragma pack(pop)


// Convex polygon clip functions
int jcv_clip_polygon_test_point( const jcv_clipper *clipper, const jcv_point p );
int jcv_clip_polygon_clip_edge( const jcv_clipper *clipper, jcv_edge *e );
void jcv_clip_polygon_fill_gaps( const jcv_clipper *clipper, jcv_context_internal *allocator,
                                 jcv_site *site );


#endif // JC_VORONOI_CLIP_H

#ifdef JC_VORONOI_CLIP_IMPLEMENTATION
#undef JC_VORONOI_CLIP_IMPLEMENTATION

// These helpers will probably end up in the main library

static inline jcv_real jcv_cross( const jcv_point a, const jcv_point b )
{
    return a.x * b.y - a.y * b.x;
}

static inline jcv_point jcv_add( jcv_point a, jcv_point b )
{
    jcv_point r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    return r;
}

static inline jcv_point jcv_sub( jcv_point a, jcv_point b )
{
    jcv_point r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    return r;
}

static inline jcv_point jcv_mul( jcv_point v, jcv_real s )
{
    jcv_point r;
    r.x = v.x * s;
    r.y = v.y * s;
    return r;
}

static inline jcv_point jcv_mix( jcv_point a, jcv_point b, jcv_real t )
{
    jcv_point r;
    r.x = a.x + ( b.x - a.x ) * t;
    r.y = a.y + ( b.y - a.y ) * t;
    return r;
}

static inline jcv_real jcv_dot( jcv_point a, jcv_point b )
{
    return a.x * b.x + a.y * b.y;
}


static inline jcv_real jcv_length( jcv_point v )
{
    return JCV_SQRT( v.x * v.x + v.y * v.y );
}

static inline jcv_real jcv_length_sq( jcv_point v )
{
    return v.x * v.x + v.y * v.y;
}

static inline jcv_real jcv_fabs( jcv_real a )
{
    return a < 0 ? -a : a;
}

// if it returns [0.0, 1.0] it's on the line segment
static inline jcv_real jcv_point_to_line_segment_t( jcv_point p, jcv_point p0, jcv_point p1 )
{
    jcv_point vpoint = jcv_sub( p, p0 );
    jcv_point vsegment = jcv_sub( p1, p0 );
    return jcv_dot( vsegment, vpoint ) / jcv_dot( vsegment, vsegment );
}

int jcv_clip_polygon_test_point( const jcv_clipper *clipper, const jcv_point p )
{
    jcv_clipping_polygon *polygon = ( jcv_clipping_polygon * )clipper->ctx;
    int num_points = polygon->num_points;

    // convex polygon
    // winding CCW
    // all polygon normals point outward
    // if the point is in front of the plane, it is outside

    int result = 1;
    for( int i = 0; i < num_points; ++i ) {
        jcv_point p0 = polygon->points[i];
        jcv_point p1 = polygon->points[( i + 1 ) % num_points];
        jcv_point n;
        n.x = p1.y - p0.y;
        n.y = p0.x - p1.x;
        jcv_point diff;
        diff.x = p.x - p0.x;
        diff.y = p.y - p0.y;

        if( jcv_dot( n, diff ) > 0 ) {
            result = 0;
            break;
        }
    }
    return result;
}

static int jcv_ray_intersect_polygon( const jcv_clipper *clipper, jcv_point p0, jcv_point p1,
                                      jcv_real *out_t0, jcv_real *out_t1 )
{
    jcv_clipping_polygon *polygon = ( jcv_clipping_polygon * )clipper->ctx;
    int num_points = polygon->num_points;

    jcv_real t0 = ( jcv_real )0;
    jcv_real t1 = ( jcv_real )1;
    jcv_point dir = jcv_sub( p1, p0 );

    for( int i = 0; i < num_points; ++i ) {
        jcv_point v0 = polygon->points[i];
        jcv_point v1 = polygon->points[( i + 1 ) % num_points];
        jcv_point n;
        n.x = v1.y - v0.y;
        n.y = -( v1.x - v0.x );

        jcv_point v0p0 = jcv_sub( p0, v0 );

        jcv_real N = -jcv_dot( v0p0, n );
        jcv_real D = jcv_dot( dir, n );
        if( jcv_fabs( D ) < 0.0001f ) { // parallel to the line
            if( N < 0 ) {
                return 0;
            }
            continue;
        }

        jcv_real t = N / D;
        if( D < 0 ) { // -> entering
            t0 = t > t0 ? t : t0;
            if( t0 > t1 ) {
                return 0;
            }
        } else { // D > 0 -> exiting
            t1 = t < t1 ? t : t1;
            if( t1 < t0 ) {
                return 0;
            }
        }
    }

    *out_t0 = t0;
    *out_t1 = t1;
    return 1;
}

int jcv_clip_polygon_clip_edge( const jcv_clipper *clipper, jcv_edge *e )
{
    // Using the box clipper to get a finite line segment
    int result = jcv_boxshape_clip( clipper, e );
    if( !result ) {
        return 0;
    }

    jcv_point p0 = e->pos[0];
    jcv_point p1 = e->pos[1];

    jcv_real t0;
    jcv_real t1;
    result = jcv_ray_intersect_polygon( clipper, p0, p1, &t0, &t1 );

    if( !result ) {
        e->pos[0] = e->pos[1];
        return 0;
    }

    e->pos[0] = jcv_mix( p0, p1, t0 );
    e->pos[1] = jcv_mix( p0, p1, t1 );
    return 1;
}

// Find the edge which the point sits on
static int jcv_find_polygon_edge( const jcv_clipper *clipper, jcv_point p )
{
    jcv_clipping_polygon *polygon = ( jcv_clipping_polygon * )clipper->ctx;

    int min_edge = -1;
    jcv_real min_dist = JCV_FLT_MAX;
    int num_points = polygon->num_points;
    for( int i = 0; i < num_points; ++i ) {
        jcv_point p0 = polygon->points[i];
        if( jcv_point_eq( &p, &p0 ) ) {
            return i;
        }

        jcv_point p1 = polygon->points[( i + 1 ) % num_points];
        jcv_point vsegment = jcv_sub( p1, p0 );
        jcv_point vpoint = jcv_sub( p, p0 );

        jcv_real t = jcv_dot( vsegment, vpoint ) / jcv_dot( vsegment, vsegment );

        if( t < ( jcv_real )0.0f || t > ( jcv_real )1.0f ) {
            continue;
        }

        jcv_point projected = jcv_add( p0, jcv_mul( vsegment, t ) );
        jcv_real distsq = jcv_length_sq( jcv_sub( p, projected ) );

        if( distsq < min_dist ) {
            min_dist = distsq;
            min_edge = i;
        }
    }
    assert( min_edge >= 0 );
    return min_edge;
}

void jcv_clip_polygon_fill_gaps( const jcv_clipper *clipper, jcv_context_internal *allocator,
                                 jcv_site *site )
{
    // They're sorted CCW, so if the current->pos[1] != next->pos[0], then we have a gap
    jcv_clipping_polygon *polygon = ( jcv_clipping_polygon * )clipper->ctx;
    int num_points = polygon->num_points;

    jcv_graphedge *current = site->edges;
    if( !current ) {
        jcv_graphedge *gap = jcv_alloc_graphedge( allocator );
        gap->neighbor = 0;
        // Pick the first edge of the polygon (which is also CCW)
        gap->pos[0] = polygon->points[0];
        gap->pos[1] = polygon->points[1];
        gap->angle  = jcv_calc_sort_metric( site, gap );
        gap->next   = 0;
        gap->edge   = jcv_create_gap_edge( allocator, site, gap );

        current = gap;
        site->edges = gap;
    }

    jcv_graphedge *next = current->next;
    if( !next ) {
        jcv_graphedge *gap = jcv_alloc_graphedge( allocator );

        int polygon_edge = jcv_find_polygon_edge( clipper, current->pos[1] );
        if( !jcv_point_eq( &current->pos[1], &polygon->points[( polygon_edge + 1 ) % num_points] ) ) {
            gap->pos[0] = current->pos[1];
            gap->pos[1] = polygon->points[( polygon_edge + 1 ) % num_points];
        } else {
            gap->pos[0] = polygon->points[( polygon_edge + 1 ) % num_points];
            gap->pos[1] = polygon->points[( polygon_edge + 2 ) % num_points];
        }

        gap->neighbor   = 0;
        gap->angle      = jcv_calc_sort_metric( site, gap );
        gap->next       = 0;
        gap->edge       = jcv_create_gap_edge( allocator, site, gap );

        gap->next = current->next;
        current->next = gap;
        current = gap;
        next = site->edges;
    }

    while( current && next ) {
        if( !jcv_point_eq( &current->pos[1], &next->pos[0] ) ) {
            int polygon_edge1 = jcv_find_polygon_edge( clipper, current->pos[1] );
            int polygon_edge2 = jcv_find_polygon_edge( clipper, next->pos[0] );

            jcv_graphedge *gap = jcv_alloc_graphedge( allocator );
            gap->pos[0] = current->pos[1];

            if( polygon_edge1 != polygon_edge2 ) {
                gap->pos[1] = polygon->points[( polygon_edge1 + 1 ) % num_points];
            } else {
                gap->pos[1] = next->pos[0];
            }

            gap->neighbor   = 0;
            gap->angle      = jcv_calc_sort_metric( site, gap );
            gap->edge       = jcv_create_gap_edge( allocator, site, gap );
            gap->next       = current->next;
            current->next   = gap;
        }

        current = current->next;
        if( current ) {
            next = current->next;
            if( !next ) {
                next = site->edges;
            }
        }
    }
}

#endif // JC_VORONOI_CLIP_IMPLEMENTATION


/*

ABOUT:

    Helper functions for clipping a vosonoi diagram against a convex polygon


LICENSE:

    The MIT License (MIT)

    Copyright (c) 2019 Mathias Westerdahl

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.


DISCLAIMER:

    This software is supplied "AS IS" without any warranties and support

USAGE:

USAGE:

    The function `jcv_clipper` struct allows for supplying a set of custom clipper functions to interact with the generating of the resulting diagram.

    #define JC_VORONOI_CLIP_IMPLEMENTATION
    #include "jc_voronoi_clip.h"

    jcv_clipping_polygon polygon;
    // Triangle
    polygon.num_points = 3;
    polygon.points = (jcv_point*)malloc(sizeof(jcv_point)*(size_t)polygon.num_points);

    polygon.points[0].x = width/2;
    polygon.points[1].x = width - width/5;
    polygon.points[2].x = width/5;
    polygon.points[0].y = height/5;
    polygon.points[1].y = height - height/5;
    polygon.points[2].y = height - height/5;

    jcv_clipper polygonclipper;
    polygonclipper.test_fn = jcv_clip_polygon_test_point;
    polygonclipper.clip_fn = jcv_clip_polygon_clip_edge;
    polygonclipper.fill_fn = jcv_clip_polygon_fill_gaps;
    polygonclipper.ctx = &polygon;

    jcv_diagram diagram;
    memset(&diagram, 0, sizeof(jcv_diagram));
    jcv_diagram_generate(count, (const jcv_point*)points, 0, clipper, &diagram);
*/
