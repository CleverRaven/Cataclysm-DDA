#include "catch/catch.hpp"

#include "avatar.h"
#include "avatar_action.h"
#include "calendar.h"
#include "character.h"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "units.h"

static void remain_centered( const tripoint &center, avatar &dummy )
{
    if( dummy.pos().x < SEEX * int( MAPSIZE / 2 ) ||
        dummy.pos().y < SEEY * int( MAPSIZE / 2 ) ||
        dummy.pos().x >= SEEX * ( 1 + int( MAPSIZE / 2 ) ) ||
        dummy.pos().y >= SEEY * ( 1 + int( MAPSIZE / 2 ) ) ) {
        dummy.setpos( center );
        // Verify that only the player is present.
        REQUIRE( g->num_creatures() == 1 );
    }
}

static int distance_test( avatar &dummy, const time_duration &time_spent )
{
    clear_map();
    dummy.set_stored_kcal( dummy.get_healthy_kcal() );
    dummy.stomach.empty();
    dummy.guts.empty();
    dummy.moves = dummy.get_speed();
    const tripoint center{ 65, 65, 0 };
    const time_point start = calendar::turn;
    int calories_spent = 0;
    int tiles_moved = 0;
    while( start + time_spent > calendar::turn ) {
        const int calories_before = dummy.get_stored_kcal();
        avatar_action::move( dummy, g->m, dummy.pos() + tripoint_east );
        ++tiles_moved;
        remain_centered( center, dummy );

        dummy.update_body();
        dummy.process_turn();
        calendar::turn += 1_turns;
        calories_spent += calories_before - dummy.get_stored_kcal();
    }
    printf( "player weight: %.2f kg, bmr: %d\n", units::to_kilogram( dummy.get_weight() ),
            dummy.get_bmr() );
    return calories_spent;
}

static bool between( const int num, const int lower, const int high )
{
    return num < high && num > lower;
}

static void test_carry( units::mass carry_weight, int min_cal, int max_cal,
                        character_movemode mode = character_movemode::CMM_WALK )
{
    item bag( "bigback" );
    REQUIRE( carry_weight >= bag.weight() );
    carry_weight -= bag.weight();
    clear_avatar();
    avatar &dummy = g->u;
    dummy.reset_move_mode();
    if( mode == character_movemode::CMM_RUN ) {
        dummy.toggle_run_mode();
    }
    dummy.i_add( bag );
    dummy.wear( bag, false );
    dummy.i_add( item( "thread", calendar::turn, units::to_gram( carry_weight ) ) );
    const int cal_spent = distance_test( dummy, 1_hours );
    CAPTURE( cal_spent );
    CHECK( between( cal_spent, min_cal, max_cal ) );
}

TEST_CASE( "naked_walk", "[hike]" )
{
    clear_avatar();
    avatar &dummy = g->u;
    const int cal_spent = distance_test( dummy, 1_hours );
    CAPTURE( cal_spent );
    CHECK( between( cal_spent, 204, 234 ) );
}

TEST_CASE( "naked_run", "[hike]" )
{
    clear_avatar();
    avatar &dummy = g->u;
    dummy.toggle_run_mode();
    const int cal_spent = distance_test( dummy, 1_hours );
    CAPTURE( cal_spent );
    CHECK( between( cal_spent, 1032, 1200 ) );
}

TEST_CASE( "walk_carry", "[hike]" )
{
    SECTION( "10 kg" ) {
        test_carry( 10000_gram, 230, 260 );
    }
    SECTION( "20 kg" ) {
        test_carry( 20000_gram, 242, 272 );
    }
    SECTION( "30 kg" ) {
        test_carry( 30000_gram, 270, 300 );
    }
    SECTION( "40 kg" ) {
        test_carry( 40000_gram, 306, 336 );
    }
    SECTION( "60 kg" ) {
        test_carry( 60000_gram, 412, 442 );
    }
}

TEST_CASE( "run_carry", "[hike]" )
{
    const character_movemode run = character_movemode::CMM_RUN;
    SECTION( "10 kg" ) {
        test_carry( 10000_gram, 1120, 1140, run );
    }
    SECTION( "20 kg" ) {
        test_carry( 20000_gram, 1180, 1200, run );
    }
    SECTION( "30 kg" ) {
        test_carry( 30000_gram, 1260, 1280, run );
    }
    SECTION( "40 kg" ) {
        test_carry( 40000_gram, 1400, 1420, run );
    }
    SECTION( "60 kg" ) {
        test_carry( 60000_gram, 1880, 1900, run );
    }
}
