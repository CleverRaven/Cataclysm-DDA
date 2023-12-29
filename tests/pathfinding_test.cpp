#include "cata_catch.h"

#include "map.h"
#include "mapdata.h"
#include "map_helpers.h"
#include "map_test_case.h"
#include "pathfinding.h"
#include "trap.h"
#include "type_id.h"

#include <vector>

static const ter_str_id ter_t_door_elocked( "t_door_elocked" );
static const ter_str_id ter_t_flat_roof( "t_flat_roof" );
static const ter_str_id ter_t_shrub_blackberry( "t_shrub_blackberry" );
static const ter_str_id ter_t_wall_wood_widened( "t_wall_wood_widened" );
static const ter_str_id ter_t_quietfarm_border_fence( "t_quietfarm_border_fence" );



using namespace map_test_case_common;
using namespace map_test_case_common::tiles;

static const tile_predicate ter_set_flat_roof_above = ter_set( ter_t_flat_roof, tripoint_above );

static std::string pathfinding_test_info( map_test_case &t, const std::vector<std::string> &actual )
{
    std::ostringstream out;
    map &here = get_map();

    using namespace map_test_case_common;

    out << "expected:\n" << printers::expected( t ) << '\n';
    out << "actual:\n" << printers::format_2d_array( t.map_tiles_str( [&]( map_test_case::tile t,
    std::ostringstream & out ) {
        out << actual[t.p.y][t.p.x];
    } ) ) << '\n';

    return out.str();
}

static std::vector<std::string> get_actual( std::vector<std::string> setup,
        const std::unordered_set<tripoint_bub_ms> &route )
{
    for( const tripoint_bub_ms &p : route ) {
        setup[p.y()][p.x()] = 'x';
    }
    return setup;
}

static void assert_route( map_test_case::tile t, const std::unordered_set<tripoint_bub_ms> &route )
{
    if( t.expect_c == 'x' ) {
        REQUIRE( route.count( tripoint_bub_ms( t.p ) ) == 1 );
    } else {
        REQUIRE( route.count( tripoint_bub_ms( t.p ) ) == 0 );
    }
}

static void add_surrounding_walls( std::vector<std::string> &input )
{
    for( std::string &s : input ) {
        s = "#" + s + "#";
    }
    int len = input[0].size();
    std::string wall( len, '#' );
    input.insert( input.begin(), wall );
    input.push_back( wall );
}

struct pathfinding_test_case {
    std::vector<std::string> setup;
    std::vector<std::string> expected_results;
    tile_predicate set_up_tiles = fail;
    ter_id floor_terrain = t_floor;

    pathfinding_test_case( const std::vector<std::string> &setup,
                           const std::vector<std::string> &expectedResults ) : setup( setup ),
        expected_results( expectedResults ) {
        add_surrounding_walls( this->setup );
        add_surrounding_walls( this->expected_results );
    }

    void test_all( const PathfindingSettings &settings ) const {
        clear_map_and_put_player_underground();

        map_test_case t;
        t.setup = setup;
        t.expected_results = expected_results;

        std::stringstream section_name;
        section_name << "pathfinding";
        section_name << t.generate_transform_combinations();

        SECTION( section_name.str() ) {
            tripoint from;
            auto set_from = [&from]( const map_test_case::tile & t ) {
                from = t.p;
            };
            tripoint to;
            auto set_to = [&to]( const map_test_case::tile & t ) {
                to = t.p;
            };
            t.for_each_tile( ifchar( ' ', ter_set( floor_terrain ) + ter_set_flat_roof_above ) ||
                             ifchar( '.', ter_set( t_dirt ) ) ||
                             ifchar( '#', ter_set( t_brick_wall ) + ter_set_flat_roof_above ) ||
                             ifchar( 'f', ter_set( floor_terrain ) + ter_set_flat_roof_above + set_from ) ||
                             ifchar( 't', ter_set( floor_terrain ) + ter_set_flat_roof_above + set_to ) || set_up_tiles );


            int zlev = t.get_origin().z;
            map &here = get_map();
            here.invalidate_map_cache( zlev );
            here.build_map_cache( zlev );

            std::unordered_set<tripoint_bub_ms> ps;
            for( const tripoint_bub_ms &p : here.route( tripoint_bub_ms( from ), tripoint_bub_ms( to ),
                    settings ) ) {
                ps.insert( p );
            }
            std::vector<std::string> actual = get_actual( t.setup, ps );
            INFO( pathfinding_test_info( t, actual ) );
            t.for_each_tile( [&]( const map_test_case::tile & t ) {
                assert_route( t, ps );
            } );
        }
    }
};

TEST_CASE( "pathfinding_basic", "[pathfinding]" )
{
    pathfinding_test_case t{
        {
            " f ",
            "   ",
            " t ",
        },
        {
            " f ",
            " x ",
            " x ",
        }
    };

    PathfindingSettings settings;
    settings.set_max_distance( 100 );
    settings.set_max_cost( 100 * 100 );
    t.test_all( settings );
}

