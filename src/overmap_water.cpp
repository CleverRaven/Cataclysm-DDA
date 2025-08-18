#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "coordinates.h"
#include "debug.h"
#include "enums.h"
#include "flood_fill.h"
#include "game.h"
#include "line.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "messages.h"
#include "omdata.h"
#include "overmap.h"
#include "overmap_noise.h"
#include "overmapbuffer.h"
#include "point.h"
#include "regional_settings.h"
#include "rng.h"
#include "type_id.h"

static const oter_str_id oter_forest_water( "forest_water" );
static const oter_str_id oter_river_c_not_ne( "river_c_not_ne" );
static const oter_str_id oter_river_c_not_nw( "river_c_not_nw" );
static const oter_str_id oter_river_c_not_se( "river_c_not_se" );
static const oter_str_id oter_river_c_not_sw( "river_c_not_sw" );
static const oter_str_id oter_river_center( "river_center" );
static const oter_str_id oter_river_east( "river_east" );
static const oter_str_id oter_river_ne( "river_ne" );
static const oter_str_id oter_river_north( "river_north" );
static const oter_str_id oter_river_nw( "river_nw" );
static const oter_str_id oter_river_se( "river_se" );
static const oter_str_id oter_river_south( "river_south" );
static const oter_str_id oter_river_sw( "river_sw" );
static const oter_str_id oter_river_west( "river_west" );

