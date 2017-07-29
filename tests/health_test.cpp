#include "catch/catch.hpp"

#include "player.h"

#include <string>
#include <sstream>
#include <numeric>

// Minimum and maximum health achievable
constexpr int min_health = -200;
constexpr int max_health = 200;
using health_distribution = std::array<double, (size_t)(max_health + 1 + (-min_health))>;

// Range of the uniform rng roll in health calculation
constexpr int roll_min = -100;
constexpr int roll_max = 100;
constexpr int roll_n = roll_max - roll_min + 1;

static health_distribution calc_cdf( const health_distribution &distribution )
{
    health_distribution ret;
    ret[ 0 ] = distribution[ 0 ];
    for( size_t i = 1; i < ret.size(); i++ ) {
        ret[ i ] = std::max( 0.0, std::min( 1.0, ret[ i - 1 ] + distribution[ i ] ) );
    }

    return ret;
}

static health_distribution propagate_health( const health_distribution &cur, int health_mod,
                                             size_t min_index, size_t max_index )
{
    health_distribution next_iteration;
    next_iteration.fill( 0.0 );
    for( size_t index = min_index; index <= max_index; index++ ) {
        if( cur[index] <= 0.0 ) {
            continue;
        }

        const int modified_health = ( int )index + min_health - health_mod;
        // Chance that our health goes down/stays/goes up
        // Multiplied by the chance that our health is at current value
        // Chance that it stays constant is the chance that roll is exactly healthy-healthy_mod
        const double stays = ( modified_health >= roll_min && modified_health <= roll_max ) ? ( 1.0 / roll_n ) : 0.0;
        // health increases if roll > modified_health
        // cdf: (modified_health-roll_min+1)/roll_n
        // cdf(modified_health) - chance that modified_health >= roll
        // chance that roll < modified_health = 1.0-cdf(modified_health)
        const int clamped_health = std::max( roll_min - 1, std::min( roll_max, modified_health ) );
        const double goes_down = ( clamped_health - roll_min + 1.0 ) / roll_n - stays;
        // Could be optimized a bit by extracting edge cases
        if( index > 0 ) {
            next_iteration[index - 1] += cur[index] * goes_down;
        } else {
            next_iteration[0] += cur[index] * goes_down;
        }
        next_iteration[index] += cur[index] * stays;
        if( index < cur.size() - 1 ) {
            next_iteration[index + 1] += cur[index] * ( 1.0 - goes_down - stays );
        } else {
            next_iteration[cur.size() - 1] += cur[index] * ( 1.0 - goes_down - stays );
        }
    }
    return next_iteration;
}

// Calculates weighted random walk probability after n iterations by pure brute force
// Weighted towards a specific number
// Clamped to [min_health, max_health]
// healthy_mod supplied as a function of iteration because we want to modify it on the fly
static health_distribution brute_health_probability( size_t iterations, int starting_health,
                                                     const std::function<int(size_t)> &get_healthy_mod )
{
    health_distribution ret;
    // Initial chance is 100% for starting health, 0% everywhere else
    ret.fill( 0.0 );
    ret[-min_health + starting_health] = 1.0;
    for( size_t i = 0; i < iterations; i++ ) {
        // No need to calculate chances for elements that certainly have 0% chance
        int min_index_signed = -min_health + starting_health - ( int )i - 1;
        int max_index_signed = -min_health + starting_health + ( int )i + 1;

        size_t min_index = ( size_t )std::max<int>( 0, min_index_signed );
        size_t max_index = ( size_t )std::min<int>( ret.size() - 1, max_index_signed );

        health_distribution next = propagate_health( ret, get_healthy_mod( i ), min_index, max_index );
        ret = next;
    }
    return ret;
}

static void compare_distributions( const health_distribution &real_distribution,
                                   const health_distribution &model_distribution )
{
    for( int health = min_health; health < max_health; health++ ) {
        INFO( "Health == " << health );
        size_t index = ( size_t )( health - min_health );
        // This epsilon is huge, but it has to be: there is a lot of jitter near 0 health
        CHECK( real_distribution[ index ] == Approx( model_distribution[ index ] ).epsilon( 0.05 ) );
    }

    CHECK( calc_cdf( real_distribution )[ real_distribution.size() - 1 ] == Approx( 1.0 ).epsilon( 0.01 ) );
    CHECK( calc_cdf( model_distribution )[ model_distribution.size() - 1 ] == Approx( 1.0 ).epsilon( 0.01 ) );
}

