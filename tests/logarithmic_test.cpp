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
