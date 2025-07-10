#include <algorithm>
#include <array>
#include <bitset>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cata_assert.h"
#include "coordinates.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "flexbuffer_json.h"
#include "hash_utils.h"
#include "json.h"
#include "line.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "messages.h"
#include "omdata.h"
#include "options.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "point.h"
#include "regional_settings.h"
#include "rng.h"
#include "simple_pathfinding.h"
#include "type_id.h"

//in a box made by p1, p2, return { corner point in direction, direction from corner to p2 }
static std::pair<tripoint_om_omt, om_direction::type> closest_corner_in_direction(
    const tripoint_om_omt &p1,
    const tripoint_om_omt &p2, const om_direction::type &direction )
{
    const int direction_index = static_cast<int>( direction );
    const point_rel_omt offset_as_rel( four_adjacent_offsets[direction_index] );
    const tripoint_rel_omt diff = p2 - p1;
    const tripoint_rel_omt offset( offset_as_rel, p1.z() );
    const tripoint_om_omt result( p1.x() + offset.x() * abs( diff.x() ),
                                  p1.y() + offset.y() * abs( diff.y() ), p1.z() );
    const tripoint_rel_omt diff2 = p2 - result;
    const point_rel_omt sign( diff2.x() == 0 ? 0 : std::copysign( 1, diff2.x() ),
                              diff2.y() == 0 ? 0 : std::copysign( 1, diff2.y() ) );
    const int cardinal = static_cast<int>( om_direction::size );
    int new_direction;
    for( new_direction = 0; new_direction < cardinal; new_direction++ ) {
        if( point_rel_omt( four_adjacent_offsets[new_direction] ) == sign ) {
            break;
        }
    }
    return std::make_pair( result, om_direction::all[new_direction] );
}

//TODO: generalize
//if point is on edge of overmap, wrap to other end
static tripoint_om_omt wrap_point( const tripoint_om_omt &p )
{
    point wrap( p.x(), p.y() );
    if( wrap.x == OMAPX - 1 || wrap.x == 0 ) {
        wrap.x = abs( wrap.x - ( OMAPX - 1 ) );
    }
    if( wrap.y == OMAPY - 1 || wrap.y == 0 ) {
        wrap.y = abs( wrap.y - ( OMAPY - 1 ) );
    }
    return tripoint_om_omt( wrap.x, wrap.y, p.z() );
}

/**
* checks whether point p is outside square corners of the overmap
* uses two rectangles to make a plus shape
*/
static bool point_outside_overmap_corner( const tripoint_om_omt &p, int corner_length )
{
    const half_open_rectangle<point_om_omt> valid_bounds(
        point_om_omt( corner_length, 0 ),
        point_om_omt( OMAPX - corner_length, OMAPY ) );
    const half_open_rectangle<point_om_omt> valid_bounds_2(
        point_om_omt( 0, corner_length ),
        point_om_omt( OMAPX, OMAPY - corner_length ) );
    return valid_bounds.contains( p.xy() ) || valid_bounds_2.contains( p.xy() );
}

//checks whether the special, if placed and rotated in dir, would replace a single water tile
static bool special_on_water( const overmap *om, const overmap_special_id &special,
                              const tripoint_om_omt &placement, om_direction::type dir )
{
    std::vector<overmap_special_locations> locs = special->required_locations();
    for( overmap_special_locations &loc : locs ) {
        if( is_water_body( om->ter( placement + om_direction::rotate( loc.p, dir ) ) ) ) {
            return true;
        }
    }
    return false;
}

static overmap_special_id segment_special( const overmap_highway_settings &highway_settings,
        bool raised )
{
    return raised ? highway_settings.segment_bridge :
           highway_settings.segment_flat;
}

//highways align to N/E, so their segments must offset due to rotation
static void highway_segment_offset( tripoint_om_omt &segment_point,
                                    om_direction::type segment_dir )
{
    if( segment_point.is_invalid() ) {
        return;
    }
    std::array<tripoint, HIGHWAY_MAX_CONNECTIONS> segment_offsets =
    { tripoint::zero, tripoint::zero, tripoint::east, tripoint::south };

    segment_point += segment_offsets[static_cast<int>( segment_dir )];
}

//highways align to N/E, so their specials must offset due to rotation
static void highway_special_offset( tripoint_om_omt &special_point,
                                    om_direction::type special_dir )
{
    if( special_point.is_invalid() ) {
        return;
    }
    std::array<tripoint, HIGHWAY_MAX_CONNECTIONS> bend_offset =
    { tripoint::zero, tripoint::east, tripoint::south_east, tripoint::south };
    special_point += bend_offset[static_cast<int>( special_dir )];
}

//truncates highway segment by bend length, accounting for offset
static point_rel_omt truncate_segment( om_direction::type dir, int last_bend_length )
{
    tripoint_om_omt bend_offset_base( 0, 0, 0 );
    highway_special_offset( bend_offset_base, dir );
    point_rel_omt current_direction( four_adjacent_offsets[static_cast<int>( dir )] );
    point_rel_omt bend_length_vector = current_direction * last_bend_length;
    return point_rel_omt( bend_offset_base.x() * current_direction.x() - bend_length_vector.x(),
                          bend_offset_base.y() * current_direction.y() - bend_length_vector.y() );
}

tripoint_om_omt overmap::find_highway_intersection_point( const overmap_special_id &special,
        const tripoint_om_omt &center, const om_direction::type &dir, int border ) const
{

    if( !special_on_water( this, special, center, dir ) ) {
        return center;
    }
    tripoint_om_omt not_on_water = tripoint_om_omt::invalid;
    const tripoint_range intersection_radius = points_in_radius( center, border );
    const half_open_rectangle<point_om_omt> bounds( point_om_omt( border, border ),
            point_om_omt( OMAPX - border, OMAPY - border ) );
    for( const tripoint_om_omt &p : intersection_radius ) {
        if( bounds.contains( p.xy() ) ) {
            if( !special_on_water( this, special, p, dir ) ) {
                not_on_water = p;
                break;
            }
        }
    }
    //if failure, return invalid point
    return not_on_water;
}

