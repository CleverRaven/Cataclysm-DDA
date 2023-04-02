#include <array>
#include <iterator>

#include "map_test_case.h"

// test some of the map_test_case helpers
TEST_CASE( "map_test_case_common", "[map_test_case]" )
{
    using namespace map_test_case_common;
    using tile = map_test_case::tile;

    auto test_tile = []( int c ) {
        tile t;
        t.setup_c = c;
        return t;
    };

    SECTION( "|| and &&" ) {
        tile_predicate f_positive = []( tile t ) {
            return t.setup_c > 0;
        };
        tile_predicate f_negative = []( tile t ) {
            return t.setup_c < 0;
        };
        tile_predicate f_less_than_five = []( tile t ) {
            return t.setup_c < 5;
        };

        CHECK( ( f_positive && f_less_than_five )( test_tile( 3 ) ) );
        CHECK_FALSE( ( f_positive && f_less_than_five )( test_tile( -1 ) ) );
        CHECK_FALSE( ( f_positive && f_less_than_five )( test_tile( 6 ) ) );

        CHECK( ( f_positive || f_less_than_five )( test_tile( -1 ) ) );
        CHECK( ( f_positive || f_less_than_five )( test_tile( 6 ) ) );
        CHECK_FALSE( ( f_positive || f_negative )( test_tile( 0 ) ) );
    }
    SECTION( "a && a && a" ) {
        tile_predicate f_true = []( auto ) {
            return true;
        };
        tile_predicate f_false = []( auto ) {
            return false;
        };

        CHECK( ( f_true && f_true && f_true )( {} ) );
        CHECK_FALSE( ( f_true && f_true && f_false )( {} ) );
        CHECK( ( f_true || f_false || f_false )( {} ) );
        CHECK( ( f_false || f_false || f_true )( {} ) );
    }
    SECTION( "inc + inc + inc" ) {
        int counter = 0;
        tile_predicate inc = [&]( auto ) {
            counter++;
            return true;
        };
        auto f = inc + inc + inc;
        CHECK( counter == 0 );
        f( {} );
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
