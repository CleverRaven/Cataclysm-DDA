#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iterator>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "city.h"
#include "common_types.h"
#include "coordinates.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "flood_fill.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "omdata.h"
#include "options.h"
#include "overmap.h"
#include "overmap_connection.h"
#include "overmapbuffer.h"
#include "point.h"
#include "regional_settings.h"
#include "rng.h"
#include "simple_pathfinding.h"
#include "type_id.h"

static const oter_str_id oter_road_nesw( "road_nesw" );
static const oter_str_id oter_road_nesw_manhole( "road_nesw_manhole" );

static const oter_type_str_id oter_type_road( "road" );

static constexpr int BUILDINGCHANCE = 4;

static pf::directed_path<point_om_omt> straight_path( const point_om_omt &source,
        om_direction::type dir, size_t len )
{
    pf::directed_path<point_om_omt> res;
    if( len == 0 ) {
        return res;
    }
    point_om_omt p = source;
    res.nodes.reserve( len );
    for( size_t i = 0; i + 1 < len; ++i ) {
        res.nodes.emplace_back( p, dir );
        p += om_direction::displace( dir );
    }
    res.nodes.emplace_back( p, om_direction::type::invalid );
    return res;
}

/*: the root is overmap::place_cities()
20:50 <kevingranade>: which is at overmap.cpp:1355 or so
20:51 <kevingranade>: the key is cs = rng(4, 17), setting the "size" of the city
20:51 <kevingranade>: which is roughly it's radius in overmap tiles
20:52 <kevingranade>: then later overmap::place_mongroups() is called
20:52 <kevingranade>: which creates a mongroup with radius city_size * 2.5 and population city_size * 80
20:53 <kevingranade>: tadaa

spawns happen at... <cue Clue music>
20:56 <kevingranade>: game:pawn_mon() in game.cpp:7380*/
void overmap::place_cities()
{
    int op_city_spacing = get_option<int>( "CITY_SPACING" );
    int op_city_size = get_option<int>( "CITY_SIZE" );
    int max_urbanity = settings->max_urban;
    if( op_city_size <= 0 ) {
        return;
    }
    // make sure city size adjust is never high enough to drop op_city_size below 2
    int city_size_adjust = std::min( urbanity - static_cast<int>( forestosity / 2.0f ),
                                     -1 * op_city_size + 2 );
    int city_space_adjust = urbanity / 2;
    int max_city_size = std::min( op_city_size + city_size_adjust, op_city_size * max_urbanity );
    if( max_city_size < op_city_size ) {
        // funny things happen if max_city_size is less than op_city_size.
        max_city_size = op_city_size;
    }
    if( op_city_spacing > 0 ) {
        city_space_adjust = std::min( city_space_adjust, op_city_spacing - 2 );
        op_city_spacing = op_city_spacing - city_space_adjust + static_cast<int>( forestosity );
    }
    // make sure not to get too extreme on the spacing if you go way far.
    op_city_spacing = std::min( op_city_spacing, 10 );

    // spacing dictates how much of the map is covered in cities
    //   city  |  cities  |   size N cities per overmap
    // spacing | % of map |  2  |  4  |  8  |  12 |  16
    //     0   |   ~99    |2025 | 506 | 126 |  56 |  31
    //     1   |    50    |1012 | 253 |  63 |  28 |  15
    //     2   |    25    | 506 | 126 |  31 |  14 |   7
    //     3   |    12    | 253 |  63 |  15 |   7 |   3
    //     4   |     6    | 126 |  31 |   7 |   3 |   1
    //     5   |     3    |  63 |  15 |   3 |   1 |   0
    //     6   |     1    |  31 |   7 |   1 |   0 |   0
    //     7   |     0    |  15 |   3 |   0 |   0 |   0
    //     8   |     0    |   7 |   1 |   0 |   0 |   0

    const double omts_per_overmap = OMAPX * OMAPY;
    const double city_map_coverage_ratio = 1.0 / std::pow( 2.0, op_city_spacing );
    const double omts_per_city = ( op_city_size * 2 + 1 ) * ( max_city_size * 2 + 1 ) * 3 / 4.0;


    const bool use_random_cities = city::get_all().empty();

    int num_cities_on_this_overmap = 0;
    std::vector<city> cities_to_place;

    if( !use_random_cities ) {
        // how many predetermined cities on this overmap?
        for( const city &c : city::get_all() ) {
            if( c.pos_om == pos() ) {
                num_cities_on_this_overmap++;
                cities_to_place.emplace_back( c );
            }
        }
    } else {
        // Random cities if no cities were defined in regional settings
        num_cities_on_this_overmap = roll_remainder( omts_per_overmap * city_map_coverage_ratio /
                                     omts_per_city );
    }

    tripoint_range city_candidates_range = points_in_radius_where(
            tripoint_om_omt( OMAPX / 2, OMAPY / 2, 0 ),
    OMAPX / 2 - max_city_size, [&]( const tripoint_om_omt & p ) {
        return ter( p ) == settings->default_oter[OVERMAP_DEPTH];
    } );
    std::vector<tripoint_om_omt> city_candidates( city_candidates_range.begin(),
            city_candidates_range.end() );

    const size_t num_cities_on_this_overmap_count = num_cities_on_this_overmap;

    // place a seed for num_cities_on_this_overmap cities, and maybe one more
    while( cities.size() < num_cities_on_this_overmap_count && !city_candidates.empty() ) {

        tripoint_om_omt selected_point;
        city tmp;
        tmp.pos_om = pos();
        if( use_random_cities ) {
            // randomly make some cities smaller or larger
            int size = rng( op_city_size - 1, max_city_size );
            if( one_in( 3 ) ) { // 33% tiny
                size = size * 1 / 3;
            } else if( one_in( 2 ) ) { // 33% small
                size = size * 2 / 3;
            } else if( one_in( 2 ) ) { // 17% large
                size = size * 3 / 2;
            } else {           // 17% huge
                size = size * 2;
            }
            // Ensure that cities are at least size 2, as city of size 1 is just a crossroad with no buildings at all
            size = std::max( size, 2 );
            size = std::min( size, 55 );
            // TODO: put cities closer to the edge when they can span overmaps
            // don't draw cities across the edge of the map, they will get clipped
            selected_point = random_entry_removed( city_candidates );
            // remove candidates within 2 tiles of selection
            for( const tripoint_om_omt &remove_radius : points_in_radius( selected_point, 2 ) ) {
                std::vector<tripoint_om_omt>::iterator removed_point =
                    std::find( city_candidates.begin(), city_candidates.end(), remove_radius );
                if( removed_point != city_candidates.end() ) {
                    city_candidates.erase( removed_point );
                }
            }

            ter_set( selected_point, oter_road_nesw ); // every city starts with an intersection
            city_tiles.insert( selected_point.xy() );
            tmp.pos = selected_point.xy();
            tmp.size = size;

        } else {
            tmp = random_entry( cities_to_place );
            selected_point = tripoint_om_omt( tmp.pos, 0 );
            ter_set( tripoint_om_omt( tmp.pos, 0 ), oter_road_nesw );
            city_tiles.insert( tmp.pos );
        }

        cities.push_back( tmp );
    }
}

