#include <memory>
#include <string>

#include "behavior.h"
#include "behavior_oracle.h"
#include "behavior_strategy.h"
#include "catch/catch.hpp"
#include "character_oracle.h"
#include "game.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "map_iterator.h"
#include "monster_oracle.h"
#include "mtype.h"
#include "npc.h"
#include "player.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "string_id.h"
#include "weather.h"

namespace behavior
{
extern sequential_t default_sequential;
extern fallback_t default_fallback;
extern sequential_until_done_t default_until_done;
} // namespace behavior

static behavior::node_t make_test_node( const std::string &goal, const behavior::status_t *status )
{
    behavior::node_t node;
    if( !goal.empty() ) {
        node.set_goal( goal );
    }
    node.add_predicate( [status]( const behavior::oracle_t *, const std::string & ) {
        return *status;
    } );
    return node;
}

TEST_CASE( "behavior_tree", "[behavior]" )
{
    behavior::status_t cold_state = behavior::status_t::running;
    behavior::status_t thirsty_state = behavior::status_t::running;
    behavior::status_t hungry_state = behavior::status_t::running;
    behavior::status_t clothes_state = behavior::status_t::running;
    behavior::status_t fire_state = behavior::status_t::running;
    behavior::status_t water_state = behavior::status_t::running;
    behavior::status_t clean_water_state = behavior::status_t::running;

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
    thirsty_state = behavior::status_t::success;
    // Later states don't matter.
    CHECK( maslows.tick( nullptr ) == "wear" );
    hungry_state = behavior::status_t::success;
    // This one either.
    CHECK( maslows.tick( nullptr ) == "wear" );
    cold_state = behavior::status_t::success;
    thirsty_state = behavior::status_t::running;
    // First need met, second branch followed.
    CHECK( maslows.tick( nullptr ) == "get_water" );
    cold_state = behavior::status_t::failure;
    // First need failed, second branch followed.
    CHECK( maslows.tick( nullptr ) == "get_water" );
    water_state = behavior::status_t::success;
    // Got water, proceed to next goal.
    CHECK( maslows.tick( nullptr ) == "clean_water" );
    clean_water_state = behavior::status_t::success;
    hungry_state = behavior::status_t::running;
    // Got clean water, proceed to food.
    CHECK( maslows.tick( nullptr ) == "get_food" );
    water_state = behavior::status_t::failure;
    clean_water_state = behavior::status_t::running;
    // Failed to get water, give up.
    CHECK( maslows.tick( nullptr ) == "get_food" );
    water_state = behavior::status_t::running;
    CHECK( maslows.tick( nullptr ) == "get_water" );
    cold_state = behavior::status_t::success;
    thirsty_state = behavior::status_t::success;
    hungry_state = behavior::status_t::running;
    // Second need also met, third branch taken.
    CHECK( maslows.tick( nullptr ) == "get_food" );
    cold_state = behavior::status_t::success;
    // Status_T::Failure also causes third branch to be taken.
    CHECK( maslows.tick( nullptr ) == "get_food" );
    thirsty_state = behavior::status_t::success;
    // Status_T::Failure in second branch too.
    CHECK( maslows.tick( nullptr ) == "get_food" );
    thirsty_state = behavior::status_t::running;
    cold_state = behavior::status_t::running;
    // First need appears again and becomes highest priority again.
    CHECK( maslows.tick( nullptr ) == "wear" );
    clothes_state = behavior::status_t::failure;
    // First alternative failed, attempting second.
    CHECK( maslows.tick( nullptr ) == "fire" );
    fire_state = behavior::status_t::failure;
    // Both alternatives failed, check other needs.
    CHECK( maslows.tick( nullptr ) == "get_water" );
    clothes_state = behavior::status_t::success;
    // Either status_t::failure or status_t::success meets requirements.
    CHECK( maslows.tick( nullptr ) == "get_water" );
    clothes_state = behavior::status_t::failure;
    fire_state = behavior::status_t::success;
    // Either order does it.
    CHECK( maslows.tick( nullptr ) == "get_water" );
    hungry_state = behavior::status_t::running;
    // Still thirsty, so changes to hunger are irrelevant.
    CHECK( maslows.tick( nullptr ) == "get_water" );
    thirsty_state = behavior::status_t::success;
    hungry_state = behavior::status_t::success;
    // All needs met, so no goals remain.
    CHECK( maslows.tick( nullptr ) == "idle" );
}

// Make assertions about loaded behaviors.
TEST_CASE( "check_npc_behavior_tree", "[npc][behavior]" )
{
    clear_map();
    behavior::tree npc_needs;
    npc_needs.add( &string_id<behavior::node_t>( "npc_needs" ).obj() );
    npc &test_npc = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( test_npc );
    behavior::character_oracle_t oracle( &test_npc );
    CHECK( npc_needs.tick( &oracle ) == "idle" );
    SECTION( "Freezing" ) {
        g->weather.temperature = 0;
        test_npc.update_bodytemp();
        CHECK( npc_needs.tick( &oracle ) == "idle" );
        test_npc.worn.push_back( item( "backpack" ) );
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
        item_location loc = item_location( test_npc, &food );
        CHECK( npc_needs.tick( &oracle ) == "eat_food" );
        loc.remove_item();
        CHECK( npc_needs.tick( &oracle ) == "idle" );
    }
    SECTION( "Thirsty" ) {
        test_npc.set_thirst( 700 );
        CHECK( npc_needs.tick( &oracle ) == "idle" );
        item &water = test_npc.i_add( item( itype_id( "water" ) ) );
        item_location loc = item_location( test_npc, &water );
        CHECK( npc_needs.tick( &oracle ) == "drink_water" );
        loc.remove_item();
        CHECK( npc_needs.tick( &oracle ) == "idle" );
    }
}

TEST_CASE( "check_monster_behavior_tree", "[monster][behavior]" )
{
    const tripoint monster_location( 5, 5, 0 );
    clear_map();
    monster &test_monster = spawn_test_monster( "mon_locust", monster_location );

    behavior::monster_oracle_t oracle( &test_monster );
    behavior::tree monster_goals;
    monster_goals.add( test_monster.type->get_goals() );

    for( const std::string &special_name : test_monster.type->special_attacks_names ) {
        test_monster.reset_special( special_name );
    }
    CHECK( monster_goals.tick( &oracle ) == "idle" );
    for( const tripoint &near_monster : g->m.points_in_radius( monster_location, 1 ) ) {
        g->m.ter_set( near_monster, ter_id( "t_grass" ) );
        g->m.furn_set( near_monster, furn_id( "f_null" ) );
    }
    SECTION( "Special Attack" ) {
        test_monster.set_special( "EAT_CROP", 0 );
        CHECK( monster_goals.tick( &oracle ) == "idle" );
        g->m.furn_set( monster_location, furn_id( "f_plant_seedling" ) );
        CHECK( monster_goals.tick( &oracle ) == "EAT_CROP" );
        test_monster.set_special( "EAT_CROP", 1 );
        CHECK( monster_goals.tick( &oracle ) == "idle" );
    }
}
