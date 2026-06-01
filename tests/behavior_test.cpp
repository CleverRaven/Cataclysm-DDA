#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "basecamp.h"
#include "behavior.h"
#include "behavior_oracle.h"
#include "behavior_strategy.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "character_attire.h"
#include "character_oracle.h"
#include "coordinates.h"
#include "creature.h"
#include "npc_decision_category.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "item.h"
#include "json_loader.h"
#include "item_group.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "mapgen.h"
#include "mapgendata.h"
#include "monattack.h"
#include "monster.h"
#include "monster_oracle.h"
#include "mtype.h"
#include "npc.h"
#include "npc_class.h"
#include "options_helpers.h"
#include "overmapbuffer.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "sounds.h"
#include "type_id.h"
#include "units.h"
#include "weather.h"
#include "weighted_list.h"

static const efftype_id effect_meth( "meth" );
static const efftype_id effect_npc_run_away( "npc_run_away" );

static const faction_id faction_your_followers( "your_followers" );

static const item_group_id Item_spawn_data_test_bottle_water( "test_bottle_water" );

static const itype_id itype_2x4( "2x4" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_corpse( "corpse" );
static const itype_id itype_frame( "frame" );
static const itype_id itype_lighter( "lighter" );
static const itype_id itype_orange( "orange" );
static const itype_id itype_pencil( "pencil" );
static const itype_id itype_sandwich_cheese_grilled( "sandwich_cheese_grilled" );
static const itype_id itype_sweater( "sweater" );
static const itype_id itype_water_clean( "water_clean" );

static const mtype_id mon_zombie( "mon_zombie" );

static const nested_mapgen_id nested_mapgen_test_seedling( "test_seedling" );

static const string_id<behavior::node_t> behavior_node_t_npc_decision( "npc_decision" );
static const string_id<behavior::node_t> behavior_node_t_npc_needs( "npc_needs" );

static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_ponywall( "t_ponywall" );
static const ter_str_id ter_t_wall( "t_wall" );

static const trait_id trait_IGNORE_SOUND( "IGNORE_SOUND" );
static const trait_id trait_RETURN_TO_START_POS( "RETURN_TO_START_POS" );

namespace behavior
{

static sequential_t default_sequential;
static fallback_t default_fallback;
static sequential_until_done_t default_until_done;
static utility_t default_utility;
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
    clear_map_without_vision();
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
        // With warmth unified under seek_warmth, inventory clothing
        // triggers can_obtain_warmth (not legacy can_wear_warmer_clothes).
        CHECK( oracle.can_obtain_warmth( "" ) == behavior::status_t::running );
        CHECK( npc_needs.tick( &oracle ) == "seek_warmth" );
        item sweater_copy = *sweater;
        test_npc.wear_item( sweater_copy );
        sweater.remove_item();
        CHECK( npc_needs.tick( &oracle ) == "idle" );
        test_npc.i_add( tool_with_ammo( itype_lighter, 20 ) );
        test_npc.i_add( item( itype_2x4 ) );
        get_map().build_map_cache( 0 );
        REQUIRE( oracle.can_make_fire( "" ) == behavior::status_t::running );
        // Fire supplies make can_obtain_warmth return running (fire is
        // a warmth strategy), so npc_obtain_warmth succeeds with seek_warmth.
        CHECK( npc_needs.tick( &oracle ) == "seek_warmth" );
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
    SECTION( "NO_NPC_FOOD suppresses hunger and thirst" ) {
        override_option no_food( "NO_NPC_FOOD", "true" );
        REQUIRE_FALSE( test_npc.needs_food() );

        // Extreme values that would normally trigger needs.
        test_npc.set_hunger( 500 );
        test_npc.set_stored_kcal( 1000 );
        test_npc.set_thirst( 700 );
        CHECK( oracle.needs_food_badly( "" ) == behavior::status_t::success );
        CHECK( oracle.needs_water_badly( "" ) == behavior::status_t::success );
        CHECK( oracle.hunger_urgency( "" ) == 0.0f );
        CHECK( oracle.thirst_urgency( "" ) == 0.0f );

        // Even with food and water in inventory, BT returns idle.
        test_npc.i_add( item( itype_sandwich_cheese_grilled ) );
        const item_group::ItemList water_items = item_group::items_from(
                    Item_spawn_data_test_bottle_water );
        test_npc.i_add( water_items.front() );
        CHECK( npc_needs.tick( &oracle ) == "idle" );
    }
    SECTION( "Thirsty" ) {
        const item_group::ItemList items = item_group::items_from( Item_spawn_data_test_bottle_water );

        // make sure only water bottles are in this group
        REQUIRE( items.size() == 1 );
        REQUIRE( item_group::group_contains_item( Item_spawn_data_test_bottle_water, itype_water_clean ) );

        test_npc.set_thirst( 700 );
        REQUIRE( oracle.needs_water_badly( "" ) == behavior::status_t::running );
        CHECK( npc_needs.tick( &oracle ) == "idle" );

        item_location water = test_npc.i_add( items.front() );

        REQUIRE( oracle.has_water( "" ) == behavior::status_t::running );
        CHECK( npc_needs.tick( &oracle ) == "drink_water" );
        water.remove_item();
        CHECK( npc_needs.tick( &oracle ) == "idle" );
    }
    SECTION( "Thirsty and hungry" ) {
        // With utility strategy, the more urgent need wins.
        // Near-starvation (stored_kcal=1000, urgency ~0.98) beats
        // moderate dehydration (thirst=700, urgency ~0.58).
        test_npc.set_thirst( 700 );
        test_npc.set_hunger( 500 );
        test_npc.set_stored_kcal( 1000 );
        REQUIRE( oracle.needs_water_badly( "" ) == behavior::status_t::running );
        REQUIRE( oracle.needs_food_badly( "" ) == behavior::status_t::running );
        const item_group::ItemList water_items = item_group::items_from(
                    Item_spawn_data_test_bottle_water );
        test_npc.i_add( water_items.front() );
        test_npc.i_add( item( itype_sandwich_cheese_grilled ) );
        REQUIRE( oracle.has_water( "" ) == behavior::status_t::running );
        REQUIRE( oracle.has_food( "" ) == behavior::status_t::running );
        CHECK( npc_needs.tick( &oracle ) == "eat_food" );
    }
    SECTION( "Hunger wins over thirst when starvation is more urgent" ) {
        // Hunger comes AFTER thirst in tree order, so under sequential_until_done
        // thirst would always win. With utility, hunger wins when its score is
        // higher. stored_kcal=1000 gives hunger_urgency ~0.98 vs
        // thirst_urgency at 600 = 0.5.
        test_npc.set_stored_kcal( 1000 );
        test_npc.set_hunger( 500 );
        test_npc.set_thirst( 600 );
        REQUIRE( oracle.needs_water_badly( "" ) == behavior::status_t::running );
        REQUIRE( oracle.needs_food_badly( "" ) == behavior::status_t::running );
        const item_group::ItemList water_items = item_group::items_from(
                    Item_spawn_data_test_bottle_water );
        test_npc.i_add( water_items.front() );
        test_npc.i_add( item( itype_sandwich_cheese_grilled ) );
        REQUIRE( oracle.has_water( "" ) == behavior::status_t::running );
        REQUIRE( oracle.has_food( "" ) == behavior::status_t::running );
        CHECK( npc_needs.tick( &oracle ) == "eat_food" );
    }
    SECTION( "Thirst wins over hunger when dehydration is more urgent" ) {
        // Guards the "score": "npc_thirst_urgency" wiring on npc_thirst.
        // Without it, thirst defaults to score 0 and loses to any scored hunger.
        // At 45% healthy kcal, starvation ~2794 > base_metabolic_rate (2500)
        // so needs_food_badly fires. But hunger_urgency (0.55) < thirst_urgency
        // at 800 (0.667), so thirst wins.
        const int healthy = test_npc.get_healthy_kcal();
        test_npc.set_stored_kcal( healthy * 45 / 100 );
        test_npc.set_hunger( 500 );
        test_npc.set_thirst( 800 );
        REQUIRE( oracle.needs_water_badly( "" ) == behavior::status_t::running );
        REQUIRE( oracle.needs_food_badly( "" ) == behavior::status_t::running );
        const item_group::ItemList water_items = item_group::items_from(
                    Item_spawn_data_test_bottle_water );
        test_npc.i_add( water_items.front() );
        test_npc.i_add( item( itype_sandwich_cheese_grilled ) );
        REQUIRE( oracle.has_water( "" ) == behavior::status_t::running );
        REQUIRE( oracle.has_food( "" ) == behavior::status_t::running );
        CHECK( npc_needs.tick( &oracle ) == "drink_water" );
    }
    SECTION( "Freezing wins over moderate thirst" ) {
        weather_manager &weather = get_weather();
        weather.temperature = units::from_fahrenheit( 0 );
        weather.clear_temp_cache();
        test_npc.update_bodytemp();
        REQUIRE( oracle.needs_warmth_badly( "" ) == behavior::status_t::running );

        test_npc.set_thirst( 700 );
        REQUIRE( oracle.needs_water_badly( "" ) == behavior::status_t::running );

        // Warm clothes + water available
        test_npc.worn.wear_item( test_npc, item( itype_backpack ), false, false );
        test_npc.i_add( item( itype_sweater ) );
        const item_group::ItemList water_items = item_group::items_from(
                    Item_spawn_data_test_bottle_water );
        test_npc.i_add( water_items.front() );
        REQUIRE( oracle.can_obtain_warmth( "" ) == behavior::status_t::running );
        REQUIRE( oracle.has_water( "" ) == behavior::status_t::running );

        // warmth_urgency (near 1.0) >> thirst_urgency (0.58)
        CHECK( npc_needs.tick( &oracle ) == "seek_warmth" );
    }
    SECTION( "Freezing with fire supplies also wins over thirst" ) {
        // Proves the score lives on npc_homeostasis (the fallback branch),
        // not on npc_obtain_warmth. If the score were misplaced on the
        // leaf, this path through npc_make_fire would be unscored.
        weather_manager &weather = get_weather();
        weather.temperature = units::from_fahrenheit( 0 );
        weather.clear_temp_cache();
        test_npc.update_bodytemp();
        REQUIRE( oracle.needs_warmth_badly( "" ) == behavior::status_t::running );

        test_npc.set_thirst( 700 );
        REQUIRE( oracle.needs_water_badly( "" ) == behavior::status_t::running );

        // Fire supplies + water, no warm clothes or shelter
        test_npc.worn.wear_item( test_npc, item( itype_backpack ), false, false );
        test_npc.i_add( tool_with_ammo( itype_lighter, 20 ) );
        test_npc.i_add( item( itype_2x4 ) );
        const item_group::ItemList water_items = item_group::items_from(
                    Item_spawn_data_test_bottle_water );
        test_npc.i_add( water_items.front() );
        // find_fire_spot needs map cache for move_cost/is_flammable.
        get_map().build_map_cache( 0 );
        // Fire supplies make can_obtain_warmth return running (fire is
        // a warmth strategy inside find_warmth_candidates).
        REQUIRE( oracle.can_obtain_warmth( "" ) == behavior::status_t::running );
        REQUIRE( oracle.can_make_fire( "" ) == behavior::status_t::running );
        REQUIRE( oracle.has_water( "" ) == behavior::status_t::running );

        CHECK( npc_needs.tick( &oracle ) == "seek_warmth" );
    }
    SECTION( "Dead tired -- sleep is feasible" ) {
        // can_sleep threshold matches needs_sleep_badly at DEAD_TIRED.
        // Utility scoring handles duty priority, not can_sleep gating.
        test_npc.set_sleepiness( 500 );
        REQUIRE( oracle.needs_sleep_badly( "" ) == behavior::status_t::running );
        REQUIRE( oracle.can_sleep( "" ) == behavior::status_t::running );
        CHECK( npc_needs.tick( &oracle ) == "go_to_sleep" );
    }
    SECTION( "Exhausted -- sleep is feasible" ) {
        test_npc.set_sleepiness( 600 );
        REQUIRE( oracle.needs_sleep_badly( "" ) == behavior::status_t::running );
        REQUIRE( oracle.can_sleep( "" ) == behavior::status_t::running );
        CHECK( npc_needs.tick( &oracle ) == "go_to_sleep" );
    }
    SECTION( "Exhausted on meth -- sleep blocked" ) {
        test_npc.set_sleepiness( 600 );
        test_npc.add_effect( effect_meth, 1_hours );
        REQUIRE( oracle.needs_sleep_badly( "" ) == behavior::status_t::running );
        CHECK( oracle.can_sleep( "" ) == behavior::status_t::failure );
        CHECK( npc_needs.tick( &oracle ) == "idle" );
    }
    SECTION( "Exhausted with stim -- sleep still feasible" ) {
        // Stim is a soft modifier in Character::can_sleep() whose effect
        // depends on comfort at the sleep location. The oracle can't
        // evaluate that, so only meth is a hard blocker.
        test_npc.set_sleepiness( 600 );
        test_npc.set_stim( 20 );
        REQUIRE( oracle.needs_sleep_badly( "" ) == behavior::status_t::running );
        CHECK( oracle.can_sleep( "" ) == behavior::status_t::running );
    }
    SECTION( "Exhausted and hungry -- hunger wins" ) {
        // sleepiness=600 (urgency 0.6), stored_kcal=1000 (urgency ~0.98)
        // Near-starvation beats moderate exhaustion.
        test_npc.set_sleepiness( 600 );
        test_npc.set_stored_kcal( 1000 );
        test_npc.set_hunger( 500 );
        REQUIRE( oracle.needs_sleep_badly( "" ) == behavior::status_t::running );
        REQUIRE( oracle.can_sleep( "" ) == behavior::status_t::running );
        REQUIRE( oracle.needs_food_badly( "" ) == behavior::status_t::running );
        test_npc.i_add( item( itype_sandwich_cheese_grilled ) );
        REQUIRE( oracle.has_food( "" ) == behavior::status_t::running );
        CHECK( npc_needs.tick( &oracle ) == "eat_food" );
    }
    SECTION( "Exhausted beats moderate thirst" ) {
        // sleepiness=800 (urgency 0.8), thirst=600 (urgency 0.5)
        test_npc.set_sleepiness( 800 );
        test_npc.set_thirst( 600 );
        REQUIRE( oracle.needs_sleep_badly( "" ) == behavior::status_t::running );
        REQUIRE( oracle.can_sleep( "" ) == behavior::status_t::running );
        REQUIRE( oracle.needs_water_badly( "" ) == behavior::status_t::running );
        const item_group::ItemList water_items = item_group::items_from(
                    Item_spawn_data_test_bottle_water );
        test_npc.i_add( water_items.front() );
        REQUIRE( oracle.has_water( "" ) == behavior::status_t::running );
        CHECK( npc_needs.tick( &oracle ) == "go_to_sleep" );
    }
    SECTION( "can_make_fire returns failure without supplies" ) {
        // Regression: can_make_fire used to return success instead of failure
        // when no FIRESTARTER or flammable items were present, causing the
        // warmth fallback to short-circuit to success before reaching shelter.
        CHECK( oracle.can_make_fire( "" ) == behavior::status_t::failure );
    }
    SECTION( "can_take_shelter outdoors with adjacent indoor tile" ) {
        map &here = get_map();
        tripoint_bub_ms adj = test_npc.pos_bub() + point::east;
        here.ter_set( adj, ter_t_floor );
        CHECK( oracle.can_take_shelter( "" ) == behavior::status_t::running );
    }
    SECTION( "can_take_shelter outdoors with no indoor tiles" ) {
        CHECK( oracle.can_take_shelter( "" ) == behavior::status_t::failure );
    }
    SECTION( "can_take_shelter ignores impassable indoor tiles" ) {
        // A pony wall is INDOORS but impassable -- not a shelter target
        map &here = get_map();
        tripoint_bub_ms adj = test_npc.pos_bub() + point::east;
        here.ter_set( adj, ter_t_ponywall );
        REQUIRE( here.has_flag( ter_furn_flag::TFLAG_INDOORS, adj ) );
        REQUIRE( here.impassable( adj ) );
        CHECK( oracle.can_take_shelter( "" ) == behavior::status_t::failure );
    }
    SECTION( "can_take_shelter already indoors" ) {
        map &here = get_map();
        here.ter_set( test_npc.pos_bub(), ter_t_floor );
        CHECK( oracle.can_take_shelter( "" ) == behavior::status_t::failure );
    }
    SECTION( "can_take_shelter detects indoor tile beyond adjacent" ) {
        map &here = get_map();
        // Distance 2 only: earlier sections in this TEST_CASE mutate global
        // weather (temperature -> 0F) without cleanup, reducing NPC vision
        // range via stale weather state. Radius-4+ detection is covered by
        // npc_find_nearby_shelters in npc_test.cpp which has a clean fixture.
        const tripoint_bub_ms shelter = test_npc.pos_bub() + tripoint( 2, 0, 0 );
        here.ter_set( shelter, ter_t_floor );
        here.invalidate_map_cache( 0 );
        here.build_map_cache( 0, true );
        REQUIRE( here.has_flag( ter_furn_flag::TFLAG_INDOORS, shelter ) );
        REQUIRE( here.passable( shelter ) );
        REQUIRE( test_npc.sees( here, shelter ) );
        CHECK( oracle.can_take_shelter( "" ) == behavior::status_t::running );
    }
    SECTION( "can_take_shelter fails beyond 6" ) {
        map &here = get_map();
        here.ter_set( test_npc.pos_bub() + tripoint( 7, 0, 0 ), ter_t_floor );
        here.invalidate_map_cache( 0 );
        here.build_map_cache( 0, true );
        CHECK( oracle.can_take_shelter( "" ) == behavior::status_t::failure );
    }
    SECTION( "can_take_shelter fails without LOS" ) {
        map &here = get_map();
        tripoint_bub_ms wall_pos = test_npc.pos_bub() + point::east;
        tripoint_bub_ms shelter = wall_pos + point::east;
        here.ter_set( wall_pos, ter_t_wall );
        here.ter_set( shelter, ter_t_floor );
        here.invalidate_map_cache( 0 );
        here.build_map_cache( 0, true );
        CHECK( oracle.can_take_shelter( "" ) == behavior::status_t::failure );
    }
    SECTION( "Freezing outdoors next to building" ) {
        weather_manager &weather = get_weather();
        weather.temperature = units::from_fahrenheit( 0 );
        weather.clear_temp_cache();
        test_npc.update_bodytemp();
        REQUIRE( oracle.needs_warmth_badly( "" ) == behavior::status_t::running );

        // No warm clothes, no fire, but indoor tile adjacent
        map &here = get_map();
        tripoint_bub_ms adj = test_npc.pos_bub() + point::east;
        here.ter_set( adj, ter_t_floor );
        REQUIRE( oracle.can_take_shelter( "" ) == behavior::status_t::running );

        // seek_warmth fires before legacy take_shelter because
        // npc_obtain_warmth is first in the warmth fallback chain.
        CHECK( npc_needs.tick( &oracle ) == "seek_warmth" );
    }
    SECTION( "Freezing outdoors with no shelter" ) {
        weather_manager &weather = get_weather();
        weather.temperature = units::from_fahrenheit( 0 );
        weather.clear_temp_cache();
        test_npc.update_bodytemp();
        REQUIRE( oracle.needs_warmth_badly( "" ) == behavior::status_t::running );
        REQUIRE( oracle.can_obtain_warmth( "" ) == behavior::status_t::failure );

        // No warmth sources, no fire supplies -> idle
        CHECK( npc_needs.tick( &oracle ) == "idle" );
    }
}

TEST_CASE( "tree_tick_full_returns_score", "[behavior]" )
{
    // Verify tick_full exposes the BT's utility score.
    // A cold, thirsty NPC with both water and warmth sources available
    // should produce a goal with a nonzero urgency score.
    clear_map_without_vision();
    set_time_to_day();
    get_weather().forced_temperature = -30_C;

    npc &test_npc = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( test_npc, true );
    test_npc.set_fac( faction_your_followers );
    test_npc.set_attitude( NPCATT_FOLLOW );

    // Make the NPC cold and thirsty so npc_needs produces a scored goal.
    test_npc.set_all_parts_temp_conv( BODYTEMP_VERY_COLD );
    test_npc.set_thirst( 700 );
    test_npc.set_stored_kcal( 55000 );
    test_npc.set_hunger( 0 );
    test_npc.worn.wear_item( test_npc, item( itype_backpack ), false, false );

    // Give warmth source (inventory sweater) and water source (inventory).
    test_npc.i_add( item( itype_sweater ) );
    const item_group::ItemList water_items = item_group::items_from(
                Item_spawn_data_test_bottle_water );
    test_npc.i_add( water_items.front() );

    map &here = get_map();
    here.build_map_cache( 0 );

    behavior::character_oracle_t oracle( &test_npc );
    behavior::tree bt;
    bt.add( &behavior_node_t_npc_needs.obj() );

    auto [goal, score] = bt.tick_full( &oracle );
    // The BT should select a needs goal with a positive urgency score.
    CHECK_FALSE( goal == "idle" );
    CHECK( score > 0.0f );
    // tick_full and tick should agree on the goal.
    CHECK( bt.tick( &oracle ) == goal );
}

TEST_CASE( "check_monster_behavior_tree_locust", "[monster][behavior]" )
{
    const tripoint_bub_ms monster_location( 5, 5, 0 );
    clear_map_without_vision();
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
    clear_map_without_vision();
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
    clear_map_without_vision();
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
    clear_map_without_vision();
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

TEST_CASE( "behavior_tree_utility_strategy", "[behavior]" )
{
    SECTION( "leaf nodes with scores" ) {
        behavior::status_t status_a = behavior::status_t::running;
        behavior::status_t status_b = behavior::status_t::running;
        behavior::status_t status_c = behavior::status_t::running;
        float score_a = 5.0f;
        float score_b = 10.0f;
        float score_c = 3.0f;

        behavior::node_t node_a = make_test_node( "goal_a", &status_a );
        node_a.set_score_function( [&score_a]( const behavior::oracle_t *, std::string_view ) {
            return score_a;
        } );

        behavior::node_t node_b = make_test_node( "goal_b", &status_b );
        node_b.set_score_function( [&score_b]( const behavior::oracle_t *, std::string_view ) {
            return score_b;
        } );

        behavior::node_t node_c = make_test_node( "goal_c", &status_c );
        node_c.set_score_function( [&score_c]( const behavior::oracle_t *, std::string_view ) {
            return score_c;
        } );

        behavior::node_t root;
        root.set_strategy( &behavior::default_utility );
        root.add_child( &node_a );
        root.add_child( &node_b );
        root.add_child( &node_c );

        behavior::tree needs;
        needs.add( &root );

        // Highest score wins
        CHECK( needs.tick( nullptr ) == "goal_b" );

        // Change scores -- now A is most urgent
        score_a = 20.0f;
        CHECK( needs.tick( nullptr ) == "goal_a" );

        // B succeeds (need met) -- skipped, A still wins
        status_b = behavior::status_t::success;
        CHECK( needs.tick( nullptr ) == "goal_a" );

        // A also succeeds -- C is the only running child
        status_a = behavior::status_t::success;
        CHECK( needs.tick( nullptr ) == "goal_c" );

        // All succeed -- idle
        status_c = behavior::status_t::success;
        CHECK( needs.tick( nullptr ) == "idle" );

        // All fail -- also idle
        status_a = behavior::status_t::failure;
        status_b = behavior::status_t::failure;
        status_c = behavior::status_t::failure;
        CHECK( needs.tick( nullptr ) == "idle" );
    }

    SECTION( "branch nodes with scores" ) {
        // Mimics the real NPC tree structure:
        // root (utility) -> branch_a (sequential) -> leaf_a1
        //                 -> branch_b (sequential) -> leaf_b1

        behavior::status_t leaf_a1_status = behavior::status_t::running;
        behavior::status_t leaf_b1_status = behavior::status_t::running;
        behavior::status_t branch_a_pred = behavior::status_t::running;
        behavior::status_t branch_b_pred = behavior::status_t::running;
        float score_a = 5.0f;
        float score_b = 10.0f;

        behavior::node_t leaf_a1 = make_test_node( "goal_a1", &leaf_a1_status );
        behavior::node_t leaf_b1 = make_test_node( "goal_b1", &leaf_b1_status );

        behavior::node_t branch_a;
        branch_a.set_strategy( &behavior::default_sequential );
        branch_a.add_predicate( [&branch_a_pred]( const behavior::oracle_t *, std::string_view ) {
            return branch_a_pred;
        } );
        branch_a.add_child( &leaf_a1 );
        branch_a.set_score_function( [&score_a]( const behavior::oracle_t *, std::string_view ) {
            return score_a;
        } );

        behavior::node_t branch_b;
        branch_b.set_strategy( &behavior::default_sequential );
        branch_b.add_predicate( [&branch_b_pred]( const behavior::oracle_t *, std::string_view ) {
            return branch_b_pred;
        } );
        branch_b.add_child( &leaf_b1 );
        branch_b.set_score_function( [&score_b]( const behavior::oracle_t *, std::string_view ) {
            return score_b;
        } );

        behavior::node_t root;
        root.set_strategy( &behavior::default_utility );
        root.add_child( &branch_a );
        root.add_child( &branch_b );

        behavior::tree needs;
        needs.add( &root );

        // Branch B has higher score, so goal_b1 is selected
        CHECK( needs.tick( nullptr ) == "goal_b1" );

        // Swap scores -- now branch A wins
        score_a = 15.0f;
        score_b = 2.0f;
        CHECK( needs.tick( nullptr ) == "goal_a1" );

        // Branch A's predicate fails -- falls through to B
        branch_a_pred = behavior::status_t::failure;
        CHECK( needs.tick( nullptr ) == "goal_b1" );
    }

    SECTION( "JSON-loaded score predicate" ) {
        // Exercises the actual node_t::load() path for "score" field parsing
        float test_score = 42.0f;
        behavior::score_predicate_map["test_urgency"] =
        [&test_score]( const behavior::oracle_t *, std::string_view ) {
            return test_score;
        };

        // Load a node from JSON with a score predicate, same as the generic factory does
        const std::string json = R"({
            "goal": "scored_goal",
            "score": "test_urgency"
        })";
        behavior::node_t node;
        JsonObject jo = json_loader::from_string( json );
        node.load( jo, "" );

        // The node should have picked up the score function from the map
        behavior::node_t root;
        root.set_strategy( &behavior::default_utility );
        root.add_child( &node );

        behavior::tree needs;
        needs.add( &root );

        CHECK( needs.tick( nullptr ) == "scored_goal" );

        // Clean up
        behavior::score_predicate_map.erase( "test_urgency" );
    }
}

