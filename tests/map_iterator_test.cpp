#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <vector>

#include "cata_catch.h"
#include "coordinates.h"
#include "line.h"
#include "map_iterator.h"
#include "point.h"

static std::array<tripoint, 9> range_1_2d_centered = {
    {   {tripoint::north_west}, {tripoint::north}, {tripoint::north_east},
        {tripoint::west}, {tripoint::zero}, {tripoint::east},
        {tripoint::south_west}, {tripoint::south}, {tripoint::south_east}
    }
};

TEST_CASE( "Radius_one_2D_square_centered_at_origin", "[tripoint_range]" )
{
    tripoint_range<tripoint> tested( tripoint::north_west, tripoint::south_east );
    REQUIRE( tested.size() == range_1_2d_centered.size() );
    for( const tripoint &candidate : tested ) {
        REQUIRE( std::find( range_1_2d_centered.begin(), range_1_2d_centered.end(), candidate ) !=
                 range_1_2d_centered.end() );
    }
}

static std::array<tripoint, 9> range_1_2d_offset = {
    {   {-5, -5, 0}, {-4, -5, 0}, {-3, -5, 0},
        {-5, -4, 0}, {-4, -4, 0}, {-3, -4, 0},
        {-5, -3, 0}, {-4, -3, 0}, {-3, -3, 0}
    }
};

TEST_CASE( "Radius_one_2D_square_centered_at_-4/-4/0", "[tripoint_range]" )
{
    tripoint_range<tripoint> tested( {-5, -5, 0}, {-3, -3, 0} );
    REQUIRE( tested.size() == range_1_2d_offset.size() );
    for( const tripoint &candidate : tested ) {
        REQUIRE( std::find( range_1_2d_offset.begin(), range_1_2d_offset.end(), candidate ) !=
                 range_1_2d_offset.end() );
    }
}

TEST_CASE( "Radius_one_2D_square_centered_at_-4/-4/0_in_abs_omt_coords", "[tripoint_range]" )
{
    tripoint_range<tripoint_abs_omt> tested( {-5, -5, 0}, {-3, -3, 0} );
    REQUIRE( tested.size() == range_1_2d_offset.size() );
    for( const tripoint_abs_omt &candidate : tested ) {
        REQUIRE( std::find( range_1_2d_offset.begin(), range_1_2d_offset.end(), candidate.raw() ) !=
                 range_1_2d_offset.end() );
    }
}

