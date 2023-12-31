#include "cata_catch.h"

#include "map.h"
#include "mapdata.h"
#include "map_helpers.h"
#include "map_test_case.h"
#include "monster.h"
#include "mtype.h"
#include "pathfinding.h"
#include "trap.h"
#include "type_id.h"

#include <vector>

static const mon_flag_str_id mon_flag_STUMBLES( "STUMBLES" );

static const ter_str_id ter_t_door_elocked( "t_door_elocked" );
static const ter_str_id ter_t_flat_roof( "t_flat_roof" );
static const ter_str_id ter_t_quietfarm_border_fence( "t_quietfarm_border_fence" );
static const ter_str_id ter_t_ramp_down_high( "t_ramp_down_high" );
static const ter_str_id ter_t_ramp_down_low( "t_ramp_down_low" );
static const ter_str_id ter_t_ramp_up_high( "t_ramp_up_high" );
static const ter_str_id ter_t_ramp_up_low( "t_ramp_up_low" );
static const ter_str_id ter_t_shrub_blackberry( "t_shrub_blackberry" );
static const ter_str_id ter_t_wall_wood_widened( "t_wall_wood_widened" );

using namespace map_test_case_common;
using namespace map_test_case_common::tiles;

static std::string pathfinding_test_info( map_test_case &t, const std::vector<std::string> &actual,
        int z )
{
    std::ostringstream out;

    out << "z level: " << z << '\n';
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
        s = string_format( "#%s#", s );
    }
    int len = input[0].size();
    std::string wall( len, '#' );
    input.insert( input.begin(), wall );
    input.push_back( wall );
}

static void add_roof( std::vector<std::vector<std::string>> &input )
{
    int len = input[0][0].size();
    std::vector<std::string> roof( input[0].size(), std::string( len, '_' ) );
    input.insert( input.begin(), roof );
}

struct pathfinding_test_case {
    std::vector<std::vector<std::string>> setup;
    std::vector<std::vector<std::string>> expected_results;
    tile_predicate set_up_tiles = fail;
    ter_id floor_terrain = t_floor;
    std::function<std::vector<tripoint_bub_ms>( const map &, const tripoint_bub_ms &, const tripoint_bub_ms & )>
    route;

    pathfinding_test_case( std::vector<std::string> setup, std::vector<std::string> expected_results ) {
        add_surrounding_walls( setup );
        add_surrounding_walls( expected_results );
        this->setup.push_back( std::move( setup ) );
        this->expected_results.push_back( std::move( expected_results ) );
        add_roof( this->setup );
        add_roof( this->expected_results );
    }

    pathfinding_test_case( std::vector<std::vector<std::string>> setup,
                           std::vector<std::vector<std::string>> expected_results ) : setup( std::move( setup ) ),
        expected_results( std::move( expected_results ) ) {
        for( std::vector<std::string> &layer : this->setup ) {
            add_surrounding_walls( layer );
        }
        for( std::vector<std::string> &layer : this->expected_results ) {
            add_surrounding_walls( layer );
        }
        add_roof( this->setup );
        add_roof( this->expected_results );
    }

    void test_all( const PathfindingSettings &settings ) const {
        test_all_fn( [&]( const map & here, const tripoint_bub_ms & from, const tripoint_bub_ms & to ) {
            return here.route( from, to, settings );
        } );
    }

    void test_all( const std::string &mon_id ) const {
        test_all_fn( [&]( const map & here, const tripoint_bub_ms & from, const tripoint_bub_ms & to ) {
            monster &mon = spawn_test_monster( mon_id, from.raw(), false );
            mon.set_dest( here.getglobal( to ) );
            // Disable random stumbling so the tests are deterministic.
            const bool can_stumble = mon.type->has_flag( mon_flag_STUMBLES );
            on_out_of_scope restore_stumbles( [&]() {
                const_cast<mtype *>( mon.type )->set_flag( mon_flag_STUMBLES, can_stumble );
            } );
            const_cast<mtype *>( mon.type )->set_flag( mon_flag_STUMBLES, false );
            int limit = 0;
            std::vector<tripoint_bub_ms> route;
            while( mon.pos_bub() != to ) {
                // Less moves than normal to be sure we capture every tile.
                mon.mod_moves( 50 );
                mon.move();
                if( mon.pos_bub() != from ) {
                    route.push_back( mon.pos_bub() );
                }
                REQUIRE( ++limit < 200 );
            }
            return route;
        } );
    }