TEST_CASE( "npc_urgency_score_predicates", "[npc][behavior]" )
{
    clear_map_without_vision();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy );
    behavior::character_oracle_t oracle( &guy );

    SECTION( "thirst_urgency scales from 0 to 1" ) {
        guy.set_thirst( 0 );
        CHECK( oracle.thirst_urgency( "" ) == Approx( 0.0f ).margin( 0.01f ) );

        guy.set_thirst( 600 );
        CHECK( oracle.thirst_urgency( "" ) == Approx( 0.5f ).margin( 0.01f ) );

        guy.set_thirst( 1200 );
        CHECK( oracle.thirst_urgency( "" ) == Approx( 1.0f ).margin( 0.01f ) );
    }

    SECTION( "hunger_urgency scales with kcal deficit" ) {
        const int healthy = guy.get_healthy_kcal();
        guy.set_stored_kcal( healthy );
        CHECK( oracle.hunger_urgency( "" ) == Approx( 0.0f ).margin( 0.01f ) );

        guy.set_stored_kcal( healthy / 2 );
        CHECK( oracle.hunger_urgency( "" ) == Approx( 0.5f ).margin( 0.05f ) );

        guy.set_stored_kcal( 0 );
        CHECK( oracle.hunger_urgency( "" ) == Approx( 1.0f ).margin( 0.01f ) );
    }

    SECTION( "warmth_urgency responds to cold bodyparts" ) {
        // Explicitly set baseline -- clear_character does not guarantee temps
        guy.set_all_parts_temp_conv( BODYTEMP_NORM );
        CHECK( oracle.warmth_urgency( "" ) < 0.01f );

        // Single cold bodypart drives the score
        guy.set_part_temp_conv( body_part_torso, BODYTEMP_VERY_COLD );
        float cold_score = oracle.warmth_urgency( "" );
        CHECK( cold_score > 0.5f );
        CHECK( cold_score < 1.0f );

        // At BODYTEMP_FREEZING the score saturates near 1.0
        guy.set_part_temp_conv( body_part_torso, BODYTEMP_FREEZING );
        CHECK( oracle.warmth_urgency( "" ) == Approx( 1.0f ).margin( 0.05f ) );
    }

    SECTION( "sleepiness_urgency scales from 0 to 1" ) {
        guy.set_sleepiness( 0 );
        CHECK( oracle.sleepiness_urgency( "" ) == Approx( 0.0f ).margin( 0.01f ) );

        guy.set_sleepiness( 500 );
        CHECK( oracle.sleepiness_urgency( "" ) == Approx( 0.5f ).margin( 0.01f ) );

        guy.set_sleepiness( 1000 );
        CHECK( oracle.sleepiness_urgency( "" ) == Approx( 1.0f ).margin( 0.01f ) );
    }

    SECTION( "score predicates registered in score_predicate_map" ) {
        CHECK( behavior::score_predicate_map.count( "npc_thirst_urgency" ) == 1 );
        CHECK( behavior::score_predicate_map.count( "npc_hunger_urgency" ) == 1 );
        CHECK( behavior::score_predicate_map.count( "npc_warmth_urgency" ) == 1 );
        CHECK( behavior::score_predicate_map.count( "npc_sleepiness_urgency" ) == 1 );
    }

    SECTION( "predicates registered in predicate_map" ) {
        CHECK( behavior::predicate_map.count( "npc_needs_sleep_badly" ) == 1 );
    }
}

