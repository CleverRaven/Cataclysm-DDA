#include <string>
#include <unordered_set>

#include "cata_catch.h"
#include "clzones.h"
#include "coordinates.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_scale_constants.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"

static const itype_id itype_bag_plastic( "bag_plastic" );
static const itype_id itype_bow_saw( "bow_saw" );
static const itype_id itype_hammer( "hammer" );
static const itype_id itype_test_battery_disposable( "test_battery_disposable" );
static const itype_id itype_test_glaive( "test_glaive" );
static const itype_id itype_test_pants_fur( "test_pants_fur" );

static const zone_type_id zone_type_LOOT_CUSTOM( "LOOT_CUSTOM" );
static const zone_type_id zone_type_LOOT_ITEM_GROUP( "LOOT_ITEM_GROUP" );

using pset = std::unordered_set<tripoint_abs_ms>;

TEST_CASE( "zones_custom", "[zones]" )
{
    WHEN( "overlapping custom zones" ) {
        clear_map();
        map &m = get_map();
        tripoint_abs_ms const zone_loc = m.get_abs( tripoint_bub_ms{ 5, 5, 0 } );
        tripoint_abs_ms const zone_hammer_end = zone_loc + tripoint::north;
        tripoint_abs_ms const zone_bowsaw_end = zone_loc + tripoint::south;
        tripoint_abs_ms const zone_testgroup_end = zone_loc + tripoint::east;
        tripoint_abs_ms const zone_groupbatt_end = zone_loc + tripoint::west;
        tripoint_abs_ms const where = m.get_abs( tripoint_bub_ms::zero );
        item hammer( itype_hammer );
        item bow_saw( itype_bow_saw );
        item pants_fur( itype_test_pants_fur );
        item batt( itype_test_battery_disposable );
        item bag_plastic( itype_bag_plastic );
        item nested_batt( itype_test_battery_disposable );
        int const num = GENERATE( 1, 2 );
        for( int i = 0; i < num; i++ ) {
            bag_plastic.put_in( nested_batt, pocket_type::CONTAINER );
        }
        CAPTURE( num, bag_plastic.display_name() );

        mapgen_place_zone( zone_loc + tripoint::north_west, zone_loc + tripoint::south_east,
                           zone_type_LOOT_CUSTOM, your_fac, {}, "completely unrelated overlap" );
        mapgen_place_zone( zone_loc, zone_hammer_end, zone_type_LOOT_CUSTOM, your_fac, {},
                           "hammer" );
        mapgen_place_zone( zone_loc, zone_bowsaw_end, zone_type_LOOT_CUSTOM, your_fac, {},
                           "c:tools,-hammer" );
        mapgen_place_zone( zone_loc, zone_testgroup_end, zone_type_LOOT_ITEM_GROUP, your_fac, {},
                           "test_event_item_spawn" );
        mapgen_place_zone( zone_loc, zone_groupbatt_end, zone_type_LOOT_ITEM_GROUP, your_fac, {},
                           "test_group_disp" );
        tripoint_abs_ms const m_zone_loc = m.get_abs( tripoint_bub_ms{-5, -5, 0 } );
        mapgen_place_zone( m_zone_loc, m_zone_loc, zone_type_LOOT_CUSTOM, your_fac, {},
                           "plastic bag" );

        zone_manager &zmgr = zone_manager::get_manager();
        REQUIRE( zmgr.get_near_zone_type_for_item( hammer, where ) == zone_type_LOOT_CUSTOM );
        REQUIRE( zmgr.get_near_zone_type_for_item( bow_saw, where ) == zone_type_LOOT_CUSTOM );
        REQUIRE( !zmgr.get_near_zone_type_for_item( item( itype_test_glaive ), where ).is_valid() );
        REQUIRE( zmgr.get_near_zone_type_for_item( pants_fur, where ) ==
                 zone_type_LOOT_ITEM_GROUP );
        REQUIRE( zmgr.get_near_zone_type_for_item( batt, where ) == zone_type_LOOT_ITEM_GROUP );
        // this should match both types but custom zone comes first
        REQUIRE( zmgr.get_near_zone_type_for_item( bag_plastic, where ) == zone_type_LOOT_CUSTOM );

        pset const hammerpoints =
            zmgr.get_near( zone_type_LOOT_CUSTOM, where, MAX_VIEW_DISTANCE, &hammer );
        REQUIRE( hammerpoints.count( tripoint_abs_ms( zone_loc ) ) == 1 );
        REQUIRE( hammerpoints.count( tripoint_abs_ms( zone_hammer_end ) ) == 1 );
        REQUIRE( hammerpoints.count( tripoint_abs_ms( zone_bowsaw_end ) ) == 0 );
        REQUIRE( hammerpoints.count( tripoint_abs_ms( zone_testgroup_end ) ) == 0 );
        REQUIRE( hammerpoints.count( tripoint_abs_ms( zone_groupbatt_end ) ) == 0 );
        pset const bowsawpoints =
            zmgr.get_near( zone_type_LOOT_CUSTOM, where, MAX_VIEW_DISTANCE, &bow_saw );
        REQUIRE( bowsawpoints.count( tripoint_abs_ms( zone_loc ) ) == 1 );
        REQUIRE( bowsawpoints.count( tripoint_abs_ms( zone_hammer_end ) ) == 0 );
        REQUIRE( bowsawpoints.count( tripoint_abs_ms( zone_bowsaw_end ) ) == 1 );
        REQUIRE( bowsawpoints.count( tripoint_abs_ms( zone_testgroup_end ) ) == 0 );
        REQUIRE( bowsawpoints.count( tripoint_abs_ms( zone_groupbatt_end ) ) == 0 );
        pset const testgrouppoints =
            zmgr.get_near( zone_type_LOOT_ITEM_GROUP, where, MAX_VIEW_DISTANCE, &pants_fur );
        REQUIRE( testgrouppoints.count( tripoint_abs_ms( zone_loc ) ) == 1 );
        REQUIRE( testgrouppoints.count( tripoint_abs_ms( zone_hammer_end ) ) == 0 );
        REQUIRE( testgrouppoints.count( tripoint_abs_ms( zone_bowsaw_end ) ) == 0 );
        REQUIRE( testgrouppoints.count( tripoint_abs_ms( zone_testgroup_end ) ) == 1 );
        REQUIRE( testgrouppoints.count( tripoint_abs_ms( zone_groupbatt_end ) ) == 0 );
        pset const groupbattpoints =
            zmgr.get_near( zone_type_LOOT_ITEM_GROUP, where, MAX_VIEW_DISTANCE, &batt );
        REQUIRE( groupbattpoints.count( tripoint_abs_ms( zone_loc ) ) == 1 );
        REQUIRE( groupbattpoints.count( tripoint_abs_ms( zone_hammer_end ) ) == 0 );
        REQUIRE( groupbattpoints.count( tripoint_abs_ms( zone_bowsaw_end ) ) == 0 );
        REQUIRE( groupbattpoints.count( tripoint_abs_ms( zone_testgroup_end ) ) == 0 );
        REQUIRE( groupbattpoints.count( tripoint_abs_ms( zone_groupbatt_end ) ) == 1 );
        pset const nestedbattpoints =
            zmgr.get_near( zone_type_LOOT_ITEM_GROUP, where, MAX_VIEW_DISTANCE, &bag_plastic );
        REQUIRE( nestedbattpoints.count( tripoint_abs_ms( zone_loc ) ) == 1 );
        REQUIRE( nestedbattpoints.count( tripoint_abs_ms( zone_hammer_end ) ) == 0 );
        REQUIRE( nestedbattpoints.count( tripoint_abs_ms( zone_bowsaw_end ) ) == 0 );
        REQUIRE( nestedbattpoints.count( tripoint_abs_ms( zone_testgroup_end ) ) == 0 );
        REQUIRE( nestedbattpoints.count( tripoint_abs_ms( zone_groupbatt_end ) ) == 1 );
        pset const nbp2 =
            zmgr.get_near( zone_type_LOOT_CUSTOM, where, MAX_VIEW_DISTANCE, &bag_plastic );
        REQUIRE( nbp2.count( tripoint_abs_ms( zone_loc ) ) == 0 );
        REQUIRE( nbp2.count( tripoint_abs_ms( zone_hammer_end ) ) == 0 );
        REQUIRE( nbp2.count( tripoint_abs_ms( zone_bowsaw_end ) ) == 0 );
        REQUIRE( nbp2.count( tripoint_abs_ms( zone_testgroup_end ) ) == 0 );
        REQUIRE( nbp2.count( tripoint_abs_ms( zone_groupbatt_end ) ) == 0 );
        REQUIRE( nbp2.count( tripoint_abs_ms( m_zone_loc ) ) == 1 ); // container matches this zone
    }
}
