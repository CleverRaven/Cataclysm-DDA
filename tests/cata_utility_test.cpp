#include "catch/catch.hpp"
#include "assertion_helpers.h"
#include "cata_utility.h"
#include "units.h"
#include "units_utility.h"

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

TEST_CASE( "divide_round_up", "[utility]" )
{
    CHECK( divide_round_up( 0, 5 ) == 0 );
    CHECK( divide_round_up( 1, 5 ) == 1 );
    CHECK( divide_round_up( 4, 5 ) == 1 );
    CHECK( divide_round_up( 5, 5 ) == 1 );
    CHECK( divide_round_up( 6, 5 ) == 2 );
}

TEST_CASE( "divide_round_up_units", "[utility]" )
{
    CHECK( divide_round_up( 0_ml, 5_ml ) == 0 );
    CHECK( divide_round_up( 1_ml, 5_ml ) == 1 );
    CHECK( divide_round_up( 4_ml, 5_ml ) == 1 );
    CHECK( divide_round_up( 5_ml, 5_ml ) == 1 );
    CHECK( divide_round_up( 6_ml, 5_ml ) == 2 );
}

TEST_CASE( "erase_if", "[utility]" )
{
    std::set<int> s{1, 2, 3, 4, 5};
    SECTION( "erase none" ) {
        CHECK_FALSE( erase_if( s, []( const auto & v ) {
            return v == 6;
        } ) );
        check_containers_equal( s, std::vector<int> {1, 2, 3, 4, 5} );
    }
    SECTION( "erase single" ) {
        CHECK( erase_if( s,  []( const auto & v ) {
            return v == 1;
        } ) );
        check_containers_equal( s, std::vector<int> {2, 3, 4, 5} );
    }

    SECTION( "erase prefix" ) {
        CHECK( erase_if( s,  []( const auto & v ) {
            return v <= 2;
        } ) );
        check_containers_equal( s, std::vector<int> { 3, 4, 5} );
    }

    SECTION( "erase middle" ) {
        CHECK( erase_if( s, []( const auto & v ) {
            return v >= 2 && v <= 4;
        } ) );
        check_containers_equal( s, std::vector<int> { 1, 5} );
    }

    SECTION( "erase suffix" ) {
        CHECK( erase_if( s, []( const auto & v ) {
            return v >= 4;
        } ) );
        check_containers_equal( s, std::vector<int> { 1, 2, 3} );
    }

    SECTION( "erase all" ) {
        CHECK( erase_if( s, []( const auto & ) {
            return true;
        } ) );
        check_containers_equal( s, std::vector<int>() );
    }
}