TEST_CASE( "npc_decision_combat_predicates", "[npc][behavior]" )
{
    clear_map();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy );
    // Allied faction ensures assess_danger considers zombies as threats,
    // matching npc_attack_test setup.
    guy.set_fac( faction_your_followers );
    behavior::character_oracle_t oracle( &guy );

    SECTION( "in_danger running with hostile creature nearby" ) {
        map &here = get_map();
        // Player underground so they don't interfere with NPC threat assessment
        get_player_character().setpos( here, guy.pos_bub() + tripoint( 0, 0, -2 ) );
        monster *zed = g->place_critter_at( mon_zombie,
                                            guy.pos_bub() + point::east );
        REQUIRE( zed != nullptr );
        REQUIRE( zed->attitude_to( guy ) == Creature::Attitude::HOSTILE );
        guy.recalc_sight_limits();
        REQUIRE( guy.sees( here, *zed ) );
        guy.regen_ai_cache();
        REQUIRE( guy.get_ai_danger() > 0 );
        CHECK( oracle.in_danger( "" ) == behavior::status_t::running );
    }
    SECTION( "in_danger failure when no danger" ) {
        guy.recalc_sight_limits();
        guy.regen_ai_cache();
        CHECK( oracle.in_danger( "" ) == behavior::status_t::failure );
    }
    SECTION( "should_flee running with run_away effect" ) {
        guy.add_effect( effect_npc_run_away, 10_turns );
        CHECK( oracle.should_flee( "" ) == behavior::status_t::running );
    }
    SECTION( "should_flee running with FLEE_TEMP attitude" ) {
        guy.set_attitude( NPCATT_FLEE_TEMP );
        CHECK( oracle.should_flee( "" ) == behavior::status_t::running );
    }
    SECTION( "should_flee failure without effect or attitude" ) {
        CHECK( oracle.should_flee( "" ) == behavior::status_t::failure );
    }
    SECTION( "has_target running with hostile creature" ) {
        map &here = get_map();
        get_player_character().setpos( here, guy.pos_bub() + tripoint( 0, 0, -2 ) );
        monster *zed = g->place_critter_at( mon_zombie,
                                            guy.pos_bub() + point::east );
        REQUIRE( zed != nullptr );
        guy.recalc_sight_limits();
        REQUIRE( guy.sees( here, *zed ) );
        guy.regen_ai_cache();
        REQUIRE( guy.get_ai_danger() > 0 );
        CHECK( oracle.has_target( "" ) == behavior::status_t::running );
    }
    SECTION( "has_target failure when no target" ) {
        guy.recalc_sight_limits();
        guy.regen_ai_cache();
        CHECK( oracle.has_target( "" ) == behavior::status_t::failure );
    }
    SECTION( "has_sound_alerts failure when quiet" ) {
        CHECK( oracle.has_sound_alerts( "" ) == behavior::status_t::failure );
    }
}