/**
* draws highway from p1 to p2
* dir1/dir2 are the directions from which p1/p2 are being drawn
* e.g. from west edge of OM to south edge would be 3/2
* connecting p1 to p2 may have no more than 2 bends
*/
Highway_path overmap::place_highway_reserved_path( const tripoint_om_omt &p1,
        const tripoint_om_omt &p2,
        int dir1, int dir2, int base_z )
{

    const overmap_highway_settings &highway_settings = settings->overmap_highway;
    // base-level segment OMT to use until finalize_highways()
    const oter_type_str_id &reserved_terrain_id = highway_settings.reserved_terrain_id;
    // upper-level segment OMT to use until finalize_highways()
    const oter_type_str_id &reserved_terrain_water_id = highway_settings.reserved_terrain_water_id;
    const int HIGHWAY_MAX_DEVIANCE = highway_settings.HIGHWAY_MAX_DEVIANCE;
    const overmap_special_id &fallback_onramp = highway_settings.fallback_onramp;

    Highway_path highway_path;
    om_direction::type direction1 = om_direction::all[dir1];
    om_direction::type direction2 = om_direction::all[dir2];
    cata_assert( direction1 != direction2 );
    //reverse directions for drawing
    direction1 = om_direction::opposite( direction1 );
    direction2 = om_direction::opposite( direction2 );
    bool parallel = om_direction::are_parallel( direction1, direction2 );
    bool north_south = direction1 == om_direction::type::north ||
                       direction1 == om_direction::type::south;

    bool p1_invalid = p1.is_invalid();
    bool p2_invalid = p2.is_invalid();
    if( !p1_invalid ) {
        if( !point_outside_overmap_corner( p1, HIGHWAY_MAX_DEVIANCE ) ) {
            debugmsg( "highway path start point outside of expected deviance" );
        }
    }
    if( !p2_invalid ) {
        if( !point_outside_overmap_corner( p2, HIGHWAY_MAX_DEVIANCE ) ) {
            debugmsg( "highway path end point outside of expected deviance" );
        }
    }

    //bools are separate from points for falling back to onramps even for valid points
    auto handle_invalid_points = [this, &fallback_onramp, &p1, &p2, &direction1, &direction2](
    const bool p1_invalid, const bool p2_invalid ) {
        if( p1_invalid || p2_invalid ) {
            if( !p1_invalid ) {
                tripoint_om_omt p1_rotated = p1;
                highway_segment_offset( p1_rotated, direction1 );
                place_special_forced( fallback_onramp, p1_rotated, direction1 );
            }
            if( !p2_invalid ) {
                tripoint_om_omt p2_rotated = p2;
                highway_segment_offset( p2_rotated, direction2 );
                place_special_forced( fallback_onramp, p2_rotated, direction2 );
            }
            add_msg_debug( debugmode::DF_HIGHWAY,
                           "overmap (%s) took invalid points and is falling back to onramp.",
                           loc.to_string_writable() );
            return true;
        }
        return false;
    };

    //if either point is invalid, fallback to onramp for valid points only
    if( handle_invalid_points( p1_invalid, p2_invalid ) ) {
        return highway_path;
    }

    add_msg_debug( debugmode::DF_HIGHWAY, "overmap (%s) attempting to draw bend from %s to %s.",
                   loc.to_string_writable(), p1.to_string_writable(), p2.to_string_writable() );

    std::vector<std::pair<tripoint_om_omt, om_direction::type>> bend_points;
    bool bend_draw_mode = !( p1.x() == p2.x() || p1.y() == p2.y() );

    //determine bends; z-values are not accounted for here
    if( bend_draw_mode ) {
        //we need to draw two bends
        if( parallel ) {
            bool two_bends = false;
            const tripoint_rel_omt diff = tripoint_rel_omt( p1 - p2 ).abs();
            if( north_south ) { // N/S
                two_bends |= diff.x() >= HIGHWAY_MAX_DEVIANCE;
            } else { // E/W
                two_bends |= diff.y() >= HIGHWAY_MAX_DEVIANCE;
            }
            if( two_bends ) {
                //invalid two-bend configuration
                if( diff.x() < HIGHWAY_MAX_DEVIANCE || diff.y() < HIGHWAY_MAX_DEVIANCE ) {
                    handle_invalid_points( true, true );
                    return highway_path;
                }
                tripoint_om_omt bend_midpoint = midpoint( p1, p2 );
                bend_points.emplace_back( closest_corner_in_direction( p1, bend_midpoint, direction1 ) );
                bend_points.emplace_back( closest_corner_in_direction( bend_midpoint, p2,
                                          bend_points.back().second ) );
            } else {
                bend_draw_mode = false;
            }
        } else { //we need to draw exactly one bend (p1->p2)
            bend_points.emplace_back( closest_corner_in_direction( p1, p2, direction1 ) );
        }
    }

    //the z-value of any tripoints is an indicator for whether the returned highway path is above water
    //but all reserved highway OMTs are placed at z=0
    //the current direction the highway path is traveling (also as vector)
    if( bend_draw_mode ) {
        place_highway_lines_with_bends( highway_path, bend_points, p1, p2, direction1, base_z );
    } else {
        //quick path for no bends
        Highway_path straight_line = place_highway_line( p1, p2, direction1, base_z );
        for( const intrahighway_node &p : straight_line ) {
            highway_path.emplace_back( p );
        }
    }

    highway_handle_special_z( highway_path, base_z );

    //place reserved terrain and specials on path
    for( const intrahighway_node &node : highway_path ) {
        pf::directed_node<tripoint_om_omt> node_z0( tripoint_om_omt(
                    node.path_node.pos.xy(), base_z ), node.path_node.dir );
        if( node.is_segment ) {
            int node_z = node.path_node.pos.z();
            bool reserve_water = node_z > base_z;

            std::vector<overmap_special_locations> locs = node.placed_special->required_locations();
            for( overmap_special_locations &loc : locs ) {
                const oter_type_str_id z0_terrain_to_place = reserve_water ?
                        reserved_terrain_water_id : reserved_terrain_id;
                om_direction::type reserved_direction = node.get_effective_dir();

                tripoint_om_omt rotated = node.path_node.pos +
                                          om_direction::rotate( loc.p, reserved_direction );
                ter_set( tripoint_om_omt( rotated.xy(), base_z ),
                         z0_terrain_to_place.obj().get_rotated( reserved_direction ) );
                //prevent road bridges intersecting
                if( rotated.z() == base_z + 1 ) {
                    ter_set( rotated, z0_terrain_to_place.obj().get_rotated( reserved_direction ) );
                }
            }
        } else {
            place_highway_supported_special( node.placed_special,
                                             node.path_node.pos, node.get_effective_dir() );
        }
    }
    highway_handle_ramps( highway_path, base_z );
    return highway_path;
}

