// Copyright (c) 2015-2023 Mathias Westerdahl
// For LICENSE (MIT), USAGE or HISTORY, see bottom of file

#ifndef JC_VORONOI_H
#define JC_VORONOI_H

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <float.h>

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef JCV_REAL_TYPE
#define JCV_REAL_TYPE float
#endif

#ifndef JCV_REAL_TYPE_EPSILON
#define JCV_REAL_TYPE_EPSILON FLT_EPSILON
#endif

#ifndef JCV_ATAN2
#define JCV_ATAN2(_Y_, _X_) atan2f(_Y_, _X_)
#endif

#ifndef JCV_SQRT
#define JCV_SQRT(_X_)       sqrtf(_X_)
#endif

#ifndef JCV_PI
#define JCV_PI 3.14159265358979323846264338327950288f
#endif

#ifndef JCV_FLT_MAX
#define JCV_FLT_MAX 3.402823466e+38F
#endif

#ifndef JCV_EDGE_INTERSECT_THRESHOLD
// Fix for Issue #40
#define JCV_EDGE_INTERSECT_THRESHOLD 1.0e-10F
#endif

typedef JCV_REAL_TYPE jcv_real;

typedef struct jcv_point_           jcv_point;
typedef struct jcv_rect_            jcv_rect;
typedef struct jcv_site_            jcv_site;
typedef struct jcv_edge_            jcv_edge;
typedef struct jcv_graphedge_       jcv_graphedge;
typedef struct jcv_delauney_edge_   jcv_delauney_edge;
typedef struct jcv_delauney_iter_   jcv_delauney_iter;
typedef struct jcv_diagram_         jcv_diagram;
typedef struct jcv_clipper_         jcv_clipper;
typedef struct jcv_context_internal_ jcv_context_internal;

/// Tests if a point is inside the final shape
typedef int ( *jcv_clip_test_point_fn )( const jcv_clipper *clipper, const jcv_point p );
/** Given an edge, and the clipper, calculates the e->pos[0] and e->pos[1]
 * Returns 0 if not successful
 */
typedef int ( *jcv_clip_edge_fn )( const jcv_clipper *clipper, jcv_edge *e );
/** Given the clipper, the site and the last edge,
 * closes any gaps in the polygon by adding new edges that follow the bounding shape
 * The internal context is use when allocating new edges.
 */
typedef void ( *jcv_clip_fillgap_fn )( const jcv_clipper *clipper, jcv_context_internal *allocator,
                                       jcv_site *s );



/**
 * Uses malloc
 * If a clipper is not supplied, a default box clipper will be used
 * If rect is null, an automatic bounding box is calculated, with an extra padding of 10 units
 * All points will be culled against the bounding rect, and all edges will be clipped against it.
 */
extern void jcv_diagram_generate( int num_points, const jcv_point *points, const jcv_rect *rect,
                                  const jcv_clipper *clipper, jcv_diagram *diagram );

typedef void *( *FJCVAllocFn )( void *userctx, size_t size );
typedef void ( *FJCVFreeFn )( void *userctx, void *p );

// Same as above, but allows the client to use a custom allocator
extern void jcv_diagram_generate_useralloc( int num_points, const jcv_point *points,
        const jcv_rect *rect, const jcv_clipper *clipper, void *userallocctx, FJCVAllocFn allocfn,
        FJCVFreeFn freefn, jcv_diagram *diagram );

// Uses free (or the registered custom free function)
extern void jcv_diagram_free( jcv_diagram *diagram );

// Returns an array of sites, where each index is the same as the original input point array.
extern const jcv_site *jcv_diagram_get_sites( const jcv_diagram *diagram );

// Returns a linked list of all the voronoi edges
// excluding the ones that lie on the borders of the bounding box.
// For a full list of edges, you need to iterate over the sites, and their graph edges.
extern const jcv_edge *jcv_diagram_get_edges( const jcv_diagram *diagram );

// Iterates over a list of edges, skipping invalid edges (where p0==p1)
extern const jcv_edge *jcv_diagram_get_next_edge( const jcv_edge *edge );

// Creates an iterator over the delauney edges of a voronoi diagram
void jcv_delauney_begin( const jcv_diagram *diagram, jcv_delauney_iter *iter );

// Steps the iterator and returns the next edge
// Returns 0 when there are no more edges
int jcv_delauney_next( jcv_delauney_iter *iter, jcv_delauney_edge *next );

// For the default clipper
extern int jcv_boxshape_test( const jcv_clipper *clipper, const jcv_point p );
extern int jcv_boxshape_clip( const jcv_clipper *clipper, jcv_edge *e );
extern void jcv_boxshape_fillgaps( const jcv_clipper *clipper, jcv_context_internal *allocator,
                                   jcv_site *s );


#pragma pack(push, 1)

struct jcv_point_ {
    jcv_real x;
    jcv_real y;
};

struct jcv_graphedge_ {
    struct jcv_graphedge_  *next;
    struct jcv_edge_       *edge;
    struct jcv_site_       *neighbor;
    jcv_point               pos[2];
    jcv_real                angle;
};

struct jcv_site_ {
    jcv_point       p;
    int             index;  // Index into the original list of points
    jcv_graphedge  *edges;  // The half edges owned by the cell
};

// The coefficients a, b and c are from the general line equation: ax * by + c = 0
struct jcv_edge_ {
    struct jcv_edge_   *next;
    jcv_site           *sites[2];
    jcv_point           pos[2];
    jcv_real            a;
    jcv_real            b;
    jcv_real            c;
};

struct jcv_delauney_iter_ {
    const jcv_edge   *sentinel;
    const jcv_edge   *current;
};

struct jcv_delauney_edge_ {
    const jcv_edge *edge;       // The voronoi edge separating the two sites
    const jcv_site *sites[2];
    jcv_point       pos[2];     // the positions of the two sites
};

struct jcv_rect_ {
    jcv_point   min;
    jcv_point   max;
};

struct jcv_clipper_ {
    jcv_clip_test_point_fn  test_fn;
    jcv_clip_edge_fn        clip_fn;
    jcv_clip_fillgap_fn     fill_fn;
    jcv_point               min;        // The bounding rect min
    jcv_point               max;        // The bounding rect max
    void                   *ctx;        // User defined context
};

struct jcv_diagram_ {
    jcv_context_internal   *internal;
    int                     numsites;
    jcv_point               min;
    jcv_point               max;
};

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // JC_VORONOI_H

#ifdef JC_VORONOI_IMPLEMENTATION
#undef JC_VORONOI_IMPLEMENTATION

#include <memory.h>

// INTERNAL FUNCTIONS

#if defined(_MSC_VER) && !defined(__cplusplus)
#define inline __inline
#endif

static const int JCV_DIRECTION_LEFT  = 0;
static const int JCV_DIRECTION_RIGHT = 1;
static const jcv_real JCV_INVALID_VALUE = ( jcv_real ) - JCV_FLT_MAX;

// jcv_real

static inline jcv_real jcv_abs( jcv_real v )
{
    return ( v < 0 ) ? -v : v;
}

static inline int jcv_real_eq( jcv_real a, jcv_real b )
{
    return jcv_abs( a - b ) < JCV_REAL_TYPE_EPSILON;
}

static inline jcv_real jcv_real_to_int( jcv_real v )
{
    return ( sizeof( jcv_real ) == 4 ) ? ( jcv_real )( int )v : ( jcv_real )( long long )v;
}

// Only used for calculating the initial bounding box
static inline jcv_real jcv_floor( jcv_real v )
{
    jcv_real i = jcv_real_to_int( v );
    return ( v < i ) ? i - 1 : i;
}