TEST_CASE( "npc_decision_sound_alert_predicates", "[npc][behavior]" )
{
    clear_map();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy );
    behavior::character_oracle_t oracle( &guy );

    SECTION( "has_sound_alerts running when alerts present" ) {
        guy.push_ai_sound_alert( guy.pos_abs() + tripoint( 10, 0, 0 ),
                                 sounds::sound_t::combat, 20 );
        CHECK( oracle.has_sound_alerts( "" ) == behavior::status_t::running );
    }
    SECTION( "has_sound_alerts suppressed for walking companion" ) {
        guy.set_attitude( NPCATT_FOLLOW );
        REQUIRE( guy.is_walking_with() );
        guy.push_ai_sound_alert( guy.pos_abs() + tripoint( 10, 0, 0 ),
                                 sounds::sound_t::combat, 20 );
        CHECK( oracle.has_sound_alerts( "" ) == behavior::status_t::failure );
    }
    SECTION( "has_sound_alerts suppressed for IGNORE_SOUND trait" ) {
        guy.set_mutation( trait_IGNORE_SOUND );
        REQUIRE( guy.has_trait( trait_IGNORE_SOUND ) );
        guy.push_ai_sound_alert( guy.pos_abs() + tripoint( 10, 0, 0 ),
                                 sounds::sound_t::combat, 20 );
        CHECK( oracle.has_sound_alerts( "" ) == behavior::status_t::failure );
    }
}

TEST_CASE( "npc_decision_duty_predicates", "[npc][behavior]" )
{
    clear_map_without_vision();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy );
    behavior::character_oracle_t oracle( &guy );

    SECTION( "displaced running when guard_pos set elsewhere" ) {
        guy.set_guard_pos( guy.pos_abs() + tripoint( 10, 0, 0 ) );
        CHECK( oracle.displaced_from_post( "" ) == behavior::status_t::running );
    }
    SECTION( "displaced failure when no guard_pos" ) {
        CHECK( oracle.displaced_from_post( "" ) == behavior::status_t::failure );
    }
    SECTION( "displaced failure when at guard_pos" ) {
        guy.set_guard_pos( guy.pos_abs() );
        CHECK( oracle.displaced_from_post( "" ) == behavior::status_t::failure );
    }
    SECTION( "displaced detects persistent guard_pos from mission" ) {
        // Refugee Center guards have npc::guard_pos set by dialogue,
        // not ai_cache.guard_pos. The predicate must check both.
        guy.guard_pos = guy.pos_abs() + tripoint( 10, 0, 0 );
        CHECK( oracle.displaced_from_post( "" ) == behavior::status_t::running );
    }
    SECTION( "duty_urgency zero without post" ) {
        CHECK( oracle.duty_urgency( "" ) == Approx( 0.0f ) );
    }
    SECTION( "on_shift follows work_hours from npc_class" ) {
        // Default work_hours [0,24] means always on shift.
        guy.set_guard_pos( guy.pos_abs() );
        calendar::turn = calendar::turn_zero + 14_hours;
        CHECK( oracle.on_shift( "" ) == behavior::status_t::running );
        calendar::turn = calendar::turn_zero + 2_hours;
        CHECK( oracle.on_shift( "" ) == behavior::status_t::running );
    }
    SECTION( "on_shift requires guard_pos" ) {
        CHECK( oracle.on_shift( "" ) == behavior::status_t::failure );
    }
    SECTION( "duty_urgency on-shift at post" ) {
        // Default work_hours [0,24]: always on shift.
        guy.set_guard_pos( guy.pos_abs() );
        calendar::turn = calendar::turn_zero + 12_hours;
        CHECK( oracle.duty_urgency( "" ) == Approx( 0.45f ) );
    }
    SECTION( "duty_urgency on-shift scales with distance" ) {
        calendar::turn = calendar::turn_zero + 12_hours;

        guy.set_guard_pos( guy.pos_abs() + tripoint::east );
        CHECK( oracle.duty_urgency( "" ) == Approx( 0.45f ) );

        guy.set_guard_pos( guy.pos_abs() + tripoint( 5, 0, 0 ) );
        CHECK( oracle.duty_urgency( "" ) == Approx( 0.45f ) );

        guy.set_guard_pos( guy.pos_abs() + tripoint( 10, 0, 0 ) );
        CHECK( oracle.duty_urgency( "" ) == Approx( 0.5f ) );

        guy.set_guard_pos( guy.pos_abs() + tripoint( 20, 0, 0 ) );
        CHECK( oracle.duty_urgency( "" ) == Approx( 0.5f ) );
    }
}

