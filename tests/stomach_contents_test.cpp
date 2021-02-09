#include <cstdio>
#include <iosfwd>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "catch/catch.hpp"
#include "character.h"
#include "item.h"
#include "player.h"
#include "player_helpers.h"
#include "stomach.h"
#include "string_formatter.h"
#include "type_id.h"
#include "units.h"

static void reset_time()
{
    calendar::turn = calendar::start_of_cataclysm;
    Character &player_character = get_player_character();
    player_character.set_stored_kcal( player_character.get_healthy_kcal() );
    player_character.set_hunger( 0 );
    clear_avatar();
}

static void pass_time( Character &p, time_duration amt )
{
    for( time_duration turns = 1_turns; turns < amt; turns += 1_turns ) {
        calendar::turn += 1_turns;
        p.update_body();
    }
}

static void clear_stomach( Character &p )
{
    p.stomach.empty();
    p.guts.empty();
}

static void set_all_vitamins( int target, Character &p )
{
    p.vitamin_set( vitamin_id( "vitA" ), target );
    p.vitamin_set( vitamin_id( "vitB" ), target );
    p.vitamin_set( vitamin_id( "vitC" ), target );
    p.vitamin_set( vitamin_id( "iron" ), target );
    p.vitamin_set( vitamin_id( "calcium" ), target );
}

// time (in minutes) it takes for the player to feel hungry
// passes time on the calendar
static time_duration time_until_hungry( Character &p )
{
    unsigned int thirty_minutes = 0;
    do {
        p.set_sleep_deprivation( 0 );
        p.set_fatigue( 0 );
        pass_time( p, 30_minutes );
        thirty_minutes++;
    } while( p.get_hunger() < 40 ); // hungry
    return thirty_minutes * 30_minutes;
}

static void print_stomach_contents( Character &p, const bool print )
{
    if( !print ) {
        return;
    }
    printf( "stomach: %d guts: %d player: %d/%d hunger: %d\n", p.stomach.get_calories(),
            p.guts.get_calories(), p.get_stored_kcal(), p.get_healthy_kcal(), p.get_hunger() );
    printf( "stomach: %d mL/ %d mL guts %d mL/ %d mL\n",
            units::to_milliliter<int>( p.stomach.contains() ),
            units::to_milliliter<int>( p.stomach.capacity( p ) ),
            units::to_milliliter<int>( p.guts.contains() ),
            units::to_milliliter<int>( p.guts.capacity( p ) ) );
    printf( "metabolic rate: %.2f\n", p.metabolic_rate() );
}

// this represents an amount of food you can eat to keep you fed for an entire day
// accounting for appropriate vitamins
static void eat_all_nutrients( player &p )
{
    // Vitamin target: 100% DV -- or 96 vitamin "units" since all vitamins currently decay every 15m.
    // Energy target: 2100 kcal -- debug target will be completely sedentary.
    item f( "debug_nutrition" );
    p.consume( f );
}

// how long does it take to starve to death
// player does not thirst or tire or require vitamins
TEST_CASE( "starve_test", "[starve][slow]" )
{
    Character &dummy = get_player_character();
    reset_time();
    clear_stomach( dummy );

    CAPTURE( dummy.metabolic_rate_base() );
    CAPTURE( dummy.activity_level_str() );
    CAPTURE( dummy.base_height() );
    CAPTURE( dummy.get_size() );
    CAPTURE( dummy.height() );
    CAPTURE( dummy.get_bmi() );
    CAPTURE( dummy.bodyweight() );
    CAPTURE( dummy.age() );
    CAPTURE( dummy.get_bmr() );

    // A specific BMR isn't the real target of this test, the number of days
    // is, but it helps to debug the test faster if this value is wrong.
    REQUIRE( dummy.get_bmr() == 1738 );

    constexpr int expected_day = 36;
    int day = 0;
    std::vector<std::string> results;

    do {
        results.push_back( string_format( "\nday %d: %d", day, dummy.get_stored_kcal() ) );
        pass_time( dummy, 1_days );
        dummy.set_thirst( 0 );
        dummy.set_fatigue( 0 );
        set_all_vitamins( 0, dummy );
        day++;
    } while( dummy.get_stored_kcal() > 0 && day < expected_day * 2 );
    CAPTURE( results );
    CHECK( day == expected_day );
}

