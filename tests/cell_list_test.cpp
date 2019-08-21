#include "catch/catch.hpp"
#include "cell_list.h"
#include "point.h"
#include "options.h"
#include "game.h"

TEST_CASE( "cell_list", "[cell_list]" )
{
    cell_list<int, 3, 10, 10> list;
    list.add( { 2, 2, 0 }, 0 );
    cell_list_range<int, 3, 10, 10> range = list.get( {1, 1, 0}, 1 );
    auto it = range.begin();
    auto end = range.end();
    while( it != end ) {
        const std::pair<tripoint, int> &ref = *it;
        CHECK( ref.first == tripoint( 2, 2, 0 ) );
        CHECK( ref.second == 0 );
        ++it;
    }
}

template< typename T, int CELL_SIZE, int MAXX, int MAXY>
static std::vector<std::pair<tripoint, int>>
        check_range( cell_list_range<T, CELL_SIZE, MAXX, MAXY> &range )
{
    std::vector<std::pair<tripoint, int>> found;
    auto it = range.begin();
    auto end = range.end();
    int prev_dist = 0;
    while( it != end ) {
        const std::pair<tripoint, int> &ref = *it;
        int dist = rl_dist( ref.first, range.get_origin() );
        INFO( ref.first );
        CHECK( prev_dist <= dist );
        CHECK( dist <= range.get_distance() );
        prev_dist = dist;
        found.push_back( ref );
        ++it;
    }
    return found;
}

static void set_trig( bool trig )
{
    get_options().get_option( "CIRCLEDIST" ).setValue( trig ? "true" : "false" );
    trigdist = trig;
}

template< typename T, int CELL_SIZE, int MAXX, int MAXY>
static std::vector<std::pair<tripoint, int>>check_range( cell_list<T, CELL_SIZE, MAXX, MAXY> &list,
        tripoint origin, int distance, bool trig )
{
    set_trig( trig );
    cell_list_range<T, CELL_SIZE, MAXX, MAXY> range = list.get( origin, distance );
    return check_range( range );
}

TEST_CASE( "cell_list_dense", "[cell_list]" )
{
    cell_list<int, 8, 132, 132> list;
    for( int x = 0; x < 132; x += 5 ) {
        for( int y = 0; y < 132; y += 5 ) {
            list.add( { x, y, 0 }, x + y );
        }
    }
    // Check with origin near map origin.
    // 11 x 11 = 121 in square distance range.
    CHECK( check_range( list, {1, 1, 0}, 50, false ).size() == 121 );
    // Considerably fewer since the corner is cut by trig_dist
    CHECK( check_range( list, {1, 1, 0}, 50, true ).size() == 98 );
    // Check lower right corner.
    CHECK( check_range( list, {129, 129, 0}, 50, false ).size() == 121 );
    CHECK( check_range( list, {129, 129, 0}, 50, true ).size() == 98 );
    // Check upper right corner.
    CHECK( check_range( list, {1, 129, 0}, 50, false ).size() == 121 );
    CHECK( check_range( list, {1, 129, 0}, 50, true ).size() == 98 );
    // Check lower left corner.
    CHECK( check_range( list, {129, 1, 0}, 50, false ).size() == 121 );
    CHECK( check_range( list, {129, 1, 0}, 50, true ).size() == 98 );
    // Check from center of grid.
    CHECK( check_range( list, {60, 60, 0}, 50, false ).size() == 440 );
    CHECK( check_range( list, {60, 60, 0}, 50, true ).size() == 332 );
}

TEST_CASE( "cell_list_empty", "[cell_list]" )
{
    cell_list<int, 8, 132, 132> list;
    CHECK( check_range( list, {1, 1, 0}, 50, false ).empty() );
    CHECK( check_range( list, {1, 1, 0}, 50, true ).empty() );
    CHECK( check_range( list, {1, 129, 0}, 50, false ).empty() );
    CHECK( check_range( list, {1, 129, 0}, 50, true ).empty() );
    CHECK( check_range( list, {129, 1, 0}, 50, false ).empty() );
    CHECK( check_range( list, {129, 1, 0}, 50, true ).empty() );
    CHECK( check_range( list, {129, 129, 0}, 50, false ).empty() );
    CHECK( check_range( list, {129, 129, 0}, 50, true ).empty() );
    CHECK( check_range( list, {61, 60, 0}, 50, false ).empty() );
    CHECK( check_range( list, {61, 60, 0}, 50, true ).empty() );
}

TEST_CASE( "cell_list_sparse", "[cell_list]" )
{
    cell_list<int, 4, 32, 32> list;
    for( int x = 0; x < 32; x += 7 ) {
        for( int y = 0; y < 32; y += 7 ) {
            list.add( { x, y, 0}, x + y );
        }
    }
    CHECK( check_range( list, {1, 1, 0}, 20, false ).size() == 16 );
    CHECK( check_range( list, {1, 1, 0}, 20, true ).size() == 13 );
    CHECK( check_range( list, {1, 31, 0}, 20, false ).size() == 12 );
    CHECK( check_range( list, {1, 31, 0}, 20, true ).size() == 9 );
    CHECK( check_range( list, {31, 1, 0}, 20, false ).size() == 12 );
    CHECK( check_range( list, {31, 1, 0}, 20, true ).size() == 9 );
    CHECK( check_range( list, {31, 31, 0}, 20, false ).size() == 9 );
    CHECK( check_range( list, {31, 31, 0}, 20, true ).size() == 8 );
    CHECK( check_range( list, {14, 14, 0}, 14, false ).size() == 24 );
    CHECK( check_range( list, {14, 14, 0}, 14, true ).size() == 12 );
}

TEST_CASE( "cell_list_reuse_range", "[cell_list]" )
{
    cell_list<int, 4, 32, 32> list;
    for( int x = 0; x < 32; x += 7 ) {
        for( int y = 0; y < 32; y += 7 ) {
            list.add( { x, y, 0}, x + y );
        }
    }
    set_trig( false );
    cell_list_range<int, 4, 32, 32> range = list.get( {14, 14, 0}, 14 );
    CHECK( check_range( range ).size() == 24 );
    CHECK( check_range( range ).size() == 24 );
}
