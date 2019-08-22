#include <memory>
#include <string>

#include "catch/catch.hpp"
#include "behavior.h"
#include "behavior_oracle.h"
#include "behavior_strategy.h"
#include "game.h"
#include "npc.h"
#include "item.h"
#include "material.h"
#include "string_id.h"
#include "weather.h"

namespace behavior
{
extern sequential_t default_sequential;
extern fallback_t default_fallback;
extern sequential_until_done_t default_until_done;
} // namespace behavior

static behavior::node_t make_test_node( std::string goal, const behavior::status_t *status )
{
    behavior::node_t node;
    if( !goal.empty() ) {
        node.set_goal( goal );
    }
    node.set_predicate( [status]( const behavior::oracle_t * ) {
        return *status;
    } );
    return node;
}

TEST_CASE( "behavior_tree", "[behavior]" )
{
    behavior::status_t cold_state = behavior::running;
    behavior::status_t thirsty_state = behavior::running;
    behavior::status_t hungry_state = behavior::running;
    behavior::status_t clothes_state = behavior::running;
    behavior::status_t fire_state = behavior::running;
    behavior::status_t water_state = behavior::running;
    behavior::status_t clean_water_state = behavior::running;

    behavior::node_t clothes = make_test_node( "wear", &clothes_state );
    behavior::node_t fire = make_test_node( "fire", &fire_state );
    behavior::node_t cold = make_test_node( "", &cold_state );
    cold.add_child( &clothes );
    cold.add_child( &fire );
    cold.set_strategy( &behavior::default_fallback );
    behavior::node_t water = make_test_node( "get_water", &water_state );
    behavior::node_t clean_water = make_test_node( "clean_water", &clean_water_state );
    behavior::node_t thirst = make_test_node( "", &thirsty_state );
    thirst.add_child( &water );
    thirst.add_child( &clean_water );
    thirst.set_strategy( &behavior::default_sequential );
    behavior::node_t hunger = make_test_node( "get_food", &hungry_state );

    behavior::node_t basic_needs;
    basic_needs.set_strategy( &behavior::default_until_done );

    basic_needs.add_child( &cold );
    basic_needs.add_child( &thirst );
    basic_needs.add_child( &hunger );

    behavior::tree maslows;
    maslows.add( &basic_needs );

    // First and therefore highest priority goal.
    CHECK( maslows.tick( nullptr ) == "wear" );
    thirsty_state = behavior::success;
    // Later states don't matter.
    CHECK( maslows.tick( nullptr ) == "wear" );
    hungry_state = behavior::success;
    // This one either.
    CHECK( maslows.tick( nullptr ) == "wear" );
    cold_state = behavior::success;
    thirsty_state = behavior::running;
    // First need met, second branch followed.
    CHECK( maslows.tick( nullptr ) == "get_water" );
    cold_state = behavior::failure;
    // First need failed, second branch followed.
    CHECK( maslows.tick( nullptr ) == "get_water" );
    water_state = behavior::success;
    // Got water, proceed to next goal.
    CHECK( maslows.tick( nullptr ) == "clean_water" );
    clean_water_state = behavior::success;
    hungry_state = behavior::running;
    // Got clean water, proceed to food.
    CHECK( maslows.tick( nullptr ) == "get_food" );
    water_state = behavior::failure;
    clean_water_state = behavior::running;
    // Failed to get water, give up.
    CHECK( maslows.tick( nullptr ) == "get_food" );
    water_state = behavior::running;
    CHECK( maslows.tick( nullptr ) == "get_water" );
    cold_state = behavior::success;
    thirsty_state = behavior::success;
    hungry_state = behavior::running;
    // Second need also met, third branch taken.
    CHECK( maslows.tick( nullptr ) == "get_food" );
    cold_state = behavior::success;
    // Failure also causes third branch to be taken.
    CHECK( maslows.tick( nullptr ) == "get_food" );
    thirsty_state = behavior::success;
    // Failure in second branch too.
    CHECK( maslows.tick( nullptr ) == "get_food" );
    thirsty_state = behavior::running;
    cold_state = behavior::running;
    // First need appears again and becomes highest priority again.
    CHECK( maslows.tick( nullptr ) == "wear" );
    clothes_state = behavior::failure;
    // First alternative failed, attempting second.
    CHECK( maslows.tick( nullptr ) == "fire" );
    fire_state = behavior::failure;
    // Both alternatives failed, check other needs.
    CHECK( maslows.tick( nullptr ) == "get_water" );
    clothes_state = behavior::success;
    // Either failure or success meets requirements.
    CHECK( maslows.tick( nullptr ) == "get_water" );
    clothes_state = behavior::failure;
    fire_state = behavior::success;
    // Either order does it.
    CHECK( maslows.tick( nullptr ) == "get_water" );
    hungry_state = behavior::running;
    // Still thirsty, so changes to hunger are irrelevant.
    CHECK( maslows.tick( nullptr ) == "get_water" );
    thirsty_state = behavior::success;
    hungry_state = behavior::success;
    // All needs met, so no goals remain.
    CHECK( maslows.tick( nullptr ) == "idle" );
}

// Make assertions about loaded behaviors.
TEST_CASE( "check_npc_behavior_tree", "[behavior]" )
{
    behavior::tree npc_needs;
    npc_needs.add( &string_id<behavior::node_t>( "npc_needs" ).obj() );
    npc test_npc;
    test_npc.normalize();
    test_npc.setpos( { 50, 50, 0 } );
    behavior::character_oracle_t oracle( &test_npc );
    CHECK( npc_needs.tick( &oracle ) == "idle" );
    SECTION( "Freezing" ) {
        g->weather.temperature = 0;
        test_npc.update_bodytemp();
        CHECK( npc_needs.tick( &oracle ) == "idle" );
        item &sweater = test_npc.i_add( item( itype_id( "sweater" ) ) );
        CHECK( npc_needs.tick( &oracle ) == "wear_warmer_clothes" );
        item sweater_copy = test_npc.i_rem( &sweater );
        test_npc.wear_item( sweater_copy );
        CHECK( npc_needs.tick( &oracle ) == "idle" );
        test_npc.i_add( item( itype_id( "lighter" ) ) );
        test_npc.i_add( item( itype_id( "2x4" ) ) );
        CHECK( npc_needs.tick( &oracle ) == "start_fire" );
    }
    SECTION( "Hungry" ) {
        test_npc.set_hunger( 500 );
        test_npc.set_stored_kcal( 1000 );
        CHECK( npc_needs.tick( &oracle ) == "idle" );
        item &food = test_npc.i_add( item( itype_id( "sandwich_cheese_grilled" ) ) );
        CHECK( npc_needs.tick( &oracle ) == "eat_food" );
        test_npc.consume( test_npc.get_item_position( &food ) );
        CHECK( npc_needs.tick( &oracle ) == "idle" );
    }
    SECTION( "Thirsty" ) {
        test_npc.set_thirst( 700 );
        CHECK( npc_needs.tick( &oracle ) == "idle" );
        item &water = test_npc.i_add( item( itype_id( "water" ) ) );
        CHECK( npc_needs.tick( &oracle ) == "drink_water" );
        test_npc.consume( test_npc.get_item_position( &water ) );
        CHECK( npc_needs.tick( &oracle ) == "idle" );
    }
}
