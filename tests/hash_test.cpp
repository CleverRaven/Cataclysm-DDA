#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <unordered_map>
#include <unordered_set>
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

struct new_test_hash {
    std::size_t operator()( const point &k ) const noexcept {
        return std::hash<uint64_t> {}( static_cast<uint64_t>( k.x ) << 32 | static_cast<uint32_t>( k.y ) );
    }
};

struct old_test_hash {
    std::size_t operator()( const point &k ) const noexcept {
        constexpr uint64_t a = 2862933555777941757;
        size_t result = k.y;
        result *= a;
        result += k.x;
        return result;
    }
};

struct xor_test_hash {
    std::size_t operator()( const point &k ) const noexcept {
        uint64_t x = static_cast<uint64_t>( k.x ) << 32 | static_cast<uint32_t>( k.y );
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        return x;
    }
};

struct slosh_test_hash {
    std::size_t operator()( const point &k ) const noexcept {
        uint64_t x = static_cast<uint64_t>( k.x ) << 32 | static_cast<uint32_t>( k.y );
        x ^= x >> 30;
        x *= 0xbf58476d1ce4e5b9U;
        x ^= x >> 27;
        x *= 0x94d049bb133111ebU;
        x ^= x >> 31;
        return x;
    }
};

#define FNV_64_PRIME static_cast<uint64_t>(0x100000001b3ULL)
#define FNV1_64_INIT static_cast<uint64_t>(0xcbf29ce484222325ULL)

struct fnv_test_hash {
    std::size_t operator()( const point &k ) const noexcept {
        const uint32_t x = k.x;
        const uint32_t y = k.y;
        uint64_t hval = FNV1_64_INIT;
        hval ^= static_cast<uint64_t>( x & 0xFF );
        hval *= FNV_64_PRIME;
        hval ^= static_cast<uint64_t>( ( x >> 8 ) & 0xFFUL );
        hval *= FNV_64_PRIME;
        hval ^= static_cast<uint64_t>( ( x >> 16 ) & 0xFFUL );
        hval *= FNV_64_PRIME;
        hval ^= static_cast<uint64_t>( ( x >> 24 ) & 0xFFUL );
        hval *= FNV_64_PRIME;
        hval ^= static_cast<uint64_t>( y & 0xFFUL );
        hval *= FNV_64_PRIME;
        hval ^= static_cast<uint64_t>( ( y >> 8 ) & 0xFFUL );
        hval *= FNV_64_PRIME;
        hval ^= static_cast<uint64_t>( ( y >> 16 ) & 0xFFUL );
        hval *= FNV_64_PRIME;
        hval ^= static_cast<uint64_t>( ( y >> 24 ) & 0xFFUL );
        hval *= FNV_64_PRIME;

        return hval;
    }
};

template <class Set>
double unordered_set_badness( Set &test_set, int min, int max )
{
    for( int x = min; x <= max; ++x ) {
        for( int y = min; y <= max; ++y ) {
            test_set.emplace( x, y );
        }
    }
    size_t num_buckets = test_set.bucket_count();
    WARN( "buckets: " <<  num_buckets );
    double const lambda = test_set.size() / double( num_buckets );
    std::array<uint64_t, 3> histogram = {0};
    double cost = 0.0;
    bool reported_bad_bucket = false;
    for( size_t i = 0; i < num_buckets; ++i ) {
        int size = test_set.bucket_size( i );
        cost += size * size;
        if( test_set.bucket_size( i ) >= 3 ) {
            if( !reported_bad_bucket ) {
                reported_bad_bucket = true;
                WARN( "Encountered bucket with " << test_set.bucket_size( i ) << " elements." );
            }
        } else {
            histogram[test_set.bucket_size( i )] += 1;
        }
    }
    cost /= test_set.size();
    INFO( "histogram[1] counts the number of hash buckets with a single element." );
    INFO( "histogram[2] counts the number of hash buckets with 2 elements." );
    INFO( "A failure here means that there are more elements landing in shared buckets" );
    INFO( "(and experiencing worse performance) than there are elements alone in their bucket." );
    CHECK( histogram[1] > histogram[2] * 2 );
    return std::max( 0.0, cost / ( 1 + lambda ) - 1 );
}

TEST_CASE( "point_set_collision_check", "[!mayfail]" )
{
    WARN( "Symmetric points" );
    {
        WARN( "Old hash" );
        std::unordered_set<point, old_test_hash> old_set;
        CHECK( unordered_set_badness( old_set, -MAX_COORDINATE, MAX_COORDINATE ) == 0.0 );
    }
    {
        WARN( "New hash" );
        std::unordered_set<point, new_test_hash> new_set;
        CHECK( unordered_set_badness( new_set, -MAX_COORDINATE, MAX_COORDINATE ) == 0.0 );
    }
    {
        WARN( "Xor hash" );
        std::unordered_set<point, xor_test_hash> xor_set;
        CHECK( unordered_set_badness( xor_set, -MAX_COORDINATE, MAX_COORDINATE ) == 0.0 );
    }
    {
        WARN( "Slosh hash" );
        std::unordered_set<point, slosh_test_hash> slosh_set;
        CHECK( unordered_set_badness( slosh_set, -MAX_COORDINATE, MAX_COORDINATE ) == 0.0 );
    }
    {
        WARN( "Fnv hash" );
        std::unordered_set<point, fnv_test_hash> fnv_set;
        CHECK( unordered_set_badness( fnv_set, -MAX_COORDINATE, MAX_COORDINATE ) == 0.0 );
    }
    WARN( "Positive points" );
    {
        WARN( "Old hash" );
        std::unordered_set<point, old_test_hash> set_sized_old;
        CHECK( unordered_set_badness( set_sized_old, 0, 11 * 12 ) == 0.0 );
    }
    {
        WARN( "New hash" );
        std::unordered_set<point, new_test_hash> set_sized_new;
        CHECK( unordered_set_badness( set_sized_new, 0, 11 * 12 ) == 0.0 );
    }
    {
        WARN( "Xor hash" );
        std::unordered_set<point, xor_test_hash> set_sized_xor;
        CHECK( unordered_set_badness( set_sized_xor, 0, 11 * 12 ) == 0.0 );
    }
    {
        WARN( "Slosh hash" );
        std::unordered_set<point, slosh_test_hash> set_sized_slosh;
        CHECK( unordered_set_badness( set_sized_slosh, 0, 11 * 12 ) == 0.0 );
    }
    {
        WARN( "Fnv hash" );
        std::unordered_set<point, fnv_test_hash> set_sized_fnv;
        CHECK( unordered_set_badness( set_sized_fnv, 0, 11 * 12 ) == 0.0 );
    }
}