void overmap::build_cities()
{

    const overmap_connection_id &overmap_connection_intra_city_road =
        settings->overmap_connection.intra_city_road_connection;
    const overmap_connection &local_road( *overmap_connection_intra_city_road );

    for( const city &c : cities ) {
        const om_direction::type start_dir = om_direction::random();
        om_direction::type cur_dir = start_dir;

        // Track placed CITY_UNIQUE buildings
        std::unordered_set<overmap_special_id> placed_unique_buildings;
        // place streets in all cardinal directions from central intersection
        do {
            build_city_street( local_road, c.pos, c.size, cur_dir, c, placed_unique_buildings );
        } while( ( cur_dir = om_direction::turn_right( cur_dir ) ) != start_dir );
    }
    flood_fill_city_tiles();
}

overmap_special_id overmap::pick_random_building_to_place( int town_dist, int town_size,
        const std::unordered_set<overmap_special_id> &placed_unique_buildings ) const
{
    const region_settings_city &city_spec = settings->get_settings_city();
    int shop_radius = city_spec.shop_radius;
    int park_radius = city_spec.park_radius;

    int shop_sigma = city_spec.shop_sigma;
    int park_sigma = city_spec.park_sigma;

    //Normally distribute shops and parks
    //Clamp at 1/2 radius to prevent houses from spawning in the city center.
    //Parks are nearly guaranteed to have a non-zero chance of spawning anywhere in the city.
    int shop_normal = shop_radius;
    if( shop_sigma > 0 ) {
        shop_normal = std::max( shop_normal, static_cast<int>( normal_roll( shop_radius, shop_sigma ) ) );
    }
    int park_normal = park_radius;
    if( park_sigma > 0 ) {
        park_normal = std::max( park_normal, static_cast<int>( normal_roll( park_radius, park_sigma ) ) );
    }
    auto building_type_to_pick = [&]() {
        if( shop_normal > town_dist ) {
            return std::mem_fn( &region_settings_city::pick_shop );
        } else if( park_normal > town_dist ) {
            return std::mem_fn( &region_settings_city::pick_park );
        } else {
            return std::mem_fn( &region_settings_city::pick_house );
        }
    };
    auto pick_building = building_type_to_pick();
    overmap_special_id ret;
    bool existing_unique;
    do {
        ret = pick_building( city_spec );
        if( ret->has_flag( "CITY_UNIQUE" ) ) {
            existing_unique = placed_unique_buildings.find( ret ) != placed_unique_buildings.end();
        } else if( ret->has_flag( "GLOBALLY_UNIQUE" ) || ret->has_flag( "OVERMAP_UNIQUE" ) ) {
            existing_unique = overmap_buffer.contains_unique_special( ret );
        } else {
            existing_unique = false;
        }
    } while( existing_unique || !ret->get_constraints().city_size.contains( town_size ) );
    return ret;
}