static std::array<tripoint, 343> range_3_3d_offset = {
    {   { 5, 5, -2}, { 6, 5, -2}, { 7, 5, -2}, { 8, 5, -2}, { 9, 5, -2}, {10, 5, -2}, {11, 5, -2},
        { 5, 6, -2}, { 6, 6, -2}, { 7, 6, -2}, { 8, 6, -2}, { 9, 6, -2}, {10, 6, -2}, {11, 6, -2},
        { 5, 7, -2}, { 6, 7, -2}, { 7, 7, -2}, { 8, 7, -2}, { 9, 7, -2}, {10, 7, -2}, {11, 7, -2},
        { 5, 8, -2}, { 6, 8, -2}, { 7, 8, -2}, { 8, 8, -2}, { 9, 8, -2}, {10, 8, -2}, {11, 8, -2},
        { 5, 9, -2}, { 6, 9, -2}, { 7, 9, -2}, { 8, 9, -2}, { 9, 9, -2}, {10, 9, -2}, {11, 9, -2},
        { 5, 10, -2}, { 6, 10, -2}, { 7, 10, -2}, { 8, 10, -2}, { 9, 10, -2}, {10, 10, -2}, {11, 10, -2},
        { 5, 11, -2}, { 6, 11, -2}, { 7, 11, -2}, { 8, 11, -2}, { 9, 11, -2}, {10, 11, -2}, {11, 11, -2},

        { 5, 5, -1}, { 6, 5, -1}, { 7, 5, -1}, { 8, 5, -1}, { 9, 5, -1}, {10, 5, -1}, {11, 5, -1},
        { 5, 6, -1}, { 6, 6, -1}, { 7, 6, -1}, { 8, 6, -1}, { 9, 6, -1}, {10, 6, -1}, {11, 6, -1},
        { 5, 7, -1}, { 6, 7, -1}, { 7, 7, -1}, { 8, 7, -1}, { 9, 7, -1}, {10, 7, -1}, {11, 7, -1},
        { 5, 8, -1}, { 6, 8, -1}, { 7, 8, -1}, { 8, 8, -1}, { 9, 8, -1}, {10, 8, -1}, {11, 8, -1},
        { 5, 9, -1}, { 6, 9, -1}, { 7, 9, -1}, { 8, 9, -1}, { 9, 9, -1}, {10, 9, -1}, {11, 9, -1},
        { 5, 10, -1}, { 6, 10, -1}, { 7, 10, -1}, { 8, 10, -1}, { 9, 10, -1}, {10, 10, -1}, {11, 10, -1},
        { 5, 11, -1}, { 6, 11, -1}, { 7, 11, -1}, { 8, 11, -1}, { 9, 11, -1}, {10, 11, -1}, {11, 11, -1},

        { 5, 5, 0}, { 6, 5, 0}, { 7, 5, 0}, { 8, 5, 0}, { 9, 5, 0}, {10, 5, 0}, {11, 5, 0},
        { 5, 6, 0}, { 6, 6, 0}, { 7, 6, 0}, { 8, 6, 0}, { 9, 6, 0}, {10, 6, 0}, {11, 6, 0},
        { 5, 7, 0}, { 6, 7, 0}, { 7, 7, 0}, { 8, 7, 0}, { 9, 7, 0}, {10, 7, 0}, {11, 7, 0},
        { 5, 8, 0}, { 6, 8, 0}, { 7, 8, 0}, { 8, 8, 0}, { 9, 8, 0}, {10, 8, 0}, {11, 8, 0},
        { 5, 9, 0}, { 6, 9, 0}, { 7, 9, 0}, { 8, 9, 0}, { 9, 9, 0}, {10, 9, 0}, {11, 9, 0},
        { 5, 10, 0}, { 6, 10, 0}, { 7, 10, 0}, { 8, 10, 0}, { 9, 10, 0}, {10, 10, 0}, {11, 10, 0},
        { 5, 11, 0}, { 6, 11, 0}, { 7, 11, 0}, { 8, 11, 0}, { 9, 11, 0}, {10, 11, 0}, {11, 11, 0},

        { 5, 5, 1}, { 6, 5, 1}, { 7, 5, 1}, { 8, 5, 1}, { 9, 5, 1}, {10, 5, 1}, {11, 5, 1},
        { 5, 6, 1}, { 6, 6, 1}, { 7, 6, 1}, { 8, 6, 1}, { 9, 6, 1}, {10, 6, 1}, {11, 6, 1},
        { 5, 7, 1}, { 6, 7, 1}, { 7, 7, 1}, { 8, 7, 1}, { 9, 7, 1}, {10, 7, 1}, {11, 7, 1},
        { 5, 8, 1}, { 6, 8, 1}, { 7, 8, 1}, { 8, 8, 1}, { 9, 8, 1}, {10, 8, 1}, {11, 8, 1},
        { 5, 9, 1}, { 6, 9, 1}, { 7, 9, 1}, { 8, 9, 1}, { 9, 9, 1}, {10, 9, 1}, {11, 9, 1},
        { 5, 10, 1}, { 6, 10, 1}, { 7, 10, 1}, { 8, 10, 1}, { 9, 10, 1}, {10, 10, 1}, {11, 10, 1},
        { 5, 11, 1}, { 6, 11, 1}, { 7, 11, 1}, { 8, 11, 1}, { 9, 11, 1}, {10, 11, 1}, {11, 11, 1},

        { 5, 5, 2}, { 6, 5, 2}, { 7, 5, 2}, { 8, 5, 2}, { 9, 5, 2}, {10, 5, 2}, {11, 5, 2},
        { 5, 6, 2}, { 6, 6, 2}, { 7, 6, 2}, { 8, 6, 2}, { 9, 6, 2}, {10, 6, 2}, {11, 6, 2},
        { 5, 7, 2}, { 6, 7, 2}, { 7, 7, 2}, { 8, 7, 2}, { 9, 7, 2}, {10, 7, 2}, {11, 7, 2},
        { 5, 8, 2}, { 6, 8, 2}, { 7, 8, 2}, { 8, 8, 2}, { 9, 8, 2}, {10, 8, 2}, {11, 8, 2},
        { 5, 9, 2}, { 6, 9, 2}, { 7, 9, 2}, { 8, 9, 2}, { 9, 9, 2}, {10, 9, 2}, {11, 9, 2},
        { 5, 10, 2}, { 6, 10, 2}, { 7, 10, 2}, { 8, 10, 2}, { 9, 10, 2}, {10, 10, 2}, {11, 10, 2},
        { 5, 11, 2}, { 6, 11, 2}, { 7, 11, 2}, { 8, 11, 2}, { 9, 11, 2}, {10, 11, 2}, {11, 11, 2},

        { 5, 5, 3}, { 6, 5, 3}, { 7, 5, 3}, { 8, 5, 3}, { 9, 5, 3}, {10, 5, 3}, {11, 5, 3},
        { 5, 6, 3}, { 6, 6, 3}, { 7, 6, 3}, { 8, 6, 3}, { 9, 6, 3}, {10, 6, 3}, {11, 6, 3},
        { 5, 7, 3}, { 6, 7, 3}, { 7, 7, 3}, { 8, 7, 3}, { 9, 7, 3}, {10, 7, 3}, {11, 7, 3},
        { 5, 8, 3}, { 6, 8, 3}, { 7, 8, 3}, { 8, 8, 3}, { 9, 8, 3}, {10, 8, 3}, {11, 8, 3},
        { 5, 9, 3}, { 6, 9, 3}, { 7, 9, 3}, { 8, 9, 3}, { 9, 9, 3}, {10, 9, 3}, {11, 9, 3},
        { 5, 10, 3}, { 6, 10, 3}, { 7, 10, 3}, { 8, 10, 3}, { 9, 10, 3}, {10, 10, 3}, {11, 10, 3},
        { 5, 11, 3}, { 6, 11, 3}, { 7, 11, 3}, { 8, 11, 3}, { 9, 11, 3}, {10, 11, 3}, {11, 11, 3},

        { 5, 5, 4}, { 6, 5, 4}, { 7, 5, 4}, { 8, 5, 4}, { 9, 5, 4}, {10, 5, 4}, {11, 5, 4},
        { 5, 6, 4}, { 6, 6, 4}, { 7, 6, 4}, { 8, 6, 4}, { 9, 6, 4}, {10, 6, 4}, {11, 6, 4},
        { 5, 7, 4}, { 6, 7, 4}, { 7, 7, 4}, { 8, 7, 4}, { 9, 7, 4}, {10, 7, 4}, {11, 7, 4},
        { 5, 8, 4}, { 6, 8, 4}, { 7, 8, 4}, { 8, 8, 4}, { 9, 8, 4}, {10, 8, 4}, {11, 8, 4},
        { 5, 9, 4}, { 6, 9, 4}, { 7, 9, 4}, { 8, 9, 4}, { 9, 9, 4}, {10, 9, 4}, {11, 9, 4},
        { 5, 10, 4}, { 6, 10, 4}, { 7, 10, 4}, { 8, 10, 4}, { 9, 10, 4}, {10, 10, 4}, {11, 10, 4},
        { 5, 11, 4}, { 6, 11, 4}, { 7, 11, 4}, { 8, 11, 4}, { 9, 11, 4}, {10, 11, 4}, {11, 11, 4}
    }
};