void overmap::place_river( const std::vector<const overmap *> &neighbor_overmaps,
                           const overmap_river_node &initial_points, int river_scale, bool major_river )
{
    const int OMAPX_edge = OMAPX - 1;
    const int OMAPY_edge = OMAPY - 1;
    const int directions = 4;
    // TODO: Retain river_scale for the river node and allow branches to extend overmaps?
    if( river_scale <= 0 ) {
        return;
    }

    std::vector <std::vector<tripoint_om_omt>> border_points_by_dir;
    for( int i = 0; i < directions; i++ ) {
        const point_rel_om &p = point_rel_om( four_adjacent_offsets[i] );
        std::vector<tripoint_om_omt> border_points;
        if( neighbor_overmaps[i] == nullptr ) {
            border_points = get_border( p, RIVER_Z, RIVER_BORDER );
        }
        border_points_by_dir.emplace_back( border_points );
    }

    // Generate control points for Bezier curve
    // the start/end of the curve are the start/end of the river
    // the remaining control points are:
    // - one-third between river start and end
    // - two-thirds between river start and end

    const point_om_omt &river_start = initial_points.river_start;
    point_om_omt river_end = initial_points.river_end;
    const int distance = rl_dist( river_start, river_end );
    // variance of control points from the initial curve
    const int amplitude = distance / 2;
    point_rel_omt one_third(
        abs( river_start.x() - river_end.x() ) * ( 1.0 / 3.0 ),
        abs( river_start.y() - river_end.y() ) * ( 1.0 / 3.0 ) );
    const int river_z = 0;

    point_om_omt control_p1 = initial_points.control_p1;
    if( control_p1.is_invalid() ) {
        point_om_omt one_third_point = point_om_omt( river_start.x() + one_third.x(),
                                       river_start.y() + one_third.y() );
        control_p1 = point_om_omt( std::clamp( one_third_point.x() + rng( amplitude, 0 ), 0, OMAPX_edge ),
                                   std::clamp( one_third_point.y() + rng( amplitude, 0 ), 0, OMAPY_edge ) );
    }
    point_om_omt control_p2 = initial_points.control_p2;
    if( control_p2.is_invalid() ) {
        point_om_omt two_third_point = point_om_omt( river_start.x() + one_third.x() * 2,
                                       river_start.y() + one_third.y() * 2 );
        control_p2 = point_om_omt(
                         std::clamp( two_third_point.x() + rng( 0, -amplitude ), 0, OMAPX_edge ),
                         std::clamp( two_third_point.y() + rng( 0, -amplitude ), 0, OMAPY_edge ) );
    }
    const int n_segs = distance / 2; // Number of Bezier curve segments to calculate
    //there should be at least four points on the curve
    if( n_segs < 4 ) {
        return;
    }
    //Note: this is a list of *segments*; the points in this list are not always adjacent!
    std::vector<point_om_omt> segmented_curve = cubic_bezier( river_start, control_p1,
            control_p2, river_end, n_segs );
    //remove index-adjacent duplicate points
    auto iter = std::unique( segmented_curve.begin(), segmented_curve.end() );
    segmented_curve.erase( iter, segmented_curve.end() );

    //line_to only draws interval (p1, p2], so add the start again
    segmented_curve.emplace( segmented_curve.begin(), river_start );

    std::vector<point_om_omt> bezier_segment; // a complete Bezier segment (all points are adjacent)
    size_t size = 0;
    int curve_size = segmented_curve.size() - 1;
    int river_check_index = 0;

    for( river_check_index = 0; river_check_index < curve_size / 3; river_check_index++ ) {
        const point_om_omt &segment_start = segmented_curve.at( river_check_index );
        if( !is_water_body_not_shore( ter( tripoint_om_omt( segment_start.x(), segment_start.y(),
                                           river_z ) ) ) ) {
            break;
        }
    }

    //if the first third of the river is already water, abort
    if( river_check_index == curve_size / 3 ) {
        return;
    }

    //if any remaining points are water, stop the river at that point
    for( int i = river_check_index; i < curve_size; i++ ) {
        const point_om_omt segment_start = segmented_curve.at( i );
        tripoint_om_omt segment_start_tri( segment_start.x(), segment_start.y(), river_z );
        const oter_id &ter_here = ter( segment_start_tri );
        if( is_water_body_not_shore( ter_here ) ) {
            river_end = segment_start;
            curve_size = i;
            break;
        }
    }

    //draw the river by placing river OMTs between curve points
    for( int i = 0; i < curve_size; i++ ) {
        bezier_segment.clear();
        bezier_segment = line_to( segmented_curve.at( i ), segmented_curve.at( i + 1 ), 0 );
        // Now, draw the actual river tiles along the segment
        for( const point_om_omt &bezier_point : bezier_segment ) {

            tripoint_om_omt meandered_point( bezier_point, RIVER_Z );
            // no meander for start/end
            if( !( i == 0 || i == curve_size - 1 ) ) {
                river_meander( river_end, meandered_point, river_scale );
            }

            auto draw_river = [&size, this]( const tripoint_om_omt & pt ) {
                size++;
                ter_set( pt, oter_river_center );
            };
            // Draw river in radius [-river_scale, +river_scale]
            for( const tripoint_om_omt &pt : points_in_radius_circ( meandered_point, river_scale ) ) {
                // river points on the edge of the overmap are automatically drawn from neighbor,
                // but only if that neighbor doesn't exist
                if( !is_water_body_not_shore( ter( pt ) ) ) {
                    if( inbounds( pt, 1 ) ) {
                        draw_river( pt );
                    } else if( inbounds( pt ) ) {
                        for( int i = 0; i < directions; i++ ) {
                            if( !border_points_by_dir[i].empty() &&
                                std::find( border_points_by_dir[i].begin(), border_points_by_dir[i].end(),
                                           pt ) != border_points_by_dir[i].end() ) {
                                draw_river( pt );
                            }
                        }
                    }
                }
            }
        }
    }

    // create river branches
    const int branch_ahead_points = std::max( 2, curve_size / 5 );
    int branch_last_end = 0;
    for( int i = 0; i < curve_size; i++ ) {
        const point_om_omt &bezier_point = segmented_curve.at( i );
        if( inbounds( bezier_point, river_scale + 1 ) &&
            one_in( settings->overmap_river.river_branch_chance ) ) {
            point_om_omt branch_end_point = point_om_omt::invalid;

            //pick an end point from later along the curve
            //TODO: make remerge branches have control points that aren't straight;
            // remerges are less common because they get stopped by the already-generated river
            if( i > branch_last_end ) {
                if( one_in( settings->overmap_river.river_branch_remerge_chance ) ) {
                    int end_branch_node = rng( i + branch_ahead_points, i + branch_ahead_points * 2 );
                    if( end_branch_node < curve_size ) {
                        branch_end_point = segmented_curve.at( end_branch_node );
                        branch_last_end = end_branch_node;
                    }
                } else {
                    //or just randomly from the current segment
                    const int rad = 64;
                    branch_end_point = point_om_omt(
                                           rng( bezier_point.x() + rad / 2, bezier_point.x() + rad ),
                                           rng( bezier_point.y() + rad / 2, bezier_point.y() + rad ) );
                }
            }
            // if the end point is valid, place a new, smaller, overmap-local branch river
            if( !branch_end_point.is_invalid() && inbounds( branch_end_point ) ) {
                place_river( neighbor_overmaps, overmap_river_node{ bezier_point, branch_end_point },
                             river_scale - settings->overmap_river.river_branch_scale_decrease );
            }
        }
    }

    add_msg_debug( debugmode::DF_OVERMAP,
                   "for overmap %s, drew river at: start = %s, control points: %s, %s, end = %s",
                   loc.to_string_writable(), river_start.to_string_writable(),
                   control_p1.to_string_writable(), control_p2.to_string_writable(),
                   river_end.to_string_writable() );

    overmap_river_node new_node = { river_start, river_end, control_p1, control_p2, size, major_river };
    rivers.push_back( new_node );
}