Highway_path overmap::place_highway_line(
    const tripoint_om_omt &sp1, const tripoint_om_omt &sp2,
    const om_direction::type &draw_direction, int base_z )
{
    const overmap_highway_settings &highway_settings = settings->overmap_highway;
    const int longest_slant_length = highway_settings.clockwise_slant.obj().longest_side();

    const point_rel_omt draw_direction_vector =
        point_rel_omt( four_adjacent_offsets[static_cast<int>( draw_direction )] );
    add_msg_debug( debugmode::DF_HIGHWAY,
                   "overmap (%s) attempting to draw line segment from %s to %s.",
                   loc.to_string_writable(), sp1.to_string_writable(), sp2.to_string_writable() );
    const tripoint_rel_omt diff = sp2 - sp1;
    const tripoint_rel_omt abs_diff = diff.abs();
    int minimum_slants = std::min( abs_diff.x(), abs_diff.y() ); //assumes length > slants

    //TODO: minor deviations in highway paths?
    bool slants_optional = minimum_slants == 0;
    Highway_path highway_line;

    tripoint_rel_omt slant_clockwise_vector;
    const tripoint_rel_omt slant_scaled_direction_vector( draw_direction_vector * longest_slant_length,
            0 );
    bool clockwise = one_in( 2 );
    //TODO: generalize
    if( !slants_optional ) {
        //get mandatory slant direction
        point_rel_omt rotate_slant( draw_direction_vector.x() == 0 ? 1 : 0,
                                    draw_direction_vector.y() == 0 ? 1 : 0 );
        slant_clockwise_vector = tripoint_rel_omt(
                                     ( abs_diff.x() != 0 ? diff.x() / abs_diff.x() : 0 ) * rotate_slant.x(),
                                     ( abs_diff.y() != 0 ? diff.y() / abs_diff.y() : 0 ) * rotate_slant.y(), 0 );
        clockwise = point_rel_omt(
                        four_adjacent_offsets[static_cast<int>(
                                om_direction::turn_right( draw_direction ) )] ) == slant_clockwise_vector.xy();
    }

    // always base z for testing terrain
    tripoint_om_omt current_point = tripoint_om_omt( sp1.xy(), base_z );
    int new_z = base_z;
    //the first placed node may not be a slant
    bool no_first_slant = true;
    const overmap_special_id &selected_slant =
        clockwise ? highway_settings.clockwise_slant :
        highway_settings.counterclockwise_slant;

    tripoint_rel_omt update_point;
    overmap_special_id selected_special;
    tripoint_om_omt place_special_at;

    // this loop check works on the assumption that slants:
    // 1) are placed first in the path
    // 2) do not make up the entire path
    while( current_point.xy() != sp2.xy() + draw_direction_vector ) {
        if( !inbounds( current_point ) ) {
            debugmsg( "highway slant pathing out of bounds; falling back to onramp" );
            highway_line.clear();
            return highway_line;
        }
        //leave space for ramps
        bool next_slant_on_water = special_on_water( this, selected_slant,
                                   current_point + slant_scaled_direction_vector + slant_clockwise_vector, draw_direction );
        bool z_change =
            special_on_water( this, selected_slant, current_point, draw_direction ) != next_slant_on_water;
        place_special_at = tripoint_om_omt( current_point.xy(), base_z );

        //TODO: odds are low, but this could fail if z_change is true for too many potential slant points
        bool is_slant = !no_first_slant && ( minimum_slants > 0 && !z_change );
        if( is_slant ) {
            new_z = next_slant_on_water ? base_z + 1 : base_z;
            if( slants_optional ) {
                clockwise = !clockwise;
                slant_clockwise_vector = tripoint_rel_omt(
                                             point_rel_omt( four_adjacent_offsets[static_cast<int>( clockwise ?
                                                                       om_direction::turn_right( draw_direction ) :
                                                                       om_direction::turn_left( draw_direction ) )] ), 0 );
            }
            //because highways are locked to N/E, account for S/W offset
            if( draw_direction == om_direction::type::south ||
                draw_direction == om_direction::type::west ) {
                place_special_at += tripoint_rel_omt( point_rel_omt( four_adjacent_offsets[
                static_cast<int>( om_direction::turn_left( draw_direction ) )] ), 0 );
            }

            selected_special = selected_slant;
            update_point = slant_scaled_direction_vector + slant_clockwise_vector;

            minimum_slants--;
        } else {
            bool is_segment_water = special_on_water( this, highway_settings.segment_flat, current_point,
                                    draw_direction );
            new_z = is_segment_water ? base_z + 1 : base_z;

            selected_special = segment_special( highway_settings, is_segment_water );
            update_point = tripoint_rel_omt( draw_direction_vector, 0 );
        }

        highway_line.emplace_back( tripoint_om_omt( place_special_at.xy(), new_z ), draw_direction,
                                   selected_special, !is_slant );
        current_point += update_point;
        no_first_slant = false;
    }

    return highway_line;
}

