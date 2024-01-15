#include <algorithm>
#include <cstddef>
#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "assertion_helpers.h"
#include "cata_utility.h"
#include "cata_catch.h"
#include "debug_menu.h"
#include "units.h"
#include "units_utility.h"

// tests both variants of string_starts_with
template <std::size_t N>
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
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
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
bool test_string_ends_with( const std::string &s1, const char( &s2 )[N] )
{
    CAPTURE( s1, s2, N );
    bool r1 =  string_ends_with( s1, s2 );
    bool r2 =  string_ends_with( s1, std::string( s2 ) );
    CHECK( r1 == r2 );
    return r1;
}

TEST_CASE( "string_starts_with", "[utility][nogame]" )
{
    CHECK( test_string_starts_with( "", "" ) );
    CHECK( test_string_starts_with( "a", "" ) );
    CHECK_FALSE( test_string_starts_with( "", "a" ) );
    CHECK( test_string_starts_with( "ab", "a" ) );
    CHECK_FALSE( test_string_starts_with( "ab", "b" ) );
    CHECK_FALSE( test_string_starts_with( "a", "ab" ) );
}

TEST_CASE( "string_ends_with", "[utility][nogame]" )
{
    CHECK( test_string_ends_with( "", "" ) );
    CHECK( test_string_ends_with( "a", "" ) );
    CHECK_FALSE( test_string_ends_with( "", "a" ) );
    CHECK( test_string_ends_with( "ba", "a" ) );
    CHECK_FALSE( test_string_ends_with( "ba", "b" ) );
    CHECK_FALSE( test_string_ends_with( "a", "ba" ) );
}

TEST_CASE( "string_ends_with_benchmark", "[.][utility][benchmark][nogame]" )
{
    const std::string s1 = "long_string_with_suffix";

    BENCHMARK( "old version" ) {
        return string_ends_with( s1, std::string( "_suffix" ) );
    };
    BENCHMARK( "new version" ) {
        return string_ends_with( s1, "_suffix" );
    };
}

TEST_CASE( "string_ends_with_season_suffix", "[utility][nogame]" )
{
    constexpr size_t suffix_len = 15;
    // NOLINTNEXTLINE(cata-use-mdarray,modernize-avoid-c-arrays)
    constexpr char season_suffix[4][suffix_len] = {
        "_season_spring", "_season_summer", "_season_autumn", "_season_winter"
    };

    CHECK( test_string_ends_with( "t_tile_season_spring", season_suffix[0] ) );
    CHECK_FALSE( test_string_ends_with( "t_tile", season_suffix[0] ) );
    CHECK_FALSE( test_string_ends_with( "t_tile_season_summer", season_suffix[0] ) );
    CHECK_FALSE( test_string_ends_with( "t_tile_season_spring1", season_suffix[0] ) );
}

TEST_CASE( "divide_round_up", "[utility][nogame]" )
{
    CHECK( divide_round_up( 0, 5 ) == 0 );
    CHECK( divide_round_up( 1, 5 ) == 1 );
    CHECK( divide_round_up( 4, 5 ) == 1 );
    CHECK( divide_round_up( 5, 5 ) == 1 );
    CHECK( divide_round_up( 6, 5 ) == 2 );
}

TEST_CASE( "divide_round_up_units", "[utility][nogame]" )
{
    CHECK( divide_round_up( 0_ml, 5_ml ) == 0 );
    CHECK( divide_round_up( 1_ml, 5_ml ) == 1 );
    CHECK( divide_round_up( 4_ml, 5_ml ) == 1 );
    CHECK( divide_round_up( 5_ml, 5_ml ) == 1 );
    CHECK( divide_round_up( 6_ml, 5_ml ) == 2 );
}

TEST_CASE( "erase_if", "[utility][nogame]" )
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

TEST_CASE( "equal_ignoring_elements", "[utility][nogame]" )
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