TEST_CASE( "npc_decision_tree_priorities", "[npc][behavior]" )
{
    clear_map();
    behavior::tree npc_decision;
    npc_decision.add( &behavior_node_t_npc_decision.obj() );
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy );
    guy.set_fac( faction_your_followers );
    behavior::character_oracle_t oracle( &guy );

    SECTION( "combat beats any need" ) {
        map &here = get_map();
        get_player_character().setpos( here, guy.pos_bub() + tripoint( 0, 0, -2 ) );
        monster *zed = g->place_critter_at( mon_zombie,
                                            guy.pos_bub() + point::east );
        REQUIRE( zed != nullptr );
        guy.recalc_sight_limits();
        guy.regen_ai_cache();
        REQUIRE( guy.get_ai_danger() > 0 );
        guy.set_stored_kcal( 1000 );
        guy.set_hunger( 500 );
        guy.i_add( item( itype_sandwich_cheese_grilled ) );
        REQUIRE( oracle.needs_food_badly( "" ) == behavior::status_t::running );
        CHECK( npc_decision.tick( &oracle ) == "fight" );
    }
    SECTION( "flee beats fight" ) {
        map &here = get_map();
        get_player_character().setpos( here, guy.pos_bub() + tripoint( 0, 0, -2 ) );
        monster *zed = g->place_critter_at( mon_zombie,
                                            guy.pos_bub() + point::east );
        REQUIRE( zed != nullptr );
        guy.recalc_sight_limits();
        guy.regen_ai_cache();
        REQUIRE( guy.get_ai_danger() > 0 );
        guy.add_effect( effect_npc_run_away, 10_turns );
        CHECK( npc_decision.tick( &oracle ) == "flee" );
    }
    SECTION( "investigation beats needs" ) {
        guy.push_ai_sound_alert( guy.pos_abs() + tripoint( 10, 0, 0 ),
                                 sounds::sound_t::combat, 20 );
        REQUIRE( oracle.has_sound_alerts( "" ) == behavior::status_t::running );
        guy.set_stored_kcal( 1000 );
        guy.set_hunger( 500 );
        guy.i_add( item( itype_sandwich_cheese_grilled ) );
        REQUIRE( oracle.needs_food_badly( "" ) == behavior::status_t::running );
        CHECK( npc_decision.tick( &oracle ) == "investigate_sound" );
    }
    SECTION( "feasible extreme need beats duty" ) {
        guy.set_thirst( 1000 );
        const item_group::ItemList water = item_group::items_from(
                                               Item_spawn_data_test_bottle_water );
        guy.i_add( water.front() );
        REQUIRE( oracle.needs_water_badly( "" ) == behavior::status_t::running );
        REQUIRE( oracle.has_water( "" ) == behavior::status_t::running );
        guy.set_guard_pos( guy.pos_abs() + tripoint( 10, 0, 0 ) );
        REQUIRE( oracle.thirst_urgency( "" ) > oracle.duty_urgency( "" ) );
        CHECK( npc_decision.tick( &oracle ) == "drink_water" );
    }
    SECTION( "infeasible need does not inflate score over duty" ) {
        // Sleepiness 800 -> urgency 0.8, but on meth -> can_sleep fails.
        // Thirst 200 -> below needs_water_badly threshold -> not running.
        // No feasible need runs, so duty should win.
        guy.set_sleepiness( 800 );
        guy.add_effect( effect_meth, 1_hours );
        guy.set_thirst( 200 );
        guy.set_guard_pos( guy.pos_abs() + tripoint( 10, 0, 0 ) );
        REQUIRE( oracle.needs_sleep_badly( "" ) == behavior::status_t::running );
        REQUIRE( oracle.can_sleep( "" ) == behavior::status_t::failure );
        CHECK( npc_decision.tick( &oracle ) == "return_to_guard_pos" );
    }
    SECTION( "duty wins when needs are below threshold" ) {
        guy.set_thirst( 200 );
        guy.set_guard_pos( guy.pos_abs() + tripoint( 10, 0, 0 ) );
        CHECK( npc_decision.tick( &oracle ) == "return_to_guard_pos" );
    }
    SECTION( "exhausted guard sleeps over duty" ) {
        // sleepiness 600 -> urgency 0.6 > duty 0.5. Policy: exhaustion
        // is dangerous (microsleeps), guard should sleep.
        guy.set_sleepiness( 600 );
        REQUIRE( oracle.needs_sleep_badly( "" ) == behavior::status_t::running );
        REQUIRE( oracle.can_sleep( "" ) == behavior::status_t::running );
        guy.set_guard_pos( guy.pos_abs() + tripoint( 10, 0, 0 ) );
        CHECK( npc_decision.tick( &oracle ) == "go_to_sleep" );
    }
    SECTION( "tired but not exhausted guard stays on duty" ) {
        // sleepiness 400 -> can_sleep passes (DEAD_TIRED=383), but
        // sleepiness_urgency(0.4) < duty_urgency(0.5) -> duty wins.
        // Default work_hours [0,24] means always on shift.
        guy.set_sleepiness( 400 );
        REQUIRE( oracle.needs_sleep_badly( "" ) == behavior::status_t::running );
        REQUIRE( oracle.can_sleep( "" ) == behavior::status_t::running );
        guy.set_guard_pos( guy.pos_abs() + tripoint( 10, 0, 0 ) );
        CHECK( npc_decision.tick( &oracle ) == "return_to_guard_pos" );
    }
    SECTION( "on-shift at post beats DEAD_TIRED sleep" ) {
        // At post during shift: duty 0.45 > sleepiness_urgency 0.383.
        // Default work_hours [0,24], so NPC is always on shift.
        guy.set_sleepiness( 383 );
        REQUIRE( oracle.needs_sleep_badly( "" ) == behavior::status_t::running );
        REQUIRE( oracle.can_sleep( "" ) == behavior::status_t::running );
        guy.set_guard_pos( guy.pos_abs() );
        CHECK( npc_decision.tick( &oracle ) == "hold_position" );
    }
    SECTION( "idle when nothing fires" ) {
        // Ensure no stale guard_pos from spawn (random traits may set it).
        guy.guard_pos = std::nullopt;
        guy.clear_ai_guard_pos();
        CHECK( npc_decision.tick( &oracle ) == "idle" );
    }
}

TEST_CASE( "npc_decision_category_mapping", "[npc][behavior]" )
{
    SECTION( "bt goals map to expected categories" ) {
        CHECK( bt_goal_to_category( "fight" ) == decision_category::combat );
        CHECK( bt_goal_to_category( "flee" ) == decision_category::combat );
        CHECK( bt_goal_to_category( "investigate_sound" ) == decision_category::investigate );
        CHECK( bt_goal_to_category( "drink_water" ) == decision_category::needs );
        CHECK( bt_goal_to_category( "eat_food" ) == decision_category::needs );
        CHECK( bt_goal_to_category( "go_to_sleep" ) == decision_category::needs );
        CHECK( bt_goal_to_category( "start_fire" ) == decision_category::needs );
        CHECK( bt_goal_to_category( "seek_warmth" ) == decision_category::needs );
        // Legacy warmth goals removed from BT; now unmodeled.
        CHECK( bt_goal_to_category( "wear_warmer_clothes" ) == decision_category::unmodeled );
        CHECK( bt_goal_to_category( "take_shelter" ) == decision_category::unmodeled );
        CHECK( bt_goal_to_category( "return_to_guard_pos" ) == decision_category::duty );
        CHECK( bt_goal_to_category( "idle" ) == decision_category::idle );
        CHECK( bt_goal_to_category( "something_unknown" ) == decision_category::unmodeled );
    }
    SECTION( "tri-state outcome classification" ) {
        using dc = decision_category;
        CHECK( std::string( classify_comparison( dc::combat, dc::combat ) ) == "converged" );
        CHECK( std::string( classify_comparison( dc::needs, dc::duty ) ) == "DIVERGED" );
        CHECK( std::string( classify_comparison( dc::needs, dc::unmodeled ) ) == "unmodeled" );
        CHECK( std::string( classify_comparison( dc::unmodeled, dc::combat ) ) == "unmodeled" );
        CHECK( std::string( classify_comparison( dc::unmodeled, dc::unmodeled ) ) == "unmodeled" );
        CHECK( std::string( classify_comparison( dc::idle, dc::idle ) ) == "converged" );
        CHECK( std::string( classify_comparison( dc::duty, dc::needs ) ) == "DIVERGED" );
    }
}

TEST_CASE( "npc_decision_bt_contract_guard_duty", "[npc][behavior]" )
{
    clear_map();
    behavior::tree npc_decision;
    npc_decision.add( &behavior_node_t_npc_decision.obj() );
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy );
    guy.set_fac( faction_your_followers );
    guy.set_mission( NPC_MISSION_GUARD );
    behavior::character_oracle_t oracle( &guy );

    SECTION( "displaced guard with mild hunger: BT chooses duty" ) {
        guy.set_guard_pos( guy.pos_abs() + tripoint( 10, 0, 0 ) );
        guy.set_hunger( 100 );
        CHECK( npc_decision.tick( &oracle ) == "return_to_guard_pos" );
    }
    SECTION( "displaced guard with extreme thirst and water: BT chooses need" ) {
        guy.set_guard_pos( guy.pos_abs() + tripoint( 10, 0, 0 ) );
        guy.set_thirst( 1000 );
        const item_group::ItemList water = item_group::items_from(
                                               Item_spawn_data_test_bottle_water );
        guy.i_add( water.front() );
        REQUIRE( oracle.needs_water_badly( "" ) == behavior::status_t::running );
        REQUIRE( oracle.has_water( "" ) == behavior::status_t::running );
        CHECK( npc_decision.tick( &oracle ) == "drink_water" );
    }
    SECTION( "displaced exhausted guard: BT chooses sleep over duty" ) {
        guy.set_guard_pos( guy.pos_abs() + tripoint( 10, 0, 0 ) );
        guy.set_sleepiness( 600 );
        REQUIRE( oracle.needs_sleep_badly( "" ) == behavior::status_t::running );
        REQUIRE( oracle.can_sleep( "" ) == behavior::status_t::running );
        CHECK( npc_decision.tick( &oracle ) == "go_to_sleep" );
    }
    SECTION( "guard at post on shift: BT returns hold_position" ) {
        guy.set_guard_pos( guy.pos_abs() );
        // Default work_hours [0,24] = always on shift. At post, on shift
        // means hold_position (duty 0.45 beats any sub-threshold need).
        CHECK( npc_decision.tick( &oracle ) == "hold_position" );
    }
}

TEST_CASE( "npc_decision_predicates_registered", "[npc][behavior]" )
{
    CHECK( behavior::predicate_map.count( "npc_in_danger" ) == 1 );
    CHECK( behavior::predicate_map.count( "npc_should_flee" ) == 1 );
    CHECK( behavior::predicate_map.count( "npc_has_target" ) == 1 );
    CHECK( behavior::predicate_map.count( "npc_has_sound_alerts" ) == 1 );
    CHECK( behavior::predicate_map.count( "npc_displaced_from_post" ) == 1 );
    CHECK( behavior::predicate_map.count( "npc_on_shift" ) == 1 );
    CHECK( behavior::predicate_map.count( "npc_is_following" ) == 1 );
    CHECK( behavior::predicate_map.count( "npc_has_goto_order" ) == 1 );
    CHECK( behavior::score_predicate_map.count( "npc_duty_urgency" ) == 1 );
    CHECK( behavior::score_predicate_map.count( "npc_following_urgency" ) == 1 );
    CHECK( behavior::score_predicate_map.count( "npc_goto_order_urgency" ) == 1 );
}

TEST_CASE( "duty_predicates_use_persistent_guard_pos_only", "[npc][behavior]" )
{
    clear_map_without_vision();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy );
    behavior::character_oracle_t oracle( &guy );

    SECTION( "displaced_from_post: failure with only temp anchor" ) {
        guy.guard_pos = std::nullopt;
        guy.set_ai_guard_pos( guy.pos_abs() + tripoint( 5, 0, 0 ) );
        REQUIRE( guy.get_effective_guard_pos().has_value() );
        REQUIRE_FALSE( guy.get_guard_post().has_value() );
        CHECK( oracle.displaced_from_post( "" ) == behavior::status_t::failure );
    }
    SECTION( "on_shift: failure with only temp anchor" ) {
        guy.guard_pos = std::nullopt;
        guy.set_ai_guard_pos( guy.pos_abs() );
        REQUIRE_FALSE( guy.get_guard_post().has_value() );
        CHECK( oracle.on_shift( "" ) == behavior::status_t::failure );
    }
    SECTION( "duty_urgency: zero with only temp anchor" ) {
        guy.guard_pos = std::nullopt;
        guy.set_ai_guard_pos( guy.pos_abs() );
        CHECK( oracle.duty_urgency( "" ) == Approx( 0.0f ) );
    }
    SECTION( "duty still works with persistent guard_pos" ) {
        guy.set_guard_pos( guy.pos_abs() );
        calendar::turn = calendar::turn_zero + 12_hours;
        CHECK( oracle.on_shift( "" ) == behavior::status_t::running );
        CHECK( oracle.duty_urgency( "" ) == Approx( 0.45f ) );
    }
}