void overmap::place_highway_lines_with_bends( Highway_path &highway_path,
        const std::vector<std::pair<tripoint_om_omt, om_direction::type>> &bend_points,
        const tripoint_om_omt &start_point, const tripoint_om_omt &end_point,
        om_direction::type direction, int base_z )
{

    const overmap_highway_settings &highway_settings = settings->overmap_highway;
    const building_bin &bends = highway_settings.bends;
    const overmap_special_id &fallback_onramp = highway_settings.fallback_onramp;

    om_direction::type current_direction = direction;

    if( bend_points.empty() ) {
        debugmsg( "no highway bends found when expected!" );
    }

    //determine all bends before placement
    std::vector<overmap_special_id> precalc_bends;
    //pairs of start/end points to build segments with
    std::vector<std::pair<tripoint_om_omt, tripoint_om_omt>> path_points;

    tripoint_om_omt previous_point = start_point;
    for( const std::pair<tripoint_om_omt, om_direction::type> &p : bend_points ) {
        precalc_bends.emplace_back( bends.pick() );
        path_points.emplace_back( previous_point, p.first );
        previous_point = p.first;
    }
    //last segment on path
    path_points.emplace_back( previous_point, end_point );

    //draw raw paths

    const int path_point_count = path_points.size();
    //bend size is guaranteed to be path count - 1
    const int bend_points_size = bend_points.size();
    overmap_special_id selected_bend;
    int last_bend_length = 0;;
    for( int i = 0; i < path_point_count; i++ ) {
        //truncate path length by bend length
        bool draw_bend = i < bend_points_size;
        if( draw_bend ) {
            selected_bend = precalc_bends[i];
            last_bend_length = selected_bend->longest_side();
        }
        om_direction::type new_bend_direction;
        int bend_z = base_z;

        if( i > 0 ) {
            //start of segment
            path_points[i].first = tripoint_om_omt( path_points[i].first.xy() +
                                                    truncate_segment( om_direction::opposite( bend_points[i - 1].second ), last_bend_length ),
                                                    path_points[i].first.z() );
        }
        if( draw_bend ) {
            //end of segment
            path_points[i].second = tripoint_om_omt( path_points[i].second.xy() +
                                    truncate_segment( current_direction, last_bend_length ),
                                    path_points[i].second.z() );
        }
        Highway_path path = place_highway_line( path_points[i].first,
                                                path_points[i].second, current_direction, base_z );
        if( path.empty() ) {
            place_special_forced( fallback_onramp, path_points[i].first,
                                  om_direction::opposite( current_direction ) );
            place_special_forced( fallback_onramp, path_points[i].second, current_direction );
        }
        for( const intrahighway_node &p : path ) {
            highway_path.emplace_back( p );
        }

        if( draw_bend ) {
            std::pair<tripoint_om_omt, om_direction::type> bend_point = bend_points[i];
            //find bend special rotation
            bool clockwise = om_direction::turn_right( current_direction ) == bend_point.second;
            om_direction::type bend_direction = clockwise ? current_direction : om_direction::turn_right(
                                                    current_direction );
            tripoint_om_omt bend_pos = bend_point.first;
            highway_special_offset( bend_pos, bend_direction );
            bend_z = special_on_water( this, selected_bend, bend_pos, bend_direction ) ? base_z + 1 : base_z;
            bend_pos = tripoint_om_omt( bend_pos.xy(), bend_z );

            highway_path.emplace_back( bend_pos, bend_direction, selected_bend, false );

            new_bend_direction = bend_point.second;
            current_direction = new_bend_direction;

        }
    }
}

std::pair<bool, std::bitset<HIGHWAY_MAX_CONNECTIONS>> overmap::highway_handle_oceans()
{
    std::bitset<HIGHWAY_MAX_CONNECTIONS> ocean_adjacent;
    const point_abs_om this_om = pos();

    if( get_option<bool>( "OVERMAP_PLACE_OCEANS" ) ) {
        // Not ideal as oceans can start later than these settings but it's at least reliably stopping before them
        const int ocean_start_north = settings->overmap_ocean.ocean_start_north == 0 ? INT_MAX :
                                      settings->overmap_ocean.ocean_start_north;
        const int ocean_start_east = settings->overmap_ocean.ocean_start_east == 0 ? INT_MAX :
                                     settings->overmap_ocean.ocean_start_east;
        const int ocean_start_west = settings->overmap_ocean.ocean_start_west == 0 ? INT_MAX :
                                     settings->overmap_ocean.ocean_start_west;
        const int ocean_start_south = settings->overmap_ocean.ocean_start_south == 0 ? INT_MAX :
                                      settings->overmap_ocean.ocean_start_south;
        // Don't place highways over the ocean
        if( this_om.y() <= -ocean_start_north || this_om.x() >= ocean_start_east ||
            this_om.y() >= ocean_start_south || this_om.x() <= -ocean_start_west ) {
            return { true, ocean_adjacent };
        }
        // Check if we need to place partial highway with different intersections
        ocean_adjacent[0] = this_om.y() - 1 == -ocean_start_north;
        ocean_adjacent[1] = this_om.x() + 1 == ocean_start_east;
        ocean_adjacent[2] = this_om.y() + 1 == ocean_start_south;
        ocean_adjacent[3] = this_om.x() - 1 == -ocean_start_west;
        if( ocean_adjacent.count() == HIGHWAY_MAX_CONNECTIONS ) {
            // This should never happen but would break everything
            debugmsg( "Not placing highways because expecting ocean on all sides?!" );
            return { true, ocean_adjacent };
        }
    }
    return { false, ocean_adjacent };
}