TEST_CASE( "map_without_keys", "[map][filter][nogame]" )
{
    std::map<std::string, std::string> map_empty;
    std::map<std::string, std::string> map_name_a = {
        { "name", "a" }
    };
    std::map<std::string, std::string> map_name_b = {
        { "name", "b" }
    };
    std::map<std::string, std::string> map_dirt_1 = {
        { "dirt", "1" }
    };
    std::map<std::string, std::string> map_dirt_2 = {
        { "dirt", "2" }
    };
    std::map<std::string, std::string> map_name_a_dirt_1 = {
        { "name", "a" },
        { "dirt", "1" }
    };
    std::map<std::string, std::string> map_name_a_dirt_2 = {
        { "name", "a" },
        { "dirt", "2" }
    };
    std::vector<std::string> dirt = { "dirt" };

    // Empty maps compare equal to maps with all keys filtered out
    CHECK( map_without_keys( map_empty, dirt ) == map_without_keys( map_dirt_1, dirt ) );
    CHECK( map_without_keys( map_empty, dirt ) == map_without_keys( map_dirt_2, dirt ) );

    // Maps are equal when all differing keys are filtered out
    // (same name, dirt filtered out)
    CHECK( map_without_keys( map_name_a, dirt ) == map_without_keys( map_name_a_dirt_1, dirt ) );
    CHECK( map_without_keys( map_name_a, dirt ) == map_without_keys( map_name_a_dirt_2, dirt ) );

    // Maps are different if some different keys remain after filtering
    // (different name, no dirt to filter out)
    CHECK_FALSE( map_without_keys( map_name_a, dirt ) == map_without_keys( map_name_b, dirt ) );
    CHECK_FALSE( map_without_keys( map_name_b, dirt ) == map_without_keys( map_name_a, dirt ) );
    // (different name, dirt filtered out)
    CHECK_FALSE( map_without_keys( map_dirt_1, dirt ) == map_without_keys( map_name_a_dirt_1, dirt ) );
    CHECK_FALSE( map_without_keys( map_dirt_2, dirt ) == map_without_keys( map_name_a_dirt_2, dirt ) );
}

TEST_CASE( "map_equal_ignoring_keys", "[map][filter][nogame]" )
{
    std::map<std::string, std::string> map_empty;
    std::map<std::string, std::string> map_name_a = {
        { "name", "a" }
    };
    std::map<std::string, std::string> map_name_b = {
        { "name", "b" }
    };
    std::map<std::string, std::string> map_dirt_1 = {
        { "dirt", "1" }
    };
    std::map<std::string, std::string> map_dirt_2 = {
        { "dirt", "2" }
    };
    std::map<std::string, std::string> map_name_a_dirt_1 = {
        { "name", "a" },
        { "dirt", "1" }
    };
    std::map<std::string, std::string> map_name_a_dirt_2 = {
        { "name", "a" },
        { "dirt", "2" }
    };
    std::set<std::string> dirt = { "dirt" };

    // Empty maps compare equal to maps with all keys filtered out
    CHECK( map_equal_ignoring_keys( map_empty, map_dirt_1, dirt ) );
    CHECK( map_equal_ignoring_keys( map_empty, map_dirt_2, dirt ) );

    // Maps are equal when all differing keys are filtered out
    // (same name, dirt filtered out)
    CHECK( map_equal_ignoring_keys( map_name_a, map_name_a_dirt_1, dirt ) );
    CHECK( map_equal_ignoring_keys( map_name_a, map_name_a_dirt_2, dirt ) );

    // Maps are different if some different keys remain after filtering
    // (different name, no dirt to filter out)
    CHECK_FALSE( map_equal_ignoring_keys( map_name_a, map_name_b, dirt ) );
    CHECK_FALSE( map_equal_ignoring_keys( map_name_b, map_name_a, dirt ) );
    // (different name, dirt filtered out)
    CHECK_FALSE( map_equal_ignoring_keys( map_dirt_1, map_name_a_dirt_1, dirt ) );
    CHECK_FALSE( map_equal_ignoring_keys( map_dirt_2, map_name_a_dirt_2, dirt ) );

    // Maps with different ignored keys are equal after filtering them out.
    std::map<std::string, std::string> rock_and_beer = {
        { "rock", "granite" },
        { "beer", "stout" }
    };
    std::map<std::string, std::string> beer_and_stone = {
        { "beer", "stout" },
        { "stone", "boulder" }
    };
    std::map<std::string, std::string> lagers_are_best = {
        { "beer", "lager" },
        { "rock", "schist" },
        { "stone", "pebble" }
    };
    std::map<std::string, std::string> major_lager = {
        {"beer", "lager" }
    };
    std::set<std::string> rock_and_stone = { "rock", "stone" };
    CHECK( map_equal_ignoring_keys( rock_and_beer, beer_and_stone, rock_and_stone ) );

    // Tests still work when one map has more keys than the other, as long as all are ignored.
    CHECK( map_equal_ignoring_keys( major_lager, lagers_are_best, rock_and_stone ) );
    CHECK( map_equal_ignoring_keys( lagers_are_best, major_lager, rock_and_stone ) );

    CHECK_FALSE( map_equal_ignoring_keys( rock_and_beer, lagers_are_best, rock_and_stone ) );
    CHECK_FALSE( map_equal_ignoring_keys( lagers_are_best, beer_and_stone, rock_and_stone ) );
}