TEST_CASE( "pathfinding_basic_wall", "[pathfinding]" )
{
    pathfinding_test_case t{
        {
            "f  ",
            "## ",
            "t  ",
        },
        {
            "fx ",
            "##x",
            "xx ",
        }
    };

    PathfindingSettings settings;
    settings.set_max_distance( 100 );
    settings.set_max_cost( 100 * 100 );
    t.test_all( settings );
}

TEST_CASE( "pathfinding_avoid", "[pathfinding]" )
{
    pathfinding_test_case t = GENERATE( pathfinding_test_case {
        // Blocked
        {
            "f",
            " ",
            "W",
            " ",
            "t",
        },
        {
            "f",
            " ",
            "W",
            " ",
            "t",
        }
    }, pathfinding_test_case {
        // Around wall.
        {
            "f  ",
            "WW ",
            "t  ",
        },
        {
            "fx ",
            "WWx",
            "xx ",
        }
    }, pathfinding_test_case {
        // Zig zag.
        {
            "f  ",
            "WW ",
            "   ",
            " WW",
            "   ",
            "WW ",
            "t  ",
        },
        {
            "fx ",
            "WWx",
            " x ",
            "xWW",
            " x ",
            "WWx",
            "xx ",
        }
    } );

    PathfindingSettings settings;
    settings.set_max_distance( 100 );
    settings.set_max_cost( 100 * 100 );

    SECTION( "air" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( t_hole ) );
        settings.set_is_flying( false );
        settings.set_avoid_air( true );
        settings.set_avoid_falling( false );

        t.test_all( settings );
    }

    SECTION( "bashing" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( t_window ) );
        settings.set_bash_strength( 100 );
        settings.set_avoid_bashing( true );

        t.test_all( settings );
    }

    SECTION( "dangerous traps" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( t_floor ) + trap_set( tr_landmine ) );
        settings.set_avoid_dangerous_traps( true );

        t.test_all( settings );
    }

    SECTION( "doors" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( t_door_c ) );
        settings.set_avoid_opening_doors( true );

        t.test_all( settings );
    }

    SECTION( "climbing" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( ter_t_quietfarm_border_fence ) );
        settings.set_climb_cost( 1 );
        settings.set_avoid_climbing( true );

        t.test_all( settings );
    }

    SECTION( "deep water" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( t_water_dp ) );
        settings.set_avoid_deep_water( true );

        t.test_all( settings );
    }

    SECTION( "falling" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( t_hole ) );
        settings.set_is_flying( false );
        settings.set_avoid_air( false );
        settings.set_avoid_falling( true );

        t.test_all( settings );
    }

    SECTION( "ground" ) {
        t.floor_terrain = t_water_sh;
        t.set_up_tiles = ifchar( 'W', ter_set( t_floor ) );
        settings.set_avoid_ground( true );

        t.test_all( settings );
    }

    SECTION( "hard ground" ) {
        t.floor_terrain = t_dirt;
        t.set_up_tiles = ifchar( 'W', ter_set( t_pavement ) );
        settings.set_avoid_hard_ground( true );

        t.test_all( settings );
    }

    SECTION( "lava" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( t_lava ) );
        settings.set_avoid_lava( true );

        t.test_all( settings );
    }

    SECTION( "pits" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( t_pit ) );
        settings.set_avoid_pits( true );

        t.test_all( settings );
    }

    SECTION( "sharp" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( ter_t_shrub_blackberry ) );
        settings.set_avoid_sharp( true );

        t.test_all( settings );
    }

    SECTION( "small passages" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( ter_t_wall_wood_widened ) );
        settings.set_avoid_small_passages( true );

        t.test_all( settings );
    }

    SECTION( "swimming" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( t_water_pool ) );
        settings.set_avoid_swimming( true );

        t.test_all( settings );
    }
}

TEST_CASE( "pathfinding_avoid_inside_door", "[pathfinding]" )
{
    // Needs a special case because the outside cache has a one tile padding.
    pathfinding_test_case t = {
        {
            "  f  ",
            ".....",
            ".....",
            ".....",
            "##D##",
            "     ",
            "  t  ",
        },
        {
            "  f  ",
            ".....",
            ".....",
            ".....",
            "##D##",
            "     ",
            "  t  ",
        },
    };

    PathfindingSettings settings;
    settings.set_max_distance( 100 );
    settings.set_max_cost( 100 * 100 );
    t.set_up_tiles = ifchar( 'D', ter_set( ter_t_door_elocked ) + ter_set_flat_roof_above );
    settings.set_avoid_opening_doors( false );

    t.test_all( settings );
}

TEST_CASE( "pathfinding_avoid_unsheltered", "[pathfinding]" )
{
    // Needs a special case because the outside cache has a one tile padding.
    pathfinding_test_case t = {
        {
            " f ",
            "   ",
            "...",
            "...",
            "...",
            "   ",
            " t ",
        },
        {
            " f ",
            "   ",
            "...",
            "...",
            "...",
            "   ",
            " t ",
        }
    };

    PathfindingSettings settings;
    settings.set_max_distance( 100 );
    settings.set_max_cost( 100 * 100 );
    settings.set_avoid_unsheltered( true );

    t.test_all( settings );
}


