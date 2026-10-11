#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "map.h"
#include "map_helpers.h"
#include "npc.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

static const efftype_id effect_sleep( "sleep" );

static const furn_str_id furn_f_bed( "f_bed" );

TEST_CASE( "no_npc_food_sleepiness_stops_short_of_dead_tired", "[npc][needs][sleep]" )
{
    clear_map_without_vision();
    override_option no_food( "NO_NPC_FOOD", "true" );
    npc &guy = spawn_npc( point_bub_ms( 50, 50 ), "test_talker" );
    clear_character( guy, true );
    REQUIRE_FALSE( guy.needs_food() );

    SECTION( "tiredness still rises past tired" ) {
        const int start = static_cast<int>( sleepiness_levels::TIRED ) + 10;
        guy.set_sleepiness( start );
        guy.update_needs( 12 );
        CHECK( guy.get_sleepiness() > start );
    }
    SECTION( "it stops just short of dead tired, with no deprivation" ) {
        guy.set_sleepiness( static_cast<int>( sleepiness_levels::DEAD_TIRED ) + 50 );
        guy.set_sleep_deprivation( SLEEP_DEPRIVATION_MINOR );
        guy.update_needs( 12 );
        CHECK( guy.get_sleepiness() == static_cast<int>( sleepiness_levels::DEAD_TIRED ) - 1 );
        CHECK( guy.get_sleep_deprivation() == 0 );
    }
    SECTION( "already past where tiredness stops rising, it comes down too" ) {
        guy.set_sleepiness( 1100 );
        guy.set_sleep_deprivation( SLEEP_DEPRIVATION_MINOR );
        guy.update_needs( 12 );
        CHECK( guy.get_sleepiness() == static_cast<int>( sleepiness_levels::DEAD_TIRED ) - 1 );
        CHECK( guy.get_sleep_deprivation() == 0 );
    }
    SECTION( "asleep, lack of sleep it already had goes away" ) {
        guy.add_effect( effect_sleep, 1_hours );
        guy.set_sleepiness( 300 );
        guy.set_sleep_deprivation( SLEEP_DEPRIVATION_MINOR );
        guy.update_needs( 12 );
        CHECK( guy.get_sleep_deprivation() == 0 );
    }
}

TEST_CASE( "sleep_readiness_reflects_bed_and_stimulants", "[npc][sleep]" )
{
    clear_map_without_vision();
    npc &guy = spawn_npc( point_bub_ms( 50, 50 ), "test_talker" );
    clear_character( guy, true );
    get_map().furn_set( guy.pos_bub(), furn_f_bed );
    guy.set_sleepiness( 600 );
    guy.set_stim( 0 );
    // well past the rng( -8, 8 ) of a sleep roll either way
    CHECK( guy.sleep_readiness() > 8 );
    guy.set_stim( 50 );
    CHECK( guy.sleep_readiness() < -8 );
}
