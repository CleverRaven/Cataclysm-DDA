#include "catch/catch.hpp"

#include "cata_utility.h"

TEST_CASE( "string_starts_with", "[utility]" )
{
    CHECK( string_starts_with( "", "" ) );
    CHECK( string_starts_with( "a", "" ) );
    CHECK_FALSE( string_starts_with( "", "a" ) );
    CHECK( string_starts_with( "ab", "a" ) );
    CHECK_FALSE( string_starts_with( "ab", "b" ) );
    CHECK_FALSE( string_starts_with( "a", "ab" ) );
}

TEST_CASE( "string_ends_with", "[utility]" )
{
    CHECK( string_ends_with( "", "" ) );
    CHECK( string_ends_with( "a", "" ) );
    CHECK_FALSE( string_ends_with( "", "a" ) );
    CHECK( string_ends_with( "ba", "a" ) );
    CHECK_FALSE( string_ends_with( "ba", "b" ) );
    CHECK_FALSE( string_ends_with( "a", "ba" ) );
}