// Only used for calculating the initial bounding box
static inline jcv_real jcv_ceil( jcv_real v )
{
    jcv_real i = jcv_real_to_int( v );
    return ( v > i ) ? i + 1 : i;
}

static inline jcv_real jcv_min( jcv_real a, jcv_real b )
{
    return a < b ? a : b;
}

static inline jcv_real jcv_max( jcv_real a, jcv_real b )
{
    return a > b ? a : b;
}

// jcv_point

static inline int jcv_point_cmp( const void *p1, const void *p2 )
{
    const jcv_point *s1 = ( const jcv_point * ) p1;
    const jcv_point *s2 = ( const jcv_point * ) p2;
    return ( s1->y != s2->y ) ? ( s1->y < s2->y ? -1 : 1 ) : ( s1->x < s2->x ? -1 : 1 );
}

static inline int jcv_point_less( const jcv_point *pt1, const jcv_point *pt2 )
{
    return ( pt1->y == pt2->y ) ? ( pt1->x < pt2->x ) : pt1->y < pt2->y;
}

static inline int jcv_point_eq( const jcv_point *pt1, const jcv_point *pt2 )
{
    return jcv_real_eq( pt1->y, pt2->y ) && jcv_real_eq( pt1->x, pt2->x );
}

static inline int jcv_point_on_box_edge( const jcv_point *pt, const jcv_point *min,
        const jcv_point *max )
{
    return pt->x == min->x || pt->y == min->y || pt->x == max->x || pt->y == max->y;
}

// corners

static const int JCV_EDGE_LEFT    = 1;
static const int JCV_EDGE_RIGHT   = 2;
static const int JCV_EDGE_BOTTOM  = 4;
static const int JCV_EDGE_TOP     = 8;

static const int JCV_CORNER_NONE          = 0;
static const int JCV_CORNER_TOP_LEFT      = 1;
static const int JCV_CORNER_BOTTOM_LEFT   = 2;
static const int JCV_CORNER_BOTTOM_RIGHT  = 3;
static const int JCV_CORNER_TOP_RIGHT     = 4;

static inline int jcv_get_edge_flags( const jcv_point *pt, const jcv_point *min,
                                      const jcv_point *max )
{
    int flags = 0;
    if( pt->x == min->x ) {
        flags |= JCV_EDGE_LEFT;
    } else if( pt->x == max->x ) {
        flags |= JCV_EDGE_RIGHT;
    }
    if( pt->y == min->y ) {
        flags |= JCV_EDGE_BOTTOM;
    } else if( pt->y == max->y ) {
        flags |= JCV_EDGE_TOP;
    }
    return flags;
}

static inline int jcv_edge_flags_to_corner( int edge_flags )
{
#define TEST_FLAGS(_FLAGS, _RETVAL) if ( (_FLAGS) == edge_flags ) return _RETVAL
    TEST_FLAGS( JCV_EDGE_TOP | JCV_EDGE_LEFT, JCV_CORNER_TOP_LEFT );
    TEST_FLAGS( JCV_EDGE_TOP | JCV_EDGE_RIGHT, JCV_CORNER_TOP_RIGHT );
    TEST_FLAGS( JCV_EDGE_BOTTOM | JCV_EDGE_LEFT, JCV_CORNER_BOTTOM_LEFT );
    TEST_FLAGS( JCV_EDGE_BOTTOM | JCV_EDGE_RIGHT, JCV_CORNER_BOTTOM_RIGHT );
#undef TEST_FLAGS
    return 0;
}

static inline int jcv_is_corner( int corner )
{
    return corner != 0;
}

static inline int jcv_corner_rotate_90( int corner )
{
    corner--;
    corner = ( corner + 1 ) % 4;
    return corner + 1;
}
static inline jcv_point jcv_corner_to_point( int corner, const jcv_point *min,
        const jcv_point *max )
{
    jcv_point p;
    if( corner == JCV_CORNER_TOP_LEFT )     {
        p.x = min->x;
        p.y = max->y;
    } else if( corner == JCV_CORNER_TOP_RIGHT )    {
        p.x = max->x;
        p.y = max->y;
    } else if( corner == JCV_CORNER_BOTTOM_LEFT )  {
        p.x = min->x;
        p.y = min->y;
    } else if( corner == JCV_CORNER_BOTTOM_RIGHT ) {
        p.x = max->x;
        p.y = min->y;
    } else                                        {
        p.x = JCV_INVALID_VALUE;
        p.y = JCV_INVALID_VALUE;
    }
    return p;
}

static inline jcv_real jcv_point_dist_sq( const jcv_point *pt1, const jcv_point *pt2 )
{
    jcv_real diffx = pt1->x - pt2->x;
    jcv_real diffy = pt1->y - pt2->y;
    return diffx * diffx + diffy * diffy;
}

static inline jcv_real jcv_point_dist( const jcv_point *pt1, const jcv_point *pt2 )
{
    return ( jcv_real )( JCV_SQRT( jcv_point_dist_sq( pt1, pt2 ) ) );
}

// Structs

#pragma pack(push, 1)

typedef struct jcv_halfedge_ {
    jcv_edge               *edge;
    struct jcv_halfedge_   *left;
    struct jcv_halfedge_   *right;
    jcv_point               vertex;
    jcv_real                y;
    int                     direction; // 0=left, 1=right
    int                     pqpos;
} jcv_halfedge;

typedef struct jcv_memoryblock_ {
    size_t sizefree;
    struct jcv_memoryblock_ *next;
    char  *memory;
} jcv_memoryblock;


typedef int ( *FJCVPriorityQueuePrint )( const void *node, int pos );

typedef struct jcv_priorityqueue_ {
    // Implements a binary heap
    int                         maxnumitems;
    int                         numitems;
    void                      **items;
} jcv_priorityqueue;


struct jcv_context_internal_ {
    void               *mem;
    jcv_edge           *edges;
    jcv_halfedge       *beachline_start;
    jcv_halfedge       *beachline_end;
    jcv_halfedge       *last_inserted;
    jcv_priorityqueue  *eventqueue;

    jcv_site           *sites;
    jcv_site           *bottomsite;
    int                 numsites;
    int                 currentsite;
    int                 _padding;

    jcv_memoryblock    *memblocks;
    jcv_edge           *edgepool;
    jcv_halfedge       *halfedgepool;
    void              **eventmem;
    jcv_clipper         clipper;

    void               *memctx; // Given by the user
    FJCVAllocFn         alloc;
    FJCVFreeFn          free;

    jcv_rect            rect;
};

#pragma pack(pop)

void jcv_diagram_free( jcv_diagram *d )
{
    jcv_context_internal *internal = d->internal;
    void *memctx = internal->memctx;
    FJCVFreeFn freefn = internal->free;
    while( internal->memblocks ) {
        jcv_memoryblock *p = internal->memblocks;
        internal->memblocks = internal->memblocks->next;
        freefn( memctx, p );
    }

    freefn( memctx, internal->mem );
}

const jcv_site *jcv_diagram_get_sites( const jcv_diagram *diagram )
{
    return diagram->internal->sites;
}

const jcv_edge *jcv_diagram_get_edges( const jcv_diagram *diagram )
{
    jcv_edge e;
    e.next = diagram->internal->edges;
    return jcv_diagram_get_next_edge( &e );
}

const jcv_edge *jcv_diagram_get_next_edge( const jcv_edge *edge )
{
    const jcv_edge *e = edge->next;
    while( e != 0 && jcv_point_eq( &e->pos[0], &e->pos[1] ) ) {
        e = e->next;
    }
    return e;
}

