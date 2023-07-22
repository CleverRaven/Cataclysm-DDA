#include <functional>
#include <new>
#include <optional>
#include <ostream>
#include <utility>
#include <vector>

#include "cached_options.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "map.h"
#include "map_helpers.h"
#include "map_iterator.h"
#include "map_test_case.h"
#include "mapdata.h"
#include "options_helpers.h"
#include "point.h"

using namespace map_test_case_common;
using namespace map_test_case_common::tiles;

static const ter_str_id ter_t_brick_wall( "t_brick_wall" );
static const ter_str_id ter_t_flat_roof( "t_flat_roof" );

static const tile_predicate ter_set_flat_roof_above = ter_set( ter_t_flat_roof, tripoint_above );

static const tile_predicate set_up_tiles_common =
    ifchar( '.', noop ) ||
    ifchar( 'X', noop ) ||
    ifchar( '#', ter_set( ter_t_brick_wall ) + ter_set_flat_roof_above ) ||
    ifchar( '^', ter_set_flat_roof_above ) ||
    fail;

static void test_reachability( std::vector<std::string> setup, bool up )
{
    map_test_case t;
    t.setup = std::move( setup );
    t.expected_results = t.setup;
    t.anchor_char = 'X';
    t.anchor_map_pos = tripoint( 60, 60, 0 );
    t.validate_anchor_point( {60, 60, 0} );
    t.generate_transform_combinations();

    map &here = get_map();
    clear_map();
    t.for_each_tile( set_up_tiles_common );

    here.invalidate_map_cache( 0 );
    here.set_transparency_cache_dirty( 0 );
    here.build_map_cache( 0, true );

    tripoint dst_shift = up ? tripoint_above : tripoint_zero;
    int rejected_cnt = 0;

    for( const tripoint &src : here.points_in_radius( t.anchor_map_pos, 5 ) ) {
        for( const tripoint &dest : here.points_in_radius( t.anchor_map_pos + dst_shift, 5 ) ) {
            bool map_sees = here.sees( src, dest, 12 );
            bool r_cache_sees = here.has_potential_los( src, dest );

            rejected_cnt += !r_cache_sees;

            INFO( "F — from, T — to" );
            INFO( map_test_case_common::printers::format_2d_array(
            t.map_tiles_str( [&]( map_test_case::tile t, std::ostream & s ) {
                s << ( t.p.xy() == src.xy() ? 'F' : t.p.xy() == dest.xy() ? 'T' : t.setup_c );
            } ) ) );
            CAPTURE( src, dest, map_sees, r_cache_sees );
            // when map_sees==true, r_cache_sees should be true,
            // but when !map_sees, r_cache_sees is allowed be false or true
            CHECK( ( !map_sees || r_cache_sees ) );
        }
    }

    // at least some lookups should be rejected by reachability cache
    CHECK( rejected_cnt > 100 );
}

TEST_CASE( "reachability_horizontal", "[map][cache][vision][los][reachability]" )
{
    test_reachability( {{
            ".............", // NOLINT(cata-text-style)
            ".............", // NOLINT(cata-text-style)
            "...####......", // NOLINT(cata-text-style)
            "...#..#......", // NOLINT(cata-text-style)
            "...#..####...", // NOLINT(cata-text-style)
            "...#..X..#...", // NOLINT(cata-text-style)
            "...#.....#...", // NOLINT(cata-text-style)
            "...#..#..#...", // NOLINT(cata-text-style)
            "...#######...", // NOLINT(cata-text-style)
            ".............", // NOLINT(cata-text-style)
            "............." // NOLINT(cata-text-style)
        }
    }, /*up*/ false );
}

TEST_CASE( "reachability_vertical", "[map][cache][vision][los][reachability]" )
{
    // vertical cache makes sense only for 3d vision
    restore_on_out_of_scope<bool> restore_fov_3d( fov_3d );
    override_option opt( "FOV_3D", "true" );
    fov_3d = true;

    test_reachability( {{
            ".............", // NOLINT(cata-text-style)
            ".............", // NOLINT(cata-text-style)
            "...####......", // NOLINT(cata-text-style)
            "...#^^#......", // NOLINT(cata-text-style)
            "...#..####...", // NOLINT(cata-text-style)
            "...#..X..#...", // NOLINT(cata-text-style)
            "...#^^...#...", // NOLINT(cata-text-style)
            "...#^^#..#...", // NOLINT(cata-text-style)
            "...#######...", // NOLINT(cata-text-style)
            "...^^^.......", // NOLINT(cata-text-style)
            "............." // NOLINT(cata-text-style)
        }
    }, /*up*/ true );
}
