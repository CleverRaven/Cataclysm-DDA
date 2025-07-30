#include "cata_catch.h"
#include "coordinates.h"
#include "overmap.h"
#include "overmap_map_data_cache.h"
#include "rng.h"

static const oter_str_id oter_road_nesw( "road_nesw" );
static const oter_str_id oter_solid_earth( "solid_earth" );

static const string_id<map_data_summary> map_data_empty( "empty_omt" );
static const string_id<map_data_summary> map_data_full( "full_omt" );

TEST_CASE( "default_OMT_is_passable", "[hordes][overmap][map_data]" )
{
    std::unique_ptr<overmap> test_overmap = std::make_unique<overmap>( point_abs_om() );
    // Pick an arbitrary OMT.
    tripoint_om_omt arbitrary_omt{ rng( 0, 179 ), rng( 0, 179 ), 0 };
    CAPTURE( test_overmap->ter( arbitrary_omt ) );
    for( point_omt_ms omt_cursor = { 0, 0 }; omt_cursor.y() < 24; ++omt_cursor.y() ) {
        for( omt_cursor.x() = 0; omt_cursor.x() < 24; ++omt_cursor.x() ) {
            CAPTURE( omt_cursor );
            CHECK( test_overmap->passable( project_combine( arbitrary_omt, omt_cursor ) ) );
        }
    }
    // Toggle to impassable.
    test_overmap->ter_set( arbitrary_omt, oter_solid_earth );
    CAPTURE( test_overmap->ter( arbitrary_omt ) );
    for( point_omt_ms omt_cursor = { 0, 0 }; omt_cursor.y() < 24; ++omt_cursor.y() ) {
        for( omt_cursor.x() = 0; omt_cursor.x() < 24; ++omt_cursor.x() ) {
            CAPTURE( omt_cursor );
            CHECK( !test_overmap->passable( project_combine( arbitrary_omt, omt_cursor ) ) );
        }
    }
    // Toggle back to passable.
    test_overmap->ter_set( arbitrary_omt, oter_road_nesw );
    CAPTURE( test_overmap->ter( arbitrary_omt ) );
    for( point_omt_ms omt_cursor = { 0, 0 }; omt_cursor.y() < 24; ++omt_cursor.y() ) {
        for( omt_cursor.x() = 0; omt_cursor.x() < 24; ++omt_cursor.x() ) {
            CAPTURE( omt_cursor );
            CHECK( test_overmap->passable( project_combine( arbitrary_omt, omt_cursor ) ) );
        }
    }
}

TEST_CASE( "check_placeholders_in_isolation", "[map_data]" )
{
    const map_data_summary &full_omt = *map_data_full;
    for( point cursor = { 0, 0 }; cursor.y < 24; ++cursor.y ) {
        for( cursor.x = 0; cursor.x < 24; ++cursor.x ) {
            CAPTURE( cursor );
            CHECK( !full_omt.passable[ cursor.y * 24 + cursor.x ] );
        }
    }
    const map_data_summary &empty_omt = *map_data_empty;
    for( point cursor = { 0, 0 }; cursor.y < 24; ++cursor.y ) {
        for( cursor.x = 0; cursor.x < 24; ++cursor.x ) {
            CAPTURE( cursor );
            CHECK( empty_omt.passable[ cursor.y * 24 + cursor.x ] );
        }
    }
}
