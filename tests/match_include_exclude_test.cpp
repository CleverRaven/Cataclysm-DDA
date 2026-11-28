#include "cata_catch.h"
#include "cata_utility.h"

TEST_CASE( "match_include_exclude_basic_includes", "[cata_utility]" )
{
    CHECK( match_include_exclude( "tank bank", "tank,bank" ) );
    CHECK( match_include_exclude( "hello tank", "tank" ) );
    CHECK( !match_include_exclude( "hello", "tank" ) );
}

TEST_CASE( "match_include_exclude_excludes_only", "[cata_utility]" )
{
    CHECK( !match_include_exclude( "house", "-house" ) );
    CHECK( !match_include_exclude( "garden", "-house" ) );
}

TEST_CASE( "match_include_exclude_mixed_rules", "[cata_utility]" )
{
    CHECK( match_include_exclude( "drive a tank", "bank,-house,tank,-car" ) );
    CHECK( !match_include_exclude( "drive a car", "bank,-house,tank,-car" ) );
    CHECK( !match_include_exclude( "big house with bank", "bank,-house,tank,-car" ) );
}

TEST_CASE( "match_include_exclude_ignores_empty_entries", "[cata_utility]" )
{
    CHECK_FALSE( match_include_exclude( "forest", "bank," ) );
    CHECK_FALSE( match_include_exclude( "forest", ",bank,,house" ) );
    CHECK_FALSE( match_include_exclude( "forest", ",," ) );
    CHECK( match_include_exclude( "bank", ",bank,,-forest," ) );
    CHECK_FALSE( match_include_exclude( "forest bank", ",bank,,-forest," ) );
}

TEST_CASE( "match_include_exclude_case_insensitive", "[cata_utility]" )
{
    CHECK( match_include_exclude( "TaNk", "tank" ) );
    CHECK( !match_include_exclude( "Car", "-car" ) );
}