void overmap::river_meander( const point_om_omt &river_end, tripoint_om_omt &current_point,
                             int river_scale )
{
    const auto random_uniform = []( int i ) -> bool {
        return rng( 0, static_cast<int>( OMAPX * 1.2 ) - 1 ) < i;
    };
    const auto random_close = []( int i ) -> bool {
        return rng( 0, static_cast<int>( OMAPX * 0.2 ) - 1 ) > i;
    };

    point_rel_omt abs_distances( abs( river_end.x() - current_point.x() ),
                                 abs( river_end.y() - current_point.y() ) );

    //as distance to river end decreases, meander closer to the river end
    if( current_point.x() != river_end.x() && ( random_uniform( abs_distances.x() ) ||
            ( random_close( abs_distances.x() ) &&
              random_close( abs_distances.y() ) ) ) ) {
        current_point += river_end.x() > current_point.x() ? tripoint::east : tripoint::west;
    }
    if( current_point.y() != river_end.y() && ( random_uniform( abs_distances.y() ) ||
            ( random_close( abs_distances.y() ) &&
              random_close( abs_distances.x() ) ) ) ) {
        current_point += river_end.y() > current_point.y() ? tripoint::south : tripoint::north;
    }
    //meander randomly, but not for rivers of size 1 (would exceed above meander)
    if( river_scale > 1 ) {
        current_point += tripoint_rel_omt( rng( -1, 1 ), rng( -1, 1 ), 0 );
    }
}