TEST_CASE( "pathfinding_allow", "[pathfinding]" )
{
    pathfinding_test_case t = GENERATE( pathfinding_test_case{
        // Not blocked
        {
            "f",
            " ",
            "W",
            " ",
            "t",
        },
        {
            "f",
            "x",
            "x",
            "x",
            "x",
        }
    }, pathfinding_test_case{
        // Not blocked, not straight
        {
            "f  ",
            "   ",
            "##W",
            "   ",
            "t  ",
        },
        {
            "f  ",
            " x ",
            "##x",
            " x ",
            "x  ",
        }
    }, pathfinding_test_case{
        // Prefer shorter
        {
            "f   ",
            "W## ",
            "t   ",
        },
        {
            "f   ",
            "x## ",
            "x   ",
        }
    }, pathfinding_test_case{
        // Prefer shorter, not straight
        {
            "f      ",
            "###W## ",
            "t      ",
        },
        {
            "fxx    ",
            "###x## ",
            "xxx    ",
        }
    } );

    PathfindingSettings settings;
    settings.set_max_distance( 100 );
    settings.set_max_cost( 100 * 100 );

    SECTION( "air" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( t_hole ) );
        settings.set_is_flying( true );
        settings.set_avoid_air( false );

        t.test_all( settings );
    }

    SECTION( "bashing" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( t_window ) );
        settings.set_bash_strength( 20 );
        settings.set_avoid_bashing( false );

        t.test_all( settings );
    }

    SECTION( "dangerous traps" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( t_floor ) + trap_set( tr_landmine ) );
        settings.set_avoid_dangerous_traps( false );

        t.test_all( settings );
    }

    SECTION( "doors" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( t_door_c ) );
        settings.set_avoid_opening_doors( false );

        t.test_all( settings );
    }

    SECTION( "climbing" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( ter_t_quietfarm_border_fence ) );
        settings.set_climb_cost( 1 );
        settings.set_avoid_climbing( false );

        t.test_all( settings );
    }

    SECTION( "deep water" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( t_water_dp ) );
        settings.set_avoid_deep_water( false );

        t.test_all( settings );
    }

    SECTION( "ground" ) {
        t.floor_terrain = t_water_sh;
        t.set_up_tiles = ifchar( 'W', ter_set( t_floor ) );
        settings.set_avoid_ground( false );

        t.test_all( settings );
    }

    SECTION( "hard ground" ) {
        t.floor_terrain = t_dirt;
        t.set_up_tiles = ifchar( 'W', ter_set( t_pavement ) );
        settings.set_avoid_hard_ground( false );

        t.test_all( settings );
    }

    SECTION( "lava" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( t_lava ) );
        settings.set_avoid_lava( false );

        t.test_all( settings );
    }

    SECTION( "pits" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( t_pit ) );
        settings.set_avoid_pits( false );

        t.test_all( settings );
    }

    SECTION( "sharp" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( ter_t_shrub_blackberry ) );
        settings.set_avoid_sharp( false );

        t.test_all( settings );
    }

    SECTION( "small passages" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( ter_t_wall_wood_widened ) );
        settings.set_avoid_small_passages( false );

        t.test_all( settings );
    }

    SECTION( "swimming" ) {
        t.set_up_tiles = ifchar( 'W', ter_set( t_water_pool ) );
        settings.set_avoid_swimming( false );

        t.test_all( settings );
    }
}

TEST_CASE( "pathfinding_allow_inside_door", "[pathfinding]" )
{
    // Needs a special case because the outside cache has a one tile padding.
    pathfinding_test_case t = {
        {
            "  f  ",
            "     ",
            "##D##",
            ".....",
            ".....",
            ".....",
            "  t  ",
        },
        {
            "  f  ",
            "  x  ",
            "##x##",
            "..x..",
            "..x..",
            "..x..",
            "  x  ",
        },
    };

    PathfindingSettings settings;
    settings.set_max_distance( 100 );
    settings.set_max_cost( 100 * 100 );
    t.set_up_tiles = ifchar( 'D', ter_set( ter_t_door_elocked ) + ter_set_flat_roof_above );
    settings.set_avoid_opening_doors( false );

    t.test_all( settings );
}

TEST_CASE( "pathfinding_allow_unsheltered", "[pathfinding]" )
{
    // Needs a special case because the outside cache has a one tile padding.
    pathfinding_test_case t = {
        {
            " f ",
            "   ",
            "...",
            "...",
            "...",
            "   ",
            " t ",
        },
        {
            " f ",
            " x ",
            ".x.",
            ".x.",
            ".x.",
            " x ",
            " x ",
        }
    };

    PathfindingSettings settings;
    settings.set_max_distance( 100 );
    settings.set_max_cost( 100 * 100 );
    // No roof.
    t.set_up_tiles = ifchar( 'W', ter_set( t_dirt ) );
    settings.set_avoid_unsheltered( false );

    t.test_all( settings );
}
