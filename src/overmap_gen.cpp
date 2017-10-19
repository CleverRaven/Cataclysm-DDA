#include "overmap_gen.h"
#include "overmap.h"
#include "omdata.h"
#include "simple_pathfinding.h"
#include "line.h"
#include "rng.h"

#include <queue>

using oter_type_id = int_id<oter_type_t>;
using oter_type_str_id = string_id<oter_type_t>;

struct city_block_stub {
    city_block_stub( const tripoint &p, om_direction::type d )
        : origin( p ), dir( d )
    {
    }

    tripoint origin;
    om_direction::type dir;
};

struct block_blueprint {
    block_blueprint() = default;
    block_blueprint( const block_blueprint & ) = default;
    block_blueprint( int w, int l, const std::set<om_direction::type> &hr )
        : width( w ), length( l ), has_road( hr )
    {}
    int width = 0;
    int length = 0;
    std::set<om_direction::type> has_road;
};

// Maximum area that a city block could occupy
// Allows roads on the edge (but only the edge)
static block_blueprint max_block_area( const overmap &om, const tripoint &origin,
                                       om_direction::type dir_lenght, int max_length,
                                       om_direction::type dir_width, int max_width )
{
    static const oter_type_id road( "road" );
    // @todo De-hardcode
    const oter_id &valid_ter = om.get_settings().default_oter;
    // It's possible for a block to be wider than it is long
    // We'll swap that later

    block_blueprint max_block_blueprint;
    max_block_blueprint.has_road.insert( om_direction::opposite( dir_lenght ) );
    max_block_blueprint.has_road.insert( om_direction::opposite( dir_width ) );
    bool done_expanding_length = false;
    bool done_expanding_width = false;
    while( true ) {
        done_expanding_length |= max_block_blueprint.length >= max_length;
        done_expanding_width |= max_block_blueprint.width >= max_width;
        if( done_expanding_length && done_expanding_width ) {
            // Reached our limits
            break;
        }

        const bool expanding_length = done_expanding_width || ( !done_expanding_length && !one_in( 3 ) );
        // Direction in which we're expanding
        const om_direction::type expand_dir = expanding_length ? dir_lenght : dir_width;
        int &expanded_edge = expanding_length ? max_block_blueprint.length : max_block_blueprint.width;
        // The edge along which we're expanding
        const om_direction::type edge_dir = expanding_length ? dir_width : dir_lenght;
        int edge_length = expanding_length ? max_block_blueprint.width : max_block_blueprint.length;

        tripoint cur_edge = origin + om_direction::displace( expand_dir, expanded_edge );
        bool &failed_expand = expanding_length ? done_expanding_length : done_expanding_width;
        // If this is true, we allow this expansion, but don't expand further
        bool had_road = false;
        for( int edge_dist = 0; edge_dist < edge_length; edge_dist++ ) {
            if( cur_edge.x <= 0 || cur_edge.y <= 0 ||
                cur_edge.x >= OMAPX - 1 || cur_edge.y >= OMAPY - 1 ) {
                failed_expand = true;
                break;
            }
            const oter_id &cur_ter = om.get_ter( cur_edge );
            if( cur_ter->type_is( road ) ) {
                had_road = true;
            } else if( cur_ter != valid_ter ) {
                failed_expand = true;
                break;
            }

            cur_edge += om_direction::displace( edge_dir );
        }

        if( !failed_expand ) {
            expanded_edge++;
        }

        // Note: we fail only after expanding
        if( had_road ) {
            failed_expand = true;
            max_block_blueprint.has_road.insert( expand_dir );
        }
    }

    return max_block_blueprint;
}

// Length of terrain we can cover with road or that has a road
static size_t free_or_road_len( const overmap &om, const tripoint &origin, om_direction::type dir, size_t max_len )
{
    // @todo This road should be the main road, not just any road
    static const oter_type_id road( "road" );
    const oter_id &valid_ter = om.get_settings().default_oter;
    const point offset = om_direction::displace( dir );
    size_t ret = 0;
    tripoint cur = origin + offset;
    while( ret < max_len ) {
        // Overmaps have a no-road border
        if( cur.x <= 0 || cur.y <= 0 || cur.x >= OMAPX - 1 || cur.y >= OMAPY - 1 ) {
            break;
        }

        const oter_id &cur_ter = om.get_ter( cur );
        if( cur_ter != valid_ter && !om.get_ter( cur )->type_is( road ) ) {
            break;
        }

        ret++;
        cur += offset;
    }

    return ret;
}