void overmap::place_lakes( const std::vector<const overmap *> &neighbor_overmaps )
{
    const point_abs_omt origin = global_base_point();
    const om_noise::om_noise_layer_lake noise_func( origin, g->get_seed() );
    double noise_threshold = settings->overmap_lake.noise_threshold_lake;

    const auto is_lake = [&]( const point_om_omt & p ) {
        return omt_lake_noise_threshold( origin, p, noise_threshold );
    };

    const oter_id lake_surface( "lake_surface" );
    const oter_id lake_shore( "lake_shore" );
    const oter_id lake_water_cube( "lake_water_cube" );
    const oter_id lake_bed( "lake_bed" );

    // We'll keep track of our visited lake points so we don't repeat the work.
    std::unordered_set<point_om_omt> visited;

    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            point_om_omt seed_point( i, j );
            if( visited.find( seed_point ) != visited.end() ) {
                continue;
            }

            // It's a lake if it exceeds the noise threshold defined in the region settings.
            if( !omt_lake_noise_threshold( origin, seed_point, noise_threshold ) ) {
                continue;
            }

            // We're going to flood-fill our lake so that we can consider the entire lake when evaluating it
            // for placement, even when the lake runs off the edge of the current overmap.
            std::vector<point_om_omt> lake_points =
                ff::point_flood_fill_4_connected( seed_point, visited, is_lake );

            // If this lake doesn't exceed our minimum size threshold, then skip it. We can use this to
            // exclude the tiny lakes that don't provide interesting map features and exist mostly as a
            // noise artifact.
            if( lake_points.size() < static_cast<size_t>
                ( settings->overmap_lake.lake_size_min ) ) {
                continue;
            }

            // Build a set of "lake" points. We're actually going to combine both the lake points
            // we just found AND all of the rivers on the map, because we want our lakes to write
            // over any rivers that are placed already. Note that the assumption here is that river
            // overmap generation (e.g. place_rivers) runs BEFORE lake overmap generation.
            std::unordered_set<point_om_omt> lake_set;
            for( auto &p : lake_points ) {
                lake_set.emplace( p );
            }

            // Before we place the lake terrain and get river points, generate a river to somewhere
            // in the lake from an existing river.
            if( !rivers.empty() ) {
                const tripoint_om_omt random_lake_point( random_entry( lake_set ), 0 );
                point_om_omt river_connection = point_om_omt::invalid;
                for( const tripoint_om_omt &find_river : closest_points_first( random_lake_point, OMAPX / 2 ) ) {
                    if( inbounds( find_river ) && ter( find_river )->is_river() ) {
                        river_connection = find_river.xy();
                        break;
                    }
                }
                if( !river_connection.is_invalid() ) {
                    place_river( neighbor_overmaps, overmap_river_node{ river_connection, random_lake_point.xy() } );
                }
            }

            for( int x = 0; x < OMAPX; x++ ) {
                for( int y = 0; y < OMAPY; y++ ) {
                    const tripoint_om_omt p( x, y, 0 );
                    if( ter( p )->is_river() ) {
                        lake_set.emplace( p.xy() );
                    }
                }
            }

            // Iterate through all of our lake points, rejecting the ones that are out of bounds. For
            // those that are inbounds, look at the 8 adjacent locations and see if they are also part
            // of our lake points set. If they are, that means that this location is entirely surrounded
            // by lake and should be considered a lake surface. If at least one adjacent location is not
            // part of this lake points set, that means this location should be considered a lake shore.
            // Either way, make the determination and set the overmap terrain.
            for( auto &p : lake_points ) {
                if( !inbounds( p ) ) {
                    continue;
                }

                bool shore = false;
                for( int ni = -1; ni <= 1 && !shore; ni++ ) {
                    for( int nj = -1; nj <= 1 && !shore; nj++ ) {
                        const point_om_omt n = p + point( ni, nj );
                        if( lake_set.find( n ) == lake_set.end() ) {
                            shore = true;
                        }
                    }
                }

                ter_set( tripoint_om_omt( p, 0 ), shore ? lake_shore : lake_surface );

                // If this is not a shore, we'll make our subsurface lake cubes and beds.
                if( !shore ) {
                    for( int z = -1; z > settings->overmap_lake.lake_depth; z-- ) {
                        ter_set( tripoint_om_omt( p, z ), lake_water_cube );
                    }
                    ter_set( tripoint_om_omt( p, settings->overmap_lake.lake_depth ), lake_bed );
                }
            }
        }
    }
}

// helper function for code deduplication, as it is needed multiple times
float overmap::calculate_ocean_gradient( const point_om_omt &p, const point_abs_om this_om )
{
    const int northern_ocean = settings->overmap_ocean.ocean_start_north;
    const int eastern_ocean = settings->overmap_ocean.ocean_start_east;
    const int western_ocean = settings->overmap_ocean.ocean_start_west;
    const int southern_ocean = settings->overmap_ocean.ocean_start_south;

    float ocean_adjust_N = 0.0f;
    float ocean_adjust_E = 0.0f;
    float ocean_adjust_W = 0.0f;
    float ocean_adjust_S = 0.0f;
    if( northern_ocean > 0 && this_om.y() <= northern_ocean * -1 ) {
        ocean_adjust_N = 0.0005f * static_cast<float>( OMAPY - p.y()
                         + std::abs( ( this_om.y() + northern_ocean ) * OMAPY ) );
    }
    if( eastern_ocean > 0 && this_om.x() >= eastern_ocean ) {
        ocean_adjust_E = 0.0005f * static_cast<float>( p.x() + ( this_om.x() - eastern_ocean )
                         * OMAPX );
    }
    if( western_ocean > 0 && this_om.x() <= western_ocean * -1 ) {
        ocean_adjust_W = 0.0005f * static_cast<float>( OMAPX - p.x()
                         + std::abs( ( this_om.x() + western_ocean ) * OMAPX ) );
    }
    if( southern_ocean > 0 && this_om.y() >= southern_ocean ) {
        ocean_adjust_S = 0.0005f * static_cast<float>( p.y() + ( this_om.y() - southern_ocean ) * OMAPY );
    }
    return std::max( { ocean_adjust_N, ocean_adjust_E, ocean_adjust_W, ocean_adjust_S } );
}