void overmap::place_building( const tripoint_om_omt &p, om_direction::type dir, const city &town,
                              std::unordered_set<overmap_special_id> &placed_unique_buildings )
{
    const tripoint_om_omt building_pos = p + om_direction::displace( dir );
    const om_direction::type building_dir = om_direction::opposite( dir );

    const int town_dist = ( trig_dist( building_pos.xy(), town.pos ) * 100 ) / std::max( town.size, 1 );

    for( size_t retries = 10; retries > 0; --retries ) {
        const overmap_special_id building_tid = pick_random_building_to_place( town_dist, town.size,
                                                placed_unique_buildings );
        if( can_place_special( *building_tid, building_pos, building_dir, false ) ) {
            std::vector<tripoint_om_omt> used_tripoints = place_special( *building_tid, building_pos,
                    building_dir, town, false, false );
            for( const tripoint_om_omt &p : used_tripoints ) {
                city_tiles.insert( p.xy() );
            }
            if( building_tid->has_flag( "CITY_UNIQUE" ) ) {
                placed_unique_buildings.emplace( building_tid );
            }
            break;
        }
    }
}

pf::directed_path<point_om_omt> overmap::lay_out_street( const overmap_connection &connection,
        const point_om_omt &source, om_direction::type dir, size_t len )
{
    const int &highway_width = settings->overmap_highway ?
                               settings->get_settings_highway().width_of_segments : 0;
    auto valid_placement = [this]( const overmap_connection & connection, const tripoint_om_omt pos,
    om_direction::type dir ) {
        if( !inbounds( pos, 1 ) ) {
            return false;  // Don't approach overmap bounds.
        }
        const oter_id &ter_id = ter( pos );
        // TODO: Make it so the city picks a bridge direction ( ns or ew ) and allows bridging over rivers in that direction with the same logic as highways
        if( ter_id->is_river() || ter_id->is_ravine() || ter_id->is_ravine_edge() ||
            ter_id->is_highway() || ter_id->is_highway_reserved() || !connection.pick_subtype_for( ter_id ) ) {
            return false;
        }
        int collisions = 0;

        for( const tripoint_om_omt &checkp : points_in_radius( pos, 1 ) ) {
            if( checkp != pos + om_direction::displace( dir, 1 ) &&
                checkp != pos + om_direction::displace( om_direction::opposite( dir ), 1 ) &&
                checkp != pos ) {
                if( ter( checkp )->get_type_id() == oter_type_road ) {
                    //Stop roads from running right next to each other
                    if( collisions >= 2 ) {
                        return false;
                    }
                    collisions++;
                }
            }
        }

        return true;
    };

    const tripoint_om_omt from( source, 0 );
    // See if we need to take another one-tile "step" further.
    const tripoint_om_omt en_pos = from + om_direction::displace( dir, len + 1 );
    if( inbounds( en_pos, 1 ) && connection.has( ter( en_pos ) ) ) {
        ++len;
    }
    size_t actual_len = 0;
    bool checked_highway = false;

    while( actual_len < len ) {
        const tripoint_om_omt pos = from + om_direction::displace( dir, actual_len );
        if( !valid_placement( connection, pos, dir ) ) {
            break;
        }
        const oter_id &ter_id = ter( pos );
        if( ter_id->is_highway_reserved() ) {
            if( !checked_highway ) {
                // Break if parallel to the highway direction
                if( are_parallel( dir, ter_id.obj().get_dir() ) ) {
                    break;
                }
                const tripoint_om_omt pos_after_highway = pos + om_direction::displace( dir, highway_width );
                // Ensure we can pass fully through
                if( !valid_placement( connection, pos_after_highway, dir ) ) {
                    break;
                }
                checked_highway = true;
            }
            // Prevent stopping under highway
            if( actual_len == len - 1 ) {
                ++len;
            }
        }

        city_tiles.insert( pos.xy() );
        ++actual_len;
        if( actual_len > 1 && connection.has( ter_id ) ) {
            break;  // Stop here.
        }
    }
    return straight_path( source, dir, actual_len );
}

