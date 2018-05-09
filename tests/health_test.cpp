#include "catch/catch.hpp"

#include "player.h"
#include "npc.h"
#include "item.h"

#include <sstream>

static void test_diet( size_t num_days, npc &dude, const std::array<int, 4> hmod_changes_per_day,
                       int min, int max )
{
    size_t num_iters = DAYS( num_days ) / HOURS( 6 );
    std::vector<int> health_samples;
    for( size_t i = 0; i < num_days; i++ ) {
        // 4 updates per day
        // They lightly correspond to breakfast, dinner, supper, night snack
        dude.mod_healthy_mod( hmod_changes_per_day[ i % 4 ],
                              sgn( hmod_changes_per_day[ i % 4 ] ) * 200 );
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
    // 1 year = 4 seasons = 8 weeks (on default settings) = 56 days
    size_t num_days = 56;
    standard_npc dude( "healthy eater" );
    // One pear is 3 healthy, 13 nutrition
    // 288 nutrition per day / 13 per pear = ~22, let's round up to 24
    // 6 pears per meal, +18 health
    // This must result in health very close to maximum possible
    // If it doesn't, maximum health isn't achievable outside debug
    test_diet( num_days, dude, {{ 18, 18, 18, 18 }}, 175, 200 );
}

// Terrible diet, worst feasible without hardcore drugs
TEST_CASE( "junk_food_diet", "[health]" )
{
    size_t num_days = 56;
    standard_npc dude( "junk eater" );
    // French fries are -1 healthy, 8 nutrition
    // 300 / 8 = ~38 per day, round up to 40, for -10 health per meal
    // Of course we're not skipping the night snack
    // This should be in the bottom 25% of possible health
    test_diet( num_days, dude, {{ -10, -10, -10, -10 }}, -200, -100 );
}

// Typical diet of an established character
TEST_CASE( "oat_diet", "[health]" )
{
    size_t num_days = 56;
    standard_npc dude( "oat eater" );
    // Oatmeal is 1 healthy, 48 nutriton
    // 6 oats per day, we're skipping night snack
    // Low, but above 0 (because oats are above 0)
    test_diet( num_days, dude, {{ 2, 2, 2, 0 }}, 0, 30 );
}

// A character that eats one meal per day, like that fad diet
// But make the meal healthy
TEST_CASE( "fasting_breakfast", "[health]" )
{
    size_t num_days = 56;
    standard_npc dude( "fasting eater" );
    // Cooked buckwheats are 2 healthy / 50 nutrition
    // 6 per day
    // Noticeably healthy, but not overwhelming
    test_diet( num_days, dude, {{ 12, 0, 0, 0 }}, 50, 100 );
}

// A junk food junkie who switched to a healthy diet for a week
TEST_CASE( "recovering_health", "[health]" )
{
    size_t num_days = 7;
    standard_npc dude( "recovering junk eater" );
    dude.set_healthy( -100 );
    // Beans and rice * 3 + broccoli = 3 * 1 + 2 healthy
    // 3 meals per day
    // Just a week should be enough to stop dying, but not enough to get healthy
    test_diet( num_days, dude, {{ 5, 5, 5, 0 }}, -50, 0 );
}