TEST_CASE( "npc_is_following_predicate", "[npc][behavior]" )
{
    clear_map_without_vision();
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 50, 50, 0 ) );
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy );
    guy.setpos( here, tripoint_bub_ms( 50, 50, 0 ) );
    guy.set_fac( faction_your_followers );
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();
    behavior::character_oracle_t oracle( &guy );

    SECTION( "failure when not walking_with" ) {
        guy.set_attitude( NPCATT_NULL );
        CHECK( oracle.npc_is_following( "" ) == behavior::status_t::failure );
    }
    SECTION( "failure when leader: leaders are not pulled toward player" ) {
        guy.set_attitude( NPCATT_LEAD );
        CHECK( oracle.npc_is_following( "" ) == behavior::status_t::failure );
    }
    SECTION( "fires even with follow_close cleared: rule controls spacing only" ) {
        guy.set_attitude( NPCATT_FOLLOW );
        guy.rules.clear_flag( ally_rule::follow_close );
        get_player_character().setpos( here, tripoint_bub_ms( 60, 50, 0 ) );
        CHECK( oracle.npc_is_following( "" ) == behavior::status_t::running );
    }
    SECTION( "fires even with guard_pos: predicate reports state, not priority" ) {
        guy.set_attitude( NPCATT_FOLLOW );
        guy.set_guard_pos( guy.pos_abs() + tripoint( 10, 0, 0 ) );
        get_player_character().setpos( here, tripoint_bub_ms( 60, 50, 0 ) );
        CHECK( oracle.npc_is_following( "" ) == behavior::status_t::running );
    }
    SECTION( "failure when player in vehicle and NPC not" ) {
        guy.set_attitude( NPCATT_FOLLOW );
        guy.rules.set_flag( ally_rule::follow_close );
        get_player_character().in_vehicle = true;
        guy.in_vehicle = false;
        CHECK( oracle.npc_is_following( "" ) == behavior::status_t::failure );
        get_player_character().in_vehicle = false;
    }
    SECTION( "success when within follow_distance" ) {
        guy.set_attitude( NPCATT_FOLLOW );
        guy.rules.set_flag( ally_rule::follow_close );
        REQUIRE( rl_dist( guy.pos_abs(), get_player_character().pos_abs() ) == 0 );
        CHECK( oracle.npc_is_following( "" ) == behavior::status_t::success );
    }
    SECTION( "running when beyond follow_distance" ) {
        guy.set_attitude( NPCATT_FOLLOW );
        guy.rules.set_flag( ally_rule::follow_close );
        get_player_character().setpos( here, tripoint_bub_ms( 60, 50, 0 ) );
        CHECK( oracle.npc_is_following( "" ) == behavior::status_t::running );
    }
    SECTION( "running when different z-level" ) {
        guy.set_attitude( NPCATT_FOLLOW );
        guy.rules.set_flag( ally_rule::follow_close );
        get_player_character().setpos( here, tripoint_bub_ms( 50, 50, -1 ) );
        CHECK( oracle.npc_is_following( "" ) == behavior::status_t::running );
    }
    SECTION( "temp anchor alone does not block follow" ) {
        guy.set_attitude( NPCATT_FOLLOW );
        guy.rules.set_flag( ally_rule::follow_close );
        guy.guard_pos = std::nullopt;
        guy.set_ai_guard_pos( guy.pos_abs() + tripoint( 3, 0, 0 ) );
        REQUIRE( guy.get_effective_guard_pos().has_value() );
        REQUIRE_FALSE( guy.get_guard_post().has_value() );
        get_player_character().setpos( here, tripoint_bub_ms( 60, 50, 0 ) );
        CHECK( oracle.npc_is_following( "" ) == behavior::status_t::running );
    }
}

// Provisional urgency thresholds -- will change when the time_to_die
// evaluation contract lands (#28681). Tests pin current behavior.
TEST_CASE( "npc_following_urgency_policy", "[npc][behavior]" )
{
    clear_map_without_vision();
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 50, 50, 0 ) );
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy );
    guy.setpos( here, tripoint_bub_ms( 50, 50, 0 ) );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.rules.set_flag( ally_rule::follow_close );
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();
    behavior::character_oracle_t oracle( &guy );

    SECTION( "follow loses to severe thirst" ) {
        get_player_character().setpos( here, tripoint_bub_ms( 70, 50, 0 ) );
        float follow_score = oracle.npc_following_urgency( "" );
        guy.set_thirst( 900 );
        float thirst_score = oracle.thirst_urgency( "" );
        CHECK( thirst_score > follow_score );
    }
    SECTION( "follow beats mild hunger" ) {
        get_player_character().setpos( here, tripoint_bub_ms( 60, 50, 0 ) );
        float follow_score = oracle.npc_following_urgency( "" );
        guy.set_stored_kcal( static_cast<int>( guy.get_healthy_kcal() * 0.85 ) );
        float hunger_score = oracle.hunger_urgency( "" );
        CHECK( follow_score > hunger_score );
    }
    SECTION( "follow_close cleared: still fires, loose radius (6)" ) {
        // follow_close rule controls preferred spacing, not whether-to-follow.
        // Cleared rule = loose follow at radius 6.
        guy.rules.clear_flag( ally_rule::follow_close );
        get_player_character().setpos( here, tripoint_bub_ms( 55, 50, 0 ) );
        REQUIRE( rl_dist( guy.pos_abs(), get_player_character().pos_abs() ) == 5 );
        // Within loose radius -> 0 urgency (already in position).
        CHECK( oracle.npc_following_urgency( "" ) == Approx( 0.0f ) );
        get_player_character().setpos( here, tripoint_bub_ms( 70, 50, 0 ) );
        // Beyond loose radius -> non-zero urgency.
        CHECK( oracle.npc_following_urgency( "" ) > 0.0f );
    }
    SECTION( "follow capped below life-threatening needs" ) {
        get_player_character().setpos( here, tripoint_bub_ms( 100, 50, 0 ) );
        float follow_score = oracle.npc_following_urgency( "" );
        guy.set_stored_kcal( static_cast<int>( guy.get_healthy_kcal() * 0.2 ) );
        float hunger_score = oracle.hunger_urgency( "" );
        CHECK( hunger_score > follow_score );
    }
}

TEST_CASE( "npc_decision_follow_tree", "[npc][behavior]" )
{
    clear_map_without_vision();
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 50, 50, 0 ) );
    behavior::tree npc_decision;
    npc_decision.add( &behavior_node_t_npc_decision.obj() );
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy );
    guy.setpos( here, tripoint_bub_ms( 50, 50, 0 ) );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.rules.set_flag( ally_rule::follow_close );
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();
    behavior::character_oracle_t oracle( &guy );

    SECTION( "follower beyond distance: follow_player" ) {
        get_player_character().setpos( here, tripoint_bub_ms( 70, 50, 0 ) );
        CHECK( npc_decision.tick( &oracle ) == "follow_player" );
    }
    SECTION( "follower at player: idle" ) {
        REQUIRE( rl_dist( guy.pos_abs(), get_player_character().pos_abs() ) == 0 );
        CHECK( npc_decision.tick( &oracle ) == "idle" );
    }
    SECTION( "extreme thirst beats following" ) {
        get_player_character().setpos( here, tripoint_bub_ms( 70, 50, 0 ) );
        guy.set_thirst( 900 );
        guy.i_add( item( itype_orange ) );
        REQUIRE( oracle.has_water( "" ) == behavior::status_t::running );
        CHECK( npc_decision.tick( &oracle ) == "drink_water" );
    }
}

TEST_CASE( "bt_goal_category_mapping_follow", "[npc][behavior]" )
{
    CHECK( bt_goal_to_category( "follow_player" ) == decision_category::follow );
    CHECK( bt_goal_to_category( "follow_embarked" ) == decision_category::follow );
}

TEST_CASE( "npc_should_embark_predicate", "[npc][behavior]" )
{
    clear_map_without_vision();
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 50, 50, 0 ) );
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.rules.set_flag( ally_rule::follow_close );
    behavior::character_oracle_t oracle( &guy );

    SECTION( "fires: walking_with + player in vehicle + NPC not in vehicle" ) {
        get_player_character().in_vehicle = true;
        guy.in_vehicle = false;
        CHECK( oracle.npc_should_embark( "" ) == behavior::status_t::running );
        CHECK( oracle.npc_embark_urgency( "" ) > 0.0f );
        get_player_character().in_vehicle = false;
    }
    SECTION( "failure: not walking_with" ) {
        guy.set_attitude( NPCATT_NULL );
        get_player_character().in_vehicle = true;
        CHECK( oracle.npc_should_embark( "" ) == behavior::status_t::failure );
        CHECK( oracle.npc_embark_urgency( "" ) == Approx( 0.0f ) );
        get_player_character().in_vehicle = false;
    }
    SECTION( "failure: player not in vehicle" ) {
        get_player_character().in_vehicle = false;
        CHECK( oracle.npc_should_embark( "" ) == behavior::status_t::failure );
    }
    SECTION( "fires while NPC is in vehicle: action handler routes to seat" ) {
        get_player_character().in_vehicle = true;
        guy.in_vehicle = true;
        CHECK( oracle.npc_should_embark( "" ) == behavior::status_t::running );
        get_player_character().in_vehicle = false;
        guy.in_vehicle = false;
    }
    SECTION( "follow predicate suppressed under embark conditions: predicates mutex" ) {
        get_player_character().in_vehicle = true;
        guy.in_vehicle = false;
        CHECK( oracle.npc_should_embark( "" ) == behavior::status_t::running );
        CHECK( oracle.npc_is_following( "" ) == behavior::status_t::failure );
        get_player_character().in_vehicle = false;
    }
}

