#pragma once
#ifndef CATA_SRC_COORDINATE_CONVERSIONS_H
#define CATA_SRC_COORDINATE_CONVERSIONS_H

#include "point.h"

/**
 * Coordinate systems used here are:
 * overmap (om): the position of an overmap. Each overmap stores
 * this as overmap::loc (access with overmap::pos()).
 * There is a unique overmap for each overmap coordinate.
 *
 * segment (seg): A segment is a unit of terrain saved to a directory.
 * Each segment contains SEG_SIZExSEG_SIZE overmap terrains, and is used only for
 * saving/loading submaps, see mapbuffer.cpp.
 * Translation from omt to seg:
 * om.x /= SEG_SIZE
 * om.y /= SEG_SIZE
 * (with special handling for negative values).
 *
 * overmap terrain (omt): the position of a overmap terrain (oter_id).
 * Each overmap contains (OMAPX * OMAPY) overmap terrains.
 * Translation from omt to om:
 * om.x /= OMAPX
 * om.y /= OMAPY
 * (with special handling for negative values).
 *
 * Z-components are never considered and simply copied.
 *
 * submap (sm): each overmap terrain contains (2*2) submaps.
 * Translating from sm to omt coordinates:
 * sm.x /= 2
 * sm.y /= 2
 *
 * map square (ms): used by @ref map, each map square may contain a single
 * piece of furniture, it has a terrain (ter_t).
 * There are SEEX*SEEY map squares in each submap.
 *
 * The class provides static translation functions, named like this:
@code
    static point <from>_to_<to>_copy(int x, int y);
    static point <from>_to_<to>_copy(const point& p);
    static tripoint <from>_to_<to>_copy(const tripoint& p);
    static void <from>_to_<to>(int &x, int &y);
    static void <from>_to_<to>(point& p);
    static void <from>_to_<to>(tripoint& p);
    static point <from>_to_<to>_remain(int &x, int &y);
    static point <from>_to_<to>_remain(point& p);
@endcode
 * Functions ending with _copy return the translated coordinates,
 * other functions change the parameters itself and don't return anything.
 * Functions ending with _remain return the translated coordinates and
 * store the remainder in the parameters.
 */

// overmap terrain to overmap
point omt_to_om_copy( const point &p );
tripoint omt_to_om_copy( const tripoint &p );
void omt_to_om( int &x, int &y );
inline void omt_to_om( point &p )
{
    omt_to_om( p.x, p.y );
}
inline void omt_to_om( tripoint &p )
{
    omt_to_om( p.x, p.y );
}
point omt_to_om_remain( int &x, int &y );
inline point omt_to_om_remain( point &p )
{
    return omt_to_om_remain( p.x, p.y );
}
// overmap to overmap terrain
point om_to_omt_copy( const point &p );
// submap to overmap terrain
point sm_to_omt_copy( const point &p );
tripoint sm_to_omt_copy( const tripoint &p );
void sm_to_omt( int &x, int &y );
inline void sm_to_omt( point &p )
{
    sm_to_omt( p.x, p.y );
}
inline void sm_to_omt( tripoint &p )
{
    sm_to_omt( p.x, p.y );
}
point sm_to_omt_remain( int &x, int &y );
inline point sm_to_omt_remain( point &p )
{
    return sm_to_omt_remain( p.x, p.y );
}
// submap to overmap, basically: x / (OMAPX * 2)
point sm_to_om_copy( const point &p );
tripoint sm_to_om_copy( const tripoint &p );
void sm_to_om( int &x, int &y );
inline void sm_to_om( point &p )
{
    sm_to_om( p.x, p.y );
}
inline void sm_to_om( tripoint &p )
{
    sm_to_om( p.x, p.y );
}
point sm_to_om_remain( int &x, int &y );
inline point sm_to_om_remain( point &p )
{
    return sm_to_om_remain( p.x, p.y );
}
// overmap terrain to submap, basically: x *= 2
inline int omt_to_sm_copy( int a )
{
    return 2 * a;
}
point omt_to_sm_copy( const point &p );
tripoint omt_to_sm_copy( const tripoint &p );
void omt_to_sm( int &x, int &y );
inline void omt_to_sm( point &p )
{
    omt_to_sm( p.x, p.y );
}
inline void omt_to_sm( tripoint &p )
{
    omt_to_sm( p.x, p.y );
}
// overmap terrain to map square
point omt_to_ms_copy( const point &p );
// overmap to submap, basically: x *= 2 * OMAPX
point om_to_sm_copy( const point &p );
tripoint om_to_sm_copy( const tripoint &p );
void om_to_sm( int &x, int &y );
inline void om_to_sm( point &p )
{
    om_to_sm( p.x, p.y );
}
inline void om_to_sm( tripoint &p )
{
    om_to_sm( p.x, p.y );
}
// map squares to submap, basically: x /= SEEX
point ms_to_sm_copy( const point &p );
tripoint ms_to_sm_copy( const tripoint &p );
void ms_to_sm( int &x, int &y );
inline void ms_to_sm( point &p )
{
    ms_to_sm( p.x, p.y );
}
inline void ms_to_sm( tripoint &p )
{
    ms_to_sm( p.x, p.y );
}
point ms_to_sm_remain( int &x, int &y );
inline point ms_to_sm_remain( point &p )
{
    return ms_to_sm_remain( p.x, p.y );
}
inline tripoint ms_to_sm_remain( tripoint &p )
{
    return tripoint( ms_to_sm_remain( p.x, p.y ), p.z );
}
// submap back to map squares, basically: x *= SEEX
// Note: this gives you the map square coordinates of the top-left corner
// of the given submap.
point sm_to_ms_copy( const point &p );
tripoint sm_to_ms_copy( const tripoint &p );
void sm_to_ms( int &x, int &y );
inline void sm_to_ms( point &p )
{
    sm_to_ms( p.x, p.y );
}
inline void sm_to_ms( tripoint &p )
{
    sm_to_ms( p.x, p.y );
}
// map squares to overmap terrain, basically: x /= SEEX * 2
point ms_to_omt_copy( const point &p );
tripoint ms_to_omt_copy( const tripoint &p );
void ms_to_omt( int &x, int &y );
inline void ms_to_omt( point &p )
{
    ms_to_omt( p.x, p.y );
}
inline void ms_to_omt( tripoint &p )
{
    ms_to_omt( p.x, p.y );
}
point ms_to_omt_remain( int &x, int &y );
inline point ms_to_omt_remain( point &p )
{
    return ms_to_omt_remain( p.x, p.y );
}
// overmap terrain to map segment.
tripoint omt_to_seg_copy( const tripoint &p );

#endif // CATA_SRC_COORDINATE_CONVERSIONS_H
