#include <cstdio>
#include <memory>

#include "avatar.h"
#include "catch/catch.hpp"
#include "calendar.h"
#include "game.h"
#include "player.h"
#include "player_helpers.h"
#include "item.h"
#include "stomach.h"
#include "units.h"
#include "type_id.h"

static void reset_time()
{
    calendar::turn = calendar::start_of_cataclysm;
    player &p = g->u;
    p.set_stored_kcal( p.get_healthy_kcal() );
    p.set_hunger( 0 );
    clear_avatar();
}

static void pass_time( player &p, time_duration amt )
{
    for( auto turns = 1_turns; turns < amt; turns += 1_turns ) {
        calendar::turn += 1_turns;
        p.update_body();
    }
}

static void clear_stomach( player &p )
{
    p.stomach.empty();
    p.guts.empty();
}

static void set_all_vitamins( int target, player &p )
{
    p.vitamin_set( vitamin_id( "vitA" ), target );
    p.vitamin_set( vitamin_id( "vitB" ), target );
    p.vitamin_set( vitamin_id( "vitC" ), target );
    p.vitamin_set( vitamin_id( "iron" ), target );
    p.vitamin_set( vitamin_id( "calcium" ), target );
}

static void print_stomach_contents( player &p, const bool print )
{
    if( !print ) {
        return;
    }
    printf( "stomach: %d guts: %d player: %d/%d hunger: %0.1f\n", p.stomach.get_calories(),
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
    p.eat( f );
}

// how long does it take to starve to death
// player does not thirst or tire or require vitamins
TEST_CASE( "starve_test", "[starve][slow]" )
{
    player &dummy = g->u;
    reset_time();
    clear_stomach( dummy );

    CAPTURE( dummy.metabolic_rate_base() );
    CAPTURE( dummy.base_height() );
    CAPTURE( dummy.get_size() );
    CAPTURE( dummy.height() );
    CAPTURE( dummy.bmi() );
    CAPTURE( dummy.bodyweight() );
    CAPTURE( dummy.age() );
    CAPTURE( dummy.bmr() );

    constexpr int expected_day = 7;
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
    CHECK( day >= expected_day );
    CHECK( day <= expected_day + 1 );
}

// how long does it take to starve to death with extreme metabolism
// player does not thirst or tire or require vitamins
TEST_CASE( "starve_test_hunger3", "[starve][slow]" )
{
    player &dummy = g->u;
    reset_time();
    clear_stomach( dummy );
    while( !( dummy.has_trait( trait_id( "HUNGER3" ) ) ) ) {
        dummy.mutate_towards( trait_id( "HUNGER3" ) );
    }
    clear_stomach( dummy );

    CAPTURE( dummy.metabolic_rate_base() );
    CAPTURE( dummy.base_height() );
    CAPTURE( dummy.height() );
    CAPTURE( dummy.bmi() );
    CAPTURE( dummy.bodyweight() );
    CAPTURE( dummy.age() );
    CAPTURE( dummy.bmr() );

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
    CHECK( day <= 3 );
    CHECK( day >= 2 );
}

// does eating enough food per day keep you alive
TEST_CASE( "all_nutrition_starve_test", "[starve][slow]" )
{
    // change this bool when editing the test
    const bool print_tests = false;
    player &dummy = g->u;
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
    CHECK( dummy.get_stored_kcal() < dummy.get_healthy_kcal() );
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
    player &dummy = g->u;
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

TEST_CASE( "One day of waiting at full calories eats up about bmr of stored calories", "[stomach]" )
{
    const bool print_tests = true;
    player &dummy = g->u;
    reset_time();
    clear_stomach( dummy );
    int kcal_before = dummy.get_stored_kcal();
    dummy.update_body( calendar::turn, calendar::turn + 1_days );
    int kcal_after = dummy.get_stored_kcal();
    CHECK( kcal_before == kcal_after + dummy.bmr() );
}

TEST_CASE( "Stomach calories become stored calories after less than 1 day", "[stomach]" )
{
    const bool print_tests = true;
    const time_duration test_time = 1_days;
    player &dummy = g->u;
    reset_time();
    clear_stomach( dummy );
    int kcal_before = dummy.max_stored_calories() - dummy.bmr();
    dummy.set_stored_kcal( kcal_before );
    dummy.stomach.mod_calories( 1000 );

    const time_point start = calendar::turn;
    for( time_point now = start; now < start + test_time; now += 30_minutes ) {
        dummy.update_body( now, now + 30_minutes );
    }

    int kcal_after = dummy.get_stored_kcal();
    int kcal_expected = kcal_before + 1000 - static_cast<float>( dummy.bmr() ) * test_time / 1_days;
    CAPTURE( dummy.stomach.get_calories() );
    CAPTURE( dummy.guts.get_calories() );
    CHECK( kcal_after >= kcal_expected * 0.95f );
    CHECK( kcal_after <= kcal_expected * 1.05f );
}

TEST_CASE( "Eating food fills up stomach calories", "[stomach]" )
{
    player &dummy = g->u;
    reset_time();
    clear_stomach( dummy );
    item food( "protein_drink", 0, 10 );
    REQUIRE( dummy.compute_effective_nutrients( food ).kcal == 100 );
    int attempts = 10;
    do {
    } while( dummy.eat( food, true ) && --attempts > 0 );
    CAPTURE( dummy.stomach.get_calories() );
    CAPTURE( dummy.guts.get_calories() );
    CHECK( dummy.stomach.get_calories() == 1000 );
    CHECK( dummy.guts.get_calories() == 0 );
}
