#include "catch/catch.hpp"
#include "assertion_helpers.h"
#include "cata_utility.h"
#include "units.h"
#include "units_utility.h"

// tests both variants of string_starts_with
template <std::size_t N>
bool test_string_starts_with( const std::string &s1, const char( &s2 )[N] )
{
    CAPTURE( s1, s2, N );
    bool r1 =  string_starts_with( s1, s2 );
    bool r2 =  string_starts_with( s1, std::string( s2 ) );
    CHECK( r1 == r2 );
    return r1;
}

// tests both variants of string_ends_with
template <std::size_t N>
bool test_string_ends_with( const std::string &s1, const char( &s2 )[N] )
{
    CAPTURE( s1, s2, N );
    bool r1 =  string_ends_with( s1, s2 );
    bool r2 =  string_ends_with( s1, std::string( s2 ) );
    CHECK( r1 == r2 );
    return r1;
}

TEST_CASE( "string_starts_with", "[utility]" )
{
    CHECK( test_string_starts_with( "", "" ) );
    CHECK( test_string_starts_with( "a", "" ) );
    CHECK_FALSE( test_string_starts_with( "", "a" ) );
    CHECK( test_string_starts_with( "ab", "a" ) );
    CHECK_FALSE( test_string_starts_with( "ab", "b" ) );
    CHECK_FALSE( test_string_starts_with( "a", "ab" ) );
}

TEST_CASE( "string_ends_with", "[utility]" )
{
    CHECK( test_string_ends_with( "", "" ) );
    CHECK( test_string_ends_with( "a", "" ) );
    CHECK_FALSE( test_string_ends_with( "", "a" ) );
    CHECK( test_string_ends_with( "ba", "a" ) );
    CHECK_FALSE( test_string_ends_with( "ba", "b" ) );
    CHECK_FALSE( test_string_ends_with( "a", "ba" ) );
}

TEST_CASE( "string_ends_with_benchmark", "[.][utility][benchmark]" )
{
    const std::string s1 = "long_string_with_suffix";

    BENCHMARK( "old version" ) {
        return string_ends_with( s1, std::string( "_suffix" ) );
    };
    BENCHMARK( "new version" ) {
        return string_ends_with( s1, "_suffix" );
    };
}

TEST_CASE( "string_ends_with_season_suffix", "[utility]" )
{
    constexpr size_t suffix_len = 15;
    constexpr char season_suffix[4][suffix_len] = {
        "_season_spring", "_season_summer", "_season_autumn", "_season_winter"
    };

    CHECK( test_string_ends_with( "t_tile_season_spring", season_suffix[0] ) );
    CHECK_FALSE( test_string_ends_with( "t_tile", season_suffix[0] ) );
    CHECK_FALSE( test_string_ends_with( "t_tile_season_summer", season_suffix[0] ) );
    CHECK_FALSE( test_string_ends_with( "t_tile_season_spring1", season_suffix[0] ) );
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