TEST_CASE( "Radius_three_3D_square_centered_at_8/8/1", "[tripoint_range]" )
{
    tripoint_range<tripoint> tested( {5, 5, -2}, {11, 11, 4} );
    REQUIRE( tested.size() == range_3_3d_offset.size() );
    for( const tripoint &candidate : tested ) {
        REQUIRE( std::find( range_3_3d_offset.begin(), range_3_3d_offset.end(), candidate ) !=
                 range_3_3d_offset.end() );
    }
}

TEST_CASE( "tripoint_range_iteration_order", "[tripoint_range]" )
{
    tripoint_range<tripoint> tested( tripoint( 4, 4, 0 ), tripoint( 6, 6, 0 ) );
    std::vector<tripoint> expected = {
        { 4, 4, 0 }, { 5, 4, 0 }, { 6, 4, 0 },
        { 4, 5, 0 }, { 5, 5, 0 }, { 6, 5, 0 },
        { 4, 6, 0 }, { 5, 6, 0 }, { 6, 6, 0 }
    };
    REQUIRE( tested.size() == expected.size() );
    int i = 0;
    for( const tripoint &pt : tested ) {
        CHECK( pt == expected[i] );
        ++i;
    }
}

// Using static functions instead of lambdas to suppress -Wmaybe-uninitialized
// false positives triggered by lambda's implicit anonymous data structure in
// debug builds
static bool tripoint_range_handle_bad_predicates_test_func( const tripoint & )
{
    return false;
}

