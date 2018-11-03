#include "catch/catch.hpp"

#include "game.h"
#include "player.h"
#include "field.h"
#include "string.h"
#include "map.h"

#include "map_helpers.h"

template<size_t N>
void full_map_test( const char *const( &setup )[N],
                    const char *const( &expected_results )[N],
                    calendar time )
{
    const ter_id t_brick_wall( "t_brick_wall" );
    const ter_id t_utility_light( "t_utility_light" );
    const efftype_id effect_narcosis( "narcosis" );

    g->place_player( tripoint( 60, 60, 0 ) );
    g->u.clear_effects();
    clear_map();

    REQUIRE( !g->u.is_blind() );
    REQUIRE( !g->u.in_sleep_state() );
    REQUIRE( !g->u.has_effect( effect_narcosis ) );

    g->u.recalc_sight_limits();

    calendar::turn = time;

    int height = N;
    REQUIRE( height > 0 );
    int width = strlen( setup[0] );

    for( const char *line : setup ) {
        REQUIRE( strlen( line ) == static_cast<size_t>( width ) );
    }

    for( const char *line : expected_results ) {
        REQUIRE( strlen( line ) == static_cast<size_t>( width ) );
    }

    tripoint origin;
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            if( setup[y][x] == 'U' ) {
                origin = g->u.pos() - point( x, y );
                break;
            }
        }
    }
    tripoint player_offset = g->u.pos() - origin;
    REQUIRE( setup[player_offset.y][player_offset.x] == 'U' );

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
                case 'U':
                    // Already handled above
                    break;
                default:
                    FAIL( "unexpected setup char '" << setup[y][x] << "'" );
            }
        }
    }

    g->m.build_map_cache( origin.z );
    g->m.update_visibility_cache( origin.z );

    const level_cache &cache = g->m.access_cache( origin.z );
    const visibility_variables &vvcache =
        g->m.get_visibility_variables_cache();

    std::ostringstream fields;
    std::ostringstream transparency;
    std::ostringstream seen;
    std::ostringstream obstructed;
    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            const tripoint p = origin + point( x, y );
            map::ApparentLight al = g->m.apparent_light_helper( cache, p );
            for( auto &pr : g->m.field_at( p ) ) {
                fields << pr.second.name() << ',';
            }
            fields << ' ';
            transparency << cache.transparency_cache[p.x][p.y] << ' ';
            seen << cache.seen_cache[p.x][p.y] << ' ';
            obstructed << ( al.obstructed ? '#' : '.' ) << ' ';
        }
        fields << '\n';
        transparency << '\n';
        seen << '\n';
        obstructed << '\n';
    }

    INFO( "origin: " << origin );
    INFO( "player: " << g->u.pos() );
    INFO( "unimpaired_range: " << g->u.unimpaired_range() );
    INFO( "fields:\n" << fields.str() );
    INFO( "transparency:\n" << transparency.str() );
    INFO( "seen:\n" << seen.str() );
    INFO( "obstructed:\n" << obstructed.str() );

    for( int y = 0; y < height; ++y ) {
        for( int x = 0; x < width; ++x ) {
            const tripoint p = origin + point( x, y );
            four_quadrants lm = cache.lm[p.x][p.y];
            std::string lm_s =
                string_format( "(%.2f,%.2f,%.2f,%.2f)",
                               lm[quadrant::NE], lm[quadrant::SE],
                               lm[quadrant::SW], lm[quadrant::NW] );
            float seen = cache.seen_cache[p.x][p.y];
            INFO( "x=" << x << ", y=" << y <<
                  ", lm=" << lm_s << ", seen=" << seen );
            lit_level level = g->m.apparent_light_at( p, vvcache );
            lit_level expected_level;
            switch( expected_results[y][x] ) {
                case 'D':
                    expected_level = LL_DARK;
                    break;
                case 'L':
                    expected_level = LL_LOW;
                    break;
                case 'B':
                    expected_level = LL_BRIGHT;
                    break;
                case '-':
                    expected_level = LL_BLANK;
                    break;
                default:
                    FAIL( "unexpected result char '" <<
                          expected_results[y][x] << "'" );
            }
            CHECK( level == expected_level );
        }
    }
}

static constexpr int midnight = HOURS( 0 );
static constexpr int midday = HOURS( 12 );

TEST_CASE( "vision_no_lights", "[shadowcasting][vision]" )
{
    constexpr const char *setup[] = {
        "   ",
        "   ",
        " U ",
    };

    constexpr const char *expected_results[] = {
        "LLL",
        "LLL",
        "LBL",
    };

    full_map_test( setup, expected_results, midnight );
}

TEST_CASE( "vision_utility_light", "[shadowcasting][vision]" )
{
    constexpr const char *setup[] = {
        " L ",
        "   ",
        " U ",
    };

    constexpr const char *expected_results[] = {
        "BBB",
        "BBB",
        "BBB",
    };

    full_map_test( setup, expected_results, midnight );
}

TEST_CASE( "vision_wall_obstructs_light", "[shadowcasting][vision]" )
{
    constexpr const char *setup[] = {
        " L ",
        "###",
        " U ",
    };

    constexpr const char *expected_results[] = {
        "---",
        "LLL",
        "LBL",
    };

    full_map_test( setup, expected_results, midnight );
}