//handle ramp placement given z-levels of path nodes
void overmap::highway_handle_ramps( Highway_path &path, int base_z )
{

    const overmap_highway_settings &highway_settings = settings->overmap_highway;
    const overmap_special_id &segment_bridge = highway_settings.segment_bridge;
    const overmap_special_id &segment_ramp = highway_settings.segment_ramp;
    const int range = path.size();

    int current_z = path[0].path_node.pos.z();
    int previous_z = current_z;

    auto fill_bridge = [&segment_bridge]( intrahighway_node & node ) {
        if( node.is_segment ) {
            node.placed_special = segment_bridge;
            node.path_node.pos += tripoint_rel_omt( 0, 0, 1 );
            node.is_ramp = false;
        }
    };

    for( int i = 0; i < range; i++ ) {
        current_z = path[i].path_node.pos.z();
        if( current_z != previous_z ) {
            if( current_z == base_z + 1 ) {
                if( i > 0 && path[i - 1].is_segment ) {
                    path[i - 1].is_ramp = true;
                }
            } else if( current_z == base_z && path[i].is_segment ) {
                bool place_ramp = true;
                if( i + 1 < range ) {
                    if( path[i + 1].path_node.pos.z() == base_z ) {
                        if( i + 2 < range ) {
                            if( path[i + 2].path_node.pos.z() == base_z + 1 ) {
                                fill_bridge( path[i] );
                                fill_bridge( path[i + 1] );
                                place_ramp = false;
                                current_z = base_z + 1;
                            }
                        }
                    } else {
                        fill_bridge( path[i] );
                        place_ramp = false;
                        current_z = base_z + 1;
                    }
                }

                if( place_ramp ) {
                    path[i].is_ramp = true;
                    path[i].ramp_down = true;
                }
            }
        }
        previous_z = current_z;
    }

    //place ramp specials
    for( int i = 0; i < range; i++ ) {
        if( path[i].is_ramp ) {
            om_direction::type ramp_dir = path[i].path_node.dir;
            tripoint_om_omt ramp_offset = path[i].path_node.pos;
            if( path[i].ramp_down ) {
                ramp_dir = om_direction::opposite( ramp_dir );
            }
            highway_segment_offset( ramp_offset, ramp_dir );
            place_special_forced( segment_ramp, ramp_offset, ramp_dir );
        }
    }
}

void overmap::highway_handle_special_z( Highway_path &highway_path, int base_z )
{
    const overmap_highway_settings &highway_settings = settings->overmap_highway;

    const int path_size = highway_path.size();
    for( int i = 1; i < path_size - 1; i++ ) {
        const intrahighway_node &current_node = highway_path[i];
        if( !current_node.is_segment ) {
            int special_z = current_node.path_node.pos.z();
            bool raised = special_z == base_z + 1;
            intrahighway_node &next_node = highway_path[i + 1];
            intrahighway_node &prev_node = highway_path[i - 1];
            if( next_node.is_segment ) {
                next_node.path_node.pos = tripoint_om_omt( next_node.path_node.pos.xy(), special_z );
                next_node.placed_special = segment_special( highway_settings, raised );
            }
            if( prev_node.is_segment ) {
                prev_node.path_node.pos = tripoint_om_omt( prev_node.path_node.pos.xy(), special_z );
                prev_node.placed_special = segment_special( highway_settings, raised );
            }
        }
    }
}

bool overmap::highway_select_end_points( const std::vector<const overmap *> &neighbor_overmaps,
        std::array<tripoint_om_omt, HIGHWAY_MAX_CONNECTIONS> &end_points,
        std::bitset<HIGHWAY_MAX_CONNECTIONS> &neighbor_connections,
        const std::bitset<HIGHWAY_MAX_CONNECTIONS> &ocean_neighbors, int base_z )
{

    const overmap_highway_settings &highway_settings = settings->overmap_highway;
    const int HIGHWAY_MAX_DEVIANCE = highway_settings.HIGHWAY_MAX_DEVIANCE;

    bool any_ocean_adjacent = ocean_neighbors.any();
    add_msg_debug( debugmode::DF_HIGHWAY,
                   "This overmap (%s) IS a highway overmap, with required connections: %s, %s, %s, %s",
                   loc.to_string_writable(),
                   neighbor_connections[0] ? "NORTH" : "",
                   neighbor_connections[1] ? "EAST" : "",
                   neighbor_connections[2] ? "SOUTH" : "",
                   neighbor_connections[3] ? "WEST" : "" );

    //if there are adjacent oceans, cut highway connections
    if( any_ocean_adjacent ) {
        for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
            if( ocean_neighbors[i] ) {
                neighbor_connections[i] = false;
            }
        }
    }

    std::bitset<HIGHWAY_MAX_CONNECTIONS> new_end_point;
    for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
        if( neighbor_connections[i] ) {
            if( neighbor_overmaps[i] != nullptr ) {
                //neighboring overmap highway edge connects to this overmap
                const tripoint_om_omt opposite_edge = neighbor_overmaps[i]->highway_connections[( i + 2 ) %
                                                      HIGHWAY_MAX_CONNECTIONS];
                if( opposite_edge == tripoint_om_omt::zero ) {
                    debugmsg( "highway connections not initialized; inter-overmap highway pathing failed" );
                    return false;
                }
                end_points[i] = wrap_point( opposite_edge );
            } else {
                new_end_point[i] = true;
            }
        }
    }

    //if going N/S or E/W, new highways tend to go straight through an overmap
    for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
        if( new_end_point[i] ) {
            tripoint_om_omt to_wrap = end_points[( i + 2 ) % HIGHWAY_MAX_CONNECTIONS];
            tripoint_om_omt fallback_random_point = random_entry(
                    get_border( point_rel_om( four_adjacent_offsets[i] ), base_z, HIGHWAY_MAX_DEVIANCE ) );
            if( to_wrap.is_invalid() ) {
                end_points[i] = fallback_random_point;
            } else {
                //additional check for an almost-straight point instead of a completely random point
                if( !x_in_y( highway_settings.straightness_chance, 1.0 ) ) {
                    tripoint_om_omt wrapped_point = wrap_point( to_wrap );
                    std::vector<tripoint_om_omt> border_points = get_border( point_rel_om( four_adjacent_offsets[i] ),
                            base_z, HIGHWAY_MAX_DEVIANCE );
                    std::vector<tripoint_om_omt> border_points_in_radius;
                    for( const tripoint_om_omt &p : border_points ) {
                        if( rl_dist( p, wrapped_point ) < HIGHWAY_MAX_DEVIANCE &&
                            point_outside_overmap_corner( p, HIGHWAY_MAX_DEVIANCE ) ) {
                            border_points_in_radius.emplace_back( p );
                        }
                    }
                    end_points[i] = random_entry( border_points_in_radius );
                } else {
                    end_points[i] = fallback_random_point;
                }
            }
            if( is_water_body( ter( end_points[i] ) ) ) {
                end_points[i] += tripoint_rel_omt( 0, 0, 1 );
            }
        }
    }
    return true;
}

