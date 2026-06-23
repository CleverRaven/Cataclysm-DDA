#include "avatar.h"
#include "cached_options.h"
#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "coordinates.h"
#include "handle_liquid.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "map_selector.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"

// *INDENT-OFF*
static const itype_id itype_test_liquid_1ml( "test_liquid_1ml" );
static const itype_id itype_test_watertight_open_sealed_container_250ml( "test_watertight_open_sealed_container_250ml" );
// *INDENT-ON*

// handle_liquid should return false when a pre-set LD_ITEM target is full
// and pour_into fails. Before the fix, handle_item_target unconditionally
// returned true, causing infinite loops in callers like handle_liquid_or_spill.
TEST_CASE( "handle_liquid_returns_false_when_target_container_is_full",
           "[liquid][handler]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();

    const tripoint_bub_ms player_pos = dummy.pos_bub();
    const tripoint_bub_ms container_pos = player_pos + tripoint::east;

    // Create a container and fill it to capacity
    item container( itype_test_watertight_open_sealed_container_250ml );
    item filler( itype_test_liquid_1ml );
    filler.charges = 250;
    REQUIRE( container.fill_with( filler, 250 ) > 0 );
    // Verify it's actually full
    REQUIRE( container.get_remaining_capacity_for_liquid( item( itype_test_liquid_1ml ), true ) == 0 );

    // Place the full container on the map and get an item_location for it
    here.add_item_or_charges( container_pos, container );
    map_stack items_at = here.i_at( container_pos );
    REQUIRE( !items_at.empty() );
    item_location cont_loc( map_cursor( container_pos ), &items_at.only_item() );
    REQUIRE( cont_loc.get_item() != nullptr );

    // Create liquid to pour
    item liquid( itype_test_liquid_1ml );
    liquid.charges = 10;

    // Pre-set the liquid target to the full container
    liquid_dest_opt target;
    target.dest_opt = LD_ITEM;
    target.item_loc = cont_loc;

    const int moves_before = dummy.get_moves();
    const int charges_before = liquid.charges;

    bool result = liquid_handler::handle_liquid( liquid, target, nullptr, 0 );

    CHECK_FALSE( result );
    CHECK( liquid.charges == charges_before );
    CHECK( dummy.get_moves() == moves_before );
}

// Regression guard: on_pickup with a spillable container should handle all
// liquid without hanging. Exercises the real item_pocket::on_pickup ->
// handle_liquid_or_spill production path.
TEST_CASE( "on_pickup_spillable_container_handles_all_liquid",
           "[liquid][spill]" )
{
    avatar &dummy = get_avatar();
    map &here = get_map();

    clear_avatar();
    clear_map_without_vision();

    restore_on_out_of_scope<test_mode_spilling_action_t> restore_spill( test_mode_spilling_action );
    test_mode_spilling_action = test_mode_spilling_action_t::spill_all;

    // Create a spillable container with liquid
    item container( itype_test_watertight_open_sealed_container_250ml );
    item liquid( itype_test_liquid_1ml );
    liquid.charges = 5;
    REQUIRE( container.put_in( liquid, pocket_type::CONTAINER ).success() );
    REQUIRE( container.is_bucket_nonempty() );

    // Call the real on_pickup path
    container.on_pickup( dummy );

    // All liquid should have been handled (spilled to ground in test mode)
    CHECK( container.is_container_empty() );

    // Liquid should be on the ground at player position
    bool found_liquid = false;
    for( const item &ground_item : here.i_at( dummy.pos_bub() ) ) {
        if( ground_item.typeId() == itype_test_liquid_1ml ) {
            found_liquid = true;
            break;
        }
    }
    CHECK( found_liquid );
}
