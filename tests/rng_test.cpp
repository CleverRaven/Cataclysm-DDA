#include <functional>
#include <vector>

#include "catch/catch.hpp"
#include "test_statistics.h"
#include "rng.h"
#include "optional.h"

static void check_remainder( float proportion )
{
    statistics<float> stats;
    const epsilon_threshold target_range{ proportion, 0.05 };
    do {
        stats.add( roll_remainder( proportion ) );
    } while( stats.n() < 100 || stats.uncertain_about( target_range ) );
    INFO( "Goal: " << proportion );
    INFO( "Result: " << stats.avg() );
    INFO( "Samples: " << stats.n() );
    CHECK( stats.test_threshold( target_range ) );
}

TEST_CASE( "roll_remainder_distribution" )
{
    check_remainder( 0.0 );
    check_remainder( 0.01 );
    check_remainder( -0.01 );
    check_remainder( 1.5 );
    check_remainder( -1.5 );
    check_remainder( 1.75 );
    check_remainder( -1.75 );
    check_remainder( 1.1 );
    check_remainder( -1.1 );
    check_remainder( 1.0 );
    check_remainder( -1.0 );
    check_remainder( 10.0 );
    check_remainder( -10.0 );
    check_remainder( 10.5 );
    check_remainder( -10.5 );
}

static void check_x_in_y( double x, double y )
{
    statistics<bool> stats( Z99_999_9 );
    const epsilon_threshold target_range{ x / y, 0.05 };
    do {
        stats.add( x_in_y( x, y ) );
    } while( stats.n() < 100 || stats.uncertain_about( target_range ) );
    INFO( "Goal: " << x << " / " << y << " ( " << x / y << " )" );
    INFO( "Result: " << stats.avg() );
    INFO( "Samples: " << stats.n() );
    CHECK( stats.test_threshold( target_range ) );
}

TEST_CASE( "x_in_y_distribution" )
{
    float y_increment = 0.01;
    for( float y = 0.1; y < 500.0; y += y_increment ) {
        y_increment *= 1.1;
        float x_increment = 0.1;
        // NOLINTNEXTLINE(clang-analyzer-security.FloatLoopCounter)
        for( float x = 0.1; x < y; x += x_increment ) {
            check_x_in_y( x, y );
            x_increment *= 1.1;
        }
    }
}

TEST_CASE( "random_entry_preserves_constness" )
{
    const std::vector<int> v0{ 4321 };
    int i0 = *random_entry_opt( v0 );
    CHECK( i0 == 4321 );

    std::vector<int> v1{ 1234 };
    int &i1 = *random_entry_opt( v1 );
    CHECK( i1 == 1234 );
    i1 = 5678;
    CHECK( v1[0] == 5678 );
}