std::vector<Highway_path> overmap::place_highways(
    const std::vector<const overmap *> &neighbor_overmaps )
{
    std::vector<Highway_path> paths;
    const int base_z = RIVER_Z;

    const overmap_highway_settings &highway_settings = settings->overmap_highway;
    // Base distance in overmaps between vertical highways ( whole overmap gap of column_seperation - 1 )
    const int c_seperation = highway_settings.grid_column_seperation;
    // Base distance in overmaps between horizontal highways ( whole overmap gap of row_seperation - 1 )
    const int r_seperation = highway_settings.grid_row_seperation;
    const int HIGHWAY_MAX_DEVIANCE = highway_settings.HIGHWAY_MAX_DEVIANCE;

    std::pair<bool, std::bitset<HIGHWAY_MAX_CONNECTIONS>> handle_oceans = highway_handle_oceans();
    bool is_ocean = handle_oceans.first;
    std::bitset<HIGHWAY_MAX_CONNECTIONS> ocean_neighbors = handle_oceans.second;

    if( is_ocean ) {
        return paths;
    }

    if( c_seperation == 0 && r_seperation == 0 ) {
        debugmsg( "Use the external option OVERMAP_PLACE_HIGHWAYS to disable highways instead" );
        return paths;
    }

    // guaranteed intersection close to (but not at) avatar start location
    if( overmap_buffer.highway_global_offset.is_invalid() ) {
        overmap_buffer.set_highway_global_offset();
        overmap_buffer.generate_highway_intersection_point( overmap_buffer.get_highway_global_offset() );
    }

    //resolve connection points, and generate new connection points
    std::array<tripoint_om_omt, HIGHWAY_MAX_CONNECTIONS> end_points;
    std::fill( end_points.begin(), end_points.end(), tripoint_om_omt::invalid );
    std::optional<std::bitset<HIGHWAY_MAX_CONNECTIONS>> is_highway_neighbors = is_highway_overmap();
    std::bitset<HIGHWAY_MAX_CONNECTIONS> neighbor_connections;

    if( !is_highway_neighbors ) {
        add_msg_debug( debugmode::DF_HIGHWAY, "This overmap (%s) is NOT a highway overmap.",
                       loc.to_string_writable() );
        return paths;
    } else {
        neighbor_connections = *is_highway_neighbors;
        if( !highway_select_end_points( neighbor_overmaps, end_points, neighbor_connections,
                                        ocean_neighbors, base_z ) ) {
            return paths;
        }
    }

    int neighbor_connection_count = neighbor_connections.count();
    overmap_special_id special;
    overmap_special_id fallback_special;

    //determine existing intersections/curves
    switch( neighbor_connection_count ) {
        case 2: {
            //draw end-to-end
            if( neighbor_connections[0] && neighbor_connections[2] ) {
                paths.emplace_back( place_highway_reserved_path( end_points[0], end_points[2], 0, 2, base_z ) );
            } else if( neighbor_connections[1] && neighbor_connections[3] ) {
                paths.emplace_back( place_highway_reserved_path( end_points[1], end_points[3], 1, 3, base_z ) );
            } else {
                for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
                    int next_edge = ( i + 1 ) % HIGHWAY_MAX_CONNECTIONS;
                    if( neighbor_connections[i] && neighbor_connections[next_edge] ) {
                        paths.emplace_back( place_highway_reserved_path( end_points[i], end_points[next_edge], i, next_edge,
                                            base_z ) );
                    }
                }
            }
            break;
        }
        case 3: {
            om_direction::type empty_direction = om_direction::type::north;
            for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
                if( !neighbor_connections[i] ) {
                    empty_direction = om_direction::type( i );
                    break;
                }
            }

            //draw 3-way intersection (favors towards closer pair of points if possible)
            //this will be very rare, usually only for ocean-adjacent OMs
            tripoint_om_omt three_point( OMAPX / 2, OMAPY / 2, 0 );
            //draw end-to-end from end points to 3-way intersection
            for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
                if( i != static_cast<int>( empty_direction ) ) {
                    paths.emplace_back( place_highway_reserved_path( end_points[i], three_point, i, ( i + 2 ) % 4,
                                        RIVER_Z ) );
                }
            }
            fallback_special = settings->overmap_highway.fallback_three_way_intersection;
            special = settings->overmap_highway.three_way_intersections.pick();
            highway_special_offset( three_point, empty_direction );
            place_highway_supported_special( special, three_point, empty_direction );
            break;
        }
        case 4: {
            // first, check pairs of corners -- we can't draw a pair of bends to a center intersection if there's no room!
            tripoint_om_omt four_point( OMAPX / 2, OMAPY / 2, 0 );
            const double corner_threshold = OMAPX / 3.0;
            std::bitset<HIGHWAY_MAX_CONNECTIONS> corners_close; //starting at NE
            for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
                corners_close[i] = rl_dist( end_points[i],
                                            end_points[( i + 1 ) % HIGHWAY_MAX_CONNECTIONS] ) < corner_threshold * std::sqrt( 2.0 );
            }
            if( corners_close.count() == 1 ) {
                //for one pair of close corners, set the intersection to their shared point
                for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
                    if( corners_close[i] ) {
                        four_point = closest_corner_in_direction( end_points[i],
                                     end_points[( i + 1 ) % HIGHWAY_MAX_CONNECTIONS],
                                     om_direction::opposite( om_direction::all[i] ) ).first;
                    }
                }
            } else if( corners_close.count() == 2 ) {
                //if there are two pairs of close corners, draw two 3-way intersections at corners and connect them
                const int THREE_WAY_COUNT = 2;
                std::array<tripoint_om_omt, THREE_WAY_COUNT> three_way_intersections;
                std::array<om_direction::type, THREE_WAY_COUNT> three_way_direction;
                special = settings->overmap_highway.three_way_intersections.pick();
                for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
                    if( corners_close[i] ) {
                        const int dir1 = i;
                        const int dir2 = ( i + 1 ) % HIGHWAY_MAX_CONNECTIONS;
                        const tripoint_om_omt &end1 = end_points[dir1];
                        const tripoint_om_omt &end2 = end_points[dir2];
                        const int three_way_index = i / 2;

                        om_direction::type opposite_dir = om_direction::opposite( om_direction::all[dir1] );
                        three_way_direction[three_way_index] = opposite_dir;
                        three_way_intersections[three_way_index] = find_highway_intersection_point( special,
                                closest_corner_in_direction( end1, end2, opposite_dir ).first, opposite_dir, HIGHWAY_MAX_DEVIANCE );
                        paths.emplace_back( place_highway_reserved_path( end1,
                                            three_way_intersections[three_way_index], dir1, ( dir1 + 2 ) % HIGHWAY_MAX_CONNECTIONS, RIVER_Z ) );
                        paths.emplace_back( place_highway_reserved_path( end2,
                                            three_way_intersections[three_way_index], dir2, ( dir2 + 2 ) % HIGHWAY_MAX_CONNECTIONS, RIVER_Z ) );
                    }
                }
                paths.emplace_back( place_highway_reserved_path( three_way_intersections[0],
                                    three_way_intersections[1],
                                    static_cast<int>( om_direction::turn_left( three_way_direction[0] ) ),
                                    static_cast<int>( om_direction::turn_left( three_way_direction[1] ) ), base_z ) );

                for( int i = 0; i < THREE_WAY_COUNT; i++ ) {
                    highway_special_offset( three_way_intersections[i], three_way_direction[i] );
                    place_highway_supported_special( special, three_way_intersections[i], three_way_direction[i] );
                }
                break;
            }

            special = settings->overmap_highway.four_way_intersections.pick();
            om_direction::type intersection_dir = om_direction::type::north;
            four_point = find_highway_intersection_point( special, four_point, intersection_dir,
                         HIGHWAY_MAX_DEVIANCE );

            //draw end-to-end from ends points to 4-way intersection
            for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
                paths.emplace_back( place_highway_reserved_path( end_points[i], four_point, i, ( i + 2 ) % 4,
                                    base_z ) );
            }
            fallback_special = settings->overmap_highway.fallback_four_way_intersection;
            place_highway_supported_special( special, four_point, om_direction::type::north );
            break;
        }
        default: // 1
            //this shouldn't happen often, but if it does, end the highway at overmap edges
            //by drawing start point -> invalid point
            tripoint_om_omt dummy_point = tripoint_om_omt::invalid;
            for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
                if( !end_points[i].is_invalid() ) {
                    paths.emplace_back( place_highway_reserved_path( end_points[i], dummy_point, i, ( i + 2 ) % 4,
                                        base_z ) );
                }
            }
    }

    highway_connections = end_points;
    return paths;
}

