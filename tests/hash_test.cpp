#include <unordered_set>
#include <vector>
#include <algorithm>
#include <cstddef>
#include <iterator>

#include "catch/catch.hpp"
#include "point.h"

#ifdef RELEASE
#include <chrono>
#endif

// A larger number for this would be GREAT, but the test isn't efficient enough to make it larger.
// Previously tried inserting into an unordered_set,
// but that was slower than appending to a vector and doing the sort+unique manually.
// This should be dramatically faster (but approximate) if we insert into a HyperLogLog instead.
#ifdef _GLIBCXX_DEBUG
// Smaller test on libstdc++ debug containers because otherwise this takes ~1 minute.
constexpr int MAX_COORDINATE = 30;
#else
constexpr int MAX_COORDINATE = 300;
#endif
constexpr int NUM_ENTRIES_2D = ( ( MAX_COORDINATE * 2 ) + 1 ) * ( ( MAX_COORDINATE * 2 ) + 1 );
constexpr int NUM_ENTRIES_3D = NUM_ENTRIES_2D * ( 21 );

static size_t count_unique_elements( std::vector<size_t> &found_elements )
{
    std::sort( found_elements.begin(), found_elements.end() );
    const auto range_end = std::unique( found_elements.begin(), found_elements.end() );
    return std::distance( found_elements.begin(), range_end );
}

TEST_CASE( "point_hash_distribution", "[hash]" )
{
    std::vector<size_t> found_hashes;
    found_hashes.reserve( NUM_ENTRIES_2D );
    size_t element_count = 0;
    for( int x = -MAX_COORDINATE; x <= MAX_COORDINATE; ++x ) {
        for( int y = -MAX_COORDINATE; y <= MAX_COORDINATE; ++y ) {
            element_count++;
            found_hashes.push_back( std::hash<point> {}( { x, y } ) );
        }
    }
    CHECK( count_unique_elements( found_hashes ) > element_count * 0.9 );
}

TEST_CASE( "tripoint_hash_distribution", "[hash]" )
{
    std::vector<size_t> found_hashes;
    found_hashes.reserve( NUM_ENTRIES_3D );
    size_t element_count = 0;
    for( int x = -MAX_COORDINATE; x <= MAX_COORDINATE; ++x ) {
        for( int y = -MAX_COORDINATE; y <= MAX_COORDINATE; ++y ) {
            for( int z = -10; z <= 10; ++z ) {
                element_count++;
                found_hashes.push_back( std::hash<tripoint> {}( { x, y, z } ) );
            }
        }
    }
    CHECK( count_unique_elements( found_hashes ) > element_count * 0.9 );
}

template<class Hash>
void put_coordinate( std::unordered_set<point, Hash> &c, int x, int y )
{
    c.emplace( x, y );
}

template<class Hash>
void put_coordinate( std::unordered_set<tripoint, Hash> &c, int x, int y )
{
    c.emplace( x, y, 0 );
}

#ifdef RELEASE
// These tests are slow and probably pointless for non-release builds

template<class CoordinateType, class Hash>
long get_set_runtime()
{
    std::unordered_set<CoordinateType, Hash> test_set;
    auto start1 = std::chrono::high_resolution_clock::now();
    // The use case is repeatedly looking up the same entries repeatedly
    // while sometimes inserting new ones.
    for( int i = 0; i < 1000; ++i ) {
        for( int x = -60; x <= 60; ++x ) {
            for( int y = -60; y <= 60; ++y ) {
                put_coordinate( test_set, x, y );
            }
        }
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>( end1 - start1 ).count();
}
#endif // RELEASE