// how long does it take to starve to death with extreme metabolism
// player does not thirst or tire or require vitamins
TEST_CASE( "starve_test_hunger3", "[starve][slow]" )
{
    Character &dummy = get_player_character();
    reset_time();
    clear_stomach( dummy );
    while( !( dummy.has_trait( trait_id( "HUNGER3" ) ) ) ) {
        dummy.mutate_towards( trait_id( "HUNGER3" ) );
    }
    clear_stomach( dummy );

    CAPTURE( dummy.metabolic_rate_base() );
    CAPTURE( dummy.activity_level_str() );
    CAPTURE( dummy.base_height() );
    CAPTURE( dummy.height() );
    CAPTURE( dummy.get_bmi() );
    CAPTURE( dummy.bodyweight() );
    CAPTURE( dummy.age() );
    CAPTURE( dummy.get_bmr() );

    std::vector<std::string> results;
    unsigned int day = 0;

    do {
        results.push_back( string_format( "\nday %d: %d", day, dummy.get_stored_kcal() ) );
        pass_time( dummy, 1_days );
        dummy.set_thirst( 0 );
        dummy.set_fatigue( 0 );
        set_all_vitamins( 0, dummy );
        day++;
    } while( dummy.get_stored_kcal() > 0 );

    CAPTURE( results );
    CHECK( day <= 12 );
    CHECK( day >= 10 );
}

// does eating enough food per day keep you alive
TEST_CASE( "all_nutrition_starve_test", "[starve][slow]" )
{
    // change this bool when editing the test
    const bool print_tests = false;
    avatar &dummy = get_avatar();
    reset_time();
    clear_stomach( dummy );
    eat_all_nutrients( dummy );
    if( print_tests ) {
        printf( "\n\n" );
    }

    for( unsigned int day = 0; day <= 20; day++ ) {
        if( print_tests ) {
            printf( "day %u: %d\n", day, dummy.get_stored_kcal() );
        }
        pass_time( dummy, 1_days );
        dummy.set_thirst( 0 );
        dummy.set_fatigue( 0 );
        eat_all_nutrients( dummy );
        print_stomach_contents( dummy, print_tests );
    }
    if( print_tests ) {
        printf( "vitamins: vitA %d vitB %d vitC %d calcium %d iron %d\n",
                dummy.vitamin_get( vitamin_id( "vitA" ) ), dummy.vitamin_get( vitamin_id( "vitB" ) ),
                dummy.vitamin_get( vitamin_id( "vitC" ) ), dummy.vitamin_get( vitamin_id( "calcium" ) ),
                dummy.vitamin_get( vitamin_id( "iron" ) ) );
        printf( "\n" );
        print_stomach_contents( dummy, print_tests );
        printf( "\n" );
    }
    CHECK( dummy.get_stored_kcal() >= dummy.get_healthy_kcal() );
    // We need to account for a day's worth of error since we're passing a day at a time and we are
    // close to 0 which is the max value for some vitamins
    CHECK( dummy.vitamin_get( vitamin_id( "vitA" ) ) >= -100 );
    CHECK( dummy.vitamin_get( vitamin_id( "vitB" ) ) >= -100 );
    CHECK( dummy.vitamin_get( vitamin_id( "vitC" ) ) >= -100 );
    CHECK( dummy.vitamin_get( vitamin_id( "iron" ) ) >= -100 );
    CHECK( dummy.vitamin_get( vitamin_id( "calcium" ) ) >= -100 );
}

