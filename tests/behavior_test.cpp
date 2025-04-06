#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "behavior.h"
#include "behavior_strategy.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character_attire.h"
#include "character_oracle.h"
#include "coordinates.h"
#include "item.h"
#include "item_group.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "map_iterator.h"
#include "mapgen.h"
#include "mapgendata.h"
#include "monattack.h"
#include "monster.h"
#include "monster_oracle.h"
#include "mtype.h"
#include "npc.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "weather.h"
#include "weighted_list.h"

static const itype_id itype_2x4( "2x4" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_corpse( "corpse" );
static const itype_id itype_frame( "frame" );
static const itype_id itype_lighter( "lighter" );
static const itype_id itype_pencil( "pencil" );
static const itype_id itype_sandwich_cheese_grilled( "sandwich_cheese_grilled" );
static const itype_id itype_sweater( "sweater" );
static const itype_id itype_water_clean( "water_clean" );

static const nested_mapgen_id nested_mapgen_test_seedling( "test_seedling" );

static const string_id<behavior::node_t> behavior_node_t_npc_needs( "npc_needs" );

namespace behavior
{
class oracle_t;

static sequential_t default_sequential;
static fallback_t default_fallback;
static sequential_until_done_t default_until_done;
} // namespace behavior

static behavior::node_t make_test_node( const std::string &goal, const behavior::status_t *status )
{
    behavior::node_t node;
    if( !goal.empty() ) {
        node.set_goal( goal );
    }
    node.add_predicate( [status]( const behavior::oracle_t *, const std::string_view ) {
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
    calendar::turn = calendar::start_of_cataclysm;
    behavior::tree npc_needs;
    npc_needs.add( &behavior_node_t_npc_needs.obj() );
    npc &test_npc = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( test_npc );
    behavior::character_oracle_t oracle( &test_npc );
    CHECK( npc_needs.tick( &oracle ) == "idle" );
    SECTION( "Freezing" ) {
        weather_manager &weather = get_weather();
        weather.temperature = units::from_fahrenheit( 0 );
        weather.clear_temp_cache();
        REQUIRE( units::to_fahrenheit( weather.get_temperature( test_npc.pos_bub() ) ) == Approx( 0 ) );
        test_npc.update_bodytemp();
        REQUIRE( oracle.needs_warmth_badly( "" ) == behavior::status_t::running );
        CHECK( npc_needs.tick( &oracle ) == "idle" );
        test_npc.worn.wear_item( test_npc, item( itype_backpack ), false, false );
        item_location sweater = test_npc.i_add( item( itype_sweater ) );
        CHECK( oracle.can_wear_warmer_clothes( "" ) == behavior::status_t::running );
        CHECK( npc_needs.tick( &oracle ) == "wear_warmer_clothes" );
        item sweater_copy = *sweater;
        test_npc.wear_item( sweater_copy );
        sweater.remove_item();
        CHECK( npc_needs.tick( &oracle ) == "idle" );
        test_npc.i_add( item( itype_lighter ) );
        test_npc.i_add( item( itype_2x4 ) );
        REQUIRE( oracle.can_make_fire( "" ) == behavior::status_t::running );
        CHECK( npc_needs.tick( &oracle ) == "start_fire" );
    }
    SECTION( "Hungry" ) {
        test_npc.set_hunger( 500 );
        test_npc.set_stored_kcal( 1000 );
        REQUIRE( oracle.needs_food_badly( "" ) == behavior::status_t::running );
        CHECK( npc_needs.tick( &oracle ) == "idle" );
        item_location food = test_npc.i_add( item( itype_sandwich_cheese_grilled ) );
        REQUIRE( oracle.has_food( "" ) == behavior::status_t::running );
        CHECK( npc_needs.tick( &oracle ) == "eat_food" );
        food.remove_item();
        CHECK( npc_needs.tick( &oracle ) == "idle" );
    }
    SECTION( "Thirsty" ) {
        item_group_id grp_bottle_water( "test_bottle_water" );
        const item_group::ItemList items = item_group::items_from( grp_bottle_water );

        // make sure only water bottles are in this group
        REQUIRE( items.size() == 1 );
        REQUIRE( item_group::group_contains_item( grp_bottle_water, itype_water_clean ) );

        test_npc.set_thirst( 700 );
        REQUIRE( oracle.needs_water_badly( "" ) == behavior::status_t::running );
        CHECK( npc_needs.tick( &oracle ) == "idle" );

        item_location water = test_npc.i_add( items.front() );

        REQUIRE( oracle.has_water( "" ) == behavior::status_t::running );
        CHECK( npc_needs.tick( &oracle ) == "drink_water" );
        water.remove_item();
        CHECK( npc_needs.tick( &oracle ) == "idle" );
    }
}

TEST_CASE( "check_monster_behavior_tree_locust", "[monster][behavior]" )
{
    const tripoint_bub_ms monster_location( 5, 5, 0 );
    clear_map();
    map &here = get_map();
    monster &test_monster = spawn_test_monster( "mon_locust", monster_location );

    behavior::monster_oracle_t oracle( &test_monster );
    behavior::tree monster_goals;
    monster_goals.add( test_monster.type->get_goals() );

    for( const std::string &special_name : test_monster.type->special_attacks_names ) {
        test_monster.reset_special( special_name );
    }
    CHECK( monster_goals.tick( &oracle ) == "idle" );
    for( const tripoint_bub_ms &near_monster : here.points_in_radius( monster_location, 1 ) ) {
        here.ter_set( near_monster, ter_id( "t_grass" ) );
        here.furn_set( near_monster, furn_id( "f_null" ) );
    }
    SECTION( "Special Attack EAT_CROP" ) {
        test_monster.set_special( "EAT_CROP", 0 );
        CHECK( monster_goals.tick( &oracle ) == "idle" );

        // Gross but I couldn't figure out how to place a sealed item without manual mapgen and the mapgen helper version doesn't let you specify rel_ms and adding that as a defaulted arg breaks the templated manual_mapgen()...
        const tripoint_abs_ms abs_pos = here.get_abs( monster_location );
        tripoint_abs_omt pos;
        point_omt_ms pos_rel;
        std::tie( pos, pos_rel ) = project_remain<coords::omt>( abs_pos );
        tinymap tm;
        tm.load( pos, true );
        mapgendata md( pos, *tm.cast_to_map(), 0.0f, calendar::turn, nullptr );
        const auto &ptr = nested_mapgens[nested_mapgen_test_seedling].funcs().pick();
        ( *ptr )->nest( md, tripoint_rel_ms( rebase_rel( pos_rel ), 0 ), "test" );

        CHECK( monster_goals.tick( &oracle ) == "EAT_CROP" );
        test_monster.set_special( "EAT_CROP", 1 );
        CHECK( monster_goals.tick( &oracle ) == "idle" );
    }
}

TEST_CASE( "check_monster_behavior_tree_shoggoth", "[monster][behavior]" )
{
    const tripoint_bub_ms monster_location( 5, 5, 0 );
    clear_map();
    map &here = get_map();
    monster &test_monster = spawn_test_monster( "mon_shoggoth", monster_location );

    behavior::monster_oracle_t oracle( &test_monster );
    behavior::tree monster_goals;
    monster_goals.add( test_monster.type->get_goals() );

    for( const std::string &special_name : test_monster.type->special_attacks_names ) {
        test_monster.reset_special( special_name );
    }
    CHECK( monster_goals.tick( &oracle ) == "idle" );
    for( const tripoint_bub_ms &near_monster : here.points_in_radius( monster_location, 1 ) ) {
        here.ter_set( near_monster, ter_id( "t_grass" ) );
        here.furn_set( near_monster, furn_id( "f_null" ) );
    }
    SECTION( "Special Attack ABSORB_ITEMS" ) {
        test_monster.set_special( "SPLIT", 0 );
        test_monster.set_special( "ABSORB_ITEMS", 0 );
        CHECK( monster_goals.tick( &oracle ) == "idle" );
        here.add_item( test_monster.pos_bub(), item( itype_frame ) );
        CHECK( monster_goals.tick( &oracle ) == "ABSORB_ITEMS" );

        mattack::absorb_items( &test_monster );
        // test that the frame is removed and no items remain
        CHECK( here.i_at( test_monster.pos_bub() ).empty() );

        test_monster.set_special( "ABSORB_ITEMS", 0 );

        // test that the monster is idle with no items around and not enough HP to split
        CHECK( monster_goals.tick( &oracle ) == "idle" );
    }
    SECTION( "Special Attack SPLIT" ) {
        test_monster.set_special( "SPLIT", 0 );
        int new_hp = test_monster.type->hp * 2 + 2;
        test_monster.set_hp( new_hp );

        // also set proper conditions for ABSORB_ITEMS to make sure SPLIT takes priority
        test_monster.set_special( "ABSORB_ITEMS", 0 );
        here.add_item( test_monster.pos_bub(), item( itype_frame ) );

        CHECK( monster_goals.tick( &oracle ) == "SPLIT" );

        mattack::split( &test_monster );

        // test that the monster returns to absorbing items after the split occurs
        CHECK( monster_goals.tick( &oracle ) == "ABSORB_ITEMS" );
        CHECK( test_monster.get_hp() < new_hp );
    }
}

TEST_CASE( "check_monster_behavior_tree_theoretical_corpse_eater", "[monster][behavior]" )
{
    const tripoint_bub_ms monster_location( 5, 5, 0 );
    clear_map();
    map &here = get_map();
    monster &test_monster = spawn_test_monster( "mon_shoggoth_flesh_only", monster_location );

    behavior::monster_oracle_t oracle( &test_monster );
    behavior::tree monster_goals;
    monster_goals.add( test_monster.type->get_goals() );

    for( const std::string &special_name : test_monster.type->special_attacks_names ) {
        test_monster.reset_special( special_name );
    }
    CHECK( monster_goals.tick( &oracle ) == "idle" );
    for( const tripoint_bub_ms &near_monster : here.points_in_radius( monster_location, 1 ) ) {
        here.ter_set( near_monster, ter_id( "t_grass" ) );
        here.furn_set( near_monster, furn_id( "f_null" ) );
    }
    SECTION( "Special Attack ABSORB_ITEMS" ) {
        test_monster.set_special( "SPLIT", 0 );
        test_monster.set_special( "ABSORB_ITEMS", 0 );
        CHECK( monster_goals.tick( &oracle ) == "idle" );

        item corpse( itype_corpse );
        corpse.force_insert_item( item( itype_pencil ), pocket_type::CONTAINER );

        here.add_item( test_monster.pos_bub(), corpse );
        CHECK( monster_goals.tick( &oracle ) == "ABSORB_ITEMS" );

        mattack::absorb_items( &test_monster );

        // test that the pencil remains after the corpse is absorbed
        CHECK( ( here.i_at( test_monster.pos_bub() ).begin() )->display_name().rfind( "pencil" ) !=
               std::string::npos );

        CHECK( monster_goals.tick( &oracle ) == "idle" );
    }
    SECTION( "Special Attack SPLIT" ) {
        test_monster.set_special( "SPLIT", 0 );
        int new_hp = test_monster.type->hp * 2 + 2;
        test_monster.set_hp( new_hp );

        // also set proper conditions for ABSORB_ITEMS to make sure SPLIT takes priority
        here.add_item( test_monster.pos_bub(), item( itype_corpse ) );
        test_monster.set_special( "ABSORB_ITEMS", 0 );

        CHECK( monster_goals.tick( &oracle ) == "SPLIT" );

        mattack::split( &test_monster );
        test_monster.set_special( "SPLIT", 1 );

        // test that the monster is idle after split since it shouldn't absorb if split is on cooldown
        CHECK( monster_goals.tick( &oracle ) == "idle" );
        CHECK( test_monster.get_hp() < new_hp );
    }
}

TEST_CASE( "check_monster_behavior_tree_theoretical_absorb", "[monster][behavior]" )
{
    // tests a monster with the ABSORB_ITEMS ability but NOT the SPLIT ability
    const tripoint_bub_ms monster_location( 5, 5, 0 );
    clear_map();
    map &here = get_map();
    monster &test_monster = spawn_test_monster( "mon_shoggoth_absorb_only", monster_location );

    behavior::monster_oracle_t oracle( &test_monster );
    behavior::tree monster_goals;
    monster_goals.add( test_monster.type->get_goals() );

    for( const std::string &special_name : test_monster.type->special_attacks_names ) {
        test_monster.reset_special( special_name );
    }
    CHECK( monster_goals.tick( &oracle ) == "idle" );
    for( const tripoint_bub_ms &near_monster : here.points_in_radius( monster_location, 1 ) ) {
        here.ter_set( near_monster, ter_id( "t_grass" ) );
        here.furn_set( near_monster, furn_id( "f_null" ) );
    }
    SECTION( "Special Attack ABSORB_ITEMS" ) {
        test_monster.set_special( "ABSORB_ITEMS", 0 );
        CHECK( monster_goals.tick( &oracle ) == "idle" );

        item corpse( itype_corpse );
        corpse.force_insert_item( item( itype_pencil ), pocket_type::CONTAINER );

        here.add_item( test_monster.pos_bub(), corpse );
        CHECK( monster_goals.tick( &oracle ) == "ABSORB_ITEMS" );

        mattack::absorb_items( &test_monster );

        // test that the pencil does not remain after the corpse is absorbed
        // because this shoggoth absorbs all matter indiscriminately
        CHECK( here.i_at( test_monster.pos_bub() ).empty() );

        CHECK( monster_goals.tick( &oracle ) == "idle" );
    }
}
