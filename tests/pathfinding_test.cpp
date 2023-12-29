#include "cata_catch.h"

#include "map.h"
#include "map_helpers.h"
#include "map_test_case.h"
#include "pathfinding.h"
#include "type_id.h"

#include <vector>

static const ter_str_id ter_t_brick_wall( "t_brick_wall" );
static const ter_str_id ter_t_flat_roof( "t_flat_roof" );
static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_utility_light( "t_utility_light" );
static const ter_str_id ter_t_window_frame( "t_window_frame" );

using namespace map_test_case_common;
using namespace map_test_case_common::tiles;

static const tile_predicate ter_set_flat_roof_above = ter_set( ter_t_flat_roof, tripoint_above );

static const tile_predicate set_up_tiles_common =
    ifchar( ' ', ter_set( ter_t_floor ) + ter_set_flat_roof_above ) ||
    ifchar( '#', ter_set( ter_t_brick_wall ) + ter_set_flat_roof_above ) ||
    fail;


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
    tile_predicate set_up_tiles = set_up_tiles_common;
    std::string section_prefix;

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
        section_name << section_prefix;
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
            t.for_each_tile( ifchar( 'f', ter_set( ter_t_floor ) + ter_set_flat_roof_above + set_from ) ||
                             ifchar( 't', ter_set( ter_t_floor ) + ter_set_flat_roof_above + set_to ) || set_up_tiles );


            int zlev = t.get_origin().z;
            map &here = get_map();

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