void jcv_delauney_begin( const jcv_diagram *diagram, jcv_delauney_iter *iter )
{
    iter->current = 0;
    iter->sentinel = jcv_diagram_get_edges( diagram );
}

int jcv_delauney_next( jcv_delauney_iter *iter, jcv_delauney_edge *next )
{
    if( iter->sentinel ) {
        iter->current = iter->sentinel;
        iter->sentinel = 0;
    } else {
        // Note: If we use the raw edges, we still get a proper delauney triangulation
        // However, the result looks less relevant to the cells contained within the bounding box
        // E.g. some cells that look isolated from each other, suddenly still are connected,
        // because they share an edge outside of the bounding box
        iter->current = jcv_diagram_get_next_edge( iter->current );
    }

    while( iter->current && ( iter->current->sites[0] == 0 || iter->current->sites[1] == 0 ) ) {
        iter->current = jcv_diagram_get_next_edge( iter->current );
    }

    if( !iter->current ) {
        return 0;
    }

    next->edge = iter->current;
    next->sites[0] = next->edge->sites[0];
    next->sites[1] = next->edge->sites[1];
    next->pos[0] = next->sites[0]->p;
    next->pos[1] = next->sites[1]->p;
    return 1;
}

static inline void *jcv_align( void *value, size_t alignment )
{
    return ( void * )( ( ( uintptr_t ) value + ( alignment - 1 ) ) & ~( alignment - 1 ) );
}

static void *jcv_alloc( jcv_context_internal *internal, size_t size )
{
    if( !internal->memblocks || internal->memblocks->sizefree < ( size + sizeof( void * ) ) ) {
        size_t blocksize = 16 * 1024;
        jcv_memoryblock *block = ( jcv_memoryblock * )internal->alloc( internal->memctx, blocksize );
        size_t offset = sizeof( jcv_memoryblock );
        block->sizefree = blocksize - offset;
        block->next = internal->memblocks;
        block->memory = ( ( char * )block ) + offset;
        internal->memblocks = block;
    }
    void *p_raw = internal->memblocks->memory;
    void *p_aligned = jcv_align( p_raw, sizeof( void * ) );
    size += ( uintptr_t )p_aligned - ( uintptr_t )p_raw;
    internal->memblocks->memory += size;
    internal->memblocks->sizefree -= size;
    return p_aligned;
}

static jcv_edge *jcv_alloc_edge( jcv_context_internal *internal )
{
    return ( jcv_edge * )jcv_alloc( internal, sizeof( jcv_edge ) );
}

static jcv_halfedge *jcv_alloc_halfedge( jcv_context_internal *internal )
{
    if( internal->halfedgepool ) {
        jcv_halfedge *edge = internal->halfedgepool;
        internal->halfedgepool = internal->halfedgepool->right;
        return edge;
    }

    return ( jcv_halfedge * )jcv_alloc( internal, sizeof( jcv_halfedge ) );
}

static jcv_graphedge *jcv_alloc_graphedge( jcv_context_internal *internal )
{
    return ( jcv_graphedge * )jcv_alloc( internal, sizeof( jcv_graphedge ) );
}

static void *jcv_alloc_fn( void *memctx, size_t size )
{
    ( void )memctx;
    return malloc( size );
}

static void jcv_free_fn( void *memctx, void *p )
{
    ( void )memctx;
    free( p );
}

// jcv_edge

static inline int jcv_is_valid( const jcv_point *p )
{
    return ( p->x != JCV_INVALID_VALUE || p->y != JCV_INVALID_VALUE ) ? 1 : 0;
}

static void jcv_edge_create( jcv_edge *e, jcv_site *s1, jcv_site *s2 )
{
    e->next = 0;
    e->sites[0] = s1;
    e->sites[1] = s2;
    e->pos[0].x = JCV_INVALID_VALUE;
    e->pos[0].y = JCV_INVALID_VALUE;
    e->pos[1].x = JCV_INVALID_VALUE;
    e->pos[1].y = JCV_INVALID_VALUE;

    // Create line equation between S1 and S2:
    // jcv_real a = -1 * (s2->p.y - s1->p.y);
    // jcv_real b = s2->p.x - s1->p.x;
    // //jcv_real c = -1 * (s2->p.x - s1->p.x) * s1->p.y + (s2->p.y - s1->p.y) * s1->p.x;
    //
    // // create perpendicular line
    // jcv_real pa = b;
    // jcv_real pb = -a;
    // //jcv_real pc = pa * s1->p.x + pb * s1->p.y;
    //
    // // Move to the mid point
    // jcv_real mx = s1->p.x + dx * jcv_real(0.5);
    // jcv_real my = s1->p.y + dy * jcv_real(0.5);
    // jcv_real pc = ( pa * mx + pb * my );

    jcv_real dx = s2->p.x - s1->p.x;
    jcv_real dy = s2->p.y - s1->p.y;
    int dx_is_larger = ( dx * dx ) > ( dy * dy ); // instead of fabs

    // Simplify it, using dx and dy
    e->c = dx * ( s1->p.x + dx * ( jcv_real )0.5 ) + dy * ( s1->p.y + dy * ( jcv_real )0.5 );

    if( dx_is_larger ) {
        e->a = ( jcv_real )1;
        e->b = dy / dx;
        e->c /= dx;
    } else {
        e->a = dx / dy;
        e->b = ( jcv_real )1;
        e->c /= dy;
    }
}

// CLIPPING
int jcv_boxshape_test( const jcv_clipper *clipper, const jcv_point p )
{
    return p.x >= clipper->min.x && p.x <= clipper->max.x &&
           p.y >= clipper->min.y && p.y <= clipper->max.y;
}

// The line equation: ax + by + c = 0
// see jcv_edge_create
int jcv_boxshape_clip( const jcv_clipper *clipper, jcv_edge *e )
{
    jcv_real pxmin = clipper->min.x;
    jcv_real pxmax = clipper->max.x;
    jcv_real pymin = clipper->min.y;
    jcv_real pymax = clipper->max.y;

    jcv_real x1, y1, x2, y2;
    jcv_point *s1;
    jcv_point *s2;
    if( e->a == ( jcv_real )1 && e->b >= ( jcv_real )0 ) {
        s1 = jcv_is_valid( &e->pos[1] ) ? &e->pos[1] : 0;
        s2 = jcv_is_valid( &e->pos[0] ) ? &e->pos[0] : 0;
    } else {
        s1 = jcv_is_valid( &e->pos[0] ) ? &e->pos[0] : 0;
        s2 = jcv_is_valid( &e->pos[1] ) ? &e->pos[1] : 0;
    }

    if( e->a == ( jcv_real )1 ) { // delta x is larger
        y1 = pymin;
        if( s1 != 0 && s1->y > pymin ) {
            y1 = s1->y;
        }
        if( y1 > pymax ) {
            y1 = pymax;
        }
        x1 = e->c - e->b * y1;
        y2 = pymax;
        if( s2 != 0 && s2->y < pymax ) {
            y2 = s2->y;
        }

        if( y2 < pymin ) {
            y2 = pymin;
        }
        x2 = ( e->c ) - ( e->b ) * y2;
        // Never occurs according to lcov
        // if( ((x1 > pxmax) & (x2 > pxmax)) | ((x1 < pxmin) & (x2 < pxmin)) )
        // {
        //     return 0;
        // }
        if( x1 > pxmax ) {
            x1 = pxmax;
            y1 = ( e->c - x1 ) / e->b;
        } else if( x1 < pxmin ) {
            x1 = pxmin;
            y1 = ( e->c - x1 ) / e->b;
        }
        if( x2 > pxmax ) {
            x2 = pxmax;
            y2 = ( e->c - x2 ) / e->b;
        } else if( x2 < pxmin ) {
            x2 = pxmin;
            y2 = ( e->c - x2 ) / e->b;
        }
    } else { // delta y is larger
        x1 = pxmin;
        if( s1 != 0 && s1->x > pxmin ) {
            x1 = s1->x;
        }
        if( x1 > pxmax ) {
            x1 = pxmax;
        }
        y1 = e->c - e->a * x1;
        x2 = pxmax;
        if( s2 != 0 && s2->x < pxmax ) {
            x2 = s2->x;
        }
        if( x2 < pxmin ) {
            x2 = pxmin;
        }
        y2 = e->c - e->a * x2;
        // Never occurs according to lcov
        // if( ((y1 > pymax) & (y2 > pymax)) | ((y1 < pymin) & (y2 < pymin)) )
        // {
        //     return 0;
        // }
        if( y1 > pymax ) {
            y1 = pymax;
            x1 = ( e->c - y1 ) / e->a;
        } else if( y1 < pymin ) {
            y1 = pymin;
            x1 = ( e->c - y1 ) / e->a;
        }
        if( y2 > pymax ) {
            y2 = pymax;
            x2 = ( e->c - y2 ) / e->a;
        } else if( y2 < pymin ) {
            y2 = pymin;
            x2 = ( e->c - y2 ) / e->a;
        }
    }

    e->pos[0].x = x1;
    e->pos[0].y = y1;
    e->pos[1].x = x2;
    e->pos[1].y = y2;

    // If the two points are equal, the result is invalid
    return ( x1 == x2 && y1 == y2 ) ? 0 : 1;
}

