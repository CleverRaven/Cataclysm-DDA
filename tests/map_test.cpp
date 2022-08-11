#include "cata_catch.h"
#include "map.h"

#include <memory>
#include <vector>

#include "avatar.h"
#include "coordinates.h"
#include "enums.h"
#include "game.h"
#include "game_constants.h"
#include "map_helpers.h"
#include "point.h"
#include "type_id.h"

TEST_CASE( "map_coordinate_conversion_functions" )
{
    map &here = get_map();

    tripoint test_point =
        GENERATE( tripoint_zero, tripoint_south, tripoint_east, tripoint_above, tripoint_below );
    tripoint_bub_ms test_bub( test_point );
    int z = GENERATE( 0, 1, -1, OVERMAP_HEIGHT, -OVERMAP_DEPTH );

    // Make sure we're not in the 'easy' case where abs_sub is zero
    if( here.get_abs_sub().x() == 0 ) {
        here.shift( point_east );
    }
    if( here.get_abs_sub().y() == 0 ) {
        here.shift( point_south );
    }
    here.vertical_shift( z );

    CAPTURE( here.get_abs_sub() );

    REQUIRE( here.get_abs_sub().x() != 0 );
    REQUIRE( here.get_abs_sub().y() != 0 );
    REQUIRE( here.get_abs_sub().z() == z );

    point_abs_ms map_origin_ms = project_to<coords::ms>( here.get_abs_sub().xy() );

    tripoint_abs_ms test_abs = map_origin_ms + test_point;

    if( test_abs.z() > OVERMAP_HEIGHT || test_abs.z() < -OVERMAP_DEPTH ) {
        return;
    }

    CAPTURE( test_bub );
    CAPTURE( test_abs );

    // Verify consistency between different implementations
    CHECK( here.getabs( test_bub ) == here.getabs( test_bub.raw() ) );
    CHECK( here.getglobal( test_bub ) == here.getglobal( test_bub.raw() ) );
    CHECK( here.getlocal( test_abs ) == here.getlocal( test_abs.raw() ) );
    CHECK( here.bub_from_abs( test_abs ) == here.bub_from_abs( test_abs.raw() ) );

    CHECK( here.getabs( test_bub ) == here.getglobal( test_bub ).raw() );
    CHECK( here.getlocal( test_abs ) == here.bub_from_abs( test_abs ).raw() );

    // Verify round-tripping
    CHECK( here.getglobal( here.bub_from_abs( test_abs ) ) == test_abs );
    CHECK( here.bub_from_abs( here.getglobal( test_point ) ).raw() == test_point );
}

TEST_CASE( "destroy_grabbed_furniture" )
{
    clear_map();
    avatar &player_character = get_avatar();
    GIVEN( "Furniture grabbed by the player" ) {
        const tripoint test_origin( 60, 60, 0 );
        map &here = get_map();
        player_character.setpos( test_origin );
        const tripoint grab_point = test_origin + tripoint_east;
        here.furn_set( grab_point, furn_id( "f_chair" ) );
        player_character.grab( object_type::FURNITURE, tripoint_east );
        REQUIRE( player_character.get_grab_type() == object_type::FURNITURE );
        WHEN( "The furniture grabbed by the player is destroyed" ) {
            here.destroy( grab_point );
            THEN( "The player's grab is released" ) {
                CHECK( player_character.get_grab_type() == object_type::NONE );
                CHECK( player_character.grab_point == tripoint_zero );
            }
        }
    }
}

TEST_CASE( "map_bounds_checking" )
{
    // FIXME: There are issues with vehicle caching between maps, because
    // vehicles are stored in the global MAPBUFFER which all maps refer to.  To
    // work around the problem we clear the map of vehicles, but this is an
    // inelegant solution.
    clear_map();
    map m;
    tripoint_abs_sm point_away_from_real_map( get_map().get_abs_sub() + point( MAPSIZE_X, 0 ) );
    m.load( point_away_from_real_map, false );
    for( int x = -1; x <= MAPSIZE_X; ++x ) {
        for( int y = -1; y <= MAPSIZE_Y; ++y ) {
            for( int z = -OVERMAP_DEPTH - 1; z <= OVERMAP_HEIGHT + 1; ++z ) {
                INFO( "( " << x << ", " << y << ", " << z << " )" );
                if( x < 0 || x >= MAPSIZE_X ||
                    y < 0 || y >= MAPSIZE_Y ||
                    z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT ) {
                    CHECK( !m.ter( tripoint{ x, y, z } ) );
                } else {
                    CHECK( m.ter( tripoint{ x, y, z } ) );
                }
            }
        }
    }
}

TEST_CASE( "tinymap_bounds_checking" )
{
    // FIXME: There are issues with vehicle caching between maps, because
    // vehicles are stored in the global MAPBUFFER which all maps refer to.  To
    // work around the problem we clear the map of vehicles, but this is an
    // inelegant solution.
    clear_map();
    tinymap m;
    tripoint_abs_sm point_away_from_real_map( get_map().get_abs_sub() + point( MAPSIZE_X, 0 ) );
    m.load( point_away_from_real_map, false );
    for( int x = -1; x <= SEEX * 2; ++x ) {
        for( int y = -1; y <= SEEY * 2; ++y ) {
            for( int z = -OVERMAP_DEPTH - 1; z <= OVERMAP_HEIGHT + 1; ++z ) {
                INFO( "( " << x << ", " << y << ", " << z << " )" );
                if( x < 0 || x >= SEEX * 2 ||
                    y < 0 || y >= SEEY * 2 ||
                    z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT ) {
                    CHECK( !m.ter( tripoint{ x, y, z } ) );
                } else {
                    CHECK( m.ter( tripoint{ x, y, z } ) );
                }
            }
        }
    }
}

TEST_CASE( "place_player_can_safely_move_multiple_submaps" )
{
    // Regression test for the situation where game::place_player would misuse
    // map::shift if the resulting shift exceeded a single submap, leading to a
    // broken active item cache.
    g->place_player( tripoint_zero );
    CHECK( get_map().check_submap_active_item_consistency().empty() );
}
