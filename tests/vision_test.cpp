#include <stddef.h>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "field.h"
#include "map.h"
#include "map_helpers.h"
#include "calendar.h"
#include "item.h"
#include "lightmap.h"
#include "shadowcasting.h"
#include "type_id.h"
#include "game_constants.h"
#include "point.h"

static void full_map_test( const std::vector<std::string> &setup,
                           const std::vector<std::string> &expected_results,
                           const time_point &time )
{
    const ter_id t_brick_wall( "t_brick_wall" );
    const ter_id t_window_frame( "t_window_frame" );
    const ter_id t_floor( "t_floor" );
    const ter_id t_utility_light( "t_utility_light" );
    const efftype_id effect_narcosis( "narcosis" );
    const ter_id t_flat_roof( "t_flat_roof" );
    const ter_id t_open_air( "t_open_air" );

    g->place_player( tripoint( 60, 60, 0 ) );
    g->u.worn.clear(); // Remove any light-emitting clothing
    g->u.clear_effects();
    clear_map();
    g->reset_light_level();

    REQUIRE( !g->u.is_blind() );
    REQUIRE( !g->u.in_sleep_state() );
    REQUIRE( !g->u.has_effect( effect_narcosis ) );

    g->u.recalc_sight_limits();

    calendar::turn = time;

    int height = setup.size();
    REQUIRE( height > 0 );
    REQUIRE( static_cast<size_t>( height ) == expected_results.size() );
    int width = setup[0].size();

    for( const std::string &line : setup ) {
        REQUIRE( line.size() == static_cast<size_t>( width ) );
    }

    for( const std::string &line : expected_results ) {
        REQUIRE( line.size() == static_cast<size_t>( width ) );
    }

    tripoint origin;
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            switch( setup[y][x] ) {
                case 'V':
                case 'U':
                case 'u':
                    origin = g->u.pos() - point( x, y );
                    if( setup[y][x] == 'V' ) {
                        item headlamp( "wearable_light_on" );
                        g->u.worn.push_back( headlamp );
                    }
                    break;
            }
        }
    }

    {
        // Sanity check on player placement
        REQUIRE( origin.z < OVERMAP_HEIGHT );
        tripoint player_offset = g->u.pos() - origin;
        REQUIRE( player_offset.y >= 0 );
        REQUIRE( player_offset.y < height );
        REQUIRE( player_offset.x >= 0 );
        REQUIRE( player_offset.x < width );
        char player_char = setup[player_offset.y][player_offset.x];
        REQUIRE( ( player_char == 'U' || player_char == 'u' || player_char == 'V' ) );
    }

    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            const tripoint p = origin + point( x, y );
            const tripoint above = p + tripoint( 0, 0, 1 );
            switch( setup[y][x] ) {
                case ' ':
                    break;
                case 'L':
                    g->m.ter_set( p, t_utility_light );
                    g->m.ter_set( above, t_flat_roof );
                    break;
                case '#':
                    g->m.ter_set( p, t_brick_wall );
                    g->m.ter_set( above, t_flat_roof );
                    break;
                case '=':
                    g->m.ter_set( p, t_window_frame );
                    g->m.ter_set( above, t_flat_roof );
                    break;
                case '-':
                case 'u':
                    g->m.ter_set( p, t_floor );
                    g->m.ter_set( above, t_flat_roof );
                    break;
                case 'U':
                case 'V':
                    // Already handled above
                    break;
                default:
                    FAIL( "unexpected setup char '" << setup[y][x] << "'" );
            }
        }
    }

    // We have to run the whole thing twice, because the first time through the
    // player's vision_threshold is based on the previous lighting level (so
    // they might, for example, have poor nightvision due to having just been
    // in daylight)
    g->m.update_visibility_cache( origin.z );
    g->m.invalidate_map_cache( origin.z );
    g->m.build_map_cache( origin.z );
    g->m.update_visibility_cache( origin.z );
    g->m.invalidate_map_cache( origin.z );
    g->m.build_map_cache( origin.z );

    const level_cache &cache = g->m.access_cache( origin.z );
    const level_cache &above_cache = g->m.access_cache( origin.z + 1 );
    const visibility_variables &vvcache =
        g->m.get_visibility_variables_cache();

    std::ostringstream fields;
    std::ostringstream transparency;
    std::ostringstream seen;
    std::ostringstream lm;
    std::ostringstream apparent_light;
    std::ostringstream obstructed;
    std::ostringstream floor_above;
    transparency << std::setprecision( 3 );
    seen << std::setprecision( 3 );
    apparent_light << std::setprecision( 3 );

    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            const tripoint p = origin + point( x, y );
            const map::apparent_light_info al = map::apparent_light_helper( cache, p );
            for( auto &pr : g->m.field_at( p ) ) {
                fields << pr.second.name() << ',';
            }
            fields << ' ';
            transparency << std::setw( 6 )
                         << cache.transparency_cache[p.x][p.y] << ' ';
            seen << std::setw( 6 ) << cache.seen_cache[p.x][p.y] << ' ';
            four_quadrants this_lm = cache.lm[p.x][p.y];
            lm << this_lm.to_string() << ' ';
            apparent_light << std::setw( 6 ) << al.apparent_light << ' ';
            obstructed << ( al.obstructed ? '#' : '.' ) << ' ';
            floor_above << ( above_cache.floor_cache[p.x][p.y] ? '#' : '.' ) << ' ';
        }
        fields << '\n';
        transparency << '\n';
        seen << '\n';
        lm << '\n';
        apparent_light << '\n';
        obstructed << '\n';
        floor_above << '\n';
    }

    INFO( "zlevels: " << g->m.has_zlevels() );
    INFO( "origin: " << origin );
    INFO( "player: " << g->u.pos() );
    INFO( "unimpaired_range: " << g->u.unimpaired_range() );
    INFO( "vision_threshold: " << vvcache.vision_threshold );
    INFO( "fields:\n" << fields.str() );
    INFO( "transparency:\n" << transparency.str() );
    INFO( "seen:\n" << seen.str() );
    INFO( "lm:\n" << lm.str() );
    INFO( "apparent_light:\n" << apparent_light.str() );
    INFO( "obstructed:\n" << obstructed.str() );
    INFO( "floor_above:\n" << floor_above.str() );

    bool success = true;
    std::ostringstream expected;
    std::ostringstream observed;

    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            const tripoint p = origin + point( x, y );
            const lit_level level = g->m.apparent_light_at( p, vvcache );
            const char exp_char = expected_results[y][x];
            if( exp_char < '0' || exp_char > '9' ) {
                FAIL( "unexpected result char '" <<
                      expected_results[y][x] << "'" );
            }
            const int expected_level = exp_char - '0';

            observed << level << ' ';
            expected << expected_level << ' ';
            if( level != expected_level ) {
                success = false;
            }
        }
        observed << '\n';
        expected << '\n';
    }

    INFO( "observed:\n" << observed.str() );
    INFO( "expected:\n" << expected.str() );
    CHECK( success );
}