void overmap::place_oceans( const std::vector<const overmap *> &neighbor_overmaps )
{
    int northern_ocean = settings->overmap_ocean.ocean_start_north;
    int eastern_ocean = settings->overmap_ocean.ocean_start_east;
    int western_ocean = settings->overmap_ocean.ocean_start_west;
    int southern_ocean = settings->overmap_ocean.ocean_start_south;

    const om_noise::om_noise_layer_ocean f( global_base_point(), g->get_seed() );
    const point_abs_om this_om = pos();

    const auto is_ocean = [&]( const point_om_omt & p ) {
        // credit to ehughsbaird for thinking up this inbounds solution to infinite flood fill lag.
        if( northern_ocean == 0 && eastern_ocean == 0 && western_ocean == 0 && southern_ocean == 0 ) {
            // you know you could just turn oceans off in global_settings.json right?
            return false;
        }
        bool inbounds = p.x() > -5 && p.y() > -5 && p.x() < OMAPX + 5 && p.y() < OMAPY + 5;
        if( !inbounds ) {
            return false;
        }
        float ocean_adjust = calculate_ocean_gradient( p, this_om );
        if( ocean_adjust == 0.0f ) {
            // It's too soon!  Too soon for an ocean!!  ABORT!!!
            return false;
        }
        return f.noise_at( p ) + ocean_adjust > settings->overmap_ocean.noise_threshold_ocean;
    };

    const oter_id ocean_surface( "ocean_surface" );
    const oter_id ocean_shore( "ocean_shore" );
    const oter_id ocean_water_cube( "ocean_water_cube" );
    const oter_id ocean_bed( "ocean_bed" );

    // This code is repeated from is_lake(), see comments there for explanation.
    std::unordered_set<point_om_omt> visited;

    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            point_om_omt seed_point( i, j );
            if( visited.find( seed_point ) != visited.end() ) {
                continue;
            }

            if( !is_ocean( seed_point ) ) {
                continue;
            }

            std::vector<point_om_omt> ocean_points =
                ff::point_flood_fill_4_connected( seed_point, visited, is_ocean );

            // Ocean size is checked like lake size, but minimum size is much bigger.
            // you could change this, if you want little tiny oceans all over the place.
            // I'm not sure why you'd want that.  Use place_lakes, my friend.
            if( ocean_points.size() < static_cast<size_t>
                ( settings->overmap_ocean.ocean_size_min ) ) {
                continue;
            }

            std::unordered_set<point_om_omt> ocean_set;
            for( auto &p : ocean_points ) {
                ocean_set.emplace( p );
            }

            for( int x = 0; x < OMAPX; x++ ) {
                for( int y = 0; y < OMAPY; y++ ) {
                    const tripoint_om_omt p( x, y, 0 );
                    if( ter( p )->is_river() ) {
                        ocean_set.emplace( p.xy() );
                    }
                }
            }

            for( auto &p : ocean_points ) {
                if( !inbounds( p ) ) {
                    continue;
                }

                bool shore = false;
                for( int ni = -1; ni <= 1 && !shore; ni++ ) {
                    for( int nj = -1; nj <= 1 && !shore; nj++ ) {
                        const point_om_omt n = p + point( ni, nj );
                        if( ocean_set.find( n ) == ocean_set.end() ) {
                            shore = true;
                        }
                    }
                }

                ter_set( tripoint_om_omt( p, 0 ), shore ? ocean_shore : ocean_surface );

                if( !shore ) {
                    for( int z = -1; z > settings->overmap_ocean.ocean_depth; z-- ) {
                        ter_set( tripoint_om_omt( p, z ), ocean_water_cube );
                    }
                    ter_set( tripoint_om_omt( p, settings->overmap_ocean.ocean_depth ), ocean_bed );
                }
            }

            // We're going to attempt to connect some points to the nearest river.
            // This isn't a lake but the code is the same, we can reuse it.  Water is water.
            const auto connect_lake_to_closest_river =
            [&]( const point_om_omt & lake_connection_point ) {
                int closest_distance = -1;
                point_om_omt closest_point;
                for( int x = 0; x < OMAPX; x++ ) {
                    for( int y = 0; y < OMAPY; y++ ) {
                        const tripoint_om_omt p( x, y, 0 );
                        if( !ter( p )->is_river() ) {
                            continue;
                        }
                        const int distance = square_dist( lake_connection_point, p.xy() );
                        if( distance < closest_distance || closest_distance < 0 ) {
                            closest_point = p.xy();
                            closest_distance = distance;
                        }
                    }
                }

                if( closest_distance > 0 ) {
                    place_river( neighbor_overmaps, overmap_river_node{ closest_point, lake_connection_point } );
                }
            };

            // Get the north and south most points in our ocean.
            auto north_south_most = std::minmax_element( ocean_points.begin(), ocean_points.end(),
            []( const point_om_omt & lhs, const point_om_omt & rhs ) {
                return lhs.y() < rhs.y();
            } );

            point_om_omt northmost = *north_south_most.first;
            point_om_omt southmost = *north_south_most.second;

            // It's possible that our northmost/southmost points in the lake are not on this overmap, because our
            // lake may extend across multiple overmaps.
            if( inbounds( northmost ) ) {
                connect_lake_to_closest_river( northmost );
            }

            if( inbounds( southmost ) ) {
                connect_lake_to_closest_river( southmost );
            }
        }
    }
}