void overmap::build_city_street(
    const overmap_connection &connection, const point_om_omt &p, int cs, om_direction::type dir,
    const city &town, std::unordered_set<overmap_special_id> &placed_unique_buildings, int block_width )
{
    int c = cs;
    int croad = cs;

    if( dir == om_direction::type::invalid ) {
        debugmsg( "Invalid road direction." );
        return;
    }
    const pf::directed_path<point_om_omt> street_path = lay_out_street( connection, p, dir, cs + 1 );

    if( street_path.nodes.size() <= 1 ) {
        return; // Don't bother.
    }
    // Build the actual street.
    build_connection( connection, street_path, 0 );
    // Grow in the stated direction, sprouting off sub-roads and placing buildings as we go.
    const auto from = std::next( street_path.nodes.begin() );
    const auto to = street_path.nodes.end();

    //Alternate wide and thin blocks
    int new_width = block_width == 2 ? rng( 3, 5 ) : 2;

    for( auto iter = from; iter != to; ++iter ) {
        --c;

        const tripoint_om_omt rp( iter->pos, 0 );
        if( c >= 2 && c < croad - block_width ) {
            croad = c;
            int left = cs - rng( 1, 3 );
            int right = cs - rng( 1, 3 );

            //Remove 1 length road nubs
            if( left == 1 ) {
                left++;
            }
            if( right == 1 ) {
                right++;
            }

            build_city_street( connection, iter->pos, left, om_direction::turn_left( dir ),
                               town, placed_unique_buildings, new_width );

            build_city_street( connection, iter->pos, right, om_direction::turn_right( dir ),
                               town, placed_unique_buildings, new_width );

            const oter_id &oter = ter( rp );
            // TODO: Get rid of the hardcoded terrain ids.
            if( one_in( 2 ) && oter->get_line() == 15 && oter->type_is( oter_type_id( "road" ) ) ) {
                ter_set( rp, oter_road_nesw_manhole.id() );
            }
        }

        if( !one_in( BUILDINGCHANCE ) ) {
            place_building( rp, om_direction::turn_left( dir ), town, placed_unique_buildings );
        }
        if( !one_in( BUILDINGCHANCE ) ) {
            place_building( rp, om_direction::turn_right( dir ), town, placed_unique_buildings );
        }
    }

    // If we're big, make a right turn at the edge of town.
    // Seems to make little neighborhoods.
    cs -= rng( 1, 3 );

    if( cs >= 2 && c == 0 ) {
        const auto &last_node = street_path.nodes.back();
        const om_direction::type rnd_dir = om_direction::turn_random( dir );
        build_city_street( connection, last_node.pos, cs, rnd_dir, town, placed_unique_buildings );
        if( one_in( 5 ) ) {
            build_city_street( connection, last_node.pos, cs, om_direction::opposite( rnd_dir ),
                               town, placed_unique_buildings, new_width );
        }
    }
}

