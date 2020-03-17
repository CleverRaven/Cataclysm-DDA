#define CATCH_CONFIG_ENABLE_PAIR_STRINGMAKER

#include "cata_algo.h"

#include "catch/catch.hpp"

static void check_cycle_finding( std::unordered_map<int, std::vector<int>> &g,
                                 std::vector<std::vector<int>> &expected )
{
    CAPTURE( g );
    std::vector<std::vector<int>> loops = cata::find_cycles( g );
    // Canonicalize the list of loops by rotating each to be lexicographically
    // least and then sorting.
    for( std::vector<int> &loop : loops ) {
        auto min = std::min_element( loop.begin(), loop.end() );
        std::rotate( loop.begin(), min, loop.end() );
    }
    std::sort( loops.begin(), loops.end() );
    CHECK( loops == expected );
}

TEST_CASE( "find_cycles_small" )
{
    std::unordered_map<int, std::vector<int>> g = {
        { 0, { 1 } },
        { 1, { 0 } },
    };
    std::vector<std::vector<int>> expected = {
        { 0, 1 },
    };
    check_cycle_finding( g, expected );
}
TEST_CASE( "find_cycles" )
{
    std::unordered_map<int, std::vector<int>> g = {
        { 0, { 0, 1 } },
        { 1, { 0, 2, 3, 17 } },
        { 2, { 1 } },
        { 3, {} },
    };
    std::vector<std::vector<int>> expected = {
        { 0 },
        { 0, 1 },
        { 1, 2 },
    };
    check_cycle_finding( g, expected );
}
