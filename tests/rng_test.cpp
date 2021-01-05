#include "catch/catch.hpp"

#include <functional>
#include <vector>

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
    float y_increment = 0.01f;
    // NOLINTNEXTLINE(clang-analyzer-security.FloatLoopCounter)
    for( float y = 0.1f; y < 500.0f; y += y_increment ) {
        y_increment *= 1.1f;
        float x_increment = 0.1f;
        // NOLINTNEXTLINE(clang-analyzer-security.FloatLoopCounter)
        for( float x = 0.1f; x < y; x += x_increment ) {
            check_x_in_y( x, y );
            x_increment *= 1.1f;
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

template<int w, int h>
static void test_pseudo_random_tiling_test( uint64_t seed )
{
    pseudo_random_matrix_tiling<w, h> mt( seed );
    std::set<std::pair<int, int>> s1;
    std::vector<std::pair<int, int>> vec1;

    for( size_t i = 0; i < w * h; ++i ) {
        const std::pair<int, int> &v = mt.get_xy( i );
        CHECK( v.first >= 0 );
        CHECK( v.first < w );
        CHECK( v.second >= 0 );
        CHECK( v.second <= h );
        s1.insert( v );
        vec1.push_back( v );
    }
    INFO( "All elements in [0..w*h) range should be distinct" );
    CAPTURE( s1 );
    CHECK( s1.size() == w * h );

    INFO( "Results should form a period of (w * h)" )
    for( size_t i = 0; i < w * h * 5; ++i ) {
        const std::pair<int, int> &v = mt.get_xy( i );
        CAPTURE( i, v );
        CHECK( vec1[i % ( w * h )] == v );
    }

    INFO( "Seed stability" );
    pseudo_random_matrix_tiling<w, h> mt2( seed );
    for( size_t i = 0; i < w * h; ++i ) {
        const std::pair<int, int> &v = mt2.get_xy( i );
        CAPTURE( i, v );
        CHECK( vec1[i] == v );
    }

    INFO( "Seed variance" );
    int diff = 0;
    pseudo_random_matrix_tiling<w, h> mt3( seed + 1 );
    for( size_t i = 0; i < w * h; ++i ) {
        const std::pair<int, int> &v = mt3.get_xy( i );
        diff += ( vec1[i] != v );
    }
    INFO( "At least half of the returned values should be different." );
    CHECK( diff >= w * h / 2 );
};

TEST_CASE( "pseudo_random_tiling_test" )
{
    uint64_t seed = GENERATE( 441747021, 466935525, 184832026, 694846310, 472039296 );
    test_pseudo_random_tiling_test<12, 12>( seed );
    test_pseudo_random_tiling_test<1, 1>( seed );
    test_pseudo_random_tiling_test<13, 7>( seed );
}
