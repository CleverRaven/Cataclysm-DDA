#include <iomanip>

#include "map.h"
#include "map_test_case.h"

// test some of the map_test_case helpers
TEST_CASE( "map_test_case_common", "[map_test_case]" )
{
    using namespace map_test_case_common;
    SECTION( "|| and &&" ) {
        std::function<bool( int )> a = []( int arg ) {
            return arg > 0;
        };
        std::function<bool( int )> b = []( int arg ) {
            return arg < 5;
        };

        auto a_and_b = a && b;
        auto a_or_b = a || b;

        CHECK( a_and_b( 1 ) );
        CHECK_FALSE( a_and_b( -1 ) );

        CHECK( a_or_b( -1 ) );
        CHECK( a_or_b( 1 ) );
        CHECK( a_or_b( 6 ) );
    }
    SECTION( "a && a && a" ) {
        std::function<bool()> a = []() {
            return true;
        };
        std::function<bool()> b = []() {
            return false;
        };

        auto aaa = a && a && a; // NOLINT it's a test, it's fine
        auto aab = a && a && b; // NOLINT
        CHECK( aaa() );
        CHECK_FALSE( aab() );
    }
    SECTION( "inc + inc + inc" ) {
        int counter = 0;
        std::function<void()> inc = [&]() {
            counter++;
        };
        auto f = inc + inc + inc;
        CHECK( counter == 0 );
        f();
        CHECK( counter == 3 );
    }
}

// checks the consistency of all transformations
// and valid coordinate calculation
TEST_CASE( "map_test_case_transform_consistency", "[map_test_case]" )
{
    using namespace map_test_case_common;
    using mtc = map_test_case;

    mtc tst;
    tst.anchor_char = '5';
    tst.setup = {
        "123",
        "456",
        "789",
    };
    tst.expected_results = {
        "123",
        "456",
        "789",
    };

    SECTION( tst.generate_transform_combinations() ) {
    }

    // assumes tst.anchor_map_pos == tripoint_zero
    CHECK( tst.get_origin() == tripoint_north_west );
    tst.validate_anchor_point( tripoint_zero );

    std::set<tripoint> tiles;
    tst.for_each_tile( [&]( mtc::tile t ) {
        tiles.emplace( t.p );
    } );

    CHECK( tiles.size() == 9 );

    std::vector<tripoint> neigh_vec(
        std::begin( eight_horizontal_neighbors ),
        std::end( eight_horizontal_neighbors )
    );
    neigh_vec.push_back( tripoint_zero );
    CHECK_THAT( std::vector<tripoint>( tiles.begin(), tiles.end() ),
                Catch::UnorderedEquals( neigh_vec ) );

    tst.for_each_tile( [&]( mtc::tile t ) {
        CHECK( t.expect_c == t.setup_c );
        CHECK( tiles.count( t.p ) == 1 );
    } );
}

// checks transformations for non-square field
TEST_CASE( "map_test_case_transform_non_square", "[map_test_case]" )
{
    using namespace map_test_case_common;
    using mtc = map_test_case;

    mtc tst;
    tst.anchor_char = '1';
    tst.setup = {
        "12345"
    };
    tst.expected_results = {
        "abcde"
    };

    SECTION( tst.generate_transform_combinations() ) {
    }
    tst.validate_anchor_point( tripoint_zero );

    CAPTURE( tst.get_width(), tst.get_height() );
    CHECK(
        ( ( tst.get_width() == 5 && tst.get_height() == 1 ) ||
          ( tst.get_width() == 1 && tst.get_height() == 5 ) )
    );

    CAPTURE( tst.get_origin() );
    CHECK( std::set<tripoint> {
        tripoint_zero, {-4, 0, 0}, {0, -4, 0}
    } .count( tst.get_origin() ) );

    tst.for_each_tile( [&]( mtc::tile t ) {
        CHECK( t.expect_c - 'a' == t.setup_c - '1' );
    } );
}
