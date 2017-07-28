#include "catch/catch.hpp"

#include "player.h"

#include <string>
#include <sstream>

// Minimum and maximum health achievable
constexpr int min_health = -200;
constexpr int max_health = 200;
using health_distribution = std::array<float, (size_t)(max_health + 1 + (-min_health))>;

// Range of the uniform rng roll in health calculation
constexpr int roll_min = -100;
constexpr int roll_max = 100;
constexpr int roll_n = roll_min + roll_max + 1;

static health_distribution propagate_health( const health_distribution &cur, int health_mod,
                                             size_t min_index, size_t max_index )
{
    health_distribution next_iteration;
    next_iteration.fill( 0.0f );
    for( size_t index = min_index; index < max_index; index++ ) {
        if( cur[index] <= 0.0f ) {
            continue;
        }

        const int modified_health = index - min_health - health_mod;
        // Chance that our health goes down/stays/goes up
        // Multiplied by the chance that our health is at current value
        // Chance that it stays constant is the chance that roll is exactly healthy-healthy_mod
        const float stays = modified_health >= roll_min && modified_health <= roll_max ? 1.0f / roll_n : 0.0f;
        // health increases if roll > modified_health
        // cdf: (modified_health-roll_min+1)/roll_n
        // cdf(modified_health) - chance that modified_health>=roll
        // chance that roll<modified_health = 1.0-cdf(modified_health)
        const float goes_down = std::max( 0.0f, ( modified_health - roll_min + 1.0f ) / roll_n );
        // Could be optimized a bit by extracting edge cases
        if( index > 0 ) {
            next_iteration[index - 1] += cur[index] * goes_down;
        } else {
            next_iteration[index] += cur[index] * goes_down;
        }
        next_iteration[index] += cur[index] * stays;
        if( index < cur.size() - 1 ) {
            next_iteration[index + 1] += cur[index] * ( 1.0f - goes_down - stays );
        } else {
            next_iteration[index] += cur[index] * ( 1.0f - goes_down - stays );
        }
    }
    return next_iteration;
}

// Calculates weighted random walk probability after n iterations by pure brute force
// Weighted towards a specific number
// Clamped to [min_health, max_health]
// Health supplied as a function because we want to modify it on the fly
static health_distribution brute_health_probability( size_t iterations, int starting_health,
                                                     const std::function<int(void)> &get_health_mod )
{
    health_distribution ret;
    // Initial chance is 100% for starting health, 0% everywhere else
    ret.fill( 0.0f );
    ret[-min_health + starting_health] = 1.0f;
    for( size_t i = 0; i < iterations; i++ ) {
        // No need to calculate chances for elements that certainly have 0% chance
        int min_index_signed = -min_health + starting_health - ( int )i;
        int max_index_signed = -min_health + starting_health + ( int )i;

        size_t min_index = ( size_t )std::max<int>( 0, min_index_signed );
        size_t max_index = ( size_t )std::min<int>( ret.size(), max_index_signed );

        health_distribution next = propagate_health( ret, get_health_mod(), min_index, max_index );
        ret = next;
    }
    return ret;
}

static void compare_distributions( const health_distribution &real_distribution,
                                   const health_distribution &model_distribution )
{
    for( int health = min_health; health < max_health; health++ ) {
        INFO( health );
        size_t index = ( size_t )( health - min_health );
        CHECK( real_distribution[ index ] == Approx( model_distribution[ index ] ).epsilon(0.1) );
    }
}

// Check if the model above matches real (RNG) results
TEST_CASE("health_model_ok", "[.health]") {
    SECTION("Constant 10 healthy_mod") {
        // Number of RNG simulations
        constexpr size_t num_sims = 100;
        constexpr int starting_health = 0;
        // Keep health mod at that
        constexpr int health_mod_constant = 10;
        constexpr size_t num_iters = 1000;

        health_distribution real_distribution;
        real_distribution.fill( 0.0f );
        player patient;
        for( size_t i = 0; i < num_sims; i++ ) {
            patient.set_healthy( starting_health );
            for( size_t j = 0; j < num_iters; j++ ) {
                patient.set_healthy_mod( health_mod_constant );
                patient.update_health( 0 );
            }
            // Health CAN get outside the range, let's just clamp
            int clamped_health = std::max( min_health, std::min( max_health, patient.get_healthy() ) );
            real_distribution[ clamped_health - min_health ] += 1.0f / num_sims;
        }

        const auto get_health_mod = []() {
            return health_mod_constant;
        };
        health_distribution model_distribution = brute_health_probability( num_iters, starting_health, get_health_mod );

        compare_distributions( real_distribution, model_distribution );
    }

    SECTION("healthy_mod starts at 0 and increases by 1 per iteration, up to 200") {
        constexpr size_t num_sims = 100;
        constexpr int starting_health = 0;
        constexpr size_t num_iters = 1000;

        health_distribution real_distribution;
        real_distribution.fill( 0.0f );
        player patient;
        for( size_t i = 0; i < num_sims; i++ ) {
            patient.set_healthy( starting_health );
            for( size_t j = 0; j < num_iters; j++ ) {
                patient.set_healthy_mod( std::min<int>( max_health, j ) );
                patient.update_health( 0 );
            }
            int clamped_health = std::max( min_health, std::min( max_health, patient.get_healthy() ) );
            real_distribution[ clamped_health - min_health ] += 1.0f / num_sims;
        }

        int counter = 0;
        const auto get_health_mod = [&counter]() {
            return std::min<int>( max_health, counter++ );
        };
        health_distribution model_distribution = brute_health_probability( num_iters, starting_health, get_health_mod );

        compare_distributions( real_distribution, model_distribution );
    }
}