// @todo Make it understand non-square buildings
static overmap_special_id pick_building_for_blueprint( const overmap &om, const block_blueprint &bp )
{
    static const overmap_special_id ugly_hardcoded_temp_id_2x2( "apartaments_no_road" );
    static const overmap_special_id ugly_hardcoded_temp_id_3x3( "Mansion_Wild" );
    if( bp.width == 3 && bp.length == 3 ) {
        return ugly_hardcoded_temp_id_3x3;
    }
    if( bp.width == 2 && bp.length == 2 ) {
        return ugly_hardcoded_temp_id_2x2;
        //om.get_settings().city_spec.pick_house();
    }
    return om.get_settings().city_spec.pick_house();
}

// Returns the northern-western-most and coords of a rotated special
// Needed for re-centering
tripoint min_size_rotated( const overmap_special &sp, om_direction::type dir )
{
    tripoint min( INT_MAX, INT_MAX, 0 );
    for( const overmap_special_terrain &ter : sp.terrains ) {
        const tripoint rotated = om_direction::rotate( ter.p, dir );
        min.x = std::min( min.x, rotated.x );
        min.y = std::min( min.y, rotated.y );
    }

    return min;
}

tripoint max_size_rotated( const overmap_special &sp, om_direction::type dir )
{
    tripoint max( INT_MIN, INT_MIN, 0 );
    for( const overmap_special_terrain &ter : sp.terrains ) {
        const tripoint rotated = om_direction::rotate( ter.p, dir );
        max.x = std::max( max.x, rotated.x );
        max.y = std::max( max.y, rotated.y );
    }

    return max;
}

// Origin should be the point in road, not inside the block
void overmap_gen::city_building_line( overmap &om, const city &new_city, const tripoint &origin,
                                      om_direction::type length_dir, int length,
                                      om_direction::type width_dir, int width )
{
    // This assumes square buildings
    // @todo Allow non-square buildings and get rid of this
    const int extra = length % width;
    // If we can't neatly divide the block, we need to fill the excess with smaller buildings
    int fill_closer = extra / 2 + extra % 2;
    int fill_further = extra / 2;
    if( one_in( 2 ) ) {
        std::swap( fill_closer, fill_further );
    }

    // Ugly: when width_dir is not north, the origin of building is not aligned with origin of block
    // The problem is, proper alignment depends on current directions (both!)
    // width_dir determines how will the building be rotated. Requires subtracting minimum coords of rotated special.
    // For length_dir, we must ensure the north-western-most square is an origin of one of the buildings
    // It could probably be achieved using some sort of clever transform, but let's just generate a rectangle and populate that
    const tripoint first_point = origin + om_direction::displace( length_dir, fill_closer ) - om_direction::displace( width_dir, width );
    const tripoint last_point = first_point + om_direction::displace( length_dir, length - 1 - extra ) + om_direction::displace( width_dir, width - 1 );
    const tripoint min_pt = tripoint( std::min( first_point.x, last_point.x ), std::min( first_point.y, last_point.y ), origin.z );
    const tripoint max_pt = tripoint( std::max( first_point.x, last_point.x ), std::max( first_point.y, last_point.y ), origin.z );
    const point length_step = om_direction::displace( length_dir, width );
    const point step_pt = point( abs( length_step.x ), abs( length_step.y ) );

    const block_blueprint bp( width, width, {} );
    
    tripoint cur_pt = min_pt;
    while( cur_pt.x >= min_pt.x || cur_pt.y >= min_pt.y ) {
        // We need to re-center the buildings
        // This isn't as easy as subtracting a rotated point (or is it?)
        const overmap_special &building = pick_building_for_blueprint( om, bp ).obj();
        const tripoint cur_origin = cur_pt - min_size_rotated( building, width_dir );
        const tripoint south_eastern_most = cur_origin + max_size_rotated( building, width_dir );
        if( south_eastern_most.x > max_pt.x || south_eastern_most.y > max_pt.y ) {
            break;
        }
        om.place_special( building, cur_origin, width_dir, new_city );
        om.place_special( *om.get_settings().city_spec.pick_house(), cur_origin, width_dir, new_city );
        cur_pt += step_pt;
    }
    // Recursive calls at smaller size
    if( fill_closer > 0 ) {
        city_building_line( om, new_city, origin - om_direction::displace( width_dir, width ) - om_direction::displace( length_dir ),
                            width_dir, width, om_direction::opposite( length_dir ), fill_closer );
    }
    if( fill_further > 0 ) {
        city_building_line( om, new_city, origin - om_direction::displace( width_dir, width ) + om_direction::displace( length_dir, length - fill_further + 1 ),
                            width_dir, width, length_dir, fill_further );
    }
}