static double chance_health_exceeds( const health_distribution &cdf, int health )
{
    if( health < min_health ) {
        return 1.0;
    }

    if( health > max_health ) {
        return 0.0;
    }

    return 1.0 - cdf[ health - min_health ];
}

// Check if the model above matches real (RNG) results
TEST_CASE("health_model_ok", "[health]") {
    SECTION( "After one iteration, chance to get -1, 0, and 1 is 100/201, 1/201, 100/201 respectively" ) {
        const auto get_healthy_mod = []( size_t ) {
            return 0;
        };
        health_distribution model_distribution = brute_health_probability( 1, 0, get_healthy_mod );
        CHECK( model_distribution[ -1 - min_health ] == Approx( 100.0 / 201.0 ).epsilon( 0.01 ) );
        CHECK( model_distribution[  0 - min_health ] == Approx( 1.0  / 201.0 ).epsilon( 0.01 ) );
        CHECK( model_distribution[  1 - min_health ] == Approx( 100.0 / 201.0 ).epsilon( 0.01 ) );
    }
    SECTION( "After 10 iterations, chances are symmetric for positive and negative health" ) {
        const auto get_healthy_mod = []( size_t ) {
            return 0;
        };
        health_distribution model_distribution = brute_health_probability( 10, 0, get_healthy_mod );
        for( int health = -10; health <= 10; health++ ) {
            CHECK( model_distribution[ health - min_health ] == Approx( model_distribution[ -health - min_health ] ).epsilon( 0.01 ) );
        }
    }
    SECTION( "Constant 10 healthy_mod" ) {
        // Number of RNG simulations
        constexpr size_t num_sims = 1000;
        constexpr int starting_health = 0;
        // Keep health mod at that
        constexpr int health_mod_constant = 10;
        constexpr size_t num_iters = 100;

        health_distribution real_distribution;
        real_distribution.fill( 0.0 );
        player patient;
        for( size_t i = 0; i < num_sims; i++ ) {
            patient.set_healthy( starting_health );
            for( size_t j = 0; j < num_iters; j++ ) {
                patient.set_healthy_mod( health_mod_constant );
                patient.update_health( 0 );
            }
            // Health CAN get outside the range, let's just clamp
            int clamped_health = std::max( min_health, std::min( max_health, patient.get_healthy() ) );
            real_distribution[ clamped_health - min_health ] += 1.0 / num_sims;
        }

        const auto get_healthy_mod = []( size_t ) {
            return health_mod_constant;
        };
        health_distribution model_distribution = brute_health_probability( num_iters, starting_health, get_healthy_mod );

        compare_distributions( real_distribution, model_distribution );
    }

    SECTION( "healthy_mod starts at 0 and increases by 1 per iteration, up to 100" ) {
        constexpr size_t num_sims = 1000;
        constexpr int starting_health = 0;
        constexpr size_t num_iters = 100;

        health_distribution real_distribution;
        real_distribution.fill( 0.0 );
        player patient;
        for( size_t i = 0; i < num_sims; i++ ) {
            patient.set_healthy( starting_health );
            for( size_t j = 0; j < num_iters; j++ ) {
                patient.set_healthy_mod( std::min<int>( 100, j ) );
                patient.update_health( 0 );
            }
            int clamped_health = std::max( min_health, std::min( max_health, patient.get_healthy() ) );
            real_distribution[ clamped_health - min_health ] += 1.0 / num_sims;
        }

        const auto get_healthy_mod = []( size_t iter ) {
            return std::min<int>( 100, iter );
        };
        health_distribution model_distribution = brute_health_probability( num_iters, starting_health, get_healthy_mod );

        compare_distributions( real_distribution, model_distribution );
    }
}