void overmap::place_rivers( const std::vector<const overmap *> &neighbor_overmaps )
{
    const int OMAPX_edge = OMAPX - 1;
    const int OMAPY_edge = OMAPY - 1;
    const int directions = neighbor_overmaps.size();
    const int max_rivers = 2;
    int river_scale = settings->overmap_river.river_scale;
    if( river_scale == 0 ) {
        return;
    }
    river_scale = 1 + std::max( 1, river_scale );
    // North/West endpoints of rivers (2 major rivers max per overmap)
    std::array<point_om_omt, max_rivers> river_start = { point_om_omt::invalid, point_om_omt::invalid };
    // East/South endpoints of rivers
    std::array<point_om_omt, max_rivers> river_end = { point_om_omt::invalid, point_om_omt::invalid };
    // forced river bezier curve control points (closer to start point)
    std::array<point_om_omt, max_rivers> control_start = { point_om_omt::invalid, point_om_omt::invalid };
    // ..closer to end point
    std::array<point_om_omt, max_rivers> control_end = { point_om_omt::invalid, point_om_omt::invalid };

    auto bound_control_point = []( const point_om_omt & p, const point_rel_omt & init_diff ) {
        point_rel_omt diff = init_diff;
        point_om_omt continue_line = p + diff;
        while( !inbounds( continue_line ) ) {
            diff = point_rel_omt( diff.x() * 0.5, diff.y() * 0.5 );
            continue_line = p + diff;
        }
        return continue_line;
    };

    std::array<bool, 4> is_start = { true, false, false, true };
    std::array<bool, 4> node_present;
    for( int i = 0; i < directions; i++ ) {
        node_present[i] = neighbor_overmaps[i] == nullptr;
    }

    // set river starts and ends from neighboring overmaps' existing rivers
    // force control points from adjacent overmap to this overmap to smooth river curve
    int preset_start_nodes = 0;
    int preset_end_nodes = 0;
    std::array<overmap_river_border, 4> river_borders;
    for( int i = 0; i < directions; i++ ) {
        river_borders[i] = setup_adjacent_river( point_rel_om( four_adjacent_offsets[i] ), RIVER_BORDER );
        const overmap_river_border &river_border = river_borders[i];
        if( !river_border.border_river_nodes.empty() ) {
            //take the first node (major river) only
            point_om_omt p = river_border.border_river_nodes_omt.front();
            const overmap_river_node *river_node = river_border.border_river_nodes.front();
            if( is_start[i] ) {
                river_start[preset_start_nodes] = p;
                point_rel_omt diff = river_node->river_end - ( river_node->control_p2.is_invalid() ?
                                     river_node->river_end : river_node->control_p2 );
                control_start[preset_start_nodes] = bound_control_point( p, diff );
            } else {
                river_end[preset_end_nodes] = p;
                point_rel_omt diff = river_node->river_start - ( river_node->control_p1.is_invalid() ?
                                     river_node->river_start : river_node->control_p1 );
                control_end[preset_end_nodes] = bound_control_point( p, diff );
            }

            node_present[i] = true;
            is_start[i] ? preset_start_nodes++ : preset_end_nodes++;
        }
    }

    //if there wasn't an neighboring river, 1 / (river frequency ^ rivers generated) chance to continue
    bool no_neighboring_rivers = preset_start_nodes == 0 && preset_end_nodes == 0;
    if( no_neighboring_rivers ) {
        if( !x_in_y( 1.0, std::pow( settings->overmap_river.river_frequency,
                                    overmap_buffer.get_major_river_count() ) ) ) {
            return;
        }
    }

    // if there are two river start/ends from neighbor overmaps,
    // they must flow N->E, W->S to avoid intersecting
    bool lock_rivers = preset_start_nodes == max_rivers || preset_end_nodes == max_rivers;

    auto generate_new_node_point = [&]( int dir ) {
        switch( dir ) {
            case 0:
                return point_om_omt( rng( RIVER_BORDER, OMAPX_edge - RIVER_BORDER ), 0 );
            case 1:
                return point_om_omt( OMAPX_edge, rng( RIVER_BORDER, OMAPY_edge - RIVER_BORDER ) );
            case 2:
                return point_om_omt( rng( RIVER_BORDER, OMAPX_edge - RIVER_BORDER ), OMAPY_edge );
            case 3:
                return point_om_omt( 0, rng( RIVER_BORDER, OMAPY_edge - RIVER_BORDER ) );
            default:
                return point_om_omt( rng( RIVER_BORDER, OMAPX_edge - RIVER_BORDER ), rng( RIVER_BORDER,
                                     OMAPY_edge - RIVER_BORDER ) );
        }
    };

    // generate brand-new river nodes
    // generate two rivers if locking
    if( lock_rivers ) {
        if( river_start[0].is_invalid() ) {
            river_start[0] = generate_new_node_point( 0 );
        }
        if( river_start[1].is_invalid() ) {
            river_start[1] = generate_new_node_point( 3 );
        }
        if( river_end[0].is_invalid() ) {
            river_end[0] = generate_new_node_point( 1 );
        }
        if( river_end[1].is_invalid() ) {
            river_end[1] = generate_new_node_point( 2 );
        }
    }
    // if no river nodes were present, generate one river based on existing adjacent overmaps
    else {
        if( river_start[0].is_invalid() ) {
            if( node_present[0] && ( !node_present[3] || one_in( 2 ) ) ) {
                river_start[0] = generate_new_node_point( 0 );
            } else if( node_present[3] ) {
                river_start[0] = generate_new_node_point( 3 );
            } else {
                river_start[0] = generate_new_node_point( -1 );
            }
        }
        if( river_end[0].is_invalid() ) {
            if( node_present[2] && ( !node_present[1] || one_in( 2 ) ) ) {
                river_end[0] = generate_new_node_point( 2 );
            } else if( node_present[1] ) {
                river_end[0] = generate_new_node_point( 1 );
            } else {
                river_end[0] = generate_new_node_point( -1 );
            }
        }
    }

    //Finally, place rivers from start point to end point
    for( size_t i = 0; i < max_rivers; i++ ) {
        point_om_omt pa = river_start.at( i );
        point_om_omt pb = river_end.at( i );
        if( !pa.is_invalid() && !pb.is_invalid() ) {
            overmap_river_node temp_node{ pa, pb, control_start.at( i ), control_end.at( i ) };
            place_river( neighbor_overmaps, temp_node, river_scale, true );
            if( no_neighboring_rivers ) {
                overmap_buffer.inc_major_river_count();
            }
        }
    }
}