std::pair<std::vector<tripoint_om_omt>, om_direction::type>
overmap::get_highway_segment_points( const pf::directed_node<tripoint_om_omt> &node ) const
{
    //highway segments can only draw N/E
    om_direction::type draw_dir = om_direction::type( static_cast<int>( node.dir ) % 2 );
    const tripoint_rel_omt draw_point_dir( point_rel_omt( four_adjacent_offsets[static_cast<int>
                                           ( om_direction::turn_right( draw_dir ) )] ), 0 );
    const tripoint_om_omt z0( node.pos.xy(), RIVER_Z );
    std::vector<tripoint_om_omt> draw_points = { z0, z0 + draw_point_dir };
    return std::make_pair( draw_points, draw_dir );
}

void overmap::finalize_highways( std::vector<Highway_path> &paths )
{
    // Segment of flat highway with a road bridge
    const overmap_special_id &segment_road_bridge = settings->overmap_highway.segment_road_bridge;

    // Replace roads that travelled over reserved highway segments
    auto handle_road_bridges = [this, &segment_road_bridge]( Highway_path & path ) {
        const int range = path.size();
        for( int i = 0; i < range; i++ ) {
            //only one of two points should ever need to be checked for roads specifically
            if( is_road( ter( path[i].path_node.pos ) ) ) {
                path[i].placed_special = segment_road_bridge;
            }
        }
    };

    for( Highway_path &path : paths ) {
        if( path.empty() ) {
            continue;
        }
        handle_road_bridges( path );

        for( const intrahighway_node &node : path ) {
            if( !is_highway_special( ter( node.path_node.pos ) ) && node.is_segment && !node.is_ramp ) {
                place_highway_supported_special( node.placed_special,
                                                 node.path_node.pos, node.get_effective_dir() );
            }
        }
    }
}

