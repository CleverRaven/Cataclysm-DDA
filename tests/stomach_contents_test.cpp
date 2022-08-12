#include <cstdio>
#include <iosfwd>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "item.h"
#include "player_helpers.h"
#include "stomach.h"
#include "string_formatter.h"
#include "type_id.h"
#include "units.h"

static const efftype_id effect_tapeworm( "tapeworm" );

static const trait_id trait_HUNGER3( "HUNGER3" );

static const vitamin_id vitamin_calcium( "calcium" );
static const vitamin_id vitamin_iron( "iron" );
static const vitamin_id vitamin_test_vit_fast( "test_vit_fast" );
static const vitamin_id vitamin_test_vit_slow( "test_vit_slow" );
static const vitamin_id vitamin_vitC( "vitC" );

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
    p.vitamin_set( vitamin_vitC, target );
    p.vitamin_set( vitamin_iron, target );
    p.vitamin_set( vitamin_calcium, target );
}

static void reset_daily_vitamins( Character &p )
{
    p.reset_daily_vitamin( vitamin_vitC );
    p.reset_daily_vitamin( vitamin_iron );
    p.reset_daily_vitamin( vitamin_calcium );
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
static void eat_all_nutrients( Character &you )
{
    // Vitamin target: 100% DV -- or 96 vitamin "units" since all vitamins currently decay every 15m.
    // Energy target: 2100 kcal -- debug target will be completely sedentary.
    item f( "debug_nutrition" );
    you.consume( f );
}

// how long does it take to starve to death
// player does not thirst or tire or require vitamins
TEST_CASE( "starve_test", "[starve][slow]" )
{
    Character &dummy = get_player_character();
    reset_time();
    clear_stomach( dummy );
    dummy.reset_activity_level();
    calendar::turn += 1_seconds;
    dummy.update_body( calendar::turn, calendar::turn );
    dummy.set_activity_level( 1.0 );

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

// do vitamins get processed correctly every day
TEST_CASE( "vitamin_process", "[vitamins]" )
{
    Character &subject = get_avatar();
    clear_avatar();
    reset_time();
    clear_stomach( subject );

    set_all_vitamins( 0, subject );
    REQUIRE( subject.vitamin_get( vitamin_iron ) == 0 );
    REQUIRE( subject.vitamin_get( vitamin_calcium ) == 0 );
    REQUIRE( subject.vitamin_get( vitamin_vitC ) == 0 );
    REQUIRE( subject.vitamin_get( vitamin_test_vit_fast ) == 0 );
    REQUIRE( subject.vitamin_get( vitamin_test_vit_slow ) == 0 );


    pass_time( subject, 1_days );

    // check
    CHECK( subject.vitamin_get( vitamin_iron ) <= -95 );
    CHECK( subject.vitamin_get( vitamin_calcium ) <= -95 );
    CHECK( subject.vitamin_get( vitamin_vitC ) <= -95 );
    CHECK( subject.vitamin_get( vitamin_iron ) >= -97 );
    CHECK( subject.vitamin_get( vitamin_calcium ) >= -97 );
    CHECK( subject.vitamin_get( vitamin_vitC ) >= -97 );

    // slow vitamin drains every 90 minutes or 16 units in a day
    CHECK( subject.vitamin_get( vitamin_test_vit_slow ) <= -15 );
    CHECK( subject.vitamin_get( vitamin_test_vit_slow ) >= -17 );

    // fast vitamin drains every 5 minutes or 288 units in a day
    CHECK( subject.vitamin_get( vitamin_test_vit_fast ) <= -287 );
    CHECK( subject.vitamin_get( vitamin_test_vit_fast ) >= -289 );



}

// do vitamins you eat get processed correctly
TEST_CASE( "vitamin_equilibrium", "[vitamins]" )
{
    Character &subject = get_avatar();
    clear_avatar();
    reset_time();
    clear_stomach( subject );

    set_all_vitamins( -100, subject );
    REQUIRE( subject.vitamin_get( vitamin_vitC ) == -100 );
    REQUIRE( subject.vitamin_get( vitamin_calcium ) == -100 );
    REQUIRE( subject.vitamin_get( vitamin_iron ) == -100 );
    item f( "debug_orange" );

    // check that 100% of daily vit C is by default 96 units
    CHECK( subject.compute_effective_nutrients( f ).get_vitamin( vitamin_vitC ) == 96 );
    subject.consume( f );


    pass_time( subject, 1_days );

    // check if something with 100% RDA will keep you at equilibrium
    CHECK( subject.vitamin_get( vitamin_iron ) <= -99 );
    CHECK( subject.vitamin_get( vitamin_calcium ) <= -99 );
    CHECK( subject.vitamin_get( vitamin_vitC ) <= -99 );
    CHECK( subject.vitamin_get( vitamin_iron ) >= -101 );
    CHECK( subject.vitamin_get( vitamin_calcium ) >= -101 );
    CHECK( subject.vitamin_get( vitamin_vitC ) >= -101 );

}

// do vitamins you eat get processed correctly
TEST_CASE( "vitamin_multivitamin", "[vitamins]" )
{
    Character &subject = get_avatar();
    clear_avatar();
    reset_time();
    clear_stomach( subject );

    set_all_vitamins( -100, subject );
    REQUIRE( subject.vitamin_get( vitamin_vitC ) == -100 );
    REQUIRE( subject.vitamin_get( vitamin_calcium ) == -100 );
    REQUIRE( subject.vitamin_get( vitamin_iron ) == -100 );
    item f( "debug_vitamins" );

    subject.consume( f );

    pass_time( subject, 1_days );

    // check if something with 100% RDA will keep you at equilibrium
    CHECK( subject.vitamin_get( vitamin_iron ) <= -99 );
    CHECK( subject.vitamin_get( vitamin_calcium ) <= -99 );
    CHECK( subject.vitamin_get( vitamin_vitC ) <= -99 );
    CHECK( subject.vitamin_get( vitamin_iron ) >= -101 );
    CHECK( subject.vitamin_get( vitamin_calcium ) >= -101 );
    CHECK( subject.vitamin_get( vitamin_vitC ) >= -101 );

}

// do vitamins you eat get processed correctly
TEST_CASE( "vitamin_daily", "[vitamins]" )
{
    Character &subject = get_avatar();
    clear_avatar();
    reset_time();
    clear_stomach( subject );
    subject.set_daily_health( 0 );

    set_all_vitamins( -100, subject );
    reset_daily_vitamins( subject );
    REQUIRE( subject.vitamin_get( vitamin_vitC ) == -100 );
    REQUIRE( subject.vitamin_get( vitamin_calcium ) == -100 );
    REQUIRE( subject.vitamin_get( vitamin_iron ) == -100 );
    REQUIRE( subject.get_daily_vitamin( vitamin_vitC ) == 0 );
    REQUIRE( subject.get_daily_vitamin( vitamin_calcium ) == 0 );
    REQUIRE( subject.get_daily_vitamin( vitamin_iron ) == 0 );
    REQUIRE( subject.get_daily_health() == 0 );
    item f( "debug_vitamins" );

    subject.consume( f );

    int hours = 0;
    while( hours < 72 ) {
        pass_time( subject, 1_hours );
        hours++;
        // check vitamins to see if health has updated
        if( subject.get_daily_vitamin( vitamin_vitC ) == 0 &&
            subject.get_daily_vitamin( vitamin_calcium ) == 0 &&
            subject.get_daily_vitamin( vitamin_iron ) == 0 ) {
            break;
        }
        //otherwise clean up any other health changes that may have happened
        subject.set_daily_health( 0 );

    }

    // check if after a day health is up and vitamins are reset
    CHECK( subject.get_daily_vitamin( vitamin_vitC ) == 0 );
    CHECK( subject.get_daily_vitamin( vitamin_calcium ) == 0 );
    CHECK( subject.get_daily_vitamin( vitamin_iron ) == 0 );
    // get that vitamin health bonus is up by 6 with maybe a recent reduction on the same timecheck
    CHECK( subject.get_daily_health() >= 5 );

}

// how long does it take to starve to death with extreme metabolism
// player does not thirst or tire or require vitamins
TEST_CASE( "starve_test_hunger3", "[starve][slow]" )
{
    Character &dummy = get_player_character();
    reset_time();
    clear_stomach( dummy );
    while( !dummy.has_trait( trait_HUNGER3 ) ) {
        dummy.mutate_towards( trait_HUNGER3 );
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
        printf( "vitamins: vitC %d calcium %d iron %d\n",
                dummy.vitamin_get( vitamin_vitC ), dummy.vitamin_get( vitamin_calcium ),
                dummy.vitamin_get( vitamin_iron ) );
        printf( "\n" );
        print_stomach_contents( dummy, print_tests );
        printf( "\n" );
    }
    CHECK( dummy.get_stored_kcal() >= dummy.get_healthy_kcal() );
    // We need to account for a day's worth of error since we're passing a day at a time and we are
    // close to 0 which is the max value for some vitamins

    // This test could be a lot better bounds are really wide on it
    CHECK( dummy.vitamin_get( vitamin_vitC ) >= -100 );
    CHECK( dummy.vitamin_get( vitamin_iron ) >= -100 );
    CHECK( dummy.vitamin_get( vitamin_calcium ) >= -100 );
}

TEST_CASE( "tape_worm_halves_nutrients" )
{
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
    while( !dummy.has_trait( trait_HUNGER3 ) ) {
        dummy.mutate_towards( trait_HUNGER3 );
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
