#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <vector>

#include "cata_catch.h"
#include "point.h"

// A larger number for this would be GREAT, but the test isn't efficient enough to make it larger.
// Previously tried inserting into an unordered_set,
// but that was slower than appending to a vector and doing the sort+unique manually.
// This should be dramatically faster (but approximate) if we insert into a HyperLogLog instead.
#ifdef _GLIBCXX_DEBUG
// Smaller test on libstdc++ debug containers because otherwise this takes ~1 minute.
constexpr int MAX_COORDINATE = 30;
#else
static constexpr int MAX_COORDINATE = 300;
#endif
static constexpr int NUM_ENTRIES_2D = ( ( MAX_COORDINATE * 2 ) + 1 ) * ( (
        MAX_COORDINATE * 2 ) + 1 );
static constexpr int NUM_ENTRIES_3D = NUM_ENTRIES_2D * 21;

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
            found_hashes.push_back( std::hash<point> {}( point{ x, y } ) );
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