template <class Map>
double unordered_map_badness( Map &test_map, int min, int max )
{
    for( int x = min; x <= max; ++x ) {
        for( int y = min; y <= max; ++y ) {
            test_map.try_emplace( { x, y }, 1 );
        }
    }
    size_t num_buckets = test_map.bucket_count();
    WARN( "buckets: " <<  num_buckets );
    double const lambda = test_map.size() / double( num_buckets );
    std::array<uint64_t, 3> histogram = {0};
    double cost = 0.0;
    bool reported_bad_bucket = false;
    for( size_t i = 0; i < num_buckets; ++i ) {
        int size = test_map.bucket_size( i );
        cost += size * size;
        if( test_map.bucket_size( i ) >= 3 ) {
            if( !reported_bad_bucket ) {
                reported_bad_bucket = true;
                WARN( "Encountered bucket with " << test_map.bucket_size( i ) << " elements." );
            }
        } else {
            histogram[test_map.bucket_size( i )] += 1;
        }
    }
    cost /= test_map.size();
    INFO( "histogram[1] counts the number of hash buckets with a single element." );
    INFO( "histogram[2] counts the number of hash buckets with 2 elements." );
    INFO( "A failure here means that there are more elements landing in shared buckets" );
    INFO( "(and experiencing worse performance) than there are elements alone in their bucket." );
    CHECK( histogram[1] > histogram[2] * 2 );
    return std::max( 0.0, cost / ( 1 + lambda ) - 1 );
}

TEST_CASE( "point_map_collision_check", "[!mayfail]" )
{
    WARN( "Symmetric points" );
    {
        WARN( "Old hash" );
        std::unordered_map<point, int, old_test_hash> old_map;
        CHECK( unordered_map_badness( old_map, -MAX_COORDINATE, MAX_COORDINATE ) == 0.0 );
    }
    {
        WARN( "New hash" );
        std::unordered_map<point, int, new_test_hash> new_map;
        CHECK( unordered_map_badness( new_map, -MAX_COORDINATE, MAX_COORDINATE ) == 0.0 );
    }
    {
        WARN( "Xor hash" );
        std::unordered_map<point, int, xor_test_hash> xor_map;
        CHECK( unordered_map_badness( xor_map, -MAX_COORDINATE, MAX_COORDINATE ) == 0.0 );
    }
    {
        WARN( "Slosh hash" );
        std::unordered_map<point, int, slosh_test_hash> slosh_map;
        CHECK( unordered_map_badness( slosh_map, -MAX_COORDINATE, MAX_COORDINATE ) == 0.0 );
    }
    {
        WARN( "Fnv hash" );
        std::unordered_map<point, int, fnv_test_hash> fnv_map;
        CHECK( unordered_map_badness( fnv_map, -MAX_COORDINATE, MAX_COORDINATE ) == 0.0 );
    }
    WARN( "Positive points" );
    {
        WARN( "Old hash" );
        std::unordered_map<point, int, old_test_hash> map_sized_old;
        CHECK( unordered_map_badness( map_sized_old, 0, 11 * 12 ) == 0.0 );
    }
    {
        WARN( "New hash" );
        std::unordered_map<point, int, new_test_hash> map_sized_new;
        CHECK( unordered_map_badness( map_sized_new, 0, 11 * 12 ) == 0.0 );
    }
    {
        WARN( "Xor hash" );
        std::unordered_map<point, int, xor_test_hash> map_sized_xor;
        CHECK( unordered_map_badness( map_sized_xor, 0, 11 * 12 ) == 0.0 );
    }
    {
        WARN( "Slosh hash" );
        std::unordered_map<point, int, slosh_test_hash> map_sized_slosh;
        CHECK( unordered_map_badness( map_sized_slosh, 0, 11 * 12 ) == 0.0 );
    }
    {
        WARN( "Fnv hash" );
        std::unordered_map<point, int, fnv_test_hash> map_sized_fnv;
        CHECK( unordered_map_badness( map_sized_fnv, 0, 11 * 12 ) == 0.0 );
    }
}

TEST_CASE( "point_hash_distribution", "[hash]" )
{
    std::vector<size_t> found_hashes;
    found_hashes.reserve( NUM_ENTRIES_2D );
    size_t element_count = 0;
    for( int x = -MAX_COORDINATE; x <= MAX_COORDINATE; ++x ) {
        for( int y = -MAX_COORDINATE; y <= MAX_COORDINATE; ++y ) {
            element_count++;
            found_hashes.push_back( new_test_hash {}( point{ x, y } ) );
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