/**
 * New city building goes like this:
 *  Place center block (nesw_road)
 *  Extend main roads from center block to city size (or less, if blocked)
 *  Create a queue of { origin, direction }
 *  For direction in { north, east, south, west }:
 * * Enqueue { city center, direction }
 *  For every element in queue:
 * * Take it down from the queue
 * * Calculate leftover area from city size and distance from city center
 * * If the leftover area is too small, go to old citygen (overmap::build_city_street)
 * * If the area is big enough, try to build a rectangle on the right side of the road
 *  * The rectangle must have size of at least 4x4 (2 for roads, then 2x2 in the center)
 *  * The rectangle must be bounded from two sides by main roads
 *  * Create the roads that form boundaries of the rectangle
 *  * While the rectangle is not filled:
 *   * Brute force all possible sizes of buildings (O(n^4), but n<10)
 *   * Pick a building to place and place it
 * * If the rectangle was placed, decrease leftover city area by area of new rectangle
 * * If leftover city area is greater than some constant:
 *  * Find two corners of the rectangle: one in current direction, other to the side
 *  * Enqueue { corner in front, direction }
 *  * Enqueue { corner to side, rotate_clockwise( direction ) }
 */
void overmap_gen::build_city( overmap &om, const tripoint &city_origin, int size )
{
    static const string_id<overmap_connection> local_road_id( "local_road" );
    const overmap_connection &local_road( *local_road_id );
    static const oter_type_id road_type( "road" );

    // Every city starts with an intersection
    om.ter( city_origin ) = oter_id( "road_nesw" );
    city new_city;
    new_city.x = city_origin.x;
    new_city.y = city_origin.y;
    new_city.s = size;

    std::queue<city_block_stub> to_build;
    // Pick random direction
    {
        om_direction::type end_dir = random_entry( om_direction::all );
        om_direction::type cur_dir = end_dir;
        do {
            cur_dir = om_direction::turn_right( cur_dir );
            to_build.emplace( city_origin, cur_dir );
        } while( cur_dir != end_dir );
    }

    // Old overmapgen roads - we only want those after all blocks are built
    std::queue<city_block_stub> small_roads;
    while( !to_build.empty() ) {
        const city_block_stub cur_block = to_build.front();
        to_build.pop();
        const tripoint &origin = cur_block.origin;
        float dist = trig_dist( origin, city_origin );
        float leftover_area = static_cast<float>( size * size ) - dist * dist;
        // @todo Less hardcoded
        if( leftover_area < 0.0f ) {
            continue;
        } else if( leftover_area < 3.0f ) {
            small_roads.emplace( origin, cur_block.dir );
            continue;
        }

        bool flip = one_in( 2 );
        const om_direction::type cur_dir = flip ? cur_block.dir : om_direction::turn_right( cur_block.dir );
        const om_direction::type width_dir = flip ? om_direction::turn_right( cur_block.dir ): cur_block.dir;

        // First we check how much road we have on the edge
        // We subtract 1 because we want an extra for the edge (and it is clearer to subtract it here)
        int max_length = free_or_road_len( om, origin, cur_dir, static_cast<int>( std::min( size / 2.0f, leftover_area / 5.0f ) ) ) - 1; 
        int max_width = free_or_road_len( om, origin, width_dir, 7 ) - 1;
        if( max_length < 4 || max_length < 4 ) {
            small_roads.emplace( origin, cur_block.dir );
            continue;
        }

        // We need to move the origin inside the 'V' formed by roads
        const tripoint inner = origin + om_direction::displace( cur_dir ) + om_direction::displace( width_dir );
        block_blueprint max_block_blueprint = max_block_area( om, inner,
                                                              cur_dir, max_length + 1,
                                                              width_dir, max_width + 1 );

        // We subtract 1 because block size includes the other edge and we want internal
        int length = max_block_blueprint.length - 1;
        int width = max_block_blueprint.width - 1;
        // If we're here, it means we have a rectangle in which we can build
        // It may be too small, though
        // If so, fallback to old city gen
        if( width < 4 || length < 4 ) {
            small_roads.emplace( origin, cur_block.dir );
            continue;
        }

        // If the end of the road is far enough (or not existent) we may want to shorten it
        // But if we do, we want at least 4 free tiles between end of the road and the other road
        // Otherwise we may get ugly "double roads"
        bool road_at_length = max_block_blueprint.has_road.count( cur_dir ) > 0;
        if( !road_at_length || length > 10 ) {
            length = rng( 4 + ( length - 4 ) / 2, length );
            if( length < max_block_blueprint.length && road_at_length ) {
                // We want those 4 free tiles here
                length = std::min( length, max_block_blueprint.length - 5 );
                road_at_length = false;
            }
        }

        // Avoid bigger blocks
        if( !road_at_length && length > 4 && length % 2 != 0 && one_in( 2 ) ) {
            length -= length % 2;
        }

        // We request 6 width blocks, but we'll only take 4 with ones
        // This is, again, to avoid double roads
        // But if the block does have a road on the end, we want to take that and special case it
        bool road_at_width = max_block_blueprint.has_road.count( width_dir ) > 0;
        if( !road_at_width && width >= 6 ) {
            width = 4;
        }

        // Place roads on all 4 sides of the block
        // Their length is padded with +1 because we want it to start at existing road, so origin is moved one tile back
        // Cast origin down to point because the function wants 2D for some reason
        const point origin_pt = point( origin.x, origin.y );
        const auto path_1 = om.lay_out_street( local_road, origin_pt, cur_dir, length + 2, true );
        om.build_connection( local_road, path_1, 0 );
        const auto path_2 = om.lay_out_street( local_road, origin_pt + om_direction::displace( width_dir, width + 1 ), cur_dir, length + 2, true );
        om.build_connection( local_road, path_2, 0 );
        const auto path_3 = om.lay_out_street( local_road, origin_pt, width_dir, width + 2, true );
        om.build_connection( local_road, path_3, 0 );
        const auto path_4 = om.lay_out_street( local_road, origin_pt + om_direction::displace( cur_dir, length + 1 ), width_dir, width + 2, true );
        om.build_connection( local_road, path_4, 0 );

        int size_first = width / 2;
        int size_second = width / 2 + width % 2;
        if( one_in( 2 ) ) {
            std::swap( size_first, size_second );
        }
        const tripoint first_line = inner - om_direction::displace( width_dir );
        const tripoint second_line = inner + om_direction::displace( width_dir, width );
        city_building_line( om, new_city, first_line, cur_dir, length, om_direction::opposite( width_dir ), size_first );
        city_building_line( om, new_city, second_line, cur_dir, length, width_dir, size_second );

        // Note: we are using the original directions here, otherwise they get flipped into inside
        to_build.emplace( origin + om_direction::displace( cur_dir, length + 1 ), cur_block.dir );
        to_build.emplace( origin + om_direction::displace( width_dir, width + 1 ), cur_block.dir );
    }

    // Now small roads
    // They must be last, otherwise they will mess up block construction
    while( !small_roads.empty() ) {
        const city_block_stub road = small_roads.front();
        small_roads.pop();
        float dist = trig_dist( road.origin, city_origin );
        float leftover_area = static_cast<float>( size * size ) - dist * dist;
        int road_len = roll_remainder( sqrt( leftover_area ) );
        om.build_city_street( local_road, { road.origin.x, road.origin.y }, road_len, road.dir, new_city );
    }

    om.cities.push_back( new_city );
}