    template <typename RouteFn>
    void test_all_fn( RouteFn &&route_fn ) const {
        clear_map_and_put_player_underground();

        map_test_case_3d t;
        REQUIRE( !setup.empty() );
        REQUIRE( setup.size() == expected_results.size() );
        for( std::size_t i = 0; i < setup.size(); ++i ) {
            map_test_case tc;
            tc.setup = setup[i];
            tc.expected_results = expected_results[i];
            t.layers.push_back( std::move( tc ) );
        }

        std::stringstream section_name;
        section_name << "pathfinding";
        section_name << t.generate_transform_combinations();

        SECTION( section_name.str() ) {
            std::optional<tripoint> from;
            auto set_from = [&from]( const map_test_case::tile & t ) {
                REQUIRE( !from );
                from = t.p;
            };
            std::optional<tripoint> to;
            auto set_to = [&to]( const map_test_case::tile & t ) {
                REQUIRE( !to );
                to = t.p;
            };
            t.for_each_tile( ifchar( ' ', ter_set( floor_terrain ) ) ||
                             ifchar( '@', ter_set( t_hole ) ) ||
                             ifchar( '_', ter_set( ter_t_flat_roof ) ) ||
                             ifchar( ',', ter_set( t_dirt ) + ter_set( t_hole, tripoint_above ) + ter_set( t_hole,
                                     tripoint_above * 2 ) + ter_set( t_hole, tripoint_above * 3 ) + ter_set( t_hole,
                                             tripoint_above * 4 ) + ter_set( t_hole, tripoint_above * 5 ) ) ||
                             ifchar( '#', ter_set( t_brick_wall ) ) ||
                             ifchar( 'f', ter_set( floor_terrain ) + set_from ) ||
                             ifchar( 't', ter_set( floor_terrain ) + set_to ) || set_up_tiles ||
                             fail );

            map &here = get_map();
            std::vector<int> z_levels = t.z_levels();
            for( int zlev : z_levels ) {
                here.invalidate_map_cache( zlev );
                here.build_map_cache( zlev );
            }

            std::unordered_map<int, std::unordered_set<tripoint_bub_ms>> ps;
            {
                // Prepare info string if the route fails.
                std::ostringstream out;
                // Don't print the roof.
                for( std::size_t i = 1; i < z_levels.size(); ++i ) {
                    out << printers::expected( t.layers[i] );
                }
                INFO( out.str() );
                for( const tripoint_bub_ms &p : route_fn( here, tripoint_bub_ms( *from ),
                        tripoint_bub_ms( *to ) ) ) {
                    ps[p.z()].insert( p );
                }
            }
            std::vector<std::vector<std::string>> actual;
            actual.reserve( z_levels.size() );
            for( std::size_t i = 0; i < z_levels.size(); ++i ) {
                actual.push_back( get_actual( t.layers[i].setup, ps[z_levels[i]] ) );
            }
            std::ostringstream out;
            // Don't print the roof.
            for( std::size_t  i = 1; i < z_levels.size(); ++i ) {
                out << pathfinding_test_info( t.layers[i], actual[i], z_levels[i] );
            }
            INFO( out.str() );
            t.for_each_tile( [&]( const map_test_case::tile & t ) {
                assert_route( t, ps[t.p.z] );
            } );
        }
    }
};

