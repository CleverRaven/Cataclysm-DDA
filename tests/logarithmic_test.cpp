#include "cata_catch.h"
#include "cata_utility.h"

TEST_CASE( "logarithmic_basic_shape", "[cata_utility]" )
{
    CHECK( logarithmic( 0.0 ) == Approx( 0.5 ).epsilon( 1e-6 ) );
    CHECK( logarithmic( -6.0 ) < logarithmic( 0.0 ) );
    CHECK( logarithmic( 6.0 ) > logarithmic( 0.0 ) );
}

TEST_CASE( "logarithmic_range_normalized_mapping", "[cata_utility]" )
{
    CHECK( float_equals( logarithmic_range( -6, 6, -6 ), 1.0 ) );
    CHECK( float_equals( logarithmic_range( -6, 6, 6 ), 0.0 ) );
    double mid = logarithmic_range( -6, 6, 0 );
    CHECK( mid > 0.0 );
    CHECK( mid < 1.0 );
}

TEST_CASE( "logarithmic_monotonic_sequence", "[cata_utility]" )
{
    const double v1 = logarithmic( -6.0 );
    const double v2 = logarithmic( -4.0 );
    const double v3 = logarithmic( -2.0 );
    const double v4 = logarithmic( 0.0 );
    const double v5 = logarithmic( 2.0 );
    const double v6 = logarithmic( 4.0 );
    const double v7 = logarithmic( 6.0 );
    CHECK( v1 < v2 );
    CHECK( v2 < v3 );
    CHECK( v3 < v4 );
    CHECK( v4 < v5 );
    CHECK( v5 < v6 );
    CHECK( v6 < v7 );
}

TEST_CASE( "logarithmic_range_monotonic_decreasing", "[cata_utility]" ) {
    const double r1 = logarithmic_range( -6, 6, -6 );
    const double r2 = logarithmic_range( -6, 6, -4 );
    const double r3 = logarithmic_range( -6, 6, -2 );
    const double r4 = logarithmic_range( -6, 6, 0 );
    const double r5 = logarithmic_range( -6, 6, 2 );
    const double r6 = logarithmic_range( -6, 6, 4 );
    const double r7 = logarithmic_range( -6, 6, 6 );
    CHECK( r1 > r2 );
    CHECK( r2 > r3 );
    CHECK( r3 > r4 );
    CHECK( r4 > r5 );
    CHECK( r5 > r6 );
    CHECK( r6 > r7 );
}