struct vision_test_case {
    std::vector<std::string> setup;
    std::vector<std::string> expected_results;
    time_point time;
    bool test_3d;

    static void transpose( std::vector<std::string> &v ) {
        if( v.empty() ) {
            return;
        }
        std::vector<std::string> new_v( v[0].size() );

        for( const std::string &col : v ) {
            for( size_t y = 0; y < new_v.size(); ++y ) {
                new_v[y].push_back( col.at( y ) );
            }
        }

        v = new_v;
    }

    void transpose() {
        transpose( setup );
        transpose( expected_results );
    }

    void reflect_x() {
        for( std::string &s : setup ) {
            std::reverse( s.begin(), s.end() );
        }
        for( std::string &s : expected_results ) {
            std::reverse( s.begin(), s.end() );
        }
    }

    void reflect_y() {
        std::reverse( setup.begin(), setup.end() );
        std::reverse( expected_results.begin(), expected_results.end() );
    }

    void test() const {
        full_map_test( setup, expected_results, time );
    }

    void test_all_transformations() const {
        // Three reflections generate all possible rotations and reflections of
        // the test case
        for( int transform = 0; transform < 8; ++transform ) {
            INFO( "test case transformation: " << transform );
            vision_test_case copy( *this );
            if( transform & 1 ) {
                copy.transpose();
            }
            if( transform & 2 ) {
                copy.reflect_x();
            }
            if( transform & 4 ) {
                copy.reflect_y();
            }
            copy.test();
        }
    }