std::optional<std::bitset<HIGHWAY_MAX_CONNECTIONS>> overmap::is_highway_overmap() const
{
    point_abs_om pos = this->pos();
    std::optional<std::bitset<HIGHWAY_MAX_CONNECTIONS>> result;

    // first, get the offset for every interhighway_node bounding this overmap
    // generate points if needed
    std::vector<point_abs_om> bounds = overmap_buffer.find_highway_intersection_bounds( pos );

    // if we're on one of those intersection OMs, return immediately with all cardinal connection points
    for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
        point_abs_om intersection_grid = bounds[i];
        if( overmap_buffer.get_overmap_highway_intersection_point( intersection_grid ).offset_pos == pos ) {
            return result.emplace( "1111" );
        }
    }

    //otherwise, check EVERY path connected to bounds, generating if necessary
    std::unordered_set<std::pair<point_abs_om, point_abs_om>, cata::tuple_hash> path_cache;

    auto check_on_path = [&pos, &path_cache]( const point_abs_om & center,
    const point_abs_om & adjacent ) {
        std::optional<std::vector<point_abs_om>> resulting_path;

        //orthogonal line must always take a set of two points in the same order for highway overmap paths
        std::vector<point_abs_om> ordered_points = { center, adjacent };
        std::sort( ordered_points.begin(), ordered_points.end(), []( const point_abs_om & p1,
        const point_abs_om & p2 ) {
            return p1.y() == p2.y() ? p1.x() < p2.x() : p1.y() < p2.y();
        } );

        //repeat paths are possible; ignore them
        std::pair< point_abs_om, point_abs_om> front_back = { ordered_points.front(), ordered_points.back() };
        if( path_cache.find( front_back ) != path_cache.end() ) {
            return resulting_path;
        }
        path_cache.insert( front_back );
        std::vector<point_abs_om> highway_oms = orthogonal_line_to( front_back.first, front_back.second );

        std::string path_debug_output;
        for( const point_abs_om &p : highway_oms ) {
            path_debug_output += p.to_string_writable() + " ";
        }
        add_msg_debug( debugmode::DF_HIGHWAY, "for %s to %s, checking path: %s",
                       center.to_string_writable(),
                       adjacent.to_string_writable(), path_debug_output );

        if( std::find( highway_oms.begin(), highway_oms.end(), pos ) != highway_oms.end() ) {
            resulting_path.emplace( highway_oms );
        }
        return resulting_path;
    };

    std::vector<std::optional<std::vector<point_abs_om>>> highway_oms;
    for( const point_abs_om &bound_point : bounds ) {
        interhighway_node bound_intersection =
            overmap_buffer.get_overmap_highway_intersection_point( bound_point );
        for( const interhighway_node &intersection : overmap_buffer.find_highway_adjacent_intersections(
                 bound_intersection.grid_pos ) ) {
            highway_oms.emplace_back( check_on_path( bound_intersection.offset_pos, intersection.offset_pos ) );
        }
    }

    std::bitset<HIGHWAY_MAX_CONNECTIONS> connections;
    for( std::optional<std::vector<point_abs_om>> path : highway_oms ) {
        if( !!path ) {
            for( const point_abs_om &p : *path ) {
                for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
                    if( p == pos + point_rel_om( four_adjacent_offsets[i] ) ) {
                        connections[i] = true;
                    }
                }
            }
            return result.emplace( connections );
        }
    }
    return result;
}

void interhighway_node::generate_offset( int intersection_max_radius )
{
    auto no_lakes = []( const point_abs_om & pt ) {
        //TODO: this can't be correct usage of default region settings...
        const overmap_lake_settings &lake_settings = overmap_buffer.get_default_settings( pt ).overmap_lake;
        return !overmap::guess_has_lake( pt, lake_settings.noise_threshold_lake,
                                         lake_settings.lake_size_min );
    };
    tripoint_abs_om as_tripoint( grid_pos, 0 );
    tripoint_range radius = points_in_radius( as_tripoint, intersection_max_radius );
    std::vector<point_abs_om> intersection_candidates;

    for( const tripoint_abs_om &p : radius ) {
        if( p != as_tripoint && no_lakes( p.xy() ) ) { //intersection cannot generate at origin
            intersection_candidates.emplace_back( p.xy() );
        }
    }
    if( intersection_candidates.empty() ) {
        debugmsg( "no highway intersections could be generated in overmap radius of %d at overmap %s; reduce lake frequency",
                  intersection_max_radius, grid_pos.to_string() );
    }
    offset_pos = random_entry( intersection_candidates );
}

void interhighway_node::serialize( JsonOut &out ) const
{
    out.start_object();
    out.member( "grid_pos", grid_pos );
    out.member( "offset_pos", offset_pos );
    out.member( "adjacent_intersections", adjacent_intersections );
    out.end_object();
}

void interhighway_node::deserialize( const JsonObject &obj )
{
    obj.read( "grid_pos", grid_pos );
    obj.read( "offset_pos", offset_pos );
    obj.read( "adjacent_intersections", adjacent_intersections );
}

void overmap::place_highway_supported_special( const overmap_special_id &special,
        const tripoint_om_omt &placement, const om_direction::type &dir )
{
    if( placement.is_invalid() ) {
        return;
    }
    place_special_forced( special, placement, dir );
    const std::vector<overmap_special_terrain> locs = special->get_terrains();

    const oter_id fallback_supports = settings->overmap_highway.fallback_supports.obj().get_first();

    for( const overmap_special_terrain &loc : locs ) {
        const tripoint_om_omt rotated = placement + om_direction::rotate( loc.p, dir );
        if( loc.terrain.id() == fallback_supports ) {
            place_special_forced( settings->overmap_highway.segment_bridge_supports,
                                  rotated, dir );
        }
    }
}
