#include "catch/catch.hpp"

#include "game.h"
#include "player.h"
#include "field.h"
#include "string.h"
#include "map.h"

#include "map_helpers.h"

void full_map_test( const std::vector<std::string> &setup,
                    const std::vector<std::string> &expected_results,
                    calendar time )
{
    const ter_id t_brick_wall( "t_brick_wall" );
    const ter_id t_window_frame( "t_window_frame" );
    const ter_id t_floor( "t_floor" );
    const ter_id t_utility_light( "t_utility_light" );
    const efftype_id effect_narcosis( "narcosis" );

    g->place_player( tripoint( 60, 60, 0 ) );
    g->reset_light_level();
    g->u.clear_effects();
    clear_map();

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
            if( setup[y][x] == 'U' || setup[y][x] == 'u' ) {
                origin = g->u.pos() - point( x, y );
                break;
            }
        }
    }

    {
        // Sanity check on player placement
        tripoint player_offset = g->u.pos() - origin;
        REQUIRE( player_offset.y >= 0 );
        REQUIRE( player_offset.y < height );
        REQUIRE( player_offset.x >= 0 );
        REQUIRE( player_offset.x < width );
        char player_char = setup[player_offset.y][player_offset.x];
        REQUIRE( ( player_char == 'U' || player_char == 'u' ) );
    }

    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            const tripoint p = origin + point( x, y );
            switch( setup[y][x] ) {
                case ' ':
                    break;
                case 'L':
                    g->m.ter_set( p, t_utility_light );
                    break;
                case '#':
                    g->m.ter_set( p, t_brick_wall );
                    break;
                case '=':
                    g->m.ter_set( p, t_window_frame );
                    break;
                case '-':
                case 'u':
                    g->m.ter_set( p, t_floor );
                    break;
                case 'U':
                    // Already handled above
                    break;
                default:
                    FAIL( "unexpected setup char '" << setup[y][x] << "'" );
            }
        }
    }

    // We have to run thw whole thing twice, because the first time through the
    // player's vision_threshold is based on the previous lighting level (so
    // they might, for example, have poor nightvision due to having just been
    // in daylight)
    g->m.update_visibility_cache( origin.z );
    g->m.build_map_cache( origin.z );
    g->m.update_visibility_cache( origin.z );
    g->m.build_map_cache( origin.z );

    const level_cache &cache = g->m.access_cache( origin.z );
    const visibility_variables &vvcache =
        g->m.get_visibility_variables_cache();

    std::ostringstream fields;
    std::ostringstream transparency;
    std::ostringstream seen;
    std::ostringstream lm;
    std::ostringstream apparent_light;
    std::ostringstream obstructed;
    transparency << std::setprecision( 3 );
    seen << std::setprecision( 3 );
    apparent_light << std::setprecision( 3 );

    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            const tripoint p = origin + point( x, y );
            map::ApparentLight al = g->m.apparent_light_helper( cache, p );
            for( auto &pr : g->m.field_at( p ) ) {
                fields << pr.second.name() << ',';
            }
            fields << ' ';
            transparency << std::setw( 6 )
                         << cache.transparency_cache[p.x][p.y] << ' ';
            seen << std::setw( 6 ) << cache.seen_cache[p.x][p.y] << ' ';
            four_quadrants this_lm = cache.lm[p.x][p.y];
            std::string lm_s =
                string_format( "(%.2f,%.2f,%.2f,%.2f)",
                               this_lm[quadrant::NE], this_lm[quadrant::SE],
                               this_lm[quadrant::SW], this_lm[quadrant::NW] );
            lm << lm_s << ' ';
            apparent_light << std::setw( 6 ) << al.apparent_light << ' ';
            obstructed << ( al.obstructed ? '#' : '.' ) << ' ';
        }
        fields << '\n';
        transparency << '\n';
        seen << '\n';
        lm << '\n';
        apparent_light << '\n';
        obstructed << '\n';
    }

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

    bool success = true;
    std::ostringstream expected;
    std::ostringstream observed;

    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            const tripoint p = origin + point( x, y );
            lit_level level = g->m.apparent_light_at( p, vvcache );
            const char exp_char = expected_results[y][x];
            if( exp_char < '0' || exp_char > '9' ) {
                FAIL( "unexpected result char '" <<
                      expected_results[y][x] << "'" );
            }
            int expected_level = exp_char - '0';

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
    calendar time;

    void test_all() {
        full_map_test( setup, expected_results, time );
    }
};

static constexpr int midnight = HOURS( 0 );
static constexpr int midday = HOURS( 12 );

// The following characters are used in these setups:
// ' ' - empty, outdoors
// '-' - empty, indoors
// 'U' - player, outdoors
// 'u' - player, indoors
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
        midday
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
            "141",
            "111",
        },
        midday
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
            "1144444166",
            "1144444466",
            "1444444444",
            "1144444444",
            "1144444444",
        },
        midday
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
            "141",
        },
        midnight
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
        midnight
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
            "141",
        },
        midnight
    };

    t.test_all();
}