TEST_CASE( "tripoint_range_handle_bad_predicates", "[tripoint_range]" )
{
    tripoint_range<tripoint> tested( tripoint( 4, 4, 0 ), tripoint( 6, 6, 0 ),
                                     tripoint_range_handle_bad_predicates_test_func );
    REQUIRE( tested.empty() );
    int visited = 0;
    for( const tripoint &pt : tested ) {
        INFO( pt );
        REQUIRE( false );
        ++visited;
    }
    CHECK( visited == 0 );
}

TEST_CASE( "tripoint_range_circle_order", "[tripoint_range]" )
{
    const tripoint center( 6, 6, 0 );
    tripoint_range<tripoint> range_test( tripoint( 4, 4, 0 ), tripoint( 8, 8,
    0 ), [center]( const tripoint & pt ) {
        return trig_dist( center, pt ) < 2.5;
    } );
    tripoint_range<tripoint> radius_test = points_in_radius_circ( center, 2 );
    std::vector<tripoint> expected = {
        { 5, 4, 0 }, { 6, 4, 0 }, { 7, 4, 0 },
        { 4, 5, 0 }, { 5, 5, 0 }, { 6, 5, 0 }, { 7, 5, 0 }, { 8, 5, 0 },
        { 4, 6, 0 }, { 5, 6, 0 }, center, { 7, 6, 0 }, { 8, 6, 0 },
        { 4, 7, 0 }, { 5, 7, 0 }, { 6, 7, 0 }, { 7, 7, 0 }, { 8, 7, 0 },
        { 5, 8, 0 }, { 6, 8, 0 }, { 7, 8, 0 },
    };
    REQUIRE( range_test.size() == expected.size() );
    REQUIRE( radius_test.size() == expected.size() );
    size_t range = 0;
    size_t radius = 0;
    for( const tripoint &pt : range_test ) {
        CHECK( pt == expected[range] );
        ++range;
    }
    for( const tripoint &pt : radius_test ) {
        CHECK( pt == expected[radius] );
        ++radius;
    }
    CHECK( range == expected.size() );
    CHECK( radius == expected.size() );
}

