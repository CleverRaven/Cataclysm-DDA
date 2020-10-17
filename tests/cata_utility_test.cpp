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


TEST_CASE( "equal_ignoring_elements", "[utility]" )
{
    SECTION( "empty sets" ) {
        CHECK( equal_ignoring_elements<std::set<int>>(
        {1, 2, 3}, {1, 2, 3}, {}
               ) );
        CHECK( equal_ignoring_elements<std::set<int>>(
                   {}, {}, {}
               ) );
        CHECK( equal_ignoring_elements<std::set<int>>(
        {1, 2, 3}, {1, 2, 3}, {1, 2, 3}
               ) );
        CHECK( equal_ignoring_elements<std::set<int>>(
        {1}, {1}, {1}
               ) );
    }

    SECTION( "single element ignored" ) {
        int el = GENERATE( -1, 0, 1, 2, 3, 4 );
        CAPTURE( el );
        CHECK( equal_ignoring_elements<std::set<int>>(
        {1, 2, 3}, {1, 2, 3}, {el}
               ) );
    }

    SECTION( "not equal, single element ignored" ) {
        int el = GENERATE( -1, 0, 4 );
        CAPTURE( el );
        CHECK_FALSE( equal_ignoring_elements<std::set<int>>(
        {1, 2, 3}, {1, 2, 4}, {el}
                     ) );
        CHECK_FALSE( equal_ignoring_elements<std::set<int>>(
        {1, 2, 3}, {2, 3}, {el}
                     ) );
        CHECK_FALSE( equal_ignoring_elements<std::set<int>>(
        {1, 2, 3}, {2, 3}, {el}
                     ) );
    }

    SECTION( "two elements ignored" ) {
        int el1 = GENERATE( 1, 2, 3 );
        int el2 = GENERATE( 1, 2, 3 );
        CAPTURE( el1, el2 );

        CHECK( equal_ignoring_elements<std::set<int>>(
        {1, 2, 3}, {1, 2, 3}, {el1, el2}
               ) );
        CHECK_FALSE( equal_ignoring_elements<std::set<int>>(
        {1, 2, 3}, {0, 1, 2, 3}, {el1, el2}
                     ) );
        CHECK_FALSE( equal_ignoring_elements<std::set<int>>(
        {0, 1, 2, 3}, {1, 2, 3}, {el1, el2}
                     ) );
        CHECK_FALSE( equal_ignoring_elements<std::set<int>>(
        {1, 2, 3, 4}, {1, 2, 3}, {el1, el2}
                     ) );
        CHECK_FALSE( equal_ignoring_elements<std::set<int>>(
        {1, 2, 3}, {1, 2, 3, 4}, {el1, el2}
                     ) );
    }

    SECTION( "random check" ) {
        std::set<int> set1 {-2, 0, 2, 4, 6};
        std::set<int> set2{0, 1, 2, 4, 5};

        int el1 = GENERATE( -2, 0, 1, 2, 3 );
        int el2 = GENERATE( 1, 2, 4, 5 );
        int el3 = GENERATE( 2, 4, 5, 6, 7 );
        std::set<int> ignored_els {el1, el2, el3};

        CAPTURE( set1, set2, ignored_els );

        // generate set symmetric difference into v
        std::vector<int> v( set1.size() + set2.size() );
        auto it = std::set_symmetric_difference( set1.begin(), set1.end(), set2.begin(), set2.end(),
                  v.begin() );
        v.resize( it - v.begin() );

        // equal_ignoring_elements is the same as checking if symmetric difference of tho sets
        // contains only "ignored" elements
        bool equal = std::all_of( v.begin(), v.end(), [&]( int i ) {
            return ignored_els.count( i );
        } );

        CHECK( equal_ignoring_elements( set1, set2, ignored_els ) == equal );
    }
}