// The line equation: ax + by + c = 0
// see jcv_edge_create
static int jcv_edge_clipline( jcv_context_internal *internal, jcv_edge *e )
{
    return internal->clipper.clip_fn( &internal->clipper, e );
}

static jcv_edge *jcv_edge_new( jcv_context_internal *internal, jcv_site *s1, jcv_site *s2 )
{
    jcv_edge *e = jcv_alloc_edge( internal );
    jcv_edge_create( e, s1, s2 );
    return e;
}


// jcv_halfedge

static void jcv_halfedge_link( jcv_halfedge *edge, jcv_halfedge *newedge )
{
    newedge->left = edge;
    newedge->right = edge->right;
    edge->right->left = newedge;
    edge->right = newedge;
}

static inline void jcv_halfedge_unlink( jcv_halfedge *he )
{
    he->left->right = he->right;
    he->right->left = he->left;
    he->left  = 0;
    he->right = 0;
}

static inline jcv_halfedge *jcv_halfedge_new( jcv_context_internal *internal, jcv_edge *e,
        int direction )
{
    jcv_halfedge *he = jcv_alloc_halfedge( internal );
    he->edge        = e;
    he->left        = 0;
    he->right       = 0;
    he->direction   = direction;
    he->pqpos       = 0;
    // These are set outside
    //he->y
    //he->vertex
    return he;
}

static void jcv_halfedge_delete( jcv_context_internal *internal, jcv_halfedge *he )
{
    he->right = internal->halfedgepool;
    internal->halfedgepool = he;
}

static inline jcv_site *jcv_halfedge_leftsite( const jcv_halfedge *he )
{
    return he->edge->sites[he->direction];
}

static inline jcv_site *jcv_halfedge_rightsite( const jcv_halfedge *he )
{
    return he->edge ? he->edge->sites[1 - he->direction] : 0;
}

static int jcv_halfedge_rightof( const jcv_halfedge *he, const jcv_point *p )
{
    const jcv_edge *e = he->edge;
    const jcv_site *topsite = e->sites[1];

    int right_of_site = ( p->x > topsite->p.x ) ? 1 : 0;
    if( right_of_site && he->direction == JCV_DIRECTION_LEFT ) {
        return 1;
    }
    if( !right_of_site && he->direction == JCV_DIRECTION_RIGHT ) {
        return 0;
    }

    jcv_real dxp, dyp, dxs, t1, t2, t3, yl;

    int above;
    if( e->a == ( jcv_real )1 ) {
        dyp = p->y - topsite->p.y;
        dxp = p->x - topsite->p.x;
        int fast = 0;
        if( ( !right_of_site & ( e->b < ( jcv_real )0 ) ) | ( right_of_site & ( e->b >=
                ( jcv_real )0 ) ) ) {
            above = dyp >= e->b * dxp;
            fast = above;
        } else {
            above = ( p->x + p->y * e->b ) > e->c;
            if( e->b < ( jcv_real )0 ) {
                above = !above;
            }
            if( !above ) {
                fast = 1;
            }
        }
        if( !fast ) {
            dxs = topsite->p.x - e->sites[0]->p.x;
            above = e->b * ( dxp * dxp - dyp * dyp )
                    < dxs * dyp * ( ( jcv_real )1 + ( jcv_real )2 * dxp / dxs + e->b * e->b );
            if( e->b < ( jcv_real )0 ) {
                above = !above;
            }
        }
    } else { // e->b == 1
        yl = e->c - e->a * p->x;
        t1 = p->y - yl;
        t2 = p->x - topsite->p.x;
        t3 = yl - topsite->p.y;
        above = t1 * t1 > ( t2 * t2 + t3 * t3 );
    }
    return ( he->direction == JCV_DIRECTION_LEFT ? above : !above );
}

// Keeps the priority queue sorted with events sorted in ascending order
// Return 1 if the edges needs to be swapped
static inline int jcv_halfedge_compare( const jcv_halfedge *he1, const jcv_halfedge *he2 )
{
    return ( he1->y == he2->y ) ? he1->vertex.x > he2->vertex.x : he1->y > he2->y;
}

static int jcv_halfedge_intersect( const jcv_halfedge *he1, const jcv_halfedge *he2,
                                   jcv_point *out )
{
    const jcv_edge *e1 = he1->edge;
    const jcv_edge *e2 = he2->edge;

    jcv_real d = e1->a * e2->b - e1->b * e2->a;
    if( ( ( jcv_real ) - JCV_EDGE_INTERSECT_THRESHOLD < d &&
          d < ( jcv_real )JCV_EDGE_INTERSECT_THRESHOLD ) ) {
        return 0;
    }
    out->x = ( e1->c * e2->b - e1->b * e2->c ) / d;
    out->y = ( e1->a * e2->c - e1->c * e2->a ) / d;

    const jcv_edge *e;
    const jcv_halfedge *he;
    if( jcv_point_less( &e1->sites[1]->p, &e2->sites[1]->p ) ) {
        he = he1;
        e = e1;
    } else {
        he = he2;
        e = e2;
    }

    int right_of_site = out->x >= e->sites[1]->p.x;
    if( ( right_of_site && he->direction == JCV_DIRECTION_LEFT ) || ( !right_of_site &&
            he->direction == JCV_DIRECTION_RIGHT ) ) {
        return 0;
    }

    return 1;
}


// Priority queue

static int jcv_pq_moveup( jcv_priorityqueue *pq, int pos )
{
    jcv_halfedge **items = ( jcv_halfedge ** )pq->items;
    jcv_halfedge *node = items[pos];

    for( int parent = ( pos >> 1 );
         pos > 1 && jcv_halfedge_compare( items[parent], node );
         pos = parent, parent = parent >> 1 ) {
        items[pos] = items[parent];
        items[pos]->pqpos = pos;
    }

    node->pqpos = pos;
    items[pos] = node;
    return pos;
}

static int jcv_pq_maxchild( jcv_priorityqueue *pq, int pos )
{
    int child = pos << 1;
    if( child >= pq->numitems ) {
        return 0;
    }
    jcv_halfedge **items = ( jcv_halfedge ** )pq->items;
    if( ( child + 1 ) < pq->numitems && jcv_halfedge_compare( items[child], items[child + 1] ) ) {
        return child + 1;
    }
    return child;
}