TEST_CASE( "tripoint_range_circle_sizes_correct", "[tripoint_range]" )
{
    /* 0:
     * ...
     * .x.
     * ...
     */
    CHECK( points_in_radius_circ( tripoint::zero, 0 ).size() == 1 );
    /* 1:
     * xxx
     * xxx
     * xxx
     */
    CHECK( points_in_radius_circ( tripoint::zero, 1 ).size() == 9 );
    /* 2:
     * .xxx.
     * xxxxx
     * xxxxx
     * xxxxx
     * .xxx.
     */
    CHECK( points_in_radius_circ( tripoint::zero, 2 ).size() == 21 );
    /* 3:
     * ..xxx..
     * .xxxxx.
     * xxxxxxx
     * xxxxxxx
     * xxxxxxx
     * .xxxxx.
     * ..xxx..
     */
    CHECK( points_in_radius_circ( tripoint::zero, 3 ).size() == 37 );
    /* 4:
     * ..xxxxx..
     * .xxxxxxx.
     * xxxxxxxxx
     * xxxxxxxxx
     * xxxxxxxxx
     * xxxxxxxxx
     * xxxxxxxxx
     * .xxxxxxx.
     * ..xxxxx..
     */
    CHECK( points_in_radius_circ( tripoint::zero, 4 ).size() == 69 );
}

// Using static functions instead of lambdas to suppress -Wmaybe-uninitialized
// false positives triggered by lambda's implicit anonymous data structure in
// debug builds
static bool tripoint_range_predicates_radius_test_func( const tripoint &pt )
{
    return pt.z < 0;
}

TEST_CASE( "tripoint_range_predicates_radius", "[tripoint_range]" )
{
    tripoint_range<tripoint> tested = points_in_radius_where( tripoint::zero, 2,
                                      tripoint_range_predicates_radius_test_func, 2 );
    std::vector<tripoint> expected = {
        { -2, -2, -2 }, { -1, -2, -2 }, { 0, -2, -2 }, { 1, -2, -2 }, { 2, -2, -2 },
        { -2, -1, -2 }, { -1, -1, -2 }, { 0, -1, -2 }, { 1, -1, -2 }, { 2, -1, -2 },
        { -2,  0, -2 }, { -1,  0, -2 }, { 0,  0, -2 }, { 1,  0, -2 }, { 2,  0, -2 },
        { -2,  1, -2 }, { -1,  1, -2 }, { 0,  1, -2 }, { 1,  1, -2 }, { 2,  1, -2 },
        { -2,  2, -2 }, { -1,  2, -2 }, { 0,  2, -2 }, { 1,  2, -2 }, { 2,  2, -2 },

        { -2, -2, -1 }, { -1, -2, -1 }, { 0, -2, -1 }, { 1, -2, -1 }, { 2, -2, -1 },
        { -2, -1, -1 }, { -1, -1, -1 }, { 0, -1, -1 }, { 1, -1, -1 }, { 2, -1, -1 },
        { -2,  0, -1 }, { -1,  0, -1 }, tripoint::below, { 1,  0, -1 }, { 2,  0, -1 },
        { -2,  1, -1 }, { -1,  1, -1 }, { 0,  1, -1 }, { 1,  1, -1 }, { 2,  1, -1 },
        { -2,  2, -1 }, { -1,  2, -1 }, { 0,  2, -1 }, { 1,  2, -1 }, { 2,  2, -1 },
    };
    REQUIRE( tested.size() == expected.size() );
    size_t i = 0;
    for( const tripoint &pt : tested ) {
        CHECK( pt == expected[i] );
        ++i;
    }
    CHECK( i == tested.size() );
}

// Using static functions instead of lambdas to suppress -Wmaybe-uninitialized
// false positives triggered by lambda's implicit anonymous data structure in
// debug builds
static bool tripoint_range_predicates_test_func( const tripoint &pt )
{
    return pt.x == 0;
}

TEST_CASE( "tripoint_range_predicates", "[tripoint_range]" )
{
    tripoint_range<tripoint> tested( tripoint::north_west, tripoint::south_east,
                                     tripoint_range_predicates_test_func );
    std::vector<tripoint> expected = {
        tripoint::north, tripoint::zero, tripoint::south
    };
    REQUIRE( tested.size() == expected.size() );
    size_t i = 0;
    for( const tripoint &pt : tested ) {
        CHECK( pt == expected[i] );
        ++i;
    }
    CHECK( i == tested.size() );
}