TEST_CASE( "check_debug_menu_string_methods", "[debug_menu][nogame]" )
{
    std::map<std::string, std::vector<std::string>> split_expect = {
        { "", { } },
        { "a", { "a" } },
        { ",a", { "a" } },
        { "a,", { "a" } },
        { ",a,", { "a" } },
        { ",,a,,", { "a" } },
        { "a,b,a\nb,фыва,,a,,,b", { "a", "b", "a\nb", "фыва", "a", "b" } },
    };
    std::map<std::string, std::vector<std::string>> joined_expects = {
        { "", { } },
        { "a", { "a" } },
        { "a,b,a\nb,фыва,a,b", { "a", "b", "a\nb", "фыва", "a", "b" } },
    };
    for( const std::pair<const std::string, std::vector<std::string>> &pair : split_expect ) {
        std::vector<std::string> split = debug_menu::string_to_iterable<std::vector<std::string>>
                                         ( pair.first, "," );
        CAPTURE( pair.first );
        CAPTURE( pair.second );
        CHECK( pair.second == split );
    }

    for( const std::pair<const std::string, std::vector<std::string>> &pair : joined_expects ) {
        std::string joined = debug_menu::iterable_to_string( pair.second, "," );
        CAPTURE( pair.first );
        CAPTURE( pair.second );
        CHECK( pair.first == joined );
    }
}

TEST_CASE( "lcmatch", "[utility][nogame]" )
{
    CHECK( lcmatch( "bo", "bo" ) == true );
    CHECK( lcmatch( "Bo", "bo" ) == true );
    CHECK( lcmatch( "Bo", "bö" ) == false );
    CHECK( lcmatch( "Bo", "co" ) == false );

    CHECK( lcmatch( "Bö", "bo" ) == true );
    CHECK( lcmatch( "Bō", "bo" ) == true );
    CHECK( lcmatch( "BÖ", "bo" ) == true );
    CHECK( lcmatch( "BÖ", "bö" ) == true );
    CHECK( lcmatch( "BÖ", "bö" ) == true );
    CHECK( lcmatch( "BŌ", "bo" ) == true );
    CHECK( lcmatch( "BŌ", "bō" ) == true );
    CHECK( lcmatch( "BŌ", "bō" ) == true );
    CHECK( lcmatch( "Bö", "co" ) == false );
    CHECK( lcmatch( "Bö", "bō" ) == false );
    CHECK( lcmatch( "Bō", "co" ) == false );
    CHECK( lcmatch( "Bō", "bö" ) == false );
    CHECK( lcmatch( "BÖ", "bō" ) == false );
    CHECK( lcmatch( "BŌ", "bö" ) == false );

    CHECK( lcmatch( "«101 борцовский приём»", "при" ) == true );
    CHECK( lcmatch( "«101 борцовский приём»", "прИ" ) == true );
    CHECK( lcmatch( "«101 борцовский приём»", "прб" ) == false );

    CHECK( lcmatch( "無効", "無" ) == true );
    CHECK( lcmatch( "無効", "無效" ) == false );
}
