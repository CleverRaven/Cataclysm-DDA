#include "avatar.h"
#include "cata_catch.h"
#include "creature_tracker.h"
#include "map.h"
#include "map_helpers.h"
#include "mapgen_helpers.h"
#include "npc.h"
#include "overmapbuffer.h"
#include "player_helpers.h"

static const npc_class_id test_npc_trader( "test_npc_trader" );
static const npc_class_id thug( "thug" );
static const update_mapgen_id update_mapgen_test_update_place_npc( "test_update_place_npc" );
static const update_mapgen_id
update_mapgen_test_update_place_npc_thug( "test_update_place_npc_thug" );
static const update_mapgen_id update_mapgen_test_update_remove_npc( "test_update_remove_npc" );

namespace
{

void check_creature( tripoint const &loc, npc_class_id const &id, bool state )
{
    Creature *critter = get_creature_tracker().creature_at( loc, true );
    if( state ) {
        REQUIRE( critter != nullptr );
        REQUIRE( critter->as_npc()->idz == id );
    } else {
        REQUIRE( critter == nullptr );
    }
}

void place_npc_and_check( map &m, tripoint const &loc, update_mapgen_id const &id,
                          npc_class_id const &nid )
{
    check_creature( loc, nid, false );
    tripoint_abs_omt const omt = project_to<coords::omt>( m.getglobal( loc ) );
    manual_mapgen( omt, manual_update_mapgen, id );
    check_creature( loc, nid, true );
}

void remove_npc_and_check( map &m, tripoint const &loc, update_mapgen_id const &id,
                           npc_class_id const &nid )
{
    check_creature( loc, nid, true );
    tripoint_abs_omt const omt = project_to<coords::omt>( m.getglobal( loc ) );
    manual_mapgen( omt, manual_update_mapgen, id );
    check_creature( loc, nid, false );
}

} // namespace

TEST_CASE( "mapgen_remove_npcs" )
{
    GIVEN( "in bounds of main map" ) {
        map &here = get_map();
        clear_map();
        clear_avatar();
        tripoint const start_loc( HALF_MAPSIZE_X + SEEX - 2, HALF_MAPSIZE_Y + SEEY - 1, 0 );
        get_avatar().setpos( start_loc );
        clear_npcs();

        tripoint_abs_omt const omt = project_to<coords::omt>( get_avatar().get_location() );
        tripoint_abs_omt const omt2 = omt + tripoint_east;
        tripoint const loc = here.getlocal( project_to<coords::ms>( omt ) );
        tripoint const loc2 = here.getlocal( project_to<coords::ms>( omt2 ) );
        tripoint const loc3 = loc2 + tripoint_east;
        place_npc_and_check( here, loc, update_mapgen_test_update_place_npc, test_npc_trader );
        REQUIRE( overmap_buffer.get_npcs_near_omt( omt, 0 ).size() == 1 );
        place_npc_and_check( here, loc2, update_mapgen_test_update_place_npc, test_npc_trader );
        REQUIRE( overmap_buffer.get_npcs_near_omt( omt2, 0 ).size() == 1 );

        WHEN( "removing NPC" ) {
            remove_npc_and_check( here, loc, update_mapgen_test_update_remove_npc, test_npc_trader );
            THEN( "NPC of same class on different submap not affected" ) {
                REQUIRE( overmap_buffer.get_npcs_near_omt( omt, 0 ).empty() );
                REQUIRE( overmap_buffer.get_npcs_near_omt( omt2, 0 ).size() == 1 );
            }

            THEN( "NPC of different class on same submap not affected" ) {
                place_npc_and_check( here, loc3, update_mapgen_test_update_place_npc_thug, thug );
                REQUIRE( overmap_buffer.get_npcs_near_omt( omt2, 0 ).size() == 2 );
                remove_npc_and_check( here, loc2, update_mapgen_test_update_remove_npc, test_npc_trader );
                REQUIRE( overmap_buffer.get_npcs_near_omt( omt2, 0 ).size() == 1 );
                check_creature( loc3, thug, true );
            }
        }
    }
}