TEST_CASE( "tape_worm_halves_nutrients" )
{
    const efftype_id effect_tapeworm( "tapeworm" );
    const bool print_tests = false;
    avatar &dummy = get_avatar();
    reset_time();
    clear_stomach( dummy );
    eat_all_nutrients( dummy );
    print_stomach_contents( dummy, print_tests );
    int regular_kcal = dummy.stomach.get_calories();
    clear_stomach( dummy );
    dummy.add_effect( effect_tapeworm, 1_days );
    eat_all_nutrients( dummy );
    print_stomach_contents( dummy, print_tests );
    int tapeworm_kcal = dummy.stomach.get_calories();

    CHECK( tapeworm_kcal == regular_kcal / 2 );
}

// reasonable length of time to pass before hunger sets in
TEST_CASE( "hunger" )
{
    // change this bool when editing the test
    const bool print_tests = false;
    avatar &dummy = get_avatar();
    reset_time();
    clear_stomach( dummy );
    dummy.initialize_stomach_contents();
    dummy.clear_effects();

    if( print_tests ) {
        printf( "\n\n" );
    }
    print_stomach_contents( dummy, print_tests );
    int hunger_time = to_minutes<int>( time_until_hungry( dummy ) );
    if( print_tests ) {
        printf( "%d minutes til hunger sets in\n", hunger_time );
        print_stomach_contents( dummy, print_tests );
        printf( "eat 2 cooked meat\n" );
    }
    CHECK( hunger_time <= 270 );
    CHECK( hunger_time >= 240 );
    item f( "meat_cooked" );
    dummy.consume( f );
    f = item( "meat_cooked" );
    dummy.consume( f );
    dummy.set_thirst( 0 );
    dummy.update_body();
    print_stomach_contents( dummy, print_tests );
    hunger_time = to_minutes<int>( time_until_hungry( dummy ) );
    if( print_tests ) {
        printf( "%d minutes til hunger sets in\n", hunger_time );
        print_stomach_contents( dummy, print_tests );
        printf( "eat 2 beansnrice\n" );
    }
    CHECK( hunger_time <= 240 );
    CHECK( hunger_time >= 210 );
    f = item( "beansnrice" );
    dummy.consume( f );
    f = item( "beansnrice" );
    dummy.consume( f );
    dummy.update_body();
    print_stomach_contents( dummy, print_tests );
    hunger_time = to_minutes<int>( time_until_hungry( dummy ) );
    if( print_tests ) {
        printf( "%d minutes til hunger sets in\n", hunger_time );
    }
    CHECK( hunger_time <= 285 );
    CHECK( hunger_time >= 240 );
    if( print_tests ) {
        print_stomach_contents( dummy, print_tests );
        printf( "eat 16 veggy\n" );
    }
    for( int i = 0; i < 16; i++ ) {
        f = item( "veggy" );
        dummy.consume( f );
    }
    dummy.update_body();
    print_stomach_contents( dummy, print_tests );
    hunger_time = to_minutes<int>( time_until_hungry( dummy ) );
    if( print_tests ) {
        printf( "%d minutes til hunger sets in\n", hunger_time );
        print_stomach_contents( dummy, print_tests );
    }
    CHECK( hunger_time <= 390 );
    CHECK( hunger_time >= 330 );
    if( print_tests ) {
        printf( "eat 16 veggy with extreme metabolism\n" );
    }
    while( !( dummy.has_trait( trait_id( "HUNGER3" ) ) ) ) {
        dummy.mutate_towards( trait_id( "HUNGER3" ) );
    }
    for( int i = 0; i < 16; i++ ) {
        f = item( "veggy" );
        dummy.consume( f );
    }
    dummy.update_body();
    print_stomach_contents( dummy, print_tests );
    hunger_time = to_minutes<int>( time_until_hungry( dummy ) );
    if( print_tests ) {
        printf( "%d minutes til hunger sets in\n", hunger_time );
        print_stomach_contents( dummy, print_tests );
    }
    CHECK( hunger_time <= 240 );
    CHECK( hunger_time >= 180 );
}