TEST_CASE("constant_health_mod", "[health]") {
    // 4 weeks of health updates
    size_t num_iters = DAYS( 28 ) / HOURS( 6 );
    const auto get_healthy_mod = []( size_t ) {
        return 10;
    };
    health_distribution model_distribution = brute_health_probability( num_iters, 0, get_healthy_mod );
    health_distribution cdf = calc_cdf( model_distribution );
    INFO( "Number of health updates:" << num_iters );
    // cdf(x) - chance that we get at most x
    // So 1-cdf(x) is the chance that we get more than x
    CHECK( chance_health_exceeds( cdf, 0 ) > 0.7 );
    CHECK( chance_health_exceeds( cdf, 5 ) > 0.5 );
    CHECK( chance_health_exceeds( cdf, 10 ) > 0.2 );
    CHECK( chance_health_exceeds( cdf, 20 ) < 0.02 );
    CHECK( chance_health_exceeds( cdf, num_iters + 1 ) <= 0.00001 );
}

TEST_CASE("healthy_breakfast_and_fasting", "[health]") {
    size_t num_iters = DAYS( 28 ) / HOURS( 6 );
    int hmod = 0;
    const auto get_healthy_mod = [&hmod]( size_t iter ) {
        if( iter % 4 == 0 ) {
            // Blackberry diet, but all of them are eaten for breakfast
            // 300 nutrition points / 15 nut per blackberry = 20 bberries
            // 20 bberies * 5 health per berry = 100 healthy_mod
            hmod = std::min( 200, hmod + 100 );
        }
        // hmod goes like: 108, 81, 60, 45, 108...
        // Average hmod during the day is 73.5
        hmod = hmod * 3 / 4;
        return hmod;
    };
    health_distribution model_distribution = brute_health_probability( num_iters, 0, get_healthy_mod );
    health_distribution cdf = calc_cdf( model_distribution );
    INFO( "Number of health updates:" << num_iters );
    CHECK( chance_health_exceeds( cdf, 0 ) > 0.99 );
    CHECK( chance_health_exceeds( cdf, 30 ) > 0.5 );
    CHECK( chance_health_exceeds( cdf, 70 ) > 0.00001 );
    CHECK( chance_health_exceeds( cdf, num_iters + 1 ) <= 0.0 );
}

TEST_CASE("long_term_healthy_diet_no_night_snacks", "[health]") {
    size_t num_iters = DAYS( 56 ) / HOURS( 6 );
    int hmod = 0;
    const auto get_healthy_mod = [&hmod]( size_t iter ) {
        // Breakfast, dinner and supper of pears
        if( iter % 4 != 3 ) {
            // 300 nutrition points / 13 nut per pears, round up = 24 pears
            // 24 pears / 3 meals = 6 pears per meal
            // 6 pears * 3 health per pear = 18 healthy_mod
            hmod = std::min( 200, hmod + 18 );
        }
        // hmod goes like: 33, 38, 42, 45, 33...
        // Average hmod during the day is 39.5
        hmod = hmod * 3 / 4;
        return hmod;
    };
    health_distribution model_distribution = brute_health_probability( num_iters, 0, get_healthy_mod );
    health_distribution cdf = calc_cdf( model_distribution );
    INFO( "Number of health updates:" << num_iters );
    CHECK( chance_health_exceeds( cdf, 20 ) > 0.98 );
    CHECK( chance_health_exceeds( cdf, 30 ) > 0.65 );
    CHECK( chance_health_exceeds( cdf, 50 ) < 0.1 );
    CHECK( chance_health_exceeds( cdf, num_iters + 1 ) <= 0.0 );
}

TEST_CASE("long_term_typical_diet", "[health]") {
    size_t num_iters = DAYS( 56 ) / HOURS( 6 );
    int hmod = 0;
    const auto get_healthy_mod = [&hmod]( size_t iter ) {
        // Breakfast, dinner and supper of oatmeal
        if( iter % 2 == 0 ) {
            // 300 nutrition points / 50 nut per oat = 6 oats
            // 6 oats / 3 meals = 2 oats per meal
            // 2 oats * 1 health per oat = 2 healthy_mod
            hmod = std::min( 200, hmod + 2 );
        }
        // hmod goes like: 2, 3, 3, 3...
        // Average hmod during the day is 2.75
        hmod = hmod * 3 / 4;
        return hmod;
    };
    health_distribution model_distribution = brute_health_probability( num_iters, 0, get_healthy_mod );
    health_distribution cdf = calc_cdf( model_distribution );
    INFO( "Number of health updates:" << num_iters );
    CHECK( chance_health_exceeds( cdf, -5 ) > 0.7 );
    CHECK( chance_health_exceeds( cdf, -1 ) > 0.5 );
    CHECK( chance_health_exceeds( cdf, 5 ) < 0.5 );
}