void overmap::clear_cities()
{
    cities.clear();
}


bool overmap::is_in_city( const tripoint_om_omt &p ) const
{
    if( !city_tiles.empty() ) {
        return city_tiles.find( p.xy() ) != city_tiles.end();
    } else {
        // Legacy handling
        return distance_to_city( p ) == 0;
    }
}

std::optional<int> overmap::distance_to_city( const tripoint_om_omt &p,
        int max_dist_to_check ) const
{
    if( !city_tiles.empty() ) {
        for( int i = 0; i <= max_dist_to_check; i++ ) {
            for( const tripoint_om_omt &tile : closest_points_first( p, i, i ) ) {
                if( is_in_city( tile ) ) {
                    return i;
                }
            }
        }
    } else {
        // Legacy handling
        const city &nearest_city = get_nearest_city( p );
        if( !!nearest_city ) {
            // 0 if within city
            return std::max( 0, nearest_city.get_distance_from( p ) - nearest_city.size );
        }
    }
    return {};
}

std::optional<int> overmap::approx_distance_to_city( const tripoint_om_omt &p,
        int max_dist_to_check ) const
{
    std::optional<int> ret;
    for( const city &elem : cities ) {
        const int dist = elem.get_distance_from( p );
        if( dist == 0 ) {
            return 0;
        }
        if( dist <= max_dist_to_check ) {
            ret = ret.has_value() ? std::min( ret.value(), dist ) : dist;
        }
    }
    return ret;
}

void overmap::flood_fill_city_tiles()
{
    std::unordered_set<point_om_omt> visited;
    // simplifies bounds checking
    const half_open_rectangle<point_om_omt> omap_bounds( point_om_omt( 0, 0 ), point_om_omt( OMAPX,
            OMAPY ) );

    // Look through every point on the overmap
    for( int y = 0; y < OMAPY; y++ ) {
        for( int x = 0; x < OMAPX; x++ ) {
            point_om_omt checked( x, y );
            // If we already looked at it in a previous flood-fill, ignore it
            if( visited.find( checked ) != visited.end() ) {
                continue;
            }
            // Is the area connected to this point enclosed by city_tiles?
            bool enclosed = true;
            // Predicate for flood-fill. Also detects if any point flood-filled to borders the edge
            // of the overmap and is thus not enclosed
            const auto is_unchecked = [&enclosed, &omap_bounds, this]( const point_om_omt & pt ) {
                if( city_tiles.find( pt ) != city_tiles.end() ) {
                    return false;
                }
                // We hit the edge of the overmap! We're free!
                if( !omap_bounds.contains( pt ) ) {
                    enclosed = false;
                    return false;
                }
                return true;
            };
            // All the points connected to this point that aren't part of a city
            std::vector<point_om_omt> area =
                ff::point_flood_fill_4_connected<std::vector>( checked, visited, is_unchecked );
            if( !enclosed ) {
                continue;
            }
            // They are enclosed, and so should be considered part of the city.
            city_tiles.reserve( city_tiles.size() + area.size() );
            for( const point_om_omt &pt : area ) {
                city_tiles.insert( pt );
            }
        }
    }
}