TEST_CASE( "pathfinding_basic", "[pathfinding]" )
{
    pathfinding_test_case t = GENERATE( pathfinding_test_case{
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
    }, pathfinding_test_case {
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
    } );

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
            " f ",
            "   ",
            "WWW",
            "   ",
            " t ",
        },
        {
            " f ",
            "   ",
            "WWW",
            "   ",
            " t ",
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
        t.set_up_tiles = ifchar( 'W', ter_set( t_floor ) + trap_set( tr_dissector ) );
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
            ",,,,,",
            ",,,,,",
            ",,,,,",
            "##D##",
            "     ",
            "  t  ",
        },
        {
            "  f  ",
            ",,,,,",
            ",,,,,",
            ",,,,,",
            "##D##",
            "     ",
            "  t  ",
        },
    };

    PathfindingSettings settings;
    settings.set_max_distance( 100 );
    settings.set_max_cost( 100 * 100 );
    t.set_up_tiles = ifchar( 'D', ter_set( ter_t_door_elocked ) );
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
            ",,,",
            ",,,",
            ",,,",
            "   ",
            " t ",
        },
        {
            " f ",
            "   ",
            ",,,",
            ",,,",
            ",,,",
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

TEST_CASE( "pathfinding_avoid_climbing_stairs", "[pathfinding]" )
{
    pathfinding_test_case t = {
        {
            {
                " f ",
                "   ",
                " D ",
                "   ",
                "   ",
            },
            {
                "   ",
                "   ",
                " U ",
                "   ",
                " t ",
            }
        },
        {
            {
                " f ",
                "   ",
                " D ",
                "   ",
                "   ",
            },
            {
                "   ",
                "   ",
                " U ",
                "   ",
                " t ",
            }
        }
    };
    t.set_up_tiles = ifchar( 'U', ter_set( t_stairs_up ) ) || ifchar( 'D', ter_set( t_stairs_down ) );

    PathfindingSettings settings;
    settings.set_max_distance( 100 );
    settings.set_max_cost( 100 * 100 );
    settings.set_avoid_climb_stairway( true );

    t.test_all( settings );
}

TEST_CASE( "pathfinding_avoid_falling", "[pathfinding]" )
{
    pathfinding_test_case t = {
        {
            {
                " f ",
                "   ",
                "@@@",
                "@@@",
            },
            {
                "###",
                "###",
                "   ",
                " t ",
            }
        },
        {
            {
                " f ",
                "   ",
                "@@@",
                "@@@",
            },
            {
                "###",
                "###",
                "   ",
                " t ",
            }
        }
    };

    PathfindingSettings settings;
    settings.set_max_distance( 100 );
    settings.set_max_cost( 100 * 100 );
    settings.set_avoid_air( false );
    settings.set_is_flying( false );
    settings.set_avoid_falling( true );

    t.test_all( settings );
}

TEST_CASE( "pathfinding_allow", "[pathfinding]" )
{
    pathfinding_test_case t = GENERATE( pathfinding_test_case{
        // Not blocked
        {
            " f ",
            "   ",
            "WWW",
            "   ",
            " t ",
        },
        {
            " f ",
            " x ",
            "WxW",
            " x ",
            " x ",
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
        t.set_up_tiles = ifchar( 'W', ter_set( t_floor ) + trap_set( tr_dissector ) );
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
            ",,,,,",
            ",,,,,",
            ",,,,,",
            "  t  ",
        },
        {
            "  f  ",
            "  x  ",
            "##x##",
            ",,x,,",
            ",,x,,",
            ",,x,,",
            "  x  ",
        },
    };

    PathfindingSettings settings;
    settings.set_max_distance( 100 );
    settings.set_max_cost( 100 * 100 );
    t.set_up_tiles = ifchar( 'D', ter_set( ter_t_door_elocked ) );
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
            ",,,",
            ",,,",
            ",,,",
            "   ",
            " t ",
        },
        {
            " f ",
            " x ",
            ",x,",
            ",x,",
            ",x,",
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

TEST_CASE( "pathfinding_allow_climbing_stairs", "[pathfinding]" )
{

    pathfinding_test_case t = GENERATE( pathfinding_test_case{
        // Simple connected
        {
            {
                " f ",
                "   ",
                " D ",
                "   ",
                "   ",
            },
            {
                "   ",
                "   ",
                " U ",
                "   ",
                " t ",
            }
        },
        {
            {
                " f ",
                " x ",
                " x ",
                "   ",
                "   ",
            },
            {
                "   ",
                "   ",
                " x ",
                " x ",
                " x ",
            }
        }
    }, pathfinding_test_case{
        // Simple disconnected
        {
            {
                " f ",
                " D ",
                "   ",
                "   ",
                "   ",
            },
            {
                "   ",
                "   ",
                "   ",
                " U ",
                " t ",
            }
        },
        {
            {
                " f ",
                " x ",
                "   ",
                "   ",
                "   ",
            },
            {
                "   ",
                "   ",
                "   ",
                " x ",
                " x ",
            }
        }
    }, pathfinding_test_case{
        // Up then down
        {
            {
                "   ",
                " D ",
                "   ",
                " D ",
                "   ",
            },
            {
                " f ",
                " U ",
                "###",
                " U ",
                " t ",
            }
        },
        {
            {
                "   ",
                " x ",
                " x ",
                " x ",
                "   ",
            },
            {
                " f ",
                " x ",
                "###",
                " x ",
                " x ",
            }
        }
    } );
    t.set_up_tiles = ifchar( 'U', ter_set( t_stairs_up ) ) || ifchar( 'D', ter_set( t_stairs_down ) );

    PathfindingSettings settings;
    settings.set_max_distance( 100 );
    settings.set_max_cost( 100 * 100 );
    settings.set_avoid_climb_stairway( false );

    t.test_all( settings );
}

TEST_CASE( "pathfinding_allow_falling", "[pathfinding]" )
{
    pathfinding_test_case t = {
        {
            {
                " f ",
                "   ",
                "@@@",
                "@@@",
            },
            {
                "###",
                "###",
                "   ",
                " t ",
            }
        },
        {
            {
                " f ",
                " x ",
                "@x@",
                "@@@",
            },
            {
                "###",
                "###",
                " x ",
                " x ",
            }
        }
    };

    PathfindingSettings settings;
    settings.set_max_distance( 100 );
    settings.set_max_cost( 100 * 100 );
    settings.set_avoid_air( false );
    settings.set_is_flying( false );
    settings.set_avoid_falling( false );

    t.test_all( settings );
}

TEST_CASE( "pathfinding_allow_ramps", "[pathfinding]" )
{
    pathfinding_test_case t = GENERATE( pathfinding_test_case{
        // Down
        {
            {
                " f ",
                "DDD",
                "ddd",
                "@@@",
                "   ",
            },
            {
                "###",
                "###",
                "UUU",
                "uuu",
                " t ",
            }
        },
        {
            {
                " f ",
                "DxD",
                "dxd",
                "@@@",
                "   ",
            },
            {
                "###",
                "###",
                "UxU",
                "uxu",
                " x ",
            }
        }
    }, pathfinding_test_case{
        // Up
        {
            {
                "   ",
                "@@@",
                "ddd",
                "DDD",
                "   ",
                " t ",
            },
            {
                " f ",
                "uuu",
                "UUU",
                "###",
                "###",
                "###",
            }
        },
        {
            {
                "   ",
                "@@@",
                "dxd",
                "DxD",
                " x ",
                " x ",
            },
            {
                " f ",
                "uxu",
                "UxU",
                "###",
                "###",
                "###",
            }
        }
    } );
    t.set_up_tiles = ifchar( 'U', ter_set( ter_t_ramp_up_high ) ) ||
                     ifchar( 'D', ter_set( ter_t_ramp_down_high ) ) ||
                     ifchar( 'u', ter_set( ter_t_ramp_up_low ) ) ||
                     ifchar( 'd', ter_set( ter_t_ramp_down_low ) );

    PathfindingSettings settings;
    settings.set_avoid_falling( true );
    settings.set_max_distance( 100 );
    settings.set_max_cost( 100 * 100 );

    t.test_all( settings );
}

TEST_CASE( "pathfinding_flying", "[pathfinding]" )
{
    pathfinding_test_case t = GENERATE( pathfinding_test_case{
        // Down
        {
            {
                " f ",
                "   ",
                "   ",
                "@@@",
                "@@@",
                "@@@",
            },
            {
                "###",
                "###",
                "###",
                "   ",
                "   ",
                " t ",
            }
        },
        {
            {
                " f ",
                " x ",
                " x ",
                "@@@",
                "@@@",
                "@@@",
            },
            {
                "###",
                "###",
                "###",
                " x ",
                " x ",
                " x ",
            }
        }
    }, pathfinding_test_case{
        // Up
        {
            {
                "@@@",
                "@@@",
                "@@@",
                "   ",
                "   ",
                " t ",
            },
            {
                " f ",
                "   ",
                "   ",
                "###",
                "###",
                "###",
            }
        },
        {
            {
                "@@@",
                "@@@",
                "@@@",
                " x ",
                " x ",
                " x ",
            },
            {
                " f ",
                " x ",
                " x ",
                "###",
                "###",
                "###",
            }
        }
    }, pathfinding_test_case{
        // Over
        {
            {
                "@@@",
                "@@@",
                "@@@",
                "@@@",
                "@@@",
                "@@@",
            },
            {
                " f ",
                "   ",
                "###",
                "###",
                "   ",
                " t ",
            }
        },
        {
            {
                "@@@",
                "@@@",
                "@x@",
                "@x@",
                "@@@",
                "@@@",
            },
            {
                " f ",
                " x ",
                "###",
                "###",
                " x ",
                " x ",
            }
        }
    } );

    PathfindingSettings settings;
    settings.set_avoid_air( false );
    settings.set_is_flying( true );
    settings.set_max_distance( 100 );
    settings.set_max_cost( 100 * 100 );

    t.test_all( settings );
}

// Integration testing with monsters.

TEST_CASE( "pathfinding_migo", "[pathfinding]" )
{
    pathfinding_test_case t = GENERATE( pathfinding_test_case{
        // Straight Line
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
    }, pathfinding_test_case{
        // Around wall.
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
    }, pathfinding_test_case{
        // Zig zag.
        {
            "f  ",
            "## ",
            "   ",
            " ##",
            "   ",
            "## ",
            "t  ",
        },
        {
            "fx ",
            "##x",
            " x ",
            "x##",
            " x ",
            "##x",
            "xx ",
        }
    }, pathfinding_test_case{
        // Bash window
        {
            "f   ",
            "##w#",
            "t   ",
        },
        {
            "fx  ",
            "##x#",
            "xx  ",
        }
    }, pathfinding_test_case{
        // Down ramp
        {
            {
                " f ",
                "DDD",
                "ddd",
                "@@@",
                "   ",
            },
            {
                "###",
                "###",
                "UUU",
                "uuu",
                " t ",
            }
        },
        {
            {
                " f ",
                "DxD",
                "ddd",  // Differs from pathfinder.
                "@@@",
                "   ",
            },
            {
                "###",
                "###",
                "UxU",
                "uxu",
                " x ",
            }
        }
    }, pathfinding_test_case{
        // Up ramp
        {
            {
                "   ",
                "@@@",
                "ddd",
                "DDD",
                "   ",
                " t ",
            },
            {
                " f ",
                "uuu",
                "UUU",
                "###",
                "###",
                "###",
            }
        },
        {
            {
                "   ",
                "@@@",
                "dxd",
                "DxD",
                " x ",
                " x ",
            },
            {
                " f ",
                "uxu",
                "UUU",  // Differs from pathfinder.
                "###",
                "###",
                "###",
            }
        }
    }, pathfinding_test_case {
        // Open door
        {
            " f ",
            "   ",
            "#O#",
            "   ",
            " t ",
        },
        {
            " f ",
            " x ",
            "#x#",
            " x ",
            " x ",
        }
    }, pathfinding_test_case{
        // Around trap
        {
            "f    ",
            "##L# ",
            "t    ",
        },
        {
            "fxxx ",
            "##L#x",
            "xxxx ",
        }
    }, pathfinding_test_case{
        // Around pit
        {
            "f    ",
            "##p# ",
            "t    ",
        },
        {
            "fxxx ",
            "##p#x",
            "xxxx ",
        }
    } );

    t.set_up_tiles = ifchar( 'w', ter_set( t_window ) ) ||
                     ifchar( 'U', ter_set( ter_t_ramp_up_high ) ) ||
                     ifchar( 'D', ter_set( ter_t_ramp_down_high ) ) ||
                     ifchar( 'u', ter_set( ter_t_ramp_up_low ) ) ||
                     ifchar( 'd', ter_set( ter_t_ramp_down_low ) ) ||
                     ifchar( '>', ter_set( t_stairs_down ) ) ||
                     ifchar( '<', ter_set( t_stairs_up ) ) ||
                     ifchar( 'L', trap_set( tr_dissector ) ) ||
                     ifchar( 'p', ter_set( t_pit ) ) ||
                     ifchar( 'O', ter_set( t_door_c ) );

    t.test_all( "mon_mi_go" );
}

TEST_CASE( "pathfinding_migo_harrier", "[pathfinding]" )
{
    pathfinding_test_case t = GENERATE( pathfinding_test_case{
        // Down
        {
            {
                " f ",
                "   ",
                "   ",
                "@@@",
                "@@@",
                "@@@",
            },
            {
                "###",
                "###",
                "###",
                "   ",
                "   ",
                " t ",
            }
        },
        {
            {
                " f ",
                " x ",
                " x ",
                "@@@",
                "@@@",
                "@@@",
            },
            {
                "###",
                "###",
                "###",
                " x ",
                " x ",
                " x ",
            }
        }
    }, pathfinding_test_case{
        // Up
        {
            {
                "@@@",
                "@@@",
                "@@@",
                "   ",
                "   ",
                " t ",
            },
            {
                " f ",
                "   ",
                "   ",
                "###",
                "###",
                "###",
            }
        },
        {
            {
                "@@@",
                "@@@",
                "@@@",
                " x ",
                " x ",
                " x ",
            },
            {
                " f ",
                " x ",
                " x ",
                "###",
                "###",
                "###",
            }
        }
    }, pathfinding_test_case{
        // Over
        {
            {
                "@@@",
                "@@@",
                "@@@",
                "@@@",
                "@@@",
                "@@@",
            },
            {
                " f ",
                "   ",
                "###",
                "###",
                "   ",
                " t ",
            }
        },
        {
            {
                "@@@",
                "@@@",
                "@x@",
                "@x@",
                "@@@",
                "@@@",
            },
            {
                " f ",
                " x ",
                "###",
                "###",
                " x ",
                " x ",
            }
        }
    } );

    t.test_all( "mon_mi_go_flying" );
}

TEST_CASE( "pathfinding_zombie", "[pathfinding]" )
{
    // Note that zombies only try straight lines to targets.
    pathfinding_test_case t = GENERATE( pathfinding_test_case{
        // Straight Line
        {
            " f ",
            "   ",
            "   ",
            "   ",
            " t ",
        },
        {
            " f ",
            " x ",
            " x ",
            " x ",
            " x ",
        }
    }, pathfinding_test_case{
        // Through trap
        {
            " f ",
            "   ",
            " L ",
            "   ",
            " t ",
        },
        {
            " f ",
            " x ",
            " x ",
            " x ",
            " x ",
        }

    }, pathfinding_test_case{
        // Through sharp
        {
            " f ",
            "   ",
            " s ",
            "   ",
            " t ",
        },
        {
            " f ",
            " x ",
            " x ",
            " x ",
            " x ",
        }

    }, pathfinding_test_case{
        // Through pit
        {
            " f ",
            "   ",
            " p ",
            "   ",
            " t ",
        },
        {
            " f ",
            " x ",
            " x ",
            " x ",
            " x ",
        }
    } );

    t.set_up_tiles = ifchar( 'w', ter_set( t_window ) ) ||
                     ifchar( 'L', trap_set( tr_dissector ) ) ||
                     ifchar( 'p', ter_set( t_pit ) ) ||
                     ifchar( 's', ter_set( ter_t_shrub_blackberry ) );

    t.test_all( "mon_zombie" );
}
