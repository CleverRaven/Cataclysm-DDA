#include "catch/catch.hpp"

#include <chrono>
#include <functional>
#include <unordered_set>

#include "map.h"

#include "enums.h"

// A larger number for this would be GREAT, but the test isn't efficient enough to make it larger.
constexpr int MAX_COORDINATE = 100;

TEST_CASE( "point_hash_distribution", "[hash]" )
{
    std::vector<point> points = closest_points_first( MAX_COORDINATE, { 0, 0 } );
    std::unordered_set<size_t> hashtogram;
    size_t element_count = 0;
    for( const point &p : points ) {
        element_count++;
        size_t sample_hash = std::hash<point> {}( p );
        hashtogram.insert( sample_hash );
    }
    CHECK( hashtogram.size() > element_count * 0.9 );
}

TEST_CASE( "tripoint_hash_distribution", "[hash]" )
{
    std::vector<point> points = closest_points_first( MAX_COORDINATE, { 0, 0 } );
    std::unordered_set<size_t> hashtogram;
    size_t element_count = 0;
    for( const point &p : points ) {
        for( int k = -10; k <= 10; ++k ) {
            element_count++;
            tripoint sample_point = { p.x, p.y, k };
            size_t sample_hash = std::hash<tripoint> {}( sample_point );
            hashtogram.insert( sample_hash );
        }
    }
    CHECK( hashtogram.size() > element_count * 0.9 );
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

// These are the old versions of the hash functions for point and tripoint.
struct legacy_point_hash {
    std::size_t operator()( const point &k ) const {
        // Circular shift y by half its width so hash(5,6) != hash(6,5).
        return std::hash<int>()( k.x ) ^ std::hash<int>()( ( k.y << 16 ) | ( k.y >> 16 ) );
    }
};

struct legacy_tripoint_hash {
    std::size_t operator()( const tripoint &k ) const {
        // Circular shift y and z so hash(5,6,7) != hash(7,6,5).
        return std::hash<int>()( k.x ) ^
               std::hash<int>()( ( k.y << 10 ) | ( k.y >> 10 ) ) ^
               std::hash<int>()( ( k.z << 20 ) | ( k.z >> 20 ) );
    }
};

// These legacy checks are expected to fail.
TEST_CASE( "legacy_point_hash_distribution", "[.]" )
{
    std::vector<point> points = closest_points_first( MAX_COORDINATE, { 0, 0 } );
    std::unordered_set<size_t> hashtogram;
    size_t element_count = 0;
    for( const point &p : points ) {
        element_count++;
        size_t sample_hash = legacy_point_hash{}( p );
        hashtogram.insert( sample_hash );
    }
    CHECK( hashtogram.size() > element_count * 0.9 );
}

TEST_CASE( "legacy_tripoint_hash_distribution", "[.]" )
{
    std::vector<point> points = closest_points_first( MAX_COORDINATE, { 0, 0 } );
    std::unordered_set<size_t> hashtogram;
    size_t element_count = 0;
    for( const point &p : points ) {
        for( int k = -10; k <= 10; ++k ) {
            element_count++;
            tripoint sample_point = { p.x, p.y, k };
            size_t sample_hash = legacy_tripoint_hash{}( sample_point );
            hashtogram.insert( sample_hash );
        }
    }
    CHECK( hashtogram.size() > element_count * 0.9 );
}

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

TEST_CASE( "legacy_point_hash_runoff", "[hash]" )
{
    long legacy_runtime = get_set_runtime<point, legacy_point_hash>();
    long new_runtime = get_set_runtime<point, std::hash<point>>();
    CHECK( new_runtime < legacy_runtime );
}

TEST_CASE( "legacy_tripoint_hash_runoff", "[hash]" )
{
    long legacy_runtime = get_set_runtime<tripoint, legacy_tripoint_hash>();
    long new_runtime = get_set_runtime<tripoint, std::hash<tripoint>>();
    CHECK( new_runtime < legacy_runtime );
}
