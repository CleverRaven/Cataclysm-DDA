#include "catch/catch.hpp"

#include "test_statistics.h"

#include "rng.h"

static void check_remainder( float proportion )
{
    statistics<float> stats;
    const epsilon_threshold confidence_level{ proportion, 0.05 };
    do {
        stats.add( roll_remainder( proportion ) );
    } while( stats.n() < 100 || stats.uncertain_about( confidence_level ) );
    INFO( "Goal: " << proportion );
    INFO( "Result: " << stats.avg() );
    INFO( "Samples: " << stats.n() );
    CHECK( stats.test_threshold( confidence_level ) );
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
    statistics<bool> stats( Z99_99 );
    const epsilon_threshold confidence_level{ x / y, 0.05 };
    do {
        stats.add( x_in_y( x, y ) );
    } while( stats.n() < 100 || stats.uncertain_about( confidence_level ) );
    INFO( "Goal: " << x << " / " << y << " ( " << x / y << " )" );
    INFO( "Result: " << stats.avg() );
    INFO( "Samples: " << stats.n() );
    CHECK( stats.test_threshold( confidence_level ) );
}

TEST_CASE( "x_in_y_distribution" )
{
    float y_increment = 0.01;
    for( float y = 0.1; y < 500.0; y += y_increment ) {
        y_increment *= 1.01;
        float x_increment = 0.1;
        for( float x = 0.1; x < y; x += x_increment ) {
            check_x_in_y( x, y );
            x_increment *= 1.1;
        }
    }
}