static int jcv_pq_movedown( jcv_priorityqueue *pq, int pos )
{
    jcv_halfedge **items = ( jcv_halfedge ** )pq->items;
    jcv_halfedge *node = items[pos];

    int child = jcv_pq_maxchild( pq, pos );
    while( child && jcv_halfedge_compare( node, items[child] ) ) {
        items[pos] = items[child];
        items[pos]->pqpos = pos;
        pos = child;
        child = jcv_pq_maxchild( pq, pos );
    }

    items[pos] = node;
    items[pos]->pqpos = pos;
    return pos;
}

static void jcv_pq_create( jcv_priorityqueue *pq, int capacity, void **buffer )
{
    pq->maxnumitems = capacity;
    pq->numitems    = 1;
    pq->items       = buffer;
}

static int jcv_pq_empty( jcv_priorityqueue *pq )
{
    return pq->numitems == 1 ? 1 : 0;
}

static int jcv_pq_push( jcv_priorityqueue *pq, void *node )
{
    assert( pq->numitems < pq->maxnumitems );
    int n = pq->numitems++;
    pq->items[n] = node;
    return jcv_pq_moveup( pq, n );
}

static void *jcv_pq_pop( jcv_priorityqueue *pq )
{
    void *node = pq->items[1];
    pq->items[1] = pq->items[--pq->numitems];
    jcv_pq_movedown( pq, 1 );
    return node;
}

static void *jcv_pq_top( jcv_priorityqueue *pq )
{
    return pq->items[1];
}

static void jcv_pq_remove( jcv_priorityqueue *pq, jcv_halfedge *node )
{
    if( pq->numitems == 1 ) {
        return;
    }
    int pos = node->pqpos;
    if( pos == 0 ) {
        return;
    }

    jcv_halfedge **items = ( jcv_halfedge ** )pq->items;

    items[pos] = items[--pq->numitems];
    if( jcv_halfedge_compare( node, items[pos] ) ) {
        jcv_pq_moveup( pq, pos );
    } else {
        jcv_pq_movedown( pq, pos );
    }
    node->pqpos = pos;
}

// internal functions

static inline jcv_site *jcv_nextsite( jcv_context_internal *internal )
{
    return ( internal->currentsite < internal->numsites ) ? &internal->sites[internal->currentsite++] :
           0;
}

static jcv_halfedge *jcv_get_edge_above_x( jcv_context_internal *internal, const jcv_point *p )
{
    // Gets the arc on the beach line at the x coordinate (i.e. right above the new site event)

    // A good guess it's close by (Can be optimized)
    jcv_halfedge *he = internal->last_inserted;
    if( !he ) {
        if( p->x < ( internal->rect.max.x - internal->rect.min.x ) / 2 ) {
            he = internal->beachline_start;
        } else {
            he = internal->beachline_end;
        }
    }

    //
    if( he == internal->beachline_start || ( he != internal->beachline_end &&
            jcv_halfedge_rightof( he, p ) ) ) {
        do {
            he = he->right;
        } while( he != internal->beachline_end && jcv_halfedge_rightof( he, p ) );

        he = he->left;
    } else {
        do {
            he = he->left;
        } while( he != internal->beachline_start && !jcv_halfedge_rightof( he, p ) );
    }

    return he;
}

static int jcv_check_circle_event( const jcv_halfedge *he1, const jcv_halfedge *he2,
                                   jcv_point *vertex )
{
    jcv_edge *e1 = he1->edge;
    jcv_edge *e2 = he2->edge;
    if( e1 == 0 || e2 == 0 || e1->sites[1] == e2->sites[1] ) {
        return 0;
    }

    return jcv_halfedge_intersect( he1, he2, vertex );
}

static void jcv_site_event( jcv_context_internal *internal, jcv_site *site )
{
    jcv_halfedge *left   = jcv_get_edge_above_x( internal, &site->p );
    jcv_halfedge *right  = left->right;
    jcv_site     *bottom = jcv_halfedge_rightsite( left );
    if( !bottom ) {
        bottom = internal->bottomsite;
    }

    jcv_edge *edge = jcv_edge_new( internal, bottom, site );
    edge->next = internal->edges;
    internal->edges = edge;

    jcv_halfedge *edge1 = jcv_halfedge_new( internal, edge, JCV_DIRECTION_LEFT );
    jcv_halfedge *edge2 = jcv_halfedge_new( internal, edge, JCV_DIRECTION_RIGHT );

    jcv_halfedge_link( left, edge1 );
    jcv_halfedge_link( edge1, edge2 );

    internal->last_inserted = right;

    jcv_point p;
    if( jcv_check_circle_event( left, edge1, &p ) ) {
        jcv_pq_remove( internal->eventqueue, left );
        left->vertex    = p;
        left->y         = p.y + jcv_point_dist( &site->p, &p );
        jcv_pq_push( internal->eventqueue, left );
    }
    if( jcv_check_circle_event( edge2, right, &p ) ) {
        edge2->vertex   = p;
        edge2->y        = p.y + jcv_point_dist( &site->p, &p );
        jcv_pq_push( internal->eventqueue, edge2 );
    }
}

// https://cp-algorithms.com/geometry/oriented-triangle-area.html
static inline jcv_real jcv_determinant( const jcv_point *a, const jcv_point *b, const jcv_point *c )
{
    return ( b->x - a->x ) * ( c->y - a->y ) - ( b->y - a->y ) * ( c->x - a->x );
}

static inline jcv_real jcv_calc_sort_metric( const jcv_site *site, const jcv_graphedge *edge )
{
    // We take the average of the two points, since we can better distinguish between very small edges
    jcv_real half = 1 / ( jcv_real )2;
    jcv_real x = ( edge->pos[0].x + edge->pos[1].x ) * half;
    jcv_real y = ( edge->pos[0].y + edge->pos[1].y ) * half;
    jcv_real diffy = y - site->p.y;
    jcv_real angle = JCV_ATAN2( diffy, x - site->p.x );
    if( diffy < 0 ) {
        angle = angle + 2 * JCV_PI;
    }
    return ( jcv_real )angle;
}

static inline int jcv_graphedge_eq( jcv_graphedge *a, jcv_graphedge *b )
{
    return jcv_real_eq( a->angle, b->angle ) && jcv_point_eq( &a->pos[0], &b->pos[0] ) &&
           jcv_point_eq( &a->pos[1], &b->pos[1] );
}

static void jcv_sortedges_insert( jcv_site *site, jcv_graphedge *edge )
{
    // Special case for the head end
    jcv_graphedge *prev = 0;
    if( site->edges == 0 || site->edges->angle >= edge->angle ) {
        edge->next = site->edges;
        site->edges = edge;
    } else {
        // Locate the node before the point of insertion
        jcv_graphedge *current = site->edges;
        while( current->next != 0 && current->next->angle < edge->angle ) {
            current = current->next;
        }
        prev = current;
        edge->next = current->next;
        current->next = edge;
    }

    // check to avoid duplicates
    if( prev && jcv_graphedge_eq( prev, edge ) ) {
        prev->next = edge->next;
    } else if( edge->next && jcv_graphedge_eq( edge, edge->next ) ) {
        edge->next = edge->next->next;
    }
}