overmap_river_border overmap::setup_adjacent_river( const point_rel_om &adjacent_om, int border )
{
    overmap_river_border adjacent_border;
    std::vector<point_om_omt> &border_river_omt = adjacent_border.border_river_omt;
    std::vector<point_om_omt> &border_river_nodes_omt = adjacent_border.border_river_nodes_omt;
    std::vector<const overmap_river_node *> &border_river_nodes = adjacent_border.border_river_nodes;
    const overmap *adjacent_overmap = overmap_buffer.get_existing( pos() + adjacent_om );

    //if overmap doesn't exist yet, no need to check it
    if( adjacent_overmap == nullptr ) {
        return adjacent_border;
    }

    const int river_z = 0;
    std::vector<tripoint_om_omt> neighbor_border = get_neighbor_border( adjacent_om, river_z, border );
    std::vector<tripoint_om_omt> overmap_border = get_border( adjacent_om, river_z, border );
    const int neighbor_border_size = neighbor_border.size();

    for( int i = 0; i < neighbor_border_size; i++ ) {
        const tripoint_om_omt &neighbor_pt = neighbor_border[i];
        const tripoint_om_omt &current_pt = overmap_border[i];
        const point_om_omt p_neighbor_point = neighbor_pt.xy();
        const point_om_omt p_current_point = current_pt.xy();

        if( is_river( adjacent_overmap->ter( neighbor_pt ) ) ) {
            //if the neighbor overmap has an adjacent river tile, set to river
            //this guarantees that an out-of-bounds point adjacent to a river is always a river
            ter_set( current_pt, oter_river_center );
            border_river_omt.emplace_back( p_current_point );
            if( adjacent_overmap->is_river_node( p_neighbor_point ) ) {
                border_river_nodes_omt.emplace_back( p_current_point );
                border_river_nodes.emplace_back( adjacent_overmap->get_river_node_at( p_neighbor_point ) );
            }
        }
    }
    return adjacent_border;
}