TEST_CASE( "follow_suppresses_duty_when_recruited", "[npc][behavior]" )
{
    clear_map_without_vision();
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 50, 50, 0 ) );
    behavior::tree npc_decision;
    npc_decision.add( &behavior_node_t_npc_decision.obj() );
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy );
    guy.set_fac( faction_your_followers );
    behavior::character_oracle_t oracle( &guy );
    calendar::turn = calendar::turn_zero + 12_hours;

    SECTION( "walking_with + guard_pos: follow fires, duty suppressed" ) {
        guy.set_attitude( NPCATT_FOLLOW );
        guy.rules.set_flag( ally_rule::follow_close );
        const tripoint_abs_ms post = guy.pos_abs() + tripoint( 5, 0, 0 );
        guy.set_guard_pos( post );
        REQUIRE( guy.myclass.is_valid() );
        REQUIRE( guy.get_guard_post().has_value() );
        const auto &[wh_start, wh_end] = guy.myclass.obj().get_work_hours();
        REQUIRE( ( wh_start == 0 && wh_end == 24 ) );
        get_player_character().setpos( here, tripoint_bub_ms( 60, 50, 0 ) );
        CHECK( oracle.npc_is_following( "" ) == behavior::status_t::running );
        CHECK( oracle.on_shift( "" ) == behavior::status_t::failure );
        CHECK( oracle.displaced_from_post( "" ) == behavior::status_t::failure );
        CHECK( oracle.duty_urgency( "" ) == Approx( 0.0f ) );
        CHECK( npc_decision.tick( &oracle ) == "follow_player" );
    }
    SECTION( "walking_with + guard_pos at-post: still follows player away" ) {
        guy.set_attitude( NPCATT_FOLLOW );
        guy.rules.set_flag( ally_rule::follow_close );
        guy.set_guard_pos( guy.pos_abs() );
        REQUIRE( guy.pos_abs() == *guy.get_guard_post() );
        get_player_character().setpos( here, tripoint_bub_ms( 70, 50, 0 ) );
        CHECK( oracle.on_shift( "" ) == behavior::status_t::failure );
        CHECK( npc_decision.tick( &oracle ) == "follow_player" );
    }
    SECTION( "non-walking_with + guard_pos displaced: duty fires" ) {
        guy.set_attitude( NPCATT_NULL );
        const tripoint_abs_ms post = guy.pos_abs() + tripoint( 5, 0, 0 );
        guy.set_guard_pos( post );
        REQUIRE( guy.get_guard_post().has_value() );
        REQUIRE_FALSE( guy.is_walking_with() );
        CHECK( oracle.on_shift( "" ) == behavior::status_t::running );
        CHECK( oracle.displaced_from_post( "" ) == behavior::status_t::running );
        CHECK( oracle.duty_urgency( "" ) > 0.0f );
        CHECK( npc_decision.tick( &oracle ) == "return_to_guard_pos" );
    }
    SECTION( "no guard_pos: follow fires, duty inert" ) {
        guy.set_attitude( NPCATT_FOLLOW );
        guy.rules.set_flag( ally_rule::follow_close );
        guy.guard_pos = std::nullopt;
        guy.clear_ai_guard_pos();
        REQUIRE_FALSE( guy.get_guard_post().has_value() );
        get_player_character().setpos( here, tripoint_bub_ms( 60, 50, 0 ) );
        CHECK( oracle.npc_is_following( "" ) == behavior::status_t::running );
        CHECK( oracle.displaced_from_post( "" ) == behavior::status_t::failure );
        CHECK( oracle.duty_urgency( "" ) == Approx( 0.0f ) );
    }
    SECTION( "WAIT + guard_pos: duty suppressed" ) {
        guy.set_attitude( NPCATT_WAIT );
        const tripoint_abs_ms post = guy.pos_abs() + tripoint( 5, 0, 0 );
        guy.set_guard_pos( post );
        REQUIRE( guy.is_walking_with() );
        CHECK( oracle.on_shift( "" ) == behavior::status_t::failure );
        CHECK( oracle.displaced_from_post( "" ) == behavior::status_t::failure );
        CHECK( oracle.duty_urgency( "" ) == Approx( 0.0f ) );
    }
    SECTION( "LEAD + guard_pos: duty suppressed" ) {
        guy.set_attitude( NPCATT_LEAD );
        const tripoint_abs_ms post = guy.pos_abs() + tripoint( 5, 0, 0 );
        guy.set_guard_pos( post );
        REQUIRE( guy.is_walking_with() );
        CHECK( oracle.on_shift( "" ) == behavior::status_t::failure );
        CHECK( oracle.displaced_from_post( "" ) == behavior::status_t::failure );
        CHECK( oracle.duty_urgency( "" ) == Approx( 0.0f ) );
    }
}

static std::string bt_goal( const npc &guy )
{
    behavior::character_oracle_t oracle( &guy );
    behavior::tree dt;
    dt.add( &behavior_node_t_npc_decision.obj() );
    return dt.tick( &oracle );
}

TEST_CASE( "npc_player_order_predicate_and_goal", "[npc][behavior]" )
{
    clear_map_without_vision();
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 50, 50, 0 ) );
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy );
    guy.setpos( here, tripoint_bub_ms( 50, 50, 0 ) );
    guy.set_fac( faction_your_followers );
    guy.set_attitude( NPCATT_FOLLOW );
    guy.rules.set_flag( ally_rule::follow_close );
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();
    behavior::character_oracle_t oracle( &guy );

    SECTION( "no order: failure" ) {
        guy.goto_to_this_pos = std::nullopt;
        CHECK( oracle.npc_has_goto_order( "" ) == behavior::status_t::failure );
    }
    SECTION( "pending order: running" ) {
        guy.goto_to_this_pos = guy.pos_abs() + tripoint( 10, 0, 0 );
        CHECK( oracle.npc_has_goto_order( "" ) == behavior::status_t::running );
    }
    SECTION( "at target: success" ) {
        guy.goto_to_this_pos = guy.pos_abs();
        CHECK( oracle.npc_has_goto_order( "" ) == behavior::status_t::success );
    }
    SECTION( "order beats follow in BT" ) {
        get_player_character().setpos( here, tripoint_bub_ms( 70, 50, 0 ) );
        guy.goto_to_this_pos = guy.pos_abs() + tripoint( 10, 0, 0 );
        CHECK( bt_goal( guy ) == "goto_ordered_position" );
    }
    SECTION( "severe thirst beats order" ) {
        guy.goto_to_this_pos = guy.pos_abs() + tripoint( 10, 0, 0 );
        guy.set_thirst( 900 );
        guy.i_add( item( itype_orange ) );
        REQUIRE( oracle.has_water( "" ) == behavior::status_t::running );
        CHECK( bt_goal( guy ) == "drink_water" );
    }
}

TEST_CASE( "bt_goal_category_mapping_order", "[npc][behavior]" )
{
    CHECK( bt_goal_to_category( "goto_ordered_position" ) == decision_category::order );
}