static void jcv_finishline( jcv_context_internal *internal, jcv_edge *e )
{
    if( !jcv_edge_clipline( internal, e ) ) {
        return;
    }

    // Make sure the graph edges are CCW
    int flip = jcv_determinant( &e->sites[0]->p, &e->pos[0], &e->pos[1] ) > ( jcv_real )0 ? 0 : 1;

    for( int i = 0; i < 2; ++i ) {
        jcv_graphedge *ge = jcv_alloc_graphedge( internal );

        ge->edge = e;
        ge->next = 0;
        ge->neighbor = e->sites[1 - i];
        ge->pos[flip] = e->pos[i];
        ge->pos[1 - flip] = e->pos[1 - i];
        ge->angle = jcv_calc_sort_metric( e->sites[i], ge );

        jcv_sortedges_insert( e->sites[i], ge );
    }
}


static void jcv_endpos( jcv_context_internal *internal, jcv_edge *e, const jcv_point *p,
                        int direction )
{
    e->pos[direction] = *p;

    if( !jcv_is_valid( &e->pos[1 - direction] ) ) {
        return;
    }

    jcv_finishline( internal, e );
}

static inline void jcv_create_corner_edge( jcv_context_internal *internal, const jcv_site *site,
        jcv_graphedge *current, jcv_graphedge *gap )
{
    gap->neighbor   = 0;
    gap->pos[0]     = current->pos[1];

    if( current->pos[1].x < internal->rect.max.x && current->pos[1].y == internal->rect.min.y ) {
        gap->pos[1].x = internal->rect.max.x;
        gap->pos[1].y = internal->rect.min.y;
    } else if( current->pos[1].x > internal->rect.min.x && current->pos[1].y == internal->rect.max.y ) {
        gap->pos[1].x = internal->rect.min.x;
        gap->pos[1].y = internal->rect.max.y;
    } else if( current->pos[1].y > internal->rect.min.y && current->pos[1].x == internal->rect.min.x ) {
        gap->pos[1].x = internal->rect.min.x;
        gap->pos[1].y = internal->rect.min.y;
    } else if( current->pos[1].y < internal->rect.max.y && current->pos[1].x == internal->rect.max.x ) {
        gap->pos[1].x = internal->rect.max.x;
        gap->pos[1].y = internal->rect.max.y;
    }

    gap->angle = jcv_calc_sort_metric( site, gap );
}

static jcv_edge *jcv_create_gap_edge( jcv_context_internal *internal, jcv_site *site,
                                      jcv_graphedge *ge )
{
    jcv_edge *edge  = jcv_alloc_edge( internal );
    edge->pos[0]    = ge->pos[0];
    edge->pos[1]    = ge->pos[1];
    edge->sites[0]  = site;
    edge->sites[1]  = 0;
    edge->a = edge->b = edge->c = 0;
    edge->next      = internal->edges;
    internal->edges = edge;
    return edge;
}

