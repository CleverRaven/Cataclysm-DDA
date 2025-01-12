#pragma once
#ifndef CATA_SRC_COORDS_FWD_H
#define CATA_SRC_COORDS_FWD_H

#include <type_traits>

struct point;
struct tripoint;

namespace coords
{

enum class origin : std::uint8_t {
    relative, // this is a special origin that can be added to any other
    abs, // the global absolute origin for the entire game
    submap, // from corner of submap
    overmap_terrain, // from corner of overmap_terrain
    overmap, // from corner of overmap
    reality_bubble, // from corner of a reality bubble (aka 'map' or 'tinymap')
};

enum class scale : std::uint8_t {
    map_square,
    submap,
    overmap_terrain,
    segment,
    overmap,
    vehicle
};

constexpr scale ms = scale::map_square;
constexpr scale sm = scale::submap;
constexpr scale omt = scale::overmap_terrain;
constexpr scale seg = scale::segment;
constexpr scale om = scale::overmap;

template<typename Point, origin Origin, scale Scale>
class coord_point_ob;

template<typename Point, origin Origin, scale Scale>
class coord_point_ib;

template<typename Point, origin Origin, scale Scale, bool InBounds = false>
using coord_point =
    std::conditional_t<InBounds, coord_point_ib<Point, Origin, Scale>, coord_point_ob<Point, Origin, Scale>>;

} // namespace coords

/** Typedefs for point types with coordinate mnemonics.
 *
 * Each name is of the form (tri)point_<origin>_<scale>(_ib) where <origin> tells you
 * the context in which the point has meaning, and <scale> tells you what one unit
 * of the point means. The optional "_ib" suffix denotes that the type is guaranteed
 * to be inbounds.
 *
 * For example:
 * point_omt_ms is the position of a map square within an overmap terrain.
 * tripoint_rel_sm is a relative tripoint submap offset.
 *
 * For more details see doc/POINTS_COORDINATES.md.
 */
/*@{*/
using point_rel_ms = coords::coord_point<point, coords::origin::relative, coords::ms>;
using point_abs_ms = coords::coord_point<point, coords::origin::abs, coords::ms>;
using point_sm_ms = coords::coord_point<point, coords::origin::submap, coords::ms>;
using point_sm_ms_ib = coords::coord_point<point, coords::origin::submap, coords::ms, true>;
using point_omt_ms = coords::coord_point<point, coords::origin::overmap_terrain, coords::ms>;
using point_bub_ms = coords::coord_point<point, coords::origin::reality_bubble, coords::ms>;
using point_bub_ms_ib =
    coords::coord_point<point, coords::origin::reality_bubble, coords::ms, true>;
using point_rel_sm = coords::coord_point<point, coords::origin::relative, coords::sm>;
using point_abs_sm = coords::coord_point<point, coords::origin::abs, coords::sm>;
using point_omt_sm = coords::coord_point<point, coords::origin::overmap_terrain, coords::sm>;
using point_om_sm = coords::coord_point<point, coords::origin::overmap, coords::sm>;
using point_bub_sm = coords::coord_point<point, coords::origin::reality_bubble, coords::sm>;
using point_bub_sm_ib =
    coords::coord_point<point, coords::origin::reality_bubble, coords::sm, true>;
using point_rel_omt = coords::coord_point<point, coords::origin::relative, coords::omt>;
using point_abs_omt = coords::coord_point<point, coords::origin::abs, coords::omt>;
using point_om_omt = coords::coord_point<point, coords::origin::overmap, coords::omt>;
using point_abs_seg = coords::coord_point<point, coords::origin::abs, coords::seg>;
using point_rel_om = coords::coord_point<point, coords::origin::relative, coords::om>;
using point_abs_om = coords::coord_point<point, coords::origin::abs, coords::om>;

using tripoint_rel_ms = coords::coord_point<tripoint, coords::origin::relative, coords::ms>;
using tripoint_abs_ms = coords::coord_point<tripoint, coords::origin::abs, coords::ms>;
using tripoint_sm_ms = coords::coord_point<tripoint, coords::origin::submap, coords::ms>;
using tripoint_sm_ms_ib = coords::coord_point<tripoint, coords::origin::submap, coords::ms, true>;
using tripoint_omt_ms = coords::coord_point<tripoint, coords::origin::overmap_terrain, coords::ms>;
using tripoint_bub_ms = coords::coord_point<tripoint, coords::origin::reality_bubble, coords::ms>;
using tripoint_bub_ms_ib =
    coords::coord_point<tripoint, coords::origin::reality_bubble, coords::ms, true>;
using tripoint_rel_sm = coords::coord_point<tripoint, coords::origin::relative, coords::sm>;
using tripoint_abs_sm = coords::coord_point<tripoint, coords::origin::abs, coords::sm>;
using tripoint_om_sm = coords::coord_point<tripoint, coords::origin::overmap, coords::sm>;
using tripoint_bub_sm = coords::coord_point<tripoint, coords::origin::reality_bubble, coords::sm>;
using tripoint_bub_sm_ib =
    coords::coord_point<tripoint, coords::origin::reality_bubble, coords::sm, true>;
using tripoint_rel_omt = coords::coord_point<tripoint, coords::origin::relative, coords::omt>;
using tripoint_abs_omt = coords::coord_point<tripoint, coords::origin::abs, coords::omt>;
using tripoint_om_omt = coords::coord_point<tripoint, coords::origin::overmap, coords::omt>;
using tripoint_abs_seg = coords::coord_point<tripoint, coords::origin::abs, coords::seg>;
using tripoint_abs_om = coords::coord_point<tripoint, coords::origin::abs, coords::om>;
/*@}*/

#endif // CATA_SRC_COORDS_FWD_H