TEST_CASE( "bt_priority_matrix", "[npc][behavior]" )
{
    clear_map_without_vision();
    map &here = get_map();
    get_player_character().setpos( here, tripoint_bub_ms( 50, 50, 0 ) );
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy );
    guy.setpos( here, tripoint_bub_ms( 50, 50, 0 ) );
    guy.set_fac( faction_your_followers );
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();
    calendar::turn = calendar::turn_zero + 12_hours;

    SECTION( "healthy follower at player: idle" ) {
        guy.set_attitude( NPCATT_FOLLOW );
        guy.rules.set_flag( ally_rule::follow_close );
        CHECK( bt_goal( guy ) == "idle" );
    }
    SECTION( "follower beyond distance: follow_player" ) {
        guy.set_attitude( NPCATT_FOLLOW );
        guy.rules.set_flag( ally_rule::follow_close );
        get_player_character().setpos( here, tripoint_bub_ms( 70, 50, 0 ) );
        CHECK( bt_goal( guy ) == "follow_player" );
    }
    SECTION( "leader: not follow_player" ) {
        guy.set_attitude( NPCATT_LEAD );
        guy.rules.set_flag( ally_rule::follow_close );
        get_player_character().setpos( here, tripoint_bub_ms( 70, 50, 0 ) );
        CHECK( bt_goal( guy ) != "follow_player" );
    }
    SECTION( "waiting NPC with follow_close: follows" ) {
        // WAIT is in is_following(), same as old cascade.
        guy.set_attitude( NPCATT_WAIT );
        guy.rules.set_flag( ally_rule::follow_close );
        get_player_character().setpos( here, tripoint_bub_ms( 70, 50, 0 ) );
        CHECK( bt_goal( guy ) == "follow_player" );
    }
    SECTION( "guard_pos + walking_with: follow beats duty" ) {
        guy.set_attitude( NPCATT_FOLLOW );
        guy.rules.set_flag( ally_rule::follow_close );
        const tripoint_abs_ms post = guy.pos_abs() + tripoint( 5, 0, 0 );
        guy.set_guard_pos( post );
        get_player_character().setpos( here, tripoint_bub_ms( 70, 50, 0 ) );
        CHECK( bt_goal( guy ) == "follow_player" );
    }
    SECTION( "guard_pos + not walking_with: duty fires" ) {
        guy.set_attitude( NPCATT_NULL );
        const tripoint_abs_ms post = guy.pos_abs() + tripoint( 5, 0, 0 );
        guy.set_guard_pos( post );
        get_player_character().setpos( here, tripoint_bub_ms( 70, 50, 0 ) );
        CHECK( bt_goal( guy ) == "return_to_guard_pos" );
    }
    SECTION( "severe thirst beats follow" ) {
        guy.set_attitude( NPCATT_FOLLOW );
        guy.rules.set_flag( ally_rule::follow_close );
        get_player_character().setpos( here, tripoint_bub_ms( 70, 50, 0 ) );
        guy.set_thirst( 900 );
        guy.i_add( item( itype_orange ) );
        behavior::character_oracle_t oracle( &guy );
        REQUIRE( oracle.needs_water_badly( "" ) == behavior::status_t::running );
        REQUIRE( oracle.has_water( "" ) == behavior::status_t::running );
        REQUIRE( oracle.thirst_urgency( "" ) > oracle.npc_following_urgency( "" ) );
        CHECK( bt_goal( guy ) == "drink_water" );
    }
    SECTION( "mild hunger loses to follow" ) {
        guy.set_attitude( NPCATT_FOLLOW );
        guy.rules.set_flag( ally_rule::follow_close );
        get_player_character().setpos( here, tripoint_bub_ms( 60, 50, 0 ) );
        guy.set_stored_kcal( static_cast<int>( guy.get_healthy_kcal() * 0.85 ) );
        CHECK( bt_goal( guy ) == "follow_player" );
    }
    SECTION( "goto order beats follow" ) {
        guy.set_attitude( NPCATT_FOLLOW );
        guy.rules.set_flag( ally_rule::follow_close );
        get_player_character().setpos( here, tripoint_bub_ms( 70, 50, 0 ) );
        guy.goto_to_this_pos = guy.pos_abs() + tripoint( 10, 0, 0 );
        CHECK( bt_goal( guy ) == "goto_ordered_position" );
    }
    SECTION( "follow_close cleared: still follows at loose radius" ) {
        guy.set_attitude( NPCATT_FOLLOW );
        guy.rules.clear_flag( ally_rule::follow_close );
        get_player_character().setpos( here, tripoint_bub_ms( 70, 50, 0 ) );
        CHECK( bt_goal( guy ) == "follow_player" );
    }
    SECTION( "follow_close cleared and within loose radius: idle" ) {
        guy.set_attitude( NPCATT_FOLLOW );
        guy.rules.clear_flag( ally_rule::follow_close );
        get_player_character().setpos( here, tripoint_bub_ms( 55, 50, 0 ) );
        REQUIRE( rl_dist( guy.pos_abs(), get_player_character().pos_abs() ) <= 6 );
        CHECK( bt_goal( guy ) == "idle" );
    }
    SECTION( "player in vehicle, NPC not: BT picks follow_embarked" ) {
        guy.set_attitude( NPCATT_FOLLOW );
        guy.rules.set_flag( ally_rule::follow_close );
        get_player_character().setpos( here, tripoint_bub_ms( 70, 50, 0 ) );
        get_player_character().in_vehicle = true;
        guy.in_vehicle = false;
        CHECK( bt_goal( guy ) == "follow_embarked" );
        get_player_character().in_vehicle = false;
    }
    SECTION( "both in vehicle: BT keeps follow_embarked so handler can re-seat" ) {
        guy.set_attitude( NPCATT_FOLLOW );
        guy.rules.set_flag( ally_rule::follow_close );
        get_player_character().in_vehicle = true;
        guy.in_vehicle = true;
        CHECK( bt_goal( guy ) == "follow_embarked" );
        get_player_character().in_vehicle = false;
        guy.in_vehicle = false;
    }
    SECTION( "camp resident idle: free_time" ) {
        guy.set_mission( NPC_MISSION_CAMP_RESIDENT );
        const tripoint_abs_omt comt = project_to<coords::omt>( guy.pos_abs() );
        here.add_camp( comt, "faction_camp" );
        std::optional<basecamp *> cbcp = overmap_buffer.find_camp( comt.xy() );
        REQUIRE( cbcp );
        ( *cbcp )->define_camp( comt, "faction_base_bare_bones_NPC_camp_0", false );
        guy.assigned_camp = comt;
        guy.guard_pos = std::nullopt;
        guy.clear_ai_guard_pos();
        CHECK( bt_goal( guy ) == "free_time" );
    }
}

// --- Camp resident state split tests ---

TEST_CASE( "duty_predicates_skip_camp_residents", "[npc][behavior]" )
{
    clear_map_without_vision();
    clear_avatar();
    get_map();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_mission( NPC_MISSION_CAMP_RESIDENT );
    guy.assigned_camp = project_to<coords::omt>( guy.pos_abs() );
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();
    calendar::turn = calendar::turn_zero + 12_hours;

    behavior::character_oracle_t oracle( &guy );

    SECTION( "RETURN_TO_START_POS suppressed" ) {
        guy.set_mutation( trait_RETURN_TO_START_POS );
        guy.regen_ai_cache();
        CHECK_FALSE( guy.get_guard_post().has_value() );
    }
    SECTION( "on_shift fails" ) {
        CHECK( oracle.on_shift( "" ) == behavior::status_t::failure );
    }
    SECTION( "duty_urgency is 0" ) {
        CHECK( oracle.duty_urgency( "" ) == 0.0f );
    }
    SECTION( "displaced_from_post fails" ) {
        CHECK( oracle.displaced_from_post( "" ) == behavior::status_t::failure );
    }
}

TEST_CASE( "bt_camp_resident_goals", "[npc][behavior]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();
    npc &guy = spawn_npc( { 50, 50 }, "test_talker" );
    clear_character( guy, true );
    guy.set_fac( faction_your_followers );
    guy.set_mission( NPC_MISSION_CAMP_RESIDENT );

    // Create a real camp so the BT predicates can resolve it.
    const tripoint_abs_omt camp_omt = project_to<coords::omt>( guy.pos_abs() );
    here.add_camp( camp_omt, "faction_camp" );
    std::optional<basecamp *> bcp = overmap_buffer.find_camp( camp_omt.xy() );
    REQUIRE( bcp );
    ( *bcp )->define_camp( camp_omt, "faction_base_bare_bones_NPC_camp_0", false );

    guy.assigned_camp = camp_omt;
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();

    SECTION( "idle at camp: free_time" ) {
        CHECK( bt_goal( guy ) == "free_time" );
    }
    SECTION( "on activity: not free_time" ) {
        guy.set_attitude( NPCATT_ACTIVITY );
        CHECK( bt_goal( guy ) != "free_time" );
    }
    SECTION( "away from camp: return_to_camp" ) {
        guy.setpos( here, tripoint_bub_ms( 50 + SEEX * 2, 50, 0 ) );
        REQUIRE( guy.pos_abs_omt() != *guy.assigned_camp );
        CHECK( bt_goal( guy ) == "return_to_camp" );
    }
    SECTION( "severe thirst beats free_time" ) {
        guy.set_thirst( 900 );
        guy.i_add( item( itype_orange ) );
        behavior::character_oracle_t oracle( &guy );
        REQUIRE( oracle.has_water( "" ) == behavior::status_t::running );
        CHECK( bt_goal( guy ) == "drink_water" );
    }
}

TEST_CASE( "bt_camp_resident_expansion_tile", "[npc][behavior]" )
{
    clear_map_without_vision();
    clear_avatar();
    map &here = get_map();

    // Place camp at mid-map so we control OMT alignment.
    const tripoint_bub_ms mid{ MAPSIZE_X / 2, MAPSIZE_Y / 2, 0 };
    const tripoint_abs_omt camp_omt = project_to<coords::omt>( here.get_abs( mid ) );

    here.add_camp( camp_omt, "faction_camp" );
    std::optional<basecamp *> bcp = overmap_buffer.find_camp( camp_omt.xy() );
    REQUIRE( bcp );
    basecamp *test_camp = *bcp;
    test_camp->define_camp( camp_omt, "faction_base_bare_bones_NPC_camp_0", false );

    // Add an expansion one OMT to the east.
    const tripoint_abs_omt expansion_omt = camp_omt + tripoint::east;
    test_camp->add_expansion( "faction_base_farm_0", expansion_omt,
                              point_rel_omt{ 1, 0 } );
    REQUIRE( test_camp->point_within_camp( expansion_omt ) );

    // Spawn NPC anywhere, then teleport to the expansion tile.
    npc &guy = spawn_npc( mid.xy(), "test_talker" );
    clear_character( guy, true );
    const tripoint_abs_ms expansion_center =
        project_to<coords::ms>( expansion_omt ) + tripoint{ SEEX, SEEY, 0 };
    const tripoint_bub_ms expansion_local = here.get_bub( expansion_center );
    REQUIRE( here.inbounds( expansion_local ) );
    guy.setpos( here, expansion_local );
    guy.set_fac( faction_your_followers );
    guy.set_mission( NPC_MISSION_CAMP_RESIDENT );
    guy.assigned_camp = camp_omt;
    guy.guard_pos = std::nullopt;
    guy.clear_ai_guard_pos();

    REQUIRE( guy.pos_abs_omt() == expansion_omt );
    REQUIRE( guy.pos_abs_omt() != camp_omt );

    SECTION( "on expansion tile: free_time, not return_to_camp" ) {
        CHECK( bt_goal( guy ) == "free_time" );
    }
    SECTION( "on expansion tile: camp predicates treat as at-camp" ) {
        behavior::character_oracle_t oracle( &guy );
        CHECK( oracle.is_camp_idle( "" ) == behavior::status_t::running );
        CHECK( oracle.is_away_from_camp( "" ) == behavior::status_t::failure );
    }
    SECTION( "on expansion tile with job: camp_work fires" ) {
        guy.job.set_all_priorities( 1 );
        REQUIRE( guy.has_job() );
        guy.last_job_scan = calendar::turn - 11_minutes;
        behavior::character_oracle_t oracle( &guy );
        CHECK( oracle.has_camp_job( "" ) == behavior::status_t::running );
        CHECK( oracle.camp_work_urgency( "" ) > 0.0f );
    }
}

TEST_CASE( "bt_goal_category_mapping_camp", "[npc][behavior]" )
{
    CHECK( bt_goal_to_category( "camp_work" ) == decision_category::camp_work );
    CHECK( bt_goal_to_category( "free_time" ) == decision_category::free_time );
    CHECK( bt_goal_to_category( "return_to_camp" ) == decision_category::camp_travel );
}
