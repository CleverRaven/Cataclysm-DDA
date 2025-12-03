#include "cata_catch.h"
#include "cata_utility.h"

TEST_CASE( "match_include_exclude basic includes", "[cata_utility]" )
{
    CHECK( match_include_exclude( "tank bank", "tank,bank" ) );
    CHECK( match_include_exclude( "hello tank", "tank" ) );
    CHECK( !match_include_exclude( "hello", "tank" ) );
}

TEST_CASE( "match_include_exclude excludes only", "[cata_utility]" )
{
    CHECK( !match_include_exclude( "house", "-house" ) );
    CHECK( match_include_exclude( "garden", "-house" ) );
}

TEST_CASE( "match_include_exclude mixed rules", "[cata_utility]" ) {
    CHECK( match_include_exclude( "drive a tank", "bank,-house,tank,-car" ) );
    CHECK( !match_include_exclude( "drive a car", "bank,-house,tank,-car" ) );
    CHECK( !match_include_exclude( "big house with bank", "bank,-house,tank,-car" ) );
}

TEST_CASE( "match_include_exclude case insensitive", "[cata_utility]" ) {
    CHECK( match_include_exclude( "TaNk", "tank" ) );
    CHECK( !match_include_exclude( "Car", "-car" ) );
}