void jcv_boxshape_fillgaps( const jcv_clipper *clipper, jcv_context_internal *allocator,
                            jcv_site *site )
{
    // They're sorted CCW, so if the current->pos[1] != next->pos[0], then we have a gap
    jcv_graphedge *current = site->edges;
    if( !current ) {
        // No edges, then it should be a single cell
        assert( allocator->numsites == 1 );

        jcv_graphedge *gap = jcv_alloc_graphedge( allocator );
        gap->neighbor   = 0;
        gap->pos[0]     = clipper->min;
        gap->pos[1].x   = clipper->max.x;
        gap->pos[1].y   = clipper->min.y;
        gap->angle      = jcv_calc_sort_metric( site, gap );
        gap->next       = 0;
        gap->edge       = jcv_create_gap_edge( allocator, site, gap );

        current = gap;
        site->edges = gap;
    }

    jcv_graphedge *next = current->next;
    if( !next ) {
        jcv_graphedge *gap = jcv_alloc_graphedge( allocator );
        jcv_create_corner_edge( allocator, site, current, gap );
        gap->edge = jcv_create_gap_edge( allocator, site, gap );

        gap->next = current->next;
        current->next = gap;
        current = gap;
        next = site->edges;
    }

    while( current && next ) {
        int current_edge_flags = jcv_get_edge_flags( &current->pos[1], &clipper->min, &clipper->max );
        if( current_edge_flags && !jcv_point_eq( &current->pos[1], &next->pos[0] ) ) {
            // Cases:
            //  Current and Next on the same border
            //  Current on one border, and Next on another border
            //  Current on the corner, Next on the border
            //  Current on the corner, Next on another border (another corner in between)

            int next_edge_flags = jcv_get_edge_flags( &next->pos[0], &clipper->min, &clipper->max );
            if( current_edge_flags & next_edge_flags ) {
                // Current and Next on the same border
                jcv_graphedge *gap = jcv_alloc_graphedge( allocator );
                gap->neighbor   = 0;
                gap->pos[0]     = current->pos[1];
                gap->pos[1]     = next->pos[0];
                gap->angle      = jcv_calc_sort_metric( site, gap );
                gap->edge       = jcv_create_gap_edge( allocator, site, gap );

                gap->next = current->next;
                current->next = gap;
            } else {
                // Current and Next on different borders
                int corner_flag = jcv_edge_flags_to_corner( current_edge_flags );
                if( corner_flag ) {
                    // we are already at one corner, so we need to find the next one
                    corner_flag = jcv_corner_rotate_90( corner_flag );
                } else {
                    // we are on the middle of a border
                    // we need to find the adjacent corner, following the borders CCW
                    if( current_edge_flags == JCV_EDGE_TOP )    {
                        corner_flag = JCV_CORNER_TOP_LEFT;
                    } else if( current_edge_flags == JCV_EDGE_LEFT )   {
                        corner_flag = JCV_CORNER_BOTTOM_LEFT;
                    } else if( current_edge_flags == JCV_EDGE_BOTTOM ) {
                        corner_flag = JCV_CORNER_BOTTOM_RIGHT;
                    } else if( current_edge_flags == JCV_EDGE_RIGHT )  {
                        corner_flag = JCV_CORNER_TOP_RIGHT;
                    }

                }
                jcv_point corner = jcv_corner_to_point( corner_flag, &clipper->min, &clipper->max );

                jcv_graphedge *gap = jcv_alloc_graphedge( allocator );
                gap->neighbor   = 0;
                gap->pos[0]     = current->pos[1];
                gap->pos[1]     = corner;
                gap->angle      = jcv_calc_sort_metric( site, gap );
                gap->edge       = jcv_create_gap_edge( allocator, site, gap );

                gap->next = current->next;
                current->next = gap;
            }
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


// Since the algorithm leaves gaps at the borders/corner, we want to fill them
static void jcv_fillgaps( jcv_diagram *diagram )
{
    jcv_context_internal *internal = diagram->internal;
    if( !internal->clipper.fill_fn ) {
        return;
    }

    for( int i = 0; i < internal->numsites; ++i ) {
        jcv_site *site = &internal->sites[i];
        internal->clipper.fill_fn( &internal->clipper, internal, site );
    }
}


static void jcv_circle_event( jcv_context_internal *internal )
{
    jcv_halfedge *left      = ( jcv_halfedge * )jcv_pq_pop( internal->eventqueue );

    jcv_halfedge *leftleft  = left->left;
    jcv_halfedge *right     = left->right;
    jcv_halfedge *rightright = right->right;
    jcv_site *bottom = jcv_halfedge_leftsite( left );
    jcv_site *top    = jcv_halfedge_rightsite( right );

    jcv_point vertex = left->vertex;
    jcv_endpos( internal, left->edge, &vertex, left->direction );
    jcv_endpos( internal, right->edge, &vertex, right->direction );

    internal->last_inserted = rightright;

    jcv_pq_remove( internal->eventqueue, right );
    jcv_halfedge_unlink( left );
    jcv_halfedge_unlink( right );
    jcv_halfedge_delete( internal, left );
    jcv_halfedge_delete( internal, right );

    int direction = JCV_DIRECTION_LEFT;
    if( bottom->p.y > top->p.y ) {
        jcv_site *temp = bottom;
        bottom = top;
        top = temp;
        direction = JCV_DIRECTION_RIGHT;
    }

    jcv_edge *edge = jcv_edge_new( internal, bottom, top );
    edge->next = internal->edges;
    internal->edges = edge;

    jcv_halfedge *he = jcv_halfedge_new( internal, edge, direction );
    jcv_halfedge_link( leftleft, he );
    jcv_endpos( internal, edge, &vertex, JCV_DIRECTION_RIGHT - direction );

    jcv_point p;
    if( jcv_check_circle_event( leftleft, he, &p ) ) {
        jcv_pq_remove( internal->eventqueue, leftleft );
        leftleft->vertex    = p;
        leftleft->y         = p.y + jcv_point_dist( &bottom->p, &p );
        jcv_pq_push( internal->eventqueue, leftleft );
    }
    if( jcv_check_circle_event( he, rightright, &p ) ) {
        he->vertex      = p;
        he->y           = p.y + jcv_point_dist( &bottom->p, &p );
        jcv_pq_push( internal->eventqueue, he );
    }
}

void jcv_diagram_generate( int num_points, const jcv_point *points, const jcv_rect *rect,
                           const jcv_clipper *clipper, jcv_diagram *d )
{
    jcv_diagram_generate_useralloc( num_points, points, rect, clipper, 0, jcv_alloc_fn, jcv_free_fn,
                                    d );
}

typedef union jcv_cast_align_struct_ {
    char   *charp;
    void  **voidpp;
} jcv_cast_align_struct;

static inline void jcv_rect_union( jcv_rect *rect, const jcv_point *p )
{
    rect->min.x = jcv_min( rect->min.x, p->x );
    rect->min.y = jcv_min( rect->min.y, p->y );
    rect->max.x = jcv_max( rect->max.x, p->x );
    rect->max.y = jcv_max( rect->max.y, p->y );
}

static inline void jcv_rect_round( jcv_rect *rect )
{
    rect->min.x = jcv_floor( rect->min.x );
    rect->min.y = jcv_floor( rect->min.y );
    rect->max.x = jcv_ceil( rect->max.x );
    rect->max.y = jcv_ceil( rect->max.y );
}

static inline void jcv_rect_inflate( jcv_rect *rect, jcv_real amount )
{
    rect->min.x -= amount;
    rect->min.y -= amount;
    rect->max.x += amount;
    rect->max.y += amount;
}

static int jcv_prune_duplicates( jcv_context_internal *internal, jcv_rect *rect )
{
    int num_sites = internal->numsites;
    jcv_site *sites = internal->sites;

    jcv_rect r;
    r.min.x = r.min.y = JCV_FLT_MAX;
    r.max.x = r.max.y = -JCV_FLT_MAX;

    int offset = 0;
    // Prune duplicates first
    for( int i = 0; i < num_sites; i++ ) {
        const jcv_site *s = &sites[i];
        // Remove duplicates, to avoid anomalies
        if( i > 0 && jcv_point_eq( &s->p, &sites[i - 1].p ) ) {
            offset++;
            continue;
        }

        sites[i - offset] = sites[i];

        jcv_rect_union( &r, &s->p );
    }
    internal->numsites -= offset;
    if( rect ) {
        *rect = r;
    }
    return offset;
}

static int jcv_prune_not_in_shape( jcv_context_internal *internal, jcv_rect *rect )
{
    int num_sites = internal->numsites;
    jcv_site *sites = internal->sites;

    jcv_rect r;
    r.min.x = r.min.y = JCV_FLT_MAX;
    r.max.x = r.max.y = -JCV_FLT_MAX;

    int offset = 0;
    for( int i = 0; i < num_sites; i++ ) {
        const jcv_site *s = &sites[i];

        if( !internal->clipper.test_fn( &internal->clipper, s->p ) ) {
            offset++;
            continue;
        }

        sites[i - offset] = sites[i];

        jcv_rect_union( &r, &s->p );
    }
    internal->numsites -= offset;
    if( rect ) {
        *rect = r;
    }
    return offset;
}

static jcv_context_internal *jcv_alloc_internal( int num_points, void *userallocctx,
        FJCVAllocFn allocfn, FJCVFreeFn freefn )
{
    // Interesting limits from Euler's equation
    // Slide 81: https://courses.cs.washington.edu/courses/csep521/01au/lectures/lecture10slides.pdf
    // Page 3: https://sites.cs.ucsb.edu/~suri/cs235/Voronoi.pdf
    size_t eventssize = ( size_t )( num_points * 2 ) * sizeof( void
                        * ); // beachline can have max 2*n-5 parabolas
    size_t sitessize = ( size_t )num_points * sizeof( jcv_site );
    size_t memsize = sizeof( jcv_priorityqueue ) + eventssize + sitessize + sizeof(
                         jcv_context_internal ) + 16u; // 16 bytes padding for alignment

    char *originalmem = ( char * )allocfn( userallocctx, memsize );
    memset( originalmem, 0, memsize );

    // align memory
    char *mem = ( char * )jcv_align( originalmem, sizeof( void * ) );

    jcv_context_internal *internal = ( jcv_context_internal * )mem;
    mem += sizeof( jcv_context_internal );
    internal->mem    = originalmem;
    internal->memctx = userallocctx;
    internal->alloc  = allocfn;
    internal->free   = freefn;

    mem = ( char * )jcv_align( mem, sizeof( void * ) );
    internal->sites = ( jcv_site * ) mem;
    mem += sitessize;

    mem = ( char * )jcv_align( mem, sizeof( void * ) );
    internal->eventqueue = ( jcv_priorityqueue * )mem;
    mem += sizeof( jcv_priorityqueue );
    assert( ( ( uintptr_t )mem & ( sizeof( void * ) -1 ) ) == 0 );

    jcv_cast_align_struct tmp;
    tmp.charp = mem;
    internal->eventmem = tmp.voidpp;

    assert( ( mem + eventssize ) <= ( originalmem + memsize ) );

    return internal;
}

void jcv_diagram_generate_useralloc( int num_points, const jcv_point *points, const jcv_rect *rect,
                                     const jcv_clipper *clipper, void *userallocctx, FJCVAllocFn allocfn, FJCVFreeFn freefn,
                                     jcv_diagram *d )
{
    if( d->internal ) {
        jcv_diagram_free( d );
    }

    jcv_context_internal *internal = jcv_alloc_internal( num_points, userallocctx, allocfn, freefn );

    internal->beachline_start = jcv_halfedge_new( internal, 0, 0 );
    internal->beachline_end = jcv_halfedge_new( internal, 0, 0 );

    internal->beachline_start->left     = 0;
    internal->beachline_start->right    = internal->beachline_end;
    internal->beachline_end->left       = internal->beachline_start;
    internal->beachline_end->right      = 0;

    internal->last_inserted = 0;

    int max_num_events = num_points * 2; // beachline can have max 2*n-5 parabolas
    jcv_pq_create( internal->eventqueue, max_num_events, ( void ** )internal->eventmem );

    internal->numsites = num_points;
    jcv_site *sites = internal->sites;

    for( int i = 0; i < num_points; ++i ) {
        sites[i].p        = points[i];
        sites[i].edges    = 0;
        sites[i].index    = i;
    }

    qsort( sites, ( size_t )num_points, sizeof( jcv_site ), jcv_point_cmp );

    jcv_clipper box_clipper;
    if( clipper == 0 ) {
        box_clipper.test_fn = jcv_boxshape_test;
        box_clipper.clip_fn = jcv_boxshape_clip;
        box_clipper.fill_fn = jcv_boxshape_fillgaps;
        clipper = &box_clipper;
    }
    internal->clipper = *clipper;

    jcv_rect tmp_rect;
    tmp_rect.min.x = tmp_rect.min.y = JCV_FLT_MAX;
    tmp_rect.max.x = tmp_rect.max.y = -JCV_FLT_MAX;
    jcv_prune_duplicates( internal, &tmp_rect );

    // Prune using the test second
    if( internal->clipper.test_fn ) {
        // e.g. used by the box clipper in the test_fn
        internal->clipper.min = rect ? rect->min : tmp_rect.min;
        internal->clipper.max = rect ? rect->max : tmp_rect.max;

        jcv_prune_not_in_shape( internal, &tmp_rect );

        // The pruning might have made the bounding box smaller
        if( !rect ) {
            // In the case of all sites being all on a horizontal or vertical line, the
            // rect area will be zero, and the diagram generation will most likely fail
            jcv_rect_round( &tmp_rect );
            jcv_rect_inflate( &tmp_rect, 10 );

            internal->clipper.min = tmp_rect.min;
            internal->clipper.max = tmp_rect.max;
        }
    }

    internal->rect = rect ? *rect : tmp_rect;

    d->min      = internal->rect.min;
    d->max      = internal->rect.max;
    d->numsites = internal->numsites;
    d->internal = internal;

    internal->bottomsite = jcv_nextsite( internal );

    jcv_priorityqueue *pq = internal->eventqueue;
    jcv_site *site = jcv_nextsite( internal );

    int finished = 0;
    while( !finished ) {
        jcv_point lowest_pq_point;
        if( !jcv_pq_empty( pq ) ) {
            jcv_halfedge *he = ( jcv_halfedge * )jcv_pq_top( pq );
            lowest_pq_point.x = he->vertex.x;
            lowest_pq_point.y = he->y;
        }

        if( site != 0 && ( jcv_pq_empty( pq ) || jcv_point_less( &site->p, &lowest_pq_point ) ) ) {
            jcv_site_event( internal, site );
            site = jcv_nextsite( internal );
        } else if( !jcv_pq_empty( pq ) ) {
            jcv_circle_event( internal );
        } else {
            finished = 1;
        }
    }

    for( jcv_halfedge *he = internal->beachline_start->right; he != internal->beachline_end;
         he = he->right ) {
        jcv_finishline( internal, he->edge );
    }

    jcv_fillgaps( d );
}

#endif // JC_VORONOI_IMPLEMENTATION

/*

ABOUT:

    A fast single file 2D voronoi diagram generator

HISTORY:
    0.9     2023-01-22  - Modified the Delauney iterator creation api
    0.8     2022-12-20  - Added fix for missing border edges
                          More robust removal of duplicate graph edges
                          Added iterator for Delauney edges
    0.7     2019-10-25  - Added support for clipping against convex polygons
                        - Added JCV_EDGE_INTERSECT_THRESHOLD for edge intersections
                        - Fixed issue where the bounds calculation wasn’t considering all points
    0.6     2018-10-21  - Removed JCV_CEIL/JCV_FLOOR/JCV_FABS
                        - Optimizations: Fewer indirections, better beach head approximation
    0.5     2018-10-14  - Fixed issue where the graph edge had the wrong edge assigned (issue #28)
                        - Fixed issue where a point was falsely passing the jcv_is_valid() test (issue #22)
                        - Fixed jcv_diagram_get_edges() so it now returns _all_ edges (issue #28)
                        - Added jcv_diagram_get_next_edge() to skip zero length edges (issue #10)
                        - Added defines JCV_CEIL/JCV_FLOOR/JCV_FLT_MAX for easier configuration
    0.4     2017-06-03  - Increased the max number of events that are preallocated
    0.3     2017-04-16  - Added clipping box as input argument (Automatically calculated if needed)
                        - Input points are pruned based on bounding box
    0.2     2016-12-30  - Fixed issue of edges not being closed properly
                        - Fixed issue when having many events
                        - Fixed edge sorting
                        - Code cleanup
    0.1                 Initial version

LICENSE:

    The MIT License (MIT)

    Copyright (c) 2015-2019 Mathias Westerdahl

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

    The input points are pruned if

        * There are duplicates points
        * The input points are outside of the bounding box (i.e. fail the clipping test function)
        * The input points are rejected by the clipper's test function

    The input bounding box is optional (calculated automatically)

    The input domain is (-FLT_MAX, FLT_MAX] (for floats)

    The api consists of these functions:

    void jcv_diagram_generate( int num_points, const jcv_point* points, const jcv_rect* rect, const jcv_clipper* clipper, jcv_diagram* diagram );
    void jcv_diagram_generate_useralloc( int num_points, const jcv_point* points, const jcv_rect* rect, const jcv_clipper* clipper, const jcv_clipper* clipper, void* userallocctx, FJCVAllocFn allocfn, FJCVFreeFn freefn, jcv_diagram* diagram );
    void jcv_diagram_free( jcv_diagram* diagram );

    const jcv_site* jcv_diagram_get_sites( const jcv_diagram* diagram );
    const jcv_edge* jcv_diagram_get_edges( const jcv_diagram* diagram );
    const jcv_edge* jcv_diagram_get_next_edge( const jcv_edge* edge );

    An example usage:

    #define JC_VORONOI_IMPLEMENTATION
    // If you wish to use doubles
    //#define JCV_REAL_TYPE double
    //#define JCV_ATAN2 atan2
    //#define JCV_FLT_MAX 1.7976931348623157E+308
    #include "jc_voronoi.h"

    void draw_edges(const jcv_diagram* diagram);
    void draw_cells(const jcv_diagram* diagram);

    void generate_and_draw(int numpoints, const jcv_point* points)
    {
        jcv_diagram diagram;
        memset(&diagram, 0, sizeof(jcv_diagram));
        jcv_diagram_generate(count, points, 0, 0, &diagram);

        draw_edges(diagram);
        draw_cells(diagram);

        jcv_diagram_free( &diagram );
    }

    void draw_edges(const jcv_diagram* diagram)
    {
        // If all you need are the edges
        const jcv_edge* edge = jcv_diagram_get_edges( diagram );
        while( edge )
        {
            draw_line(edge->pos[0], edge->pos[1]);
            edge = jcv_diagram_get_next_edge(edge);
        }
    }

    void draw_cells(const jcv_diagram* diagram)
    {
        // If you want to draw triangles, or relax the diagram,
        // you can iterate over the sites and get all edges easily
        const jcv_site* sites = jcv_diagram_get_sites( diagram );
        for( int i = 0; i < diagram->numsites; ++i )
        {
            const jcv_site* site = &sites[i];

            const jcv_graphedge* e = site->edges;
            while( e )
            {
                draw_triangle( site->p, e->pos[0], e->pos[1]);
                e = e->next;
            }
        }
    }

    // Here is a simple example of how to do the relaxations of the cells
    void relax_points(const jcv_diagram* diagram, jcv_point* points)
    {
        const jcv_site* sites = jcv_diagram_get_sites(diagram);
        for( int i = 0; i < diagram->numsites; ++i )
        {
            const jcv_site* site = &sites[i];
            jcv_point sum = site->p;
            int count = 1;

            const jcv_graphedge* edge = site->edges;

            while( edge )
            {
                sum.x += edge->pos[0].x;
                sum.y += edge->pos[0].y;
                ++count;
                edge = edge->next;
            }

            points[site->index].x = sum.x / count;
            points[site->index].y = sum.y / count;
        }
    }

 */