void overmap::build_river_shores( const std::vector<const overmap *> &neighbor_overmaps,
                                  const tripoint_om_omt &p )
{
    const int neighbor_overmap_size = neighbor_overmaps.size();
    // TODO: Change this to the flag (or an else that sets a different bool?) and add handling for bridge maps to get overwritten by the correct tile then replaced
    if( !is_ot_match( "river", ter( p ), ot_match_type::prefix ) ) {
        return;
    }
    // Find and assign shores where they need to be.
    int mask = 0;
    int multiplier = 1;
    auto mask_add = [&multiplier, &mask]() {
        mask += ( 1 * multiplier );
    };
    const point_om_omt as_point( p.x(), p.y() );
    // TODO: Expand to eight_horizontal_neighbors dealing with corners here instead of afterwards?
    for( int i = 0; i < neighbor_overmap_size; i++ ) {
        const tripoint_om_omt p_offset = p + point_rel_omt( four_adjacent_offsets[i] );

        if( !inbounds( p_offset ) || is_water_body( ter( p_offset ) ) ) {
            //out of bounds OMTs adjacent to border river OMTs are always river OMTs
            mask_add();
        }
        multiplier *= 2;
    }

    auto river_ter = [&]() {
        const std::array<oter_str_id, 16> river_ters = {
            oter_forest_water, // 0 = no adjacent rivers.
            oter_river_south, // 1 = N adjacent river
            oter_river_west, // 2 = E adjacent river
            oter_river_sw, // 3 = 1+2 = N+E adjacent rivers
            oter_river_north, // 4 = S adjacent river
            oter_forest_water, // 5 = 1+4 = N+S adjacent rivers, no map though
            oter_river_nw, // 6 = 2+4 = E+S adjacent rivers
            oter_river_west, // 7 = 1+2+4 = N+E+S adjacent rivers
            oter_river_east, // 8 = W adjacent river
            oter_river_se, // 9 = 1+8 = N+W adjacent rivers
            oter_forest_water, // 10 = 2+8 = E+W adjacent rivers, no map though
            oter_river_south, // 11 = 1+2+8 = N+E+W adjacent rivers
            oter_river_ne, // 12 = 4+8 = S+W adjacent rivers
            oter_river_east, // 13 = 1+4+8 = N+S+W adjacent rivers
            oter_river_north, // 14 = 2+4+8 = E+S+W adjacent rivers
            oter_river_center // 15 = 1+2+4+8 = N+E+S+W adjacent rivers.
        };

        if( mask == 15 ) {
            // Trim corners if neccessary
            // We assume if corner are not inbounds then we are placed because of neighbouring overmap river
            const std::array<oter_str_id, 4> trimmed_corner_ters = { oter_river_c_not_ne, oter_river_c_not_se, oter_river_c_not_sw, oter_river_c_not_nw };
            for( int i = 0; i < 4; i++ ) {
                const tripoint_om_omt &corner = p + point_rel_omt( four_ordinal_directions[i] );
                if( inbounds( corner ) && !is_water_body( ter( corner ) ) ) {
                    return trimmed_corner_ters[i];
                }
            }
        }
        return river_ters[mask];
    };
    ter_set( p, river_ter() );
}
