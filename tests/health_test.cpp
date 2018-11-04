#include "catch/catch.hpp"

#include "player.h"
#include "npc.h"
#include "item.h"

#include <sstream>

static void test_diet( const time_duration dur, npc &dude,
                       const std::array<int, 4> hmod_changes_per_day, int min, int max )
{
    std::vector<int> health_samples;
    for( time_duration i = 0_turns; i < dur; i += 1_days ) {
        const size_t index = to_days<int>( i ) % 4;
        // They lightly correspond to breakfast, dinner, supper, night snack
        // No, they don't. The correspond to one meal each day.
        dude.mod_healthy_mod( hmod_changes_per_day[ index ],
                              sgn( hmod_changes_per_day[ index ] ) * 200 );
        dude.update_health();
        health_samples.emplace_back( dude.get_healthy() );
    }

    std::stringstream ss;
    for( int i : health_samples ) {
        ss << i << ", ";
    }
    INFO( "Health samples: " << ss.str() );
    CHECK( dude.get_healthy() >= min );
    CHECK( dude.get_healthy() <= max );
}

// Maximum possible health in feasible environment
TEST_CASE( "max_healthy_mod_feasible", "[health]" )
{
    standard_npc dude( "healthy eater" );
    // One pear is 3 healthy, 13 nutrition
    // 288 nutrition per day / 13 per pear = ~22, let's round up to 24
    // 6 pears per meal, +18 health
    // This must result in health very close to maximum possible
    // If it doesn't, maximum health isn't achievable outside debug
    test_diet( calendar::year_length(), dude, {{ 18, 18, 18, 18 }}, 175, 200 );
}

// Terrible diet, worst feasible without hardcore drugs
TEST_CASE( "junk_food_diet", "[health]" )
{
    standard_npc dude( "junk eater" );
    // French fries are -1 healthy, 8 nutrition
    // 300 / 8 = ~38 per day, round up to 40, for -10 health per meal
    // Of course we're not skipping the night snack
    // This should be in the bottom 25% of possible health
    test_diet( calendar::year_length(), dude, {{ -10, -10, -10, -10 }}, -200, -100 );
}

// Typical diet of an established character
TEST_CASE( "oat_diet", "[health]" )
{
    standard_npc dude( "oat eater" );
    // Oatmeal is 1 healthy, 48 nutriton
    // 6 oats per day, we're skipping night snack
    // Low, but above 0 (because oats are above 0)
    test_diet( calendar::year_length(), dude, {{ 2, 2, 2, 0 }}, 0, 30 );
}

// A character that eats one meal per day, like that fad diet
// But make the meal healthy
TEST_CASE( "fasting_breakfast", "[health]" )
{
    standard_npc dude( "fasting eater" );
    // Cooked buckwheats are 2 healthy / 50 nutrition
    // 6 per day
    // Noticeably healthy, but not overwhelming
    test_diet( calendar::year_length(), dude, {{ 12, 0, 0, 0 }}, 50, 100 );
}

// A junk food junkie who switched to a healthy diet for a week
TEST_CASE( "recovering_health", "[health]" )
{
    standard_npc dude( "recovering junk eater" );
    dude.set_healthy( -100 );
    // Beans and rice * 3 + broccoli = 3 * 1 + 2 healthy
    // 3 meals per day
    // Just a week should be enough to stop dying, but not enough to get healthy
    test_diet( 7_days, dude, {{ 5, 5, 5, 0 }}, -50, 0 );
}