    void test_all() const {
        // Disabling 3d tests for now since 3d sight casting is actually
        // different (it sees round corners more).
        if( test_3d ) {
            INFO( "using 3d casting" );
            fov_3d = true;
            test_all_transformations();
        }
        {
            INFO( "using 2d casting" );
            fov_3d = false;
            test_all_transformations();
        }
    }
};

static const time_point midnight = calendar::turn_zero + 0_hours;
static const time_point midday = calendar::turn_zero + 12_hours;

// The following characters are used in these setups:
// ' ' - empty, outdoors
// '-' - empty, indoors
// 'U' - player, outdoors
// 'u' - player, indoors
// 'V' - player, with light in inventory
// 'L' - light, indoors
// '#' - wall
// '=' - window frame

TEST_CASE( "vision_daylight", "[shadowcasting][vision]" )
{
    vision_test_case t {
        {
            "   ",
            "   ",
            " U ",
        },
        {
            "444",
            "444",
            "444",
        },
        midday,
        true
    };

    t.test_all();
}

TEST_CASE( "vision_day_indoors", "[shadowcasting][vision]" )
{
    vision_test_case t {
        {
            "###",
            "#u#",
            "###",
        },
        {
            "111",
            "111",
            "111",
        },
        midday,
        true
    };

    t.test_all();
}

TEST_CASE( "vision_light_shining_in", "[shadowcasting][vision]" )
{
    vision_test_case t {
        {
            "##########",
            "#--------#",
            "#u-------#",
            "#--------=",
            "##########",
        },
        {
            "1144444666",
            "1144444466",
            "1144444444",
            "1144444444",
            "1144444444",
        },
        midday,
        false // 3D FOV gives different results here due to it seeing round corners more
    };

    t.test_all();
}

TEST_CASE( "vision_no_lights", "[shadowcasting][vision]" )
{
    vision_test_case t {
        {
            "   ",
            " U ",
        },
        {
            "111",
            "111",
        },
        midnight,
        true
    };

    t.test_all();
}

TEST_CASE( "vision_utility_light", "[shadowcasting][vision]" )
{
    vision_test_case t {
        {
            " L ",
            "   ",
            " U ",
        },
        {
            "444",
            "444",
            "444",
        },
        midnight,
        true
    };

    t.test_all();
}

TEST_CASE( "vision_wall_obstructs_light", "[shadowcasting][vision]" )
{
    vision_test_case t {
        {
            " L ",
            "###",
            " U ",
        },
        {
            "666",
            "111",
            "111",
        },
        midnight,
        true
    };

    t.test_all();
}

TEST_CASE( "vision_wall_can_be_lit_by_player", "[shadowcasting][vision]" )
{
    vision_test_case t {
        {
            " V",
            "  ",
            "  ",
            "##",
            "--",
        },
        {
            "44",
            "44",
            "44",
            "44",
            "66",
        },
        midnight,
        true
    };

    t.test_all();
}

TEST_CASE( "vision_see_wall_in_moonlight", "[shadowcasting][vision]" )
{
    const time_duration till_full_moon = calendar::season_length() / 3;
    // Verify that I've picked the full_moon time correctly.
    CHECK( get_moon_phase( calendar::turn_zero + till_full_moon ) == MOON_FULL );
    // Want a night time
    const int days_till_full_moon = to_days<int>( till_full_moon );

    vision_test_case t {
        {
            "---",
            "###",
            "   ",
            "   ",
            " U ",
        },
        {
            "666",
            "111",
            "111",
            "111",
            "111",
        },
        DAYS( days_till_full_moon ),
        true
    };

    t.test_all();
}
