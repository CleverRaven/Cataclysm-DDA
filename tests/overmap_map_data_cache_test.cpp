#include <bitset>
#include <memory>
#include <string>

#include "cata_catch.h"
#include "catacharset.h"
#include "coordinates.h"
#include "overmap.h"
#include "overmap_map_data_cache.h"
#include "point.h"
#include "rng.h"
#include "type_id.h"

static const oter_str_id oter_road_nesw( "road_nesw" );
static const oter_str_id oter_solid_earth( "solid_earth" );

static const string_id<map_data_summary> map_data_summary_empty_omt( "empty_omt" );
static const string_id<map_data_summary> map_data_summary_full_omt( "full_omt" );

TEST_CASE( "default_OMT_is_passable", "[hordes][overmap][map_data]" )
{
    std::unique_ptr<overmap> test_overmap = std::make_unique<overmap>( point_abs_om() );
    // Pick an arbitrary OMT.weapname_
    tripoint_om_omt arbitrary_omt{ rng( 0, 179 ), rng( 0, 179 ), 0 };
    CAPTURE( test_overmap->ter( arbitrary_omt ) );
    for( point_omt_ms omt_cursor; omt_cursor.y() < 24; ++omt_cursor.y() ) {
        for( omt_cursor.x() = 0; omt_cursor.x() < 24; ++omt_cursor.x() ) {
            CAPTURE( omt_cursor );
            CHECK( test_overmap->passable( project_combine( arbitrary_omt, omt_cursor ) ) );
        }
    }
    // Toggle to impassable.
    test_overmap->ter_set( arbitrary_omt, oter_solid_earth );
    CAPTURE( test_overmap->ter( arbitrary_omt ) );
    for( point_omt_ms omt_cursor; omt_cursor.y() < 24; ++omt_cursor.y() ) {
        for( omt_cursor.x() = 0; omt_cursor.x() < 24; ++omt_cursor.x() ) {
            CAPTURE( omt_cursor );
            CHECK( !test_overmap->passable( project_combine( arbitrary_omt, omt_cursor ) ) );
        }
    }
    // Toggle back to passable.
    test_overmap->ter_set( arbitrary_omt, oter_road_nesw );
    CAPTURE( test_overmap->ter( arbitrary_omt ) );
    for( point_omt_ms omt_cursor; omt_cursor.y() < 24; ++omt_cursor.y() ) {
        for( omt_cursor.x() = 0; omt_cursor.x() < 24; ++omt_cursor.x() ) {
            CAPTURE( omt_cursor );
            CHECK( test_overmap->passable( project_combine( arbitrary_omt, omt_cursor ) ) );
        }
    }
}

TEST_CASE( "check_placeholders_in_isolation", "[map_data]" )
{
    const map_data_summary &full_omt = *map_data_summary_full_omt;
    for( point cursor; cursor.y < 24; ++cursor.y ) {
        for( cursor.x = 0; cursor.x < 24; ++cursor.x ) {
            CAPTURE( cursor );
            CHECK( !full_omt.passable[ cursor.y * 24 + cursor.x ] );
        }
    }
    const map_data_summary &empty_omt = *map_data_summary_empty_omt;
    for( point cursor; cursor.y < 24; ++cursor.y ) {
        for( cursor.x = 0; cursor.x < 24; ++cursor.x ) {
            CAPTURE( cursor );
            CHECK( empty_omt.passable[ cursor.y * 24 + cursor.x ] );
        }
    }
}

TEST_CASE( "round_trip_bitset_to_base64", "[map_data][hordes]" )
{
    map_data_summary test_summary;
    int bitset_size = test_summary.passable.size();
    REQUIRE( bitset_size == 576 );
    REQUIRE( test_summary.passable.none() );
    // Set some bits to yes at random.
    for( int i = 0; i < bitset_size; ++i ) {
        test_summary.passable.set( i, one_in( 5 ) );
    }
    // Verify we set at least something.
    REQUIRE( test_summary.passable.any() );
    std::string packed_bitset = base64_encode_bitset( test_summary.passable );
    CAPTURE( packed_bitset );
    map_data_summary copied_summary;
    base64_decode_bitset( packed_bitset, copied_summary.passable );
    CHECK( test_summary.passable == copied_summary.passable );
}
