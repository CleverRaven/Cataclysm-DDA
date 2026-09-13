#include <algorithm>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "activity_actor_definitions.h"
#include "activity_item_handling.h"
#include "activity_handlers.h"
#include "avatar.h"
#include "bionics.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_utility.h"
#include "character.h"
#include "character_attire.h"
#include "character_id.h"
#include "clzones.h"
#include "construction.h"
#include "coordinates.h"
#include "craft_command.h"
#include "craft_reservation.h"
#include "crafting.h"
#include "crafting_enums.h"
#include "enums.h"
#include "faction.h"
#include "flag.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "game_constants.h"
#include "item.h"
#include "item_components.h"
#include "item_location.h"
#include "inventory.h"
#include "item_uid.h"
#include "item_wakeup.h"
#include "json.h"
#include "json_loader.h"
#include "map.h"
#include "map_helpers.h"
#include "map_helpers_tests.h"
#include "map_iterator.h"
#include "map_selector.h"
#include "mapdata.h"
#include "memory_fast.h"
#include "messages.h"
#include "npc.h"
#include "pimpl.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "projectile.h"
#include "recipe.h"
#include "requirements.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "vehicle_selector.h"
#include "visitable.h"
#include "vpart_position.h"

static const activity_id ACT_MULTIPLE_CONSTRUCTION( "ACT_MULTIPLE_CONSTRUCTION" );

static const bionic_id test_bio_reserve_toggled_pseudo( "test_bio_reserve_toggled_pseudo" );
static const bionic_id test_bio_reserve_two_pseudo( "test_bio_reserve_two_pseudo" );
static const bionic_id test_bio_reserve_weapon( "test_bio_reserve_weapon" );

static const construction_str_id
construction_test_constr_pit_shallow( "test_constr_pit_shallow" );

static const field_type_str_id field_fd_fire( "fd_fire" );

static const furn_str_id furn_f_plant_harvest( "f_plant_harvest" );
static const furn_str_id furn_f_plant_seed( "f_plant_seed" );
static const furn_str_id furn_f_standing_tank( "f_standing_tank" );
static const furn_str_id furn_test_f_reserve_charged( "test_f_reserve_charged" );
static const furn_str_id furn_test_f_reserve_qual( "test_f_reserve_qual" );

static const itype_id itype_2x4( "2x4" );
static const itype_id itype_M24( "M24" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_bat( "bat" );
static const itype_id itype_battery( "battery" );
static const itype_id itype_canteen( "canteen" );
static const itype_id itype_cudgel( "cudgel" );
static const itype_id itype_debug_backpack( "debug_backpack" );
static const itype_id itype_fertilizer( "fertilizer" );
static const itype_id itype_fire( "fire" );
static const itype_id itype_hammer( "hammer" );
static const itype_id itype_leather_belt( "leather_belt" );
static const itype_id itype_microwave( "microwave" );
static const itype_id itype_mop( "mop" );
static const itype_id itype_pot( "pot" );
static const itype_id itype_seed_hops( "seed_hops" );
static const itype_id itype_sickle( "sickle" );
static const itype_id itype_soldering_iron_portable( "soldering_iron_portable" );
static const itype_id itype_test_reserve_bionic_powered_tool(
    "test_reserve_bionic_powered_tool" );
static const itype_id itype_test_reserve_bionic_rod( "test_reserve_bionic_rod" );
static const itype_id itype_test_reserve_bionic_tool_1( "test_reserve_bionic_tool_1" );
static const itype_id itype_test_reserve_bolted_powered_tool(
    "test_reserve_bolted_powered_tool" );
static const itype_id itype_test_reserve_cabled_tool( "test_reserve_cabled_tool" );
static const itype_id itype_test_reserve_charge_stack( "test_reserve_charge_stack" );
static const itype_id itype_test_reserve_charged_tool( "test_reserve_charged_tool" );
static const itype_id itype_test_reserve_liquid( "test_reserve_liquid" );
static const itype_id itype_test_reserve_multitool_ab( "test_reserve_multitool_ab" );
static const itype_id itype_test_reserve_multitool_ab_charged(
    "test_reserve_multitool_ab_charged" );
static const itype_id itype_test_reserve_pseudo_kiln_b( "test_reserve_pseudo_kiln_b" );
static const itype_id itype_test_reserve_pseudo_kiln_charged(
    "test_reserve_pseudo_kiln_charged" );
static const itype_id itype_test_reserve_tool_a( "test_reserve_tool_a" );
static const itype_id itype_test_reserve_tool_a_good( "test_reserve_tool_a_good" );
static const itype_id itype_test_reserve_tool_b( "test_reserve_tool_b" );
static const itype_id itype_test_vitfood( "test_vitfood" );
static const itype_id itype_water( "water" );
static const itype_id itype_water_clean( "water_clean" );
static const itype_id itype_welder( "welder" );

static const quality_id qual_BOIL( "BOIL" );
static const quality_id qual_DIG( "DIG" );
static const quality_id qual_TEST_RESERVE_A( "TEST_RESERVE_A" );
static const quality_id qual_TEST_RESERVE_B( "TEST_RESERVE_B" );

static const recipe_id recipe_cudgel_test_charged_fast_stepless(
    "cudgel_test_charged_fast_stepless" );
static const recipe_id recipe_cudgel_test_consecutive_unattended(
    "cudgel_test_consecutive_unattended" );
static const recipe_id recipe_cudgel_test_first_step_unattended(
    "cudgel_test_first_step_unattended" );
static const recipe_id recipe_cudgel_test_only_unattended(
    "cudgel_test_only_unattended" );
static const recipe_id recipe_cudgel_test_root_unattended(
    "cudgel_test_root_unattended" );
static const recipe_id recipe_cudgel_test_steps_basic(
    "cudgel_test_steps_basic" );
static const recipe_id recipe_cudgel_test_steps_charged(
    "cudgel_test_steps_charged" );
static const recipe_id recipe_cudgel_test_steps_two_tools(
    "cudgel_test_steps_two_tools" );
static const recipe_id recipe_cudgel_test_timeout_recipe(
    "cudgel_test_timeout_recipe" );
static const recipe_id recipe_cudgel_test_unattended_charged(
    "cudgel_test_unattended_charged" );
static const recipe_id recipe_cudgel_test_unattended_charged_big(
    "cudgel_test_unattended_charged_big" );
static const recipe_id recipe_cudgel_test_unattended_fire_tool(
    "cudgel_test_unattended_fire_tool" );
static const recipe_id recipe_cudgel_test_unattended_furn_qual(
    "cudgel_test_unattended_furn_qual" );
static const recipe_id recipe_cudgel_test_unattended_intrinsic_tool(
    "cudgel_test_unattended_intrinsic_tool" );
static const recipe_id recipe_cudgel_test_unattended_liquid_reuse(
    "cudgel_test_unattended_liquid_reuse" );
static const recipe_id recipe_cudgel_test_unattended_powered_qual(
    "cudgel_test_unattended_powered_qual" );
static const recipe_id recipe_cudgel_test_unattended_presence_or(
    "cudgel_test_unattended_presence_or" );
static const recipe_id recipe_cudgel_test_unattended_presence_tool(
    "cudgel_test_unattended_presence_tool" );
static const recipe_id recipe_cudgel_test_unattended_qual_and_charges(
    "cudgel_test_unattended_qual_and_charges" );
static const recipe_id recipe_cudgel_test_unattended_qual_or_counts(
    "cudgel_test_unattended_qual_or_counts" );
static const recipe_id recipe_cudgel_test_unattended_qual_or_wide_first(
    "cudgel_test_unattended_qual_or_wide_first" );
static const recipe_id recipe_cudgel_test_unattended_root_presence(
    "cudgel_test_unattended_root_presence" );
static const recipe_id recipe_cudgel_test_unattended_simple(
    "cudgel_test_unattended_simple" );
static const recipe_id recipe_cudgel_test_unattended_three_of_a(
    "cudgel_test_unattended_three_of_a" );
static const recipe_id recipe_cudgel_test_unattended_three_of_a_four_of_b(
    "cudgel_test_unattended_three_of_a_four_of_b" );
static const recipe_id recipe_cudgel_test_unattended_tree_qual(
    "cudgel_test_unattended_tree_qual" );
static const recipe_id recipe_cudgel_test_unattended_two_groups_of_a(
    "cudgel_test_unattended_two_groups_of_a" );
static const recipe_id recipe_cudgel_test_unattended_two_of_a(
    "cudgel_test_unattended_two_of_a" );
static const recipe_id recipe_cudgel_test_unattended_two_qualities(
    "cudgel_test_unattended_two_qualities" );
static const recipe_id recipe_cudgel_test_unattended_with_qual(
    "cudgel_test_unattended_with_qual" );
static const recipe_id recipe_water_clean_test_unattended_boil(
    "water_clean_test_unattended_boil" );
static const recipe_id recipe_water_clean_test_unattended_liquid(
    "water_clean_test_unattended_liquid" );

static const requirement_id requirement_data_test_reserve_vehicle_weld(
    "test_reserve_vehicle_weld" );

static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_door_c( "t_door_c" );
static const ter_str_id ter_t_door_locked( "t_door_locked" );
static const ter_str_id ter_t_greenhouse_tilled( "t_greenhouse_tilled" );
static const ter_str_id ter_t_tree( "t_tree" );
static const ter_str_id ter_t_wall( "t_wall" );
static const ter_str_id ter_t_water_sh( "t_water_sh" );

static const trait_id trait_BURROW( "BURROW" );
static const trait_id trait_BURROWLARGE( "BURROWLARGE" );
static const trait_id trait_TEST_RESERVE_QUALITIES( "TEST_RESERVE_QUALITIES" );
static const trait_id trait_TEST_RESERVE_QUALITIES_2( "TEST_RESERVE_QUALITIES_2" );

static const vpart_id vpart_frame( "frame" );
static const vpart_id vpart_small_storage_battery( "small_storage_battery" );
static const vpart_id vpart_tank( "tank" );
static const vpart_id vpart_test_vp_reserve_qual( "test_vp_reserve_qual" );
static const vpart_id vpart_test_vp_reserve_welder( "test_vp_reserve_welder" );
static const vpart_id vpart_test_vp_reserve_welder_b( "test_vp_reserve_welder_b" );
static const vpart_id vpart_water_faucet( "water_faucet" );

static const vproto_id vehicle_prototype_none( "none" );
static const vproto_id vehicle_prototype_test_shopping_cart( "test_shopping_cart" );

static const zone_type_id zone_type_FARM_PLOT( "FARM_PLOT" );

static step_tool_alloc fire_presence_alloc()
{
    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::map;
    alloc.sel.comp.type = itype_fire;
    alloc.sel.comp.count = -1;
    return alloc;
}

static craft_reservation_index::record make_item_record( const int64_t owner_token,
        const int64_t item_uid, const time_point expires_at )
{
    craft_reservation_index::record rec;
    rec.craft_uid = owner_token;
    rec.provider_item_uids.push_back( item_uid );
    rec.expires_at = expires_at;
    return rec;
}

TEST_CASE( "attention_recipe_loads_attention_field", "[craft][attention][schema]" )
{
    const recipe &r = recipe_cudgel_test_unattended_simple.obj();
    REQUIRE( r.has_steps() );
    const std::vector<recipe_step> &steps = r.steps();
    REQUIRE( steps.size() == 2 );
    CHECK( steps[0].attention == step_attention::none );
    CHECK( steps[1].attention == step_attention::unattended );
}

TEST_CASE( "attention_recipe_loads_max_time_and_grace_period",
           "[craft][attention][schema]" )
{
    const recipe &r = recipe_cudgel_test_timeout_recipe.obj();
    const recipe_step &cure = r.steps().back();
    REQUIRE( cure.attention == step_attention::unattended );
    REQUIRE( cure.max_time.has_value() );
    CHECK( *cure.max_time == 20_minutes );
    REQUIRE( cure.grace_period.has_value() );
    CHECK( *cure.grace_period == 5_minutes );
}

TEST_CASE( "has_attention_steps_detects_unattended_step",
           "[craft][attention][schema]" )
{
    CHECK( recipe_cudgel_test_unattended_simple.obj().has_attention_steps() );
    CHECK( recipe_cudgel_test_first_step_unattended.obj().has_attention_steps() );
    CHECK( recipe_cudgel_test_only_unattended.obj().has_attention_steps() );
    CHECK_FALSE( recipe_cudgel_test_steps_basic.obj().has_attention_steps() );
}

TEST_CASE( "has_remaining_attention_steps_excludes_completed_steps",
           "[craft][attention][schema]" )
{
    const recipe &r = recipe_cudgel_test_unattended_simple.obj();
    REQUIRE( r.steps().size() == 2 );
    CHECK( r.has_remaining_attention_steps( 0 ) );
    CHECK( r.has_remaining_attention_steps( 1 ) );
    CHECK_FALSE( r.has_remaining_attention_steps( 2 ) );
}

TEST_CASE( "has_remaining_attention_steps_negative_clamps_to_zero",
           "[craft][attention][schema]" )
{
    const recipe &r = recipe_cudgel_test_first_step_unattended.obj();
    CHECK( r.has_remaining_attention_steps( -5 ) );
}

TEST_CASE( "has_remaining_attention_steps_consecutive_unattended",
           "[craft][attention][schema]" )
{
    const recipe &r = recipe_cudgel_test_consecutive_unattended.obj();
    REQUIRE( r.steps().size() == 4 );
    CHECK( r.has_remaining_attention_steps( 0 ) );
    CHECK( r.has_remaining_attention_steps( 1 ) );
    CHECK( r.has_remaining_attention_steps( 2 ) );
    CHECK_FALSE( r.has_remaining_attention_steps( 3 ) );
}

TEST_CASE( "attention_recipe_with_quality_loads_quality_requirement",
           "[craft][attention][schema]" )
{
    const recipe &r = recipe_cudgel_test_unattended_with_qual.obj();
    const recipe_step &bake = r.steps().back();
    REQUIRE( bake.attention == step_attention::unattended );
    bool has_oven = false;
    for( const std::vector<quality_requirement> &group : bake.requirements.get_qualities() ) {
        for( const quality_requirement &q : group ) {
            if( q.type.str() == "OVEN" && q.level >= 1 ) {
                has_oven = true;
            }
        }
    }
    CHECK( has_oven );
}

TEST_CASE( "craft_data_persists_passive_counter_bounds",
           "[craft][attention][persist]" )
{
    item craft( recipe_cudgel_test_unattended_simple.obj().result(), calendar::turn );
    item ingredient( itype_2x4, calendar::turn );
    item built( &recipe_cudgel_test_unattended_simple.obj(), 1, ingredient );

    REQUIRE( built.is_craft() );
    built.set_passive_started_at( calendar::turn );
    built.set_ready_at( calendar::turn + 10_minutes );
    built.set_passive_start_counter( 3000000 );
    built.set_passive_end_counter( 7000000 );

    std::ostringstream ss;
    JsonOut jsout( ss );
    built.serialize( jsout );

    item restored;
    restored.deserialize( json_loader::from_string( ss.str() ).get_object() );

    REQUIRE( restored.is_craft() );
    CHECK( restored.get_passive_started_at() == calendar::turn );
    CHECK( restored.get_ready_at() == calendar::turn + 10_minutes );
    CHECK( restored.get_passive_start_counter() == 3000000 );
    CHECK( restored.get_passive_end_counter() == 7000000 );
}

TEST_CASE( "craft_data_default_passive_counters_are_zero",
           "[craft][attention][persist]" )
{
    item ingredient( itype_2x4, calendar::turn );
    item built( &recipe_cudgel_test_unattended_simple.obj(), 1, ingredient );
    REQUIRE( built.is_craft() );
    CHECK( built.get_passive_start_counter() == 0 );
    CHECK( built.get_passive_end_counter() == 0 );
}

TEST_CASE( "craft_data_serialize_omits_zero_passive_counters",
           "[craft][attention][persist]" )
{
    item ingredient( itype_2x4, calendar::turn );
    item built( &recipe_cudgel_test_unattended_simple.obj(), 1, ingredient );
    REQUIRE( built.is_craft() );

    std::ostringstream ss;
    JsonOut jsout( ss );
    built.serialize( jsout );
    const std::string out = ss.str();
    CHECK( out.find( "passive_start_counter" ) == std::string::npos );
    CHECK( out.find( "passive_end_counter" ) == std::string::npos );
}

TEST_CASE( "craft_tname_projects_progress_during_passive_step",
           "[craft][attention][display]" )
{
    item ingredient( itype_2x4, calendar::turn );
    item built( &recipe_cudgel_test_unattended_simple.obj(), 1, ingredient );
    REQUIRE( built.is_craft() );

    const time_point t0 = calendar::turn;
    built.item_counter = 1000000; // 10%
    built.set_passive_started_at( t0 - 5_minutes );
    built.set_ready_at( t0 + 5_minutes );
    built.set_passive_start_counter( 1000000 );
    built.set_passive_end_counter( 9000000 );

    const std::string name = built.tname();
    // halfway between 10% and 90% = 50%
    CHECK( name.find( "50%" ) != std::string::npos );
}

TEST_CASE( "craft_tname_clamps_projection_at_step_boundaries",
           "[craft][attention][display]" )
{
    item ingredient( itype_2x4, calendar::turn );
    item built( &recipe_cudgel_test_unattended_simple.obj(), 1, ingredient );
    REQUIRE( built.is_craft() );

    const time_point t0 = calendar::turn;
    built.item_counter = 0;
    built.set_passive_start_counter( 1000000 );
    built.set_passive_end_counter( 9000000 );

    SECTION( "before passive_started_at" ) {
        built.set_passive_started_at( t0 + 5_minutes );
        built.set_ready_at( t0 + 15_minutes );
        const std::string name = built.tname();
        CHECK( name.find( "10%" ) != std::string::npos );
    }
    SECTION( "after ready_at" ) {
        built.set_passive_started_at( t0 - 20_minutes );
        built.set_ready_at( t0 - 10_minutes );
        const std::string name = built.tname();
        CHECK( name.find( "90%" ) != std::string::npos );
    }
}

TEST_CASE( "craft_tname_falls_back_to_item_counter_outside_passive",
           "[craft][attention][display]" )
{
    item ingredient( itype_2x4, calendar::turn );
    item built( &recipe_cudgel_test_unattended_simple.obj(), 1, ingredient );
    REQUIRE( built.is_craft() );
    built.item_counter = 4200000; // 42%

    const std::string name = built.tname();
    CHECK( name.find( "42%" ) != std::string::npos );
}

TEST_CASE( "craft_tname_never_decreases_below_item_counter",
           "[craft][attention][display]" )
{
    item ingredient( itype_2x4, calendar::turn );
    item built( &recipe_cudgel_test_unattended_simple.obj(), 1, ingredient );
    REQUIRE( built.is_craft() );

    const time_point t0 = calendar::turn;
    built.item_counter = 8500000; // 85%
    built.set_passive_started_at( t0 );
    built.set_ready_at( t0 + 10_minutes );
    built.set_passive_start_counter( 1000000 );
    built.set_passive_end_counter( 9000000 );

    const std::string name = built.tname();
    CHECK( name.find( "85%" ) != std::string::npos );
}

TEST_CASE( "craft_tname_uses_saved_ready_at_during_pause",
           "[craft][attention][display]" )
{
    item ingredient( itype_2x4, calendar::turn );
    item built( &recipe_cudgel_test_unattended_simple.obj(), 1, ingredient );
    REQUIRE( built.is_craft() );

    const time_point t0 = calendar::turn;
    built.item_counter = 1000000;
    built.set_passive_started_at( t0 - 5_minutes );
    built.set_passive_start_counter( 1000000 );
    built.set_passive_end_counter( 9000000 );

    // While paused, ready_at is repurposed as polling cursor; saved_ready_at
    // holds the original deadline.  Display must use the saved value.
    built.set_ready_at( t0 + 1_minutes );
    built.set_saved_ready_at( t0 + 5_minutes );

    const std::string name = built.tname();
    CHECK( name.find( "50%" ) != std::string::npos );
}

TEST_CASE( "craft_data_persists_attention_runtime_fields",
           "[craft][attention][persist]" )
{
    item ingredient( itype_2x4, calendar::turn );
    item built( &recipe_cudgel_test_unattended_simple.obj(), 1, ingredient );
    REQUIRE( built.is_craft() );

    std::vector<attention_plan> plans( 2 );
    plans[1].choice = step_choice::set_timer;
    plans[1].alarm_offset = 7_minutes;
    built.set_step_plans( plans );

    built.set_passive_started_at( calendar::turn );
    built.set_ready_at( calendar::turn + 10_minutes );
    built.set_alarm_at( calendar::turn + 7_minutes );
    built.set_fail_at( calendar::turn + 25_minutes );
    built.set_pause_started_at( calendar::turn + 2_minutes );
    built.set_saved_ready_at( calendar::turn + 11_minutes );
    built.set_saved_alarm_at( calendar::turn + 8_minutes );
    built.set_saved_fail_at( calendar::turn + 26_minutes );
    built.set_crafter_id( character_id( 42 ) );

    std::ostringstream ss;
    JsonOut jsout( ss );
    built.serialize( jsout );

    item restored;
    restored.deserialize( json_loader::from_string( ss.str() ).get_object() );

    REQUIRE( restored.is_craft() );
    REQUIRE( restored.get_step_plans().size() == 2 );
    CHECK( restored.get_step_plans()[1].choice == step_choice::set_timer );
    REQUIRE( restored.get_step_plans()[1].alarm_offset.has_value() );
    CHECK( *restored.get_step_plans()[1].alarm_offset == 7_minutes );
    CHECK( restored.get_alarm_at() == calendar::turn + 7_minutes );
    CHECK( restored.get_fail_at() == calendar::turn + 25_minutes );
    CHECK( restored.get_pause_started_at() == calendar::turn + 2_minutes );
    CHECK( restored.get_saved_ready_at() == calendar::turn + 11_minutes );
    CHECK( restored.get_saved_alarm_at() == calendar::turn + 8_minutes );
    CHECK( restored.get_saved_fail_at() == calendar::turn + 26_minutes );
    CHECK( restored.get_crafter_id() == character_id( 42 ) );
}

TEST_CASE( "craft_data_persists_step_tool_allocs", "[craft][attention][persist]" )
{
    // Round-trip a single allocation directly, so this covers serialization
    // independent of the load-time recipe-shape validation (exercised by the
    // craft_data_validates_* cases).
    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::player;
    alloc.sel.comp.type = itype_soldering_iron_portable;
    alloc.sel.comp.count = 8;
    alloc.step_count_units = 8;
    alloc.consumed_buckets = 5;
    alloc.root_derived = true;

    std::ostringstream ss;
    JsonOut jsout( ss );
    alloc.serialize( jsout );

    step_tool_alloc restored;
    restored.deserialize( json_loader::from_string( ss.str() ).get_object() );

    CHECK( restored.sel.use_from == usage_from::player );
    CHECK( restored.sel.comp.type == itype_soldering_iron_portable );
    CHECK( restored.sel.comp.count == 8 );
    CHECK( restored.step_count_units == 8 );
    CHECK( restored.consumed_buckets == 5 );
    CHECK( restored.root_derived );
}

TEST_CASE( "craft_data_resets_stale_step_tool_allocs_on_size_mismatch",
           "[craft][attention][persist][migration]" )
{
    item ingredient( itype_2x4, calendar::turn );
    item_components comps;
    comps.add( ingredient );
    item built( &recipe_cudgel_test_unattended_simple.obj(), 1, comps,
                std::vector<item_comp> {} );
    REQUIRE( built.is_craft() );
    REQUIRE( built.get_making().steps().size() == 2 );

    // A save whose allocation rows do not match the recipe's step count (a
    // legacy flat save deserializes to zero rows the same way).
    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::player;
    alloc.sel.comp.type = itype_soldering_iron_portable;
    alloc.sel.comp.count = 8;
    alloc.step_count_units = 8;
    alloc.consumed_buckets = 3;
    built.set_step_tool_allocs( { { alloc } } );
    built.set_tools_to_continue( true );

    std::ostringstream ss;
    JsonOut jsout( ss );
    built.serialize( jsout );

    item restored;
    restored.deserialize( json_loader::from_string( ss.str() ).get_object() );

    REQUIRE( restored.is_craft() );
    REQUIRE( restored.get_making().steps().size() == 2 );
    CHECK( restored.get_step_tool_allocs().empty() );
    CHECK_FALSE( restored.has_tools_to_continue() );
}

TEST_CASE( "craft_data_resets_step_tool_allocs_on_tool_shape_change",
           "[craft][attention][persist][migration]" )
{
    item ingredient( itype_2x4, calendar::turn );
    item_components comps;
    comps.add( ingredient );
    item built( &recipe_cudgel_test_unattended_simple.obj(), 1, comps,
                std::vector<item_comp> {} );
    REQUIRE( built.is_craft() );
    REQUIRE( built.get_making().steps().size() == 2 );

    // Right row count, but the allocation references a tool the recipe's steps
    // no longer list (a tool-group edit that preserved the step count).
    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::player;
    alloc.sel.comp.type = itype_soldering_iron_portable;
    alloc.sel.comp.count = 8;
    alloc.step_count_units = 8;
    built.set_step_tool_allocs( { {}, { alloc } } );
    built.set_tools_to_continue( true );
    // Mid-flight on the unattended step, with live passive timers.
    built.set_current_step( 1 );
    built.set_passive_started_at( calendar::turn );
    built.set_ready_at( calendar::turn + 10_minutes );

    std::ostringstream ss;
    JsonOut jsout( ss );
    built.serialize( jsout );

    item restored;
    restored.deserialize( json_loader::from_string( ss.str() ).get_object() );

    REQUIRE( restored.is_craft() );
    REQUIRE( restored.get_making().steps().size() == 2 );
    CHECK( restored.get_step_tool_allocs().empty() );
    CHECK_FALSE( restored.has_tools_to_continue() );
    // Scrubbing the allocs also drops the passive timers, so the step cannot
    // finish unmetered on load.
    CHECK( restored.get_passive_started_at() == calendar::before_time_starts );
    CHECK( restored.get_ready_at() == calendar::before_time_starts );
}

TEST_CASE( "craft_data_validates_step_tool_alloc_shape_on_load",
           "[craft][attention][persist][migration]" )
{
    REQUIRE( recipe_cudgel_test_steps_charged.obj().steps().size() == 3 );

    const auto round_trip = []( const std::vector<std::vector<step_tool_alloc>> &allocs ) -> item {
        item ingredient( itype_2x4, calendar::turn );
        item_components comps;
        comps.add( ingredient );
        item built( &recipe_cudgel_test_steps_charged.obj(), 1, comps,
        std::vector<item_comp> {} );
        built.set_step_tool_allocs( allocs );
        built.set_tools_to_continue( true );
        std::ostringstream ss;
        JsonOut jsout( ss );
        built.serialize( jsout );
        item restored;
        restored.deserialize( json_loader::from_string( ss.str() ).get_object() );
        return restored;
    };

    step_tool_alloc good;
    good.sel.use_from = usage_from::both;
    good.sel.comp.type = itype_soldering_iron_portable;
    good.sel.comp.count = 40;
    good.step_count_units = 40;
    good.consumed_buckets = 4;

    GIVEN( "allocations matching the recipe's tool groups" ) {
        item restored = round_trip( { {}, { good }, {} } );
        REQUIRE( restored.is_craft() );

        THEN( "they are preserved on load" ) {
            REQUIRE( restored.get_step_tool_allocs().size() == 3 );
            REQUIRE_FALSE( restored.get_step_tool_allocs()[1].empty() );
            CHECK( restored.get_step_tool_allocs()[1][0].consumed_buckets == 4 );
            CHECK( restored.has_tools_to_continue() );
        }
    }

    GIVEN( "an allocation whose count no longer matches the tool group" ) {
        step_tool_alloc stale = good;
        stale.sel.comp.count = 99;
        item restored = round_trip( { {}, { stale }, {} } );
        REQUIRE( restored.is_craft() );

        THEN( "the allocations are dropped for a rebuild" ) {
            CHECK( restored.get_step_tool_allocs().empty() );
            CHECK_FALSE( restored.has_tools_to_continue() );
        }
    }

    GIVEN( "an allocation with an out-of-range consumed bucket count" ) {
        step_tool_alloc corrupt = good;
        corrupt.consumed_buckets = 99;
        item restored = round_trip( { {}, { corrupt }, {} } );
        REQUIRE( restored.is_craft() );

        THEN( "the allocations are dropped for a rebuild" ) {
            CHECK( restored.get_step_tool_allocs().empty() );
            CHECK_FALSE( restored.has_tools_to_continue() );
        }
    }

    GIVEN( "an allocation whose units disagree with its selected count" ) {
        step_tool_alloc inconsistent = good;
        inconsistent.step_count_units = 7;
        item restored = round_trip( { {}, { inconsistent }, {} } );
        REQUIRE( restored.is_craft() );

        THEN( "the allocations are dropped for a rebuild" ) {
            CHECK( restored.get_step_tool_allocs().empty() );
            CHECK_FALSE( restored.has_tools_to_continue() );
        }
    }

    GIVEN( "a charged allocation with no usable source" ) {
        step_tool_alloc sourceless = good;
        sourceless.sel.use_from = usage_from::none;
        item restored = round_trip( { {}, { sourceless }, {} } );
        REQUIRE( restored.is_craft() );

        THEN( "the allocations are dropped for a rebuild" ) {
            CHECK( restored.get_step_tool_allocs().empty() );
            CHECK_FALSE( restored.has_tools_to_continue() );
        }
    }
}

TEST_CASE( "craft_data_validates_stepless_step_tool_allocs_on_load",
           "[craft][attention][persist][migration]" )
{
    const recipe &rec = recipe_cudgel_test_charged_fast_stepless.obj();
    REQUIRE_FALSE( rec.has_steps() );
    REQUIRE( rec.simple_requirements().get_tools().size() == 1 );
    const tool_comp tool = rec.simple_requirements().get_tools()[0].front();

    const auto round_trip = [&rec]( const std::vector<std::vector<step_tool_alloc>> &allocs ) -> item {
        item ingredient( itype_2x4, calendar::turn );
        item_components comps;
        comps.add( ingredient );
        item built( &rec, 1, comps, std::vector<item_comp> {} );
        built.set_step_tool_allocs( allocs );
        built.set_tools_to_continue( true );
        std::ostringstream ss;
        JsonOut jsout( ss );
        built.serialize( jsout );
        item restored;
        restored.deserialize( json_loader::from_string( ss.str() ).get_object() );
        return restored;
    };

    const auto alloc_for = [&tool]( int count, int consumed ) -> step_tool_alloc {
        step_tool_alloc a;
        a.sel.use_from = usage_from::both;
        a.sel.comp.type = tool.type;
        a.sel.comp.count = count;
        a.step_count_units = std::max( 0, count );
        a.consumed_buckets = consumed;
        return a;
    };

    GIVEN( "a stepless allocation matching the recipe tool" ) {
        item restored = round_trip( { { alloc_for( tool.count, 4 ) } } );
        REQUIRE( restored.is_craft() );
        THEN( "it is preserved" ) {
            REQUIRE( restored.get_step_tool_allocs().size() == 1 );
            REQUIRE( restored.get_step_tool_allocs()[0].size() == 1 );
            CHECK( restored.get_step_tool_allocs()[0][0].consumed_buckets == 4 );
            CHECK( restored.has_tools_to_continue() );
        }
    }

    GIVEN( "a stepless charged allocation with no usable source" ) {
        step_tool_alloc sourceless = alloc_for( tool.count, 4 );
        sourceless.sel.use_from = usage_from::none;
        item restored = round_trip( { { sourceless } } );
        REQUIRE( restored.is_craft() );
        THEN( "the unsourced allocation is dropped for a rebuild" ) {
            CHECK( restored.get_step_tool_allocs().empty() );
            CHECK_FALSE( restored.has_tools_to_continue() );
        }
    }

    GIVEN( "a stepless allocation whose count the recipe no longer offers" ) {
        item restored = round_trip( { { alloc_for( tool.count + 1, 4 ) } } );
        REQUIRE( restored.is_craft() );
        THEN( "the stale allocation is dropped for a rebuild" ) {
            CHECK( restored.get_step_tool_allocs().empty() );
            CHECK_FALSE( restored.has_tools_to_continue() );
        }
    }

    GIVEN( "more allocations than the recipe has tool groups" ) {
        item restored = round_trip( { { alloc_for( tool.count, 4 ), alloc_for( tool.count, 4 ) } } );
        REQUIRE( restored.is_craft() );
        THEN( "the mismatched shape is dropped for a rebuild" ) {
            CHECK( restored.get_step_tool_allocs().empty() );
            CHECK_FALSE( restored.has_tools_to_continue() );
        }
    }

    GIVEN( "no allocations when the recipe needs a tool" ) {
        item restored = round_trip( {} );
        REQUIRE( restored.is_craft() );
        THEN( "the unmetered shape is dropped for a rebuild" ) {
            CHECK( restored.get_step_tool_allocs().empty() );
            CHECK_FALSE( restored.has_tools_to_continue() );
        }
    }
}

TEST_CASE( "craft_data_resets_step_tool_allocs_on_group_reorder",
           "[craft][attention][persist][migration]" )
{
    REQUIRE( recipe_cudgel_test_steps_two_tools.obj().steps().size() == 1 );

    const auto round_trip = []( const std::vector<std::vector<step_tool_alloc>> &allocs ) -> item {
        item ingredient( itype_2x4, calendar::turn );
        item_components comps;
        comps.add( ingredient );
        item built( &recipe_cudgel_test_steps_two_tools.obj(), 1, comps,
        std::vector<item_comp> {} );
        built.set_step_tool_allocs( allocs );
        built.set_tools_to_continue( true );
        std::ostringstream ss;
        JsonOut jsout( ss );
        built.serialize( jsout );
        item restored;
        restored.deserialize( json_loader::from_string( ss.str() ).get_object() );
        return restored;
    };

    const auto presence = []( const itype_id & type ) -> step_tool_alloc {
        step_tool_alloc a;
        a.sel.use_from = usage_from::map;
        a.sel.comp.type = type;
        a.sel.comp.count = -1;
        return a;
    };

    GIVEN( "allocations lined up with the step's tool groups in order" ) {
        item restored = round_trip( { { presence( itype_soldering_iron_portable ), presence( itype_hammer ) } } );
        REQUIRE( restored.is_craft() );

        THEN( "they are preserved" ) {
            REQUIRE( restored.get_step_tool_allocs().size() == 1 );
            CHECK( restored.get_step_tool_allocs()[0].size() == 2 );
            CHECK( restored.has_tools_to_continue() );
        }
    }

    GIVEN( "a duplicate that no longer fits the second group positionally" ) {
        item restored = round_trip( { { presence( itype_soldering_iron_portable ), presence( itype_soldering_iron_portable ) } } );
        REQUIRE( restored.is_craft() );

        THEN( "the stale shape is dropped" ) {
            CHECK( restored.get_step_tool_allocs().empty() );
            CHECK_FALSE( restored.has_tools_to_continue() );
        }
    }
}

TEST_CASE( "craft_data_root_alloc_shape_follows_timed_steps",
           "[craft][attention][persist][migration]" )
{
    const recipe &rec = recipe_cudgel_test_root_unattended.obj();
    REQUIRE( rec.steps().size() == 2 );
    REQUIRE( rec.steps()[0].attention != step_attention::unattended );
    REQUIRE( rec.steps()[1].attention == step_attention::unattended );
    const std::vector<std::vector<tool_comp>> &root_groups =
            rec.root_requirements().get_tools();
    REQUIRE( root_groups.size() == 1 );
    REQUIRE_FALSE( root_groups[0].empty() );
    const tool_comp root_tool = root_groups[0].front();

    const auto round_trip = [&rec]( const std::vector<std::vector<step_tool_alloc>> &allocs ) -> item {
        item ingredient( itype_2x4, calendar::turn );
        item_components comps;
        comps.add( ingredient );
        item built( &rec, 1, comps, std::vector<item_comp> {} );
        built.set_step_tool_allocs( allocs );
        built.set_tools_to_continue( true );
        std::ostringstream ss;
        JsonOut jsout( ss );
        built.serialize( jsout );
        item restored;
        restored.deserialize( json_loader::from_string( ss.str() ).get_object() );
        return restored;
    };

    const auto root_alloc = [&root_tool]( int units ) -> step_tool_alloc {
        step_tool_alloc a;
        a.sel.use_from = usage_from::both;
        a.sel.comp.type = root_tool.type;
        a.sel.comp.count = root_tool.count;
        a.step_count_units = units;
        a.root_derived = true;
        return a;
    };

    GIVEN( "a root allocation on every timed step" ) {
        // Per-step shares must sum to the tool's whole-craft total.
        const int half = root_tool.count / 2;
        item restored = round_trip( { { root_alloc( root_tool.count - half ) }, { root_alloc( half ) } } );
        REQUIRE( restored.is_craft() );

        THEN( "it is preserved" ) {
            REQUIRE( restored.get_step_tool_allocs().size() == 2 );
            CHECK( restored.get_step_tool_allocs()[0].size() == 1 );
            CHECK( restored.get_step_tool_allocs()[1].size() == 1 );
            CHECK( restored.has_tools_to_continue() );
        }
    }

    GIVEN( "a root allocation missing from a timed step" ) {
        item restored = round_trip( { { root_alloc( root_tool.count ) }, {} } );
        REQUIRE( restored.is_craft() );

        THEN( "the stale shape is dropped" ) {
            CHECK( restored.get_step_tool_allocs().empty() );
            CHECK_FALSE( restored.has_tools_to_continue() );
        }
    }
}

TEST_CASE( "craft_data_default_step_plan_serializes_minimally",
           "[craft][attention][persist]" )
{
    item ingredient( itype_2x4, calendar::turn );
    item built( &recipe_cudgel_test_unattended_simple.obj(), 1, ingredient );
    REQUIRE( built.is_craft() );

    std::vector<attention_plan> plans( 2 );  // all defaults: do_wait, no alarm_offset
    built.set_step_plans( plans );

    std::ostringstream ss;
    JsonOut jsout( ss );
    built.serialize( jsout );

    item restored;
    restored.deserialize( json_loader::from_string( ss.str() ).get_object() );

    REQUIRE( restored.is_craft() );
    REQUIRE( restored.get_step_plans().size() == 2 );
    CHECK( restored.get_step_plans()[0].choice == step_choice::do_wait );
    CHECK_FALSE( restored.get_step_plans()[0].alarm_offset.has_value() );
}

TEST_CASE( "craft_data_resets_stale_passive_state_on_step_plan_size_mismatch",
           "[craft][attention][persist][migration]" )
{
    item ingredient( itype_2x4, calendar::turn );
    item_components comps;
    comps.add( ingredient );
    item built( &recipe_cudgel_test_unattended_simple.obj(), 1, comps,
                std::vector<item_comp> {} );
    REQUIRE( built.is_craft() );

    // Simulate a save written under a different recipe (3 steps) before the
    // recipe was edited down to 2 steps.
    std::vector<attention_plan> stale( 3 );
    stale[1].choice = step_choice::set_timer;
    stale[1].alarm_offset = 5_minutes;
    built.set_step_plans( stale );
    built.set_passive_started_at( calendar::turn );
    built.set_ready_at( calendar::turn + 10_minutes );
    built.set_alarm_at( calendar::turn + 5_minutes );
    built.set_fail_at( calendar::turn + 25_minutes );
    built.set_passive_start_counter( 1000000 );
    built.set_passive_end_counter( 5000000 );

    std::ostringstream ss;
    JsonOut jsout( ss );
    built.serialize( jsout );

    item restored;
    restored.deserialize( json_loader::from_string( ss.str() ).get_object() );

    REQUIRE( restored.is_craft() );
    CHECK( restored.get_step_plans().empty() );
    CHECK( restored.get_passive_started_at() == calendar::before_time_starts );
    CHECK( restored.get_ready_at() == calendar::before_time_starts );
    CHECK( restored.get_alarm_at() == calendar::before_time_starts );
    CHECK( restored.get_fail_at() == calendar::before_time_starts );
    CHECK( restored.get_passive_start_counter() == 0 );
    CHECK( restored.get_passive_end_counter() == 0 );
}

TEST_CASE( "craft_apply_resume_replan_targets_correct_alarm_slot",
           "[craft][attention][resume][alarm]" )
{
    clear_map();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_simple.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 1 );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    SECTION( "live passive step: arms alarm_at" ) {
        on_map.set_passive_started_at( calendar::turn );
        on_map.set_ready_at( calendar::turn + 10_minutes );
        std::vector<attention_plan> plans( 2 );
        plans[1].choice = step_choice::set_timer;
        plans[1].alarm_offset = 8_minutes;
        on_map.set_step_plans( plans );

        craft_apply_resume_replan( loc );

        CHECK( on_map.get_alarm_at() == calendar::turn + 8_minutes );
        CHECK( on_map.get_saved_alarm_at() == calendar::before_time_starts );
    }

    SECTION( "env-paused passive step: arms saved_alarm_at" ) {
        on_map.set_passive_started_at( calendar::turn );
        on_map.set_ready_at( calendar::turn + 1_minutes );
        on_map.set_saved_ready_at( calendar::turn + 10_minutes );
        std::vector<attention_plan> plans( 2 );
        plans[1].choice = step_choice::set_timer;
        plans[1].alarm_offset = 8_minutes;
        on_map.set_step_plans( plans );

        craft_apply_resume_replan( loc );

        CHECK( on_map.get_alarm_at() == calendar::before_time_starts );
        CHECK( on_map.get_saved_alarm_at() == calendar::turn + 8_minutes );
    }

    SECTION( "removing timer clears both alarm slots" ) {
        on_map.set_passive_started_at( calendar::turn );
        on_map.set_ready_at( calendar::turn + 10_minutes );
        on_map.set_alarm_at( calendar::turn + 8_minutes );
        on_map.set_saved_alarm_at( calendar::turn + 8_minutes );
        std::vector<attention_plan> plans( 2 );
        plans[1].choice = step_choice::do_wait;
        on_map.set_step_plans( plans );

        craft_apply_resume_replan( loc );

        CHECK( on_map.get_alarm_at() == calendar::before_time_starts );
        CHECK( on_map.get_saved_alarm_at() == calendar::before_time_starts );
    }

    SECTION( "before passive entry: no-op" ) {
        // passive_started_at left at before_time_starts.
        std::vector<attention_plan> plans( 2 );
        plans[1].choice = step_choice::set_timer;
        plans[1].alarm_offset = 8_minutes;
        on_map.set_step_plans( plans );

        craft_apply_resume_replan( loc );

        CHECK( on_map.get_alarm_at() == calendar::before_time_starts );
        CHECK( on_map.get_saved_alarm_at() == calendar::before_time_starts );
    }
}

TEST_CASE( "craft_unattended_charged_tool_debits_at_step_completion",
           "[craft][attention][charge]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item iron = tool_with_ammo( itype_soldering_iron_portable, 50 );
    REQUIRE( iron.ammo_remaining() == 50 );
    u.i_add( iron );
    u.invalidate_crafting_inventory();

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_charged.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );

    // Position at the unattended Cure step (idx 1) carrying its charged tool.
    on_map.set_current_step( 1 );
    on_map.set_passive_started_at( calendar::turn );
    on_map.set_ready_at( calendar::turn + 10_minutes );
    on_map.set_crafter_id( u.getID() );

    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::both;
    alloc.sel.comp.type = itype_soldering_iron_portable;
    alloc.sel.comp.count = 20;
    alloc.step_count_units = 20;
    on_map.set_step_tool_allocs( { {}, { alloc } } );

    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );
    REQUIRE( u.craft_consume_passive_step_tools( on_map, calendar::turn + 10_minutes, loc ) );

    u.invalidate_crafting_inventory();
    CHECK( get_remaining_charges( itype_soldering_iron_portable ) == 30 );
    REQUIRE( on_map.get_step_tool_allocs().size() == 2 );
    REQUIRE_FALSE( on_map.get_step_tool_allocs()[1].empty() );
    CHECK( on_map.get_step_tool_allocs()[1][0].consumed_buckets == 20 );
}

TEST_CASE( "craft_unattended_charged_tool_drains_per_tick_and_pauses",
           "[craft][attention][charge]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_charged.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    on_map.set_current_step( 1 );
    on_map.set_passive_started_at( calendar::turn );
    on_map.set_ready_at( calendar::turn + 10_minutes );
    on_map.set_crafter_id( u.getID() );

    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::both;
    alloc.sel.comp.type = itype_soldering_iron_portable;
    alloc.sel.comp.count = 20;
    alloc.step_count_units = 20;
    on_map.set_step_tool_allocs( { {}, { alloc } } );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );
    get_item_wakeups().rebuild_for_item( loc );

    GIVEN( "the crafter carries enough charges for the whole step" ) {
        u.i_add( tool_with_ammo( itype_soldering_iron_portable, 50 ) );
        u.invalidate_crafting_inventory();

        WHEN( "an env tick fires halfway through the step" ) {
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check,
                                       calendar::turn + 5_minutes, loc );
            u.invalidate_crafting_inventory();

            THEN( "the buckets so far are drained and the step keeps running" ) {
                CHECK( get_remaining_charges( itype_soldering_iron_portable ) == 39 );
                CHECK( on_map.get_pause_started_at() == calendar::before_time_starts );
            }
        }
    }

    GIVEN( "the step's charged tool is not present" ) {
        u.invalidate_crafting_inventory();

        WHEN( "an env tick fires mid-step" ) {
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check,
                                       calendar::turn + 5_minutes, loc );

            THEN( "the step pauses with its real deadline parked" ) {
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
                CHECK( on_map.get_ready_at() == calendar::turn + 6_minutes );
                CHECK( on_map.get_saved_ready_at() == calendar::turn + 10_minutes );
            }

            AND_WHEN( "the tool returns and the polling tick fires" ) {
                u.i_add( tool_with_ammo( itype_soldering_iron_portable, 50 ) );
                u.invalidate_crafting_inventory();
                craft_actualize_scheduled( on_map, item_wakeup_kind::env_check,
                                           calendar::turn + 6_minutes, loc );

                THEN( "the step unpauses and slides its deadline forward" ) {
                    CHECK( on_map.get_pause_started_at() == calendar::before_time_starts );
                    CHECK( on_map.get_ready_at() == calendar::turn + 11_minutes );
                    // Entry slides with the deadline so the paused minute is not
                    // counted as progress by the passive debit fraction.
                    CHECK( on_map.get_passive_started_at() == calendar::turn + 1_minutes );
                }
            }

            AND_WHEN( "a ready-check poll fires while the tool is still absent" ) {
                craft_actualize_scheduled( on_map, item_wakeup_kind::ready_check,
                                           calendar::turn + 6_minutes, loc );

                THEN( "the step stays paused instead of resuming on the quality gate" ) {
                    CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
                    CHECK( on_map.get_current_step() == 1 );
                }
            }
        }
    }
}

TEST_CASE( "craft_unattended_charged_tool_offset_crafter_uses_map_source",
           "[craft][attention][charge]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms craft_pos( 75, 75, 0 );
    // Crafter stands well outside PICKUP_RANGE of the workbench craft.
    u.setpos( here, tripoint_bub_ms( 60, 60, 0 ) );

    // The charged tool sits on the map at the craft, not in the crafter's pack.
    item &map_iron = here.add_item( craft_pos, tool_with_ammo( itype_soldering_iron_portable, 50 ) );
    REQUIRE( map_iron.ammo_remaining() == 50 );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_charged.obj(), 1, ingredient );
    item &on_map = here.add_item( craft_pos, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 1 );
    on_map.set_passive_started_at( calendar::turn );
    on_map.set_ready_at( calendar::turn + 10_minutes );
    on_map.set_crafter_id( u.getID() );

    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::map;
    alloc.sel.comp.type = itype_soldering_iron_portable;
    alloc.sel.comp.count = 20;
    alloc.step_count_units = 20;
    on_map.set_step_tool_allocs( { {}, { alloc } } );

    item_location loc( map_cursor( here.get_abs( craft_pos ) ), &on_map );

    REQUIRE( u.craft_consume_passive_step_tools( on_map, calendar::turn + 10_minutes, loc ) );

    CHECK( map_iron.ammo_remaining() == 30 );
    REQUIRE( on_map.get_step_tool_allocs().size() == 2 );
    REQUIRE_FALSE( on_map.get_step_tool_allocs()[1].empty() );
    CHECK( on_map.get_step_tool_allocs()[1][0].consumed_buckets == 20 );
}

TEST_CASE( "recipe_unattended_charged_tool_finalizes_without_reject",
           "[craft][attention][charge][schema]" )
{
    const recipe &r = recipe_cudgel_test_unattended_charged.obj();
    REQUIRE( r.has_steps() );
    const recipe_step &cure = r.steps().back();
    REQUIRE( cure.attention == step_attention::unattended );

    bool has_charged_tool = false;
    for( const std::vector<tool_comp> &group : cure.requirements.get_tools() ) {
        for( const tool_comp &tc : group ) {
            if( tc.type == itype_soldering_iron_portable && tc.count > 0 ) {
                has_charged_tool = true;
            }
        }
    }
    CHECK( has_charged_tool );
}

TEST_CASE( "craft_unattended_or_group_uses_noncharged_alternative_free",
           "[craft][attention][charge]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    u.i_add( tool_with_ammo( itype_soldering_iron_portable, 50 ) );
    u.invalidate_crafting_inventory();

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_charged.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    on_map.set_current_step( 1 );
    on_map.set_passive_started_at( calendar::turn );
    on_map.set_ready_at( calendar::turn + 10_minutes );
    on_map.set_crafter_id( u.getID() );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "the pinned selection is a non-charged OR alternative" ) {
        step_tool_alloc alloc;
        alloc.sel.use_from = usage_from::none;
        alloc.sel.comp.type = itype_soldering_iron_portable;
        alloc.sel.comp.count = -1;
        alloc.step_count_units = 0;
        on_map.set_step_tool_allocs( { {}, { alloc } } );

        WHEN( "the step completes" ) {
            REQUIRE( u.craft_consume_passive_step_tools(
                         on_map, calendar::turn + 10_minutes, loc ) );
            u.invalidate_crafting_inventory();

            THEN( "no charges are drained but the step still trues up" ) {
                CHECK( get_remaining_charges( itype_soldering_iron_portable ) == 50 );
                CHECK( on_map.get_step_tool_allocs()[1][0].consumed_buckets == 20 );
            }
        }
    }

    GIVEN( "the pinned selection is the charged OR alternative" ) {
        step_tool_alloc alloc;
        alloc.sel.use_from = usage_from::both;
        alloc.sel.comp.type = itype_soldering_iron_portable;
        alloc.sel.comp.count = 20;
        alloc.step_count_units = 20;
        on_map.set_step_tool_allocs( { {}, { alloc } } );

        WHEN( "the step completes" ) {
            REQUIRE( u.craft_consume_passive_step_tools(
                         on_map, calendar::turn + 10_minutes, loc ) );
            u.invalidate_crafting_inventory();

            THEN( "the full charged amount is drained" ) {
                CHECK( get_remaining_charges( itype_soldering_iron_portable ) == 30 );
            }
        }
    }
}

TEST_CASE( "craft_unattended_charged_tools_shared_pool_no_partial_debit",
           "[craft][attention][charge]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    // Enough charges to satisfy one allocation but not both together.
    u.i_add( tool_with_ammo( itype_soldering_iron_portable, 30 ) );
    u.invalidate_crafting_inventory();

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_charged.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    on_map.set_current_step( 1 );
    on_map.set_passive_started_at( calendar::turn );
    on_map.set_ready_at( calendar::turn + 10_minutes );
    on_map.set_crafter_id( u.getID() );

    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::both;
    alloc.sel.comp.type = itype_soldering_iron_portable;
    alloc.sel.comp.count = 20;
    alloc.step_count_units = 20;
    on_map.set_step_tool_allocs( { {}, { alloc, alloc } } );

    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    REQUIRE_FALSE( u.craft_consume_passive_step_tools(
                       on_map, calendar::turn + 10_minutes, loc ) );

    u.invalidate_crafting_inventory();
    CHECK( get_remaining_charges( itype_soldering_iron_portable ) == 30 );
    REQUIRE( on_map.get_step_tool_allocs()[1].size() == 2 );
    CHECK( on_map.get_step_tool_allocs()[1][0].consumed_buckets == 0 );
    CHECK( on_map.get_step_tool_allocs()[1][1].consumed_buckets == 0 );
}

TEST_CASE( "craft_unattended_charged_tool_completion_pauses_on_partial_charges",
           "[craft][attention][charge]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    // 25 charges clears the start-only gate for a 40-charge step but cannot
    // cover the full completion debit.
    u.i_add( tool_with_ammo( itype_soldering_iron_portable, 25 ) );
    u.invalidate_crafting_inventory();

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_charged_big.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    on_map.set_current_step( 1 );
    on_map.set_passive_started_at( calendar::turn );
    on_map.set_ready_at( calendar::turn + 10_minutes );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 2 ) );

    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::both;
    alloc.sel.comp.type = itype_soldering_iron_portable;
    alloc.sel.comp.count = 40;
    alloc.step_count_units = 40;
    on_map.set_step_tool_allocs( { {}, { alloc } } );

    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    craft_actualize_scheduled( on_map, item_wakeup_kind::ready_check,
                               calendar::turn + 10_minutes, loc );

    REQUIRE( loc.get_item() != nullptr );
    CHECK( on_map.get_current_step() == 1 );
    CHECK( on_map.get_pause_started_at() == calendar::turn + 10_minutes );
    CHECK( on_map.get_saved_ready_at() == calendar::turn + 10_minutes );
}

TEST_CASE( "craft_unattended_charged_tool_overdue_load_pauses_when_drained",
           "[craft][attention][charge][overdue]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    // No charged tool present anywhere.

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_charged.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    on_map.set_current_step( 1 );
    on_map.set_passive_started_at( calendar::turn );
    on_map.set_ready_at( calendar::turn + 10_minutes );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 2 ) );

    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::both;
    alloc.sel.comp.type = itype_soldering_iron_portable;
    alloc.sel.comp.count = 20;
    alloc.step_count_units = 20;
    on_map.set_step_tool_allocs( { {}, { alloc } } );

    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    craft_resolve_overdue_passive( on_map, calendar::turn + 20_minutes, loc );

    REQUIRE( loc.get_item() != nullptr );
    CHECK( on_map.get_current_step() == 1 );
    CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
}

TEST_CASE( "craft_unattended_noncharged_tool_offset_crafter_uses_map_source",
           "[craft][attention][charge]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms craft_pos( 75, 75, 0 );
    u.setpos( here, tripoint_bub_ms( 60, 60, 0 ) );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_charged.obj(), 1, ingredient );
    item &on_map = here.add_item( craft_pos, placed );
    on_map.set_current_step( 1 );
    on_map.set_passive_started_at( calendar::turn );
    on_map.set_ready_at( calendar::turn + 10_minutes );
    on_map.set_crafter_id( u.getID() );

    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::map;
    alloc.sel.comp.type = itype_soldering_iron_portable;
    alloc.sel.comp.count = -1;
    alloc.step_count_units = 0;
    on_map.set_step_tool_allocs( { {}, { alloc } } );
    item_location loc( map_cursor( here.get_abs( craft_pos ) ), &on_map );

    GIVEN( "the non-charged tool sits on the map at the craft" ) {
        here.add_item( craft_pos, item( itype_soldering_iron_portable, calendar::turn ) );

        THEN( "the step trues up without pausing" ) {
            CHECK( u.craft_consume_passive_step_tools( on_map, calendar::turn + 10_minutes, loc ) );
            CHECK( on_map.get_step_tool_allocs()[1][0].consumed_buckets == 20 );
        }
    }

    GIVEN( "the non-charged tool is absent from the craft and the crafter" ) {
        THEN( "the step consume fails instead of running free" ) {
            CHECK_FALSE( u.craft_consume_passive_step_tools(
                             on_map, calendar::turn + 10_minutes, loc ) );
        }
    }

    GIVEN( "the tool is removed after its last bucket but before completion" ) {
        item &tool = here.add_item( craft_pos, item( itype_soldering_iron_portable, calendar::turn ) );
        // A near-end tick reaches bucket 20 while the tool is still present.
        REQUIRE( u.craft_consume_passive_step_tools(
                     on_map, calendar::turn + 9_minutes + 30_seconds, loc ) );
        REQUIRE( on_map.get_step_tool_allocs()[1][0].consumed_buckets == 20 );

        WHEN( "the tool is gone at the completion true-up" ) {
            here.i_rem( craft_pos, &tool );
            on_map.set_tools_to_continue( true );

            THEN( "the verifier fails and clears tools_to_continue" ) {
                CHECK_FALSE( u.verify_step_tools( on_map, 1, craft_pos, PICKUP_RANGE,
                                                  /*pin_to_map=*/true ) );
                CHECK_FALSE( on_map.has_tools_to_continue() );
            }
        }
    }

    GIVEN( "ready dispatch fires with the non-charged tool missing" ) {
        on_map.set_step_plans( std::vector<attention_plan>( 2 ) );
        on_map.set_tools_to_continue( true );
        std::vector<std::vector<step_tool_alloc>> drift_allocs = on_map.get_step_tool_allocs();
        drift_allocs[1][0].consumed_buckets = 20;
        on_map.set_step_tool_allocs( drift_allocs );

        WHEN( "craft_actualize_scheduled runs the ready_check handler" ) {
            craft_actualize_scheduled( on_map, item_wakeup_kind::ready_check,
                                       calendar::turn + 10_minutes, loc );

            THEN( "the step pauses without closing and tools_to_continue clears" ) {
                REQUIRE( loc.get_item() != nullptr );
                CHECK( on_map.get_current_step() == 1 );
                CHECK( on_map.get_pause_started_at() == calendar::turn + 10_minutes );
                CHECK_FALSE( on_map.has_tools_to_continue() );
            }
        }
    }
}

TEST_CASE( "craft_resolve_overdue_passive_chains_preserve_wall_time",
           "[craft][attention][resume][overdue]" )
{
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_consecutive_unattended.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );

    // Position at the first unattended step (Cure A, idx 1) with passive
    // state stamped manually.  Step 2 (Cure B) is also unattended; step 3
    // (Finish) is active.
    on_map.set_current_step( 1 );
    on_map.set_passive_started_at( calendar::turn );
    on_map.set_ready_at( calendar::turn + 10_minutes );
    on_map.set_crafter_id( u.getID() );
    std::vector<attention_plan> plans( 4 );
    on_map.set_step_plans( plans );

    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    SECTION( "now past both passive deadlines: chain advances to active step" ) {
        // step 1 ends at turn+10m, step 2 chained at turn+10m ends at turn+20m.
        // Now at turn+25m: both overdue, both should drain.
        craft_resolve_overdue_passive( on_map, calendar::turn + 25_minutes, loc );

        CHECK( on_map.get_current_step() == 3 );
        CHECK( on_map.get_passive_started_at() == calendar::before_time_starts );
    }

    SECTION( "now past first deadline only: chain stops at second step alive" ) {
        // step 1 ends at turn+10m, step 2 chained at turn+10m ends at turn+20m.
        // Now at turn+15m: step 2 is mid-flight, not overdue.
        craft_resolve_overdue_passive( on_map, calendar::turn + 15_minutes, loc );

        CHECK( on_map.get_current_step() == 2 );
        CHECK( on_map.get_passive_started_at() == calendar::turn + 10_minutes );
        CHECK( on_map.get_ready_at() == calendar::turn + 20_minutes );
    }
}

TEST_CASE( "chained_unattended_step_stays_live_without_resolvable_crafter",
           "[craft][attention][overdue][chain]" )
{
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_consecutive_unattended.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );

    // Cure A (idx 1) in flight with Cure B (idx 2) unattended behind it, so
    // closing Cure A has to chain into another passive step.
    on_map.set_current_step( 1 );
    on_map.set_passive_started_at( calendar::turn );
    on_map.set_ready_at( calendar::turn + 10_minutes );
    on_map.set_passive_start_counter( 2500000 );
    on_map.set_passive_end_counter( 5000000 );
    on_map.set_step_plans( std::vector<attention_plan>( 4 ) );
    // Crafter cannot be looked up: an NPC that left the bubble, or a craft
    // that never got a crafter stamped onto it.
    on_map.set_crafter_id( character_id() );
    REQUIRE_FALSE( on_map.get_crafter_id().is_valid() );

    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "an overdue unattended step whose crafter cannot be resolved" ) {
        WHEN( "the ready dispatch runs" ) {
            craft_actualize_scheduled( on_map, item_wakeup_kind::ready_check,
                                       calendar::turn + 10_minutes, loc );
            REQUIRE( loc.get_item() != nullptr );

            THEN( "the craft still carries a completion deadline" ) {
                CAPTURE( on_map.get_current_step() );
                CHECK( on_map.get_ready_at() != calendar::before_time_starts );
            }

            // Equal bounds switch the tname projection off, which pins the
            // displayed percentage to the step that just closed.
            THEN( "progress bounds still span a range" ) {
                CAPTURE( on_map.get_current_step() );
                CAPTURE( on_map.get_passive_start_counter() );
                CAPTURE( on_map.get_passive_end_counter() );
                CHECK( on_map.get_passive_end_counter() >
                       on_map.get_passive_start_counter() );
            }

            THEN( "waiting out the remaining passive time reaches the active step" ) {
                craft_resolve_overdue_passive( on_map, calendar::turn + 3_hours, loc );
                REQUIRE( loc.get_item() != nullptr );
                CHECK( on_map.get_current_step() == 3 );
            }
        }
    }
}

TEST_CASE( "chained_unattended_step_holds_for_absent_npc_crafter",
           "[craft][attention][overdue][chain]" )
{
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_consecutive_unattended.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );

    on_map.set_current_step( 1 );
    on_map.set_passive_started_at( calendar::turn );
    on_map.set_ready_at( calendar::turn + 10_minutes );
    on_map.set_passive_start_counter( 2500000 );
    on_map.set_passive_end_counter( 5000000 );
    on_map.set_step_plans( std::vector<attention_plan>( 4 ) );
    // A recorded crafter who is not the avatar and is not a loaded NPC: an
    // NPC that walked out of the reality bubble mid-craft.
    const character_id absent_npc( u.getID().get_value() + 1000 );
    on_map.set_crafter_id( absent_npc );
    REQUIRE( on_map.get_crafter_id().is_valid() );

    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "an overdue unattended step whose crafter is an absent NPC" ) {
        WHEN( "the ready dispatch runs" ) {
            craft_actualize_scheduled( on_map, item_wakeup_kind::ready_check,
                                       calendar::turn + 10_minutes, loc );
            REQUIRE( loc.get_item() != nullptr );

            THEN( "the step is held rather than advanced" ) {
                CHECK( on_map.get_current_step() == 1 );
            }

            THEN( "a retry deadline keeps the craft reachable" ) {
                CHECK( on_map.get_ready_at() == calendar::turn + 11_minutes );
            }
        }
    }
}

TEST_CASE( "craft_actualize_ready_fail_at_precedes_ready",
           "[craft][attention][overdue][fail]" )
{
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_timeout_recipe.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );

    on_map.set_current_step( 1 );
    on_map.set_passive_started_at( calendar::turn );
    on_map.set_ready_at( calendar::turn + 10_minutes );
    on_map.set_fail_at( calendar::turn + 25_minutes );
    on_map.set_crafter_id( u.getID() );
    std::vector<attention_plan> plans( 2 );
    on_map.set_step_plans( plans );

    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    craft_resolve_overdue_passive( on_map, calendar::turn + 30_minutes, loc );

    CHECK( loc.get_item() == nullptr );
}

TEST_CASE( "craft_stamp_passive_entry_batch_scales_fail_at",
           "[craft][attention][fail][batch]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    const recipe &rec = recipe_cudgel_test_timeout_recipe.obj();

    // Single unit: ready_at and fail_at match the per-unit values.
    SECTION( "batch of one is unchanged" ) {
        item ingredient( itype_2x4, calendar::turn );
        item placed( &rec, 1, ingredient );
        item &on_map = here.add_item( origin, placed );
        REQUIRE( on_map.is_craft() );
        REQUIRE( on_map.get_making_batch_size() == 1 );
        on_map.set_current_step( 1 );
        on_map.set_crafter_id( u.getID() );
        on_map.set_step_plans( std::vector<attention_plan>( 2 ) );

        item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );
        craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

        // Cure: time 10m -> ready; max_time 20m + grace 5m -> fail 25m.
        CHECK( on_map.get_ready_at() == calendar::turn + 10_minutes );
        CHECK( on_map.get_fail_at() == calendar::turn + 25_minutes );
    }

    // Batch of eight (no batch_time_factors -> none -> x8):
    //   ready_at = entry + 10m*8 = 80m
    //   fail_at  = entry + (20m + 5m)*8 = 200m
    SECTION( "batch of eight scales both deadlines, fail stays after ready" ) {
        const int batch = 8;
        item ingredient( itype_2x4, calendar::turn );
        item placed( &rec, batch, ingredient );
        item &on_map = here.add_item( origin, placed );
        REQUIRE( on_map.is_craft() );
        REQUIRE( on_map.get_making_batch_size() == batch );
        on_map.set_current_step( 1 );
        on_map.set_crafter_id( u.getID() );
        on_map.set_step_plans( std::vector<attention_plan>( 2 ) );

        item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );
        craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

        CHECK( on_map.get_ready_at() == calendar::turn + 80_minutes );
        CHECK( on_map.get_fail_at() == calendar::turn + 200_minutes );
        // Ruin deadline must stay strictly after completion.
        CHECK( on_map.get_fail_at() > on_map.get_ready_at() );
    }
}

TEST_CASE( "craft_batch_completes_instead_of_vanishing",
           "[craft][attention][fail][batch]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    const recipe &rec = recipe_cudgel_test_timeout_recipe.obj();
    const int batch = 8;
    item ingredient( itype_2x4, calendar::turn );
    item placed( &rec, batch, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 1 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 2 ) );

    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );
    craft_stamp_passive_entry( on_map, u, calendar::turn, loc );
    get_item_wakeups().rebuild_for_item( loc );

    // Past the batch-scaled ready_at (80m) but well before the scaled fail_at
    // (200m): the step finalizes and spawns the result rather than being
    // destroyed by a ruin deadline that elapsed before completion.
    craft_resolve_overdue_passive( on_map, calendar::turn + 81_minutes, loc );

    bool found_cudgel = false;
    for( const item &it : here.i_at( origin ) ) {
        if( it.typeId() == itype_cudgel ) {
            found_cudgel = true;
        }
    }
    CHECK( found_cudgel );
}

TEST_CASE( "craft_terminal_unattended_liquid_parks_for_collection",
           "[craft][attention][liquid]" )
{
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms craft_pos( 60, 60, 0 );
    // Avatar on the craft tile: a liquid step still parks (collection is always
    // explicit, so proximity does not gate it).
    u.setpos( here, craft_pos );

    SECTION( "liquid result parks at full progress instead of finalizing" ) {
        item ingredient( itype_water, calendar::turn );
        item placed( &recipe_water_clean_test_unattended_liquid.obj(), 1, ingredient );
        item &on_map = here.add_item( craft_pos, placed );
        REQUIRE( on_map.is_craft() );

        on_map.set_current_step( 0 );
        on_map.set_passive_started_at( calendar::turn );
        on_map.set_ready_at( calendar::turn + 10_minutes );
        on_map.set_crafter_id( u.getID() );
        std::vector<attention_plan> plans( 1 );
        on_map.set_step_plans( plans );

        item_location loc( map_cursor( here.get_abs( craft_pos ) ), &on_map );
        get_item_wakeups().rebuild_for_item( loc );
        const int64_t uid = on_map.uid().get_value();

        // Finalizing a liquid here would hit the pour prompt and abort the test
        // (cata_assert( !test_mode ) in uilist); parking is what prevents that.
        craft_resolve_overdue_passive( on_map, calendar::turn + 10_minutes + 1_turns, loc );

        REQUIRE( loc.get_item() != nullptr );
        item *parked = loc.get_item();
        CHECK( parked->is_craft() );
        CHECK( parked->tname().find( "100%" ) != std::string::npos );
        // Parked, so no ready wakeup remains to re-fire.
        CHECK_FALSE( get_item_wakeups().is_scheduled( uid, item_wakeup_kind::ready_check ) );
    }

    SECTION( "solid result still finalizes when overdue" ) {
        item ingredient( itype_2x4, calendar::turn );
        item placed( &recipe_cudgel_test_only_unattended.obj(), 1, ingredient );
        item &on_map = here.add_item( craft_pos, placed );
        REQUIRE( on_map.is_craft() );

        on_map.set_current_step( 0 );
        on_map.set_passive_started_at( calendar::turn );
        on_map.set_ready_at( calendar::turn + 10_minutes );
        on_map.set_crafter_id( u.getID() );
        std::vector<attention_plan> plans( 1 );
        on_map.set_step_plans( plans );

        item_location loc( map_cursor( here.get_abs( craft_pos ) ), &on_map );

        craft_resolve_overdue_passive( on_map, calendar::turn + 10_minutes + 1_turns, loc );

        // Non-liquid result finalizes normally; the parking gate is liquid-only.
        CHECK( loc.get_item() == nullptr );
    }
}

TEST_CASE( "craft_stamp_passive_entry_carries_step_progress_fraction",
           "[craft][attention][migration]" )
{
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_simple.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );

    on_map.set_current_step( 1 );
    std::vector<attention_plan> plans( 2 );
    on_map.set_step_plans( plans );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    SECTION( "no prior progress: fresh full-duration stamp" ) {
        on_map.set_step_progress( 0.0 );
        craft_stamp_passive_entry( on_map, u, calendar::turn, loc );
        CHECK( on_map.get_passive_started_at() == calendar::turn );
        CHECK( on_map.get_ready_at() > calendar::turn );
    }

    SECTION( "half-step prior progress: passive entry back-dated half-step" ) {
        const recipe &rec = recipe_cudgel_test_unattended_simple.obj();
        const crafting_cost_context ctx = crafting_cost_context::for_recipe( u, rec );
        const double active_budget = rec.step_budget_moves(
                                         u, 1, on_map.get_making_batch_size(), ctx );
        REQUIRE( active_budget > 0.0 );
        on_map.set_step_progress( active_budget * 0.5 );

        craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

        // Entry back-dated; full passive duration preserved.
        CHECK( on_map.get_passive_started_at() < calendar::turn );
        const time_duration step_dur = on_map.get_ready_at() -
                                       on_map.get_passive_started_at();
        const time_duration elapsed = calendar::turn -
                                      on_map.get_passive_started_at();
        CHECK( elapsed > 0_seconds );
        CHECK( elapsed < step_dur );
    }

    SECTION( "step_progress at or beyond budget: clamped to full step elapsed" ) {
        const recipe &rec = recipe_cudgel_test_unattended_simple.obj();
        const crafting_cost_context ctx = crafting_cost_context::for_recipe( u, rec );
        const double active_budget = rec.step_budget_moves(
                                         u, 1, on_map.get_making_batch_size(), ctx );
        REQUIRE( active_budget > 0.0 );
        on_map.set_step_progress( active_budget * 2.0 );

        craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

        // Fraction clamps at 1.0; ready_at lands at now.
        CHECK( on_map.get_ready_at() == calendar::turn );
    }
}

TEST_CASE( "craft_tname_freezes_projection_during_env_pause",
           "[craft][attention][display]" )
{
    item ingredient( itype_2x4, calendar::turn );
    item built( &recipe_cudgel_test_unattended_simple.obj(), 1, ingredient );
    REQUIRE( built.is_craft() );

    const time_point t0 = calendar::turn;
    built.item_counter = 0;
    built.set_passive_start_counter( 1000000 );
    built.set_passive_end_counter( 9000000 );

    // Entered 5m ago, paused 2m ago.  Elapsed clamps to 3m of 10m = 30%
    // of the 1M..9M window -> 3.4M -> 34%.
    built.set_passive_started_at( t0 - 5_minutes );
    built.set_pause_started_at( t0 - 2_minutes );
    built.set_ready_at( t0 + 1_minutes );
    built.set_saved_ready_at( t0 + 5_minutes );

    const std::string name = built.tname();
    CHECK( name.find( "34%" ) != std::string::npos );
}

TEST_CASE( "craft_terminal_removal_cancels_pending_wakeups",
           "[craft][attention][wakeup]" )
{
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_timeout_recipe.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );

    on_map.set_current_step( 1 );
    on_map.set_passive_started_at( calendar::turn );
    on_map.set_ready_at( calendar::turn + 10_minutes );
    on_map.set_fail_at( calendar::turn + 25_minutes );
    on_map.set_crafter_id( u.getID() );
    std::vector<attention_plan> plans( 2 );
    plans[1].choice = step_choice::set_timer;
    plans[1].alarm_offset = 5_minutes;
    on_map.set_step_plans( plans );

    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );
    on_map.set_alarm_at( calendar::turn + 5_minutes );
    get_item_wakeups().rebuild_for_item( loc );
    const int64_t uid = on_map.uid().get_value();
    REQUIRE( get_item_wakeups().is_scheduled( uid, item_wakeup_kind::ready_check ) );
    REQUIRE( get_item_wakeups().is_scheduled( uid, item_wakeup_kind::fail_check ) );
    REQUIRE( get_item_wakeups().is_scheduled( uid, item_wakeup_kind::alarm ) );

    craft_resolve_overdue_passive( on_map, calendar::turn + 30_minutes, loc );

    REQUIRE( loc.get_item() == nullptr );
    CHECK_FALSE( get_item_wakeups().is_scheduled( uid, item_wakeup_kind::ready_check ) );
    CHECK_FALSE( get_item_wakeups().is_scheduled( uid, item_wakeup_kind::fail_check ) );
    CHECK_FALSE( get_item_wakeups().is_scheduled( uid, item_wakeup_kind::alarm ) );
}

TEST_CASE( "craft_env_unpause_alarm_clears_when_already_due",
           "[craft][attention][resume][alarm]" )
{
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_simple.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );

    on_map.set_current_step( 1 );
    on_map.set_crafter_id( u.getID() );
    std::vector<attention_plan> plans( 2 );
    plans[1].choice = step_choice::set_timer;
    plans[1].alarm_offset = 5_minutes;
    on_map.set_step_plans( plans );

    // saved_alarm_at < pause_started_at: slid alarm_at lands in the past
    // after restore; must fire inline.
    const time_point t0 = calendar::turn;
    on_map.set_passive_started_at( t0 );
    on_map.set_ready_at( t0 + 1_minutes );
    on_map.set_saved_ready_at( t0 + 30_minutes );
    on_map.set_pause_started_at( t0 + 5_minutes );
    on_map.set_saved_alarm_at( t0 + 2_minutes );
    on_map.set_alarm_at( calendar::before_time_starts );

    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    craft_actualize_scheduled( on_map, item_wakeup_kind::ready_check,
                               t0 + 10_minutes, loc );

    CHECK( on_map.get_alarm_at() == calendar::before_time_starts );
    CHECK( on_map.get_pause_started_at() == calendar::before_time_starts );
}

TEST_CASE( "craft_stamp_arms_env_check_when_step_has_env_requirements",
           "[craft][attention][env_check]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    u.i_add( item( itype_microwave, calendar::turn ) );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_with_qual.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 1 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 2 ) );

    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );
    craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

    // Step has OVEN quality requirement -> env_check_at armed at now+1min,
    // clamped under ready_at.
    REQUIRE( on_map.get_passive_started_at() == calendar::turn );
    CHECK( on_map.get_env_check_at() != calendar::before_time_starts );
    CHECK( on_map.get_env_check_at() <= on_map.get_ready_at() );
    CHECK( get_item_wakeups().is_scheduled( on_map.uid().get_value(),
                                            item_wakeup_kind::env_check ) );
}

TEST_CASE( "craft_stamp_arms_env_check_for_a_grounded_step_with_no_env_requirements",
           "[craft][attention][env_check]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_simple.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 1 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 2 ) );

    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );
    craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

    REQUIRE( on_map.get_passive_started_at() == calendar::turn );
    // A grounded step holds a craft-site lock even with nothing to bind, and the lock
    // needs a poll to refresh it before the lease expires.
    CHECK( on_map.get_reserved_tile() == here.get_abs( origin ) );
    CHECK( on_map.get_env_check_at() != calendar::before_time_starts );
    CHECK( get_item_wakeups().is_scheduled( on_map.uid().get_value(),
                                            item_wakeup_kind::env_check ) );
}
TEST_CASE( "craft_stamp_arms_env_check_and_debits_entry_for_charged_alloc",
           "[craft][attention][charge][env_check]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    u.i_add( tool_with_ammo( itype_soldering_iron_portable, 50 ) );
    u.invalidate_crafting_inventory();

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_simple.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 1 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 2 ) );

    // A charged (root-derived) allocation on a step with no step-level env
    // requirements still needs metering.
    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::both;
    alloc.sel.comp.type = itype_soldering_iron_portable;
    alloc.sel.comp.count = 20;
    alloc.step_count_units = 20;
    alloc.root_derived = true;
    on_map.set_step_tool_allocs( { {}, { alloc } } );

    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );
    craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

    REQUIRE( on_map.get_passive_started_at() == calendar::turn );
    CHECK( on_map.get_env_check_at() != calendar::before_time_starts );
    CHECK( get_item_wakeups().is_scheduled( on_map.uid().get_value(),
                                            item_wakeup_kind::env_check ) );
    u.invalidate_crafting_inventory();
    CHECK( on_map.get_step_tool_allocs()[1][0].consumed_buckets >= 1 );
    CHECK( get_remaining_charges( itype_soldering_iron_portable ) < 50 );
}

TEST_CASE( "craft_env_check_rearms_for_charged_only_step",
           "[craft][attention][charge][env_check]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    u.i_add( tool_with_ammo( itype_soldering_iron_portable, 50 ) );
    u.invalidate_crafting_inventory();

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_simple.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    on_map.set_current_step( 1 );
    on_map.set_passive_started_at( calendar::turn );
    on_map.set_ready_at( calendar::turn + 10_minutes );
    on_map.set_crafter_id( u.getID() );

    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::both;
    alloc.sel.comp.type = itype_soldering_iron_portable;
    alloc.sel.comp.count = 20;
    alloc.step_count_units = 20;
    alloc.root_derived = true;
    on_map.set_step_tool_allocs( { {}, { alloc } } );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    // A mid-step env tick on a step with only a charged allocation (no
    // step-level env requirement) must keep polling so the drain continues.
    craft_actualize_scheduled( on_map, item_wakeup_kind::env_check,
                               calendar::turn + 5_minutes, loc );
    u.invalidate_crafting_inventory();

    CHECK( on_map.get_pause_started_at() == calendar::before_time_starts );
    CHECK( on_map.get_env_check_at() != calendar::before_time_starts );
    CHECK( get_item_wakeups().is_scheduled( on_map.uid().get_value(),
                                            item_wakeup_kind::env_check ) );
}

TEST_CASE( "craft_env_check_restore_rearms_for_charged_only_step",
           "[craft][attention][charge][env_check]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    u.i_add( tool_with_ammo( itype_soldering_iron_portable, 50 ) );
    u.invalidate_crafting_inventory();

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_simple.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    on_map.set_current_step( 1 );
    on_map.set_crafter_id( u.getID() );

    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::both;
    alloc.sel.comp.type = itype_soldering_iron_portable;
    alloc.sel.comp.count = 20;
    alloc.step_count_units = 20;
    alloc.root_derived = true;
    on_map.set_step_tool_allocs( { {}, { alloc } } );

    // Installed pause state, as if a prior shortfall paused the step.
    const time_point t0 = calendar::turn;
    on_map.set_passive_started_at( t0 - 2_minutes );
    on_map.set_ready_at( t0 + 1_minutes );
    on_map.set_saved_ready_at( t0 + 8_minutes );
    on_map.set_pause_started_at( t0 - 1_minutes );

    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    // Restoring from pause on a charged-only step must keep polling.
    craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0, loc );

    CHECK( on_map.get_pause_started_at() == calendar::before_time_starts );
    CHECK( on_map.get_env_check_at() != calendar::before_time_starts );
    CHECK( get_item_wakeups().is_scheduled( on_map.uid().get_value(),
                                            item_wakeup_kind::env_check ) );
}

TEST_CASE( "craft_env_check_dispatch_pauses_when_quality_missing",
           "[craft][attention][env_check]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    // No OVEN quality present: the quality gate fails and the step pauses.

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_with_qual.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 1 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 2 ) );

    const time_point t0 = calendar::turn;
    on_map.set_passive_started_at( t0 );
    on_map.set_ready_at( t0 + 10_minutes );

    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    const time_point fire_time = t0 + 1_minutes;
    craft_actualize_scheduled( on_map, item_wakeup_kind::env_check,
                               fire_time, loc );

    CHECK( on_map.get_pause_started_at() == fire_time );
    CHECK( on_map.get_saved_ready_at() == t0 + 10_minutes );
    CHECK( on_map.get_ready_at() == fire_time + 1_minutes );
    CHECK( on_map.get_env_check_at() == calendar::before_time_starts );
}

TEST_CASE( "craft_env_check_dispatch_restores_when_quality_returns",
           "[craft][attention][env_check]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_with_qual.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 1 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 2 ) );

    // Manually install a pause state, then satisfy env and dispatch env_check.
    const time_point t0 = calendar::turn;
    on_map.set_passive_started_at( t0 - 2_minutes );
    on_map.set_ready_at( t0 + 1_minutes ); // pause polling cursor
    on_map.set_saved_ready_at( t0 + 8_minutes );
    on_map.set_pause_started_at( t0 - 1_minutes );

    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    // Restore environment.
    u.i_add( item( itype_microwave, calendar::turn ) );
    craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0, loc );

    CHECK( on_map.get_pause_started_at() == calendar::before_time_starts );
    CHECK( on_map.get_ready_at() == t0 + 8_minutes + 1_minutes );
    CHECK( on_map.get_saved_ready_at() == calendar::before_time_starts );
    CHECK( on_map.get_env_check_at() != calendar::before_time_starts );
    CHECK( on_map.get_env_check_at() <= on_map.get_ready_at() );
}

TEST_CASE( "craft_env_check_dispatch_clamps_cursor_under_ready_at",
           "[craft][attention][env_check]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    u.i_add( item( itype_microwave, calendar::turn ) );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_with_qual.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 1 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 2 ) );

    // Short remaining: ready_at is 30s away.  Cursor must clamp to ready_at,
    // not now+1min which would poll past completion.
    const time_point t0 = calendar::turn;
    on_map.set_passive_started_at( t0 - 9_minutes - 30_seconds );
    on_map.set_ready_at( t0 + 30_seconds );

    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );
    craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0, loc );

    CHECK( on_map.get_env_check_at() == t0 + 30_seconds );
}

TEST_CASE( "craft_actualize_ready_via_helper_still_pauses_on_env_loss",
           "[craft][attention][env_check][regression]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_with_qual.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 1 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 2 ) );

    const time_point t0 = calendar::turn;
    on_map.set_passive_started_at( t0 );
    on_map.set_ready_at( t0 + 10_minutes );

    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    // ready_check at ready_at with no OVEN tool must pause the craft.
    craft_actualize_scheduled( on_map, item_wakeup_kind::ready_check,
                               t0 + 10_minutes, loc );

    CHECK( on_map.get_pause_started_at() == t0 + 10_minutes );
    CHECK( on_map.get_saved_ready_at() == t0 + 10_minutes );
    CHECK( on_map.get_ready_at() == t0 + 11_minutes );
    CHECK( on_map.get_env_check_at() == calendar::before_time_starts );
}

TEST_CASE( "craft_activity_do_wait_env_check_fires_per_turn",
           "[craft][attention][env_check][actor]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    // No OVEN: per-turn env_check inside do_wait branch must trip pause.

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_with_qual.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 1 );
    on_map.set_crafter_id( u.getID() );
    std::vector<attention_plan> plans( 2 );
    plans[1].choice = step_choice::do_wait;
    on_map.set_step_plans( plans );

    const time_point t0 = calendar::turn;
    on_map.set_passive_started_at( t0 );
    on_map.set_ready_at( t0 + 10_minutes );

    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );
    REQUIRE( on_map.get_pause_started_at() == calendar::before_time_starts );

    craft_activity_actor craft_actor( loc, /*is_long=*/false );
    u.activity = player_activity( craft_actor );
    u.activity.targets.push_back( loc );

    u.activity.do_turn( u );

    CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
    CHECK( on_map.get_saved_ready_at() == t0 + 10_minutes );
}

TEST_CASE( "reconcile_walks_avatar_inventory_for_env_check",
           "[craft][attention][env_check][reconcile]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    u.setpos( get_map(), tripoint_bub_ms( 60, 60, 0 ) );

    item ingredient( itype_2x4, calendar::turn );
    item carried( &recipe_cudgel_test_unattended_with_qual.obj(), 1, ingredient );
    carried.set_current_step( 1 );
    carried.set_crafter_id( u.getID() );
    carried.set_step_plans( std::vector<attention_plan>( 2 ) );

    const time_point t0 = calendar::turn;
    carried.set_passive_started_at( t0 );
    carried.set_ready_at( t0 + 10_minutes );
    carried.set_env_check_at( t0 + 1_minutes );

    item_location placed = u.i_add( carried );
    REQUIRE( placed );
    REQUIRE( placed->is_craft() );
    const int64_t uid = placed->uid().get_value();

    // Clear any schedule that i_add may have triggered, then exercise the
    // same iteration map::reconcile_loaded_items uses for character inventory
    // (Character::all_items_loc() + rebuild_for_item per location).
    get_item_wakeups().cancel_all( uid );
    REQUIRE_FALSE( get_item_wakeups().is_scheduled( uid, item_wakeup_kind::env_check ) );

    for( item_location &loc : u.all_items_loc() ) {
        if( loc && loc.get_item() != nullptr ) {
            get_item_wakeups().rebuild_for_item( loc );
        }
    }

    CHECK( get_item_wakeups().is_scheduled( uid, item_wakeup_kind::env_check ) );
    CHECK( get_item_wakeups().is_scheduled( uid, item_wakeup_kind::ready_check ) );
}

TEST_CASE( "compute_inflight_alarm_choices_for_resume_timer_modal",
           "[craft][attention][modal]" )
{
    const time_point started = calendar::turn_zero;

    GIVEN( "a 10-minute step with ready_at at started+10min" ) {
        const time_point ready = started + 10_minutes;

        WHEN( "8 minutes have passed (2 minutes remaining)" ) {
            const inflight_alarm_choices c = compute_inflight_alarm_choices(
                                                 started, ready, started + 8_minutes );
            THEN( "finish is offered but five-before is disabled" ) {
                CHECK( c.remaining == 2_minutes );
                CHECK( c.finish_enabled );
                CHECK_FALSE( c.five_before_enabled );
            }
            THEN( "finish offset resolves to ready_at when added to step start" ) {
                REQUIRE( c.finish_offset.has_value() );
                CHECK( started + *c.finish_offset == ready );
                CHECK_FALSE( c.five_before_offset.has_value() );
            }
        }

        WHEN( "3 minutes have passed (7 minutes remaining)" ) {
            const inflight_alarm_choices c = compute_inflight_alarm_choices(
                                                 started, ready, started + 3_minutes );
            THEN( "both finish and five-before choices are enabled" ) {
                CHECK( c.remaining == 7_minutes );
                CHECK( c.finish_enabled );
                CHECK( c.five_before_enabled );
            }
            THEN( "both offsets are step-start-anchored and resolve to ready_at and ready_at - 5min" ) {
                REQUIRE( c.finish_offset.has_value() );
                REQUIRE( c.five_before_offset.has_value() );
                CHECK( started + *c.finish_offset == ready );
                CHECK( started + *c.five_before_offset == ready - 5_minutes );
            }
        }

        WHEN( "12 minutes have passed (step is overdue)" ) {
            const inflight_alarm_choices c = compute_inflight_alarm_choices(
                                                 started, ready, started + 12_minutes );
            THEN( "no timer choices are offered" ) {
                CHECK( c.remaining == -2_minutes );
                CHECK_FALSE( c.finish_enabled );
                CHECK_FALSE( c.five_before_enabled );
                CHECK_FALSE( c.finish_offset.has_value() );
                CHECK_FALSE( c.five_before_offset.has_value() );
            }
        }
    }

    GIVEN( "a paused step whose live_ready_at has been slid forward" ) {
        // live_ready_at = passive_started_at + slid duration via env pause.
        const time_point ready = started + 15_minutes;

        WHEN( "evaluated 6 minutes after step start" ) {
            const inflight_alarm_choices c = compute_inflight_alarm_choices(
                                                 started, ready, started + 6_minutes );
            THEN( "finish offset stays step-start-anchored, not slid-ready-anchored" ) {
                REQUIRE( c.finish_offset.has_value() );
                CHECK( *c.finish_offset == 15_minutes );
                CHECK( started + *c.finish_offset == ready );
            }
        }
    }

    GIVEN( "a step env-paused at minute 4 of an originally 10-minute deadline" ) {
        const time_point saved_ready_at = started + 10_minutes;
        const time_point pause_started = started + 4_minutes;

        WHEN( "helper is called with eval_now = pause_started" ) {
            const inflight_alarm_choices c = compute_inflight_alarm_choices(
                                                 started, saved_ready_at, pause_started );
            THEN( "remaining reflects the time left at the moment of pause" ) {
                CHECK( c.remaining == 6_minutes );
                CHECK( c.finish_enabled );
                CHECK( c.five_before_enabled );
            }
            THEN( "offsets stay step-start-anchored against saved_ready_at" ) {
                REQUIRE( c.finish_offset.has_value() );
                REQUIRE( c.five_before_offset.has_value() );
                CHECK( started + *c.finish_offset == saved_ready_at );
                CHECK( started + *c.five_before_offset == saved_ready_at - 5_minutes );
            }
        }
    }
}

TEST_CASE( "provider_quality_level_ignores_merely_contained_items",
           "[craft][attention][reservation][quality]" )
{
    clear_avatar();
    clear_map();

    GIVEN( "a qualifying tool inside a backpack" ) {
        item backpack( itype_backpack );
        item tool( itype_test_reserve_tool_a );
        REQUIRE( tool.get_quality( qual_TEST_RESERVE_A ) >= 1 );
        REQUIRE( backpack.put_in( tool, pocket_type::CONTAINER ).success() );

        THEN( "the backpack is credited with the tool's quality by the recursive accessor" ) {
            CHECK( backpack.get_quality( qual_TEST_RESERVE_A ) >= 1 );
        }

        THEN( "but it supplies none of that quality in its own right" ) {
            CHECK( provider_quality_level( backpack, qual_TEST_RESERVE_A,
                                           nullptr, true ) < 1 );
        }

        THEN( "the contained tool still supplies it" ) {
            const item &nested = *backpack.all_items_top( pocket_type::CONTAINER ).front();
            CHECK( provider_quality_level( nested, qual_TEST_RESERVE_A,
                                           nullptr, true ) >= 1 );
        }
    }

    GIVEN( "a plain tool" ) {
        item tool( itype_test_reserve_tool_a );

        THEN( "it supplies its own quality" ) {
            CHECK( provider_quality_level( tool, qual_TEST_RESERVE_A, nullptr,
                                           true ) >= 1 );
        }
    }

    GIVEN( "a pot, which carries BOIL" ) {
        item pot( itype_pot );

        THEN( "it supplies BOIL while empty, which is what a boil step binds" ) {
            CHECK( provider_quality_level( pot, qual_BOIL, nullptr, true ) >= 1 );
        }

        WHEN( "it is filled" ) {
            item water( itype_water, calendar::turn, 1 );
            REQUIRE( pot.put_in( water, pocket_type::CONTAINER ).success() );

            THEN( "it no longer supplies BOIL under strict boiling" ) {
                CHECK( provider_quality_level( pot, qual_BOIL, nullptr,
                                               true ) < 1 );
            }
        }
    }
}

TEST_CASE( "requirement_gate_counts_distinct_quality_providers",
           "[craft][attention][reservation][enforcement][quality][counting]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    const requirement_data &req =
        recipe_cudgel_test_unattended_two_of_a.obj().steps()[0].requirements;
    REQUIRE( req.get_qualities().size() == 1 );
    REQUIRE( req.get_qualities()[0][0].count == 2 );

    GIVEN( "one qualifying tool inside a container" ) {
        item bag( itype_backpack );
        REQUIRE( bag.put_in( item( itype_test_reserve_tool_a ), pocket_type::CONTAINER ).success() );
        here.add_item( origin, bag );
        u.invalidate_crafting_inventory();
        const inventory &crafting_inv = u.crafting_inventory();

        THEN( "the container is not credited beside the tool it holds" ) {
            CHECK_FALSE( req.can_make_with_inventory( &u, crafting_inv, return_true<item> ) );
        }

        THEN( "the default metric still counts them both" ) {
            CHECK( crafting_inv.has_quality( qual_TEST_RESERVE_A, 1, 2 ) );
        }
    }

    GIVEN( "one stack of a charge-counted qualifying item" ) {
        item stack( itype_test_reserve_charge_stack );
        stack.charges = 100;
        here.add_item( origin, stack );
        u.invalidate_crafting_inventory();
        const inventory &crafting_inv = u.crafting_inventory();

        THEN( "the stack is one provider rather than its charge count" ) {
            CHECK_FALSE( req.can_make_with_inventory( &u, crafting_inv, return_true<item> ) );
        }

        THEN( "the default metric still counts its charges" ) {
            CHECK( crafting_inv.has_quality( qual_TEST_RESERVE_A, 1, 2 ) );
        }
    }

    GIVEN( "two loose qualifying tools" ) {
        here.add_item( origin, item( itype_test_reserve_tool_a ) );
        here.add_item( origin, item( itype_test_reserve_tool_a ) );
        u.invalidate_crafting_inventory();
        const inventory &crafting_inv = u.crafting_inventory();

        THEN( "two genuine providers satisfy the requirement" ) {
            CHECK( req.can_make_with_inventory( &u, crafting_inv, return_true<item> ) );
        }
    }
}

TEST_CASE( "intrinsic_qualities_count_provider_occurrences",
           "[craft][attention][reservation][binding][intrinsic]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    GIVEN( "a bionic exposing two pseudo items of one quality" ) {
        u.add_bionic( test_bio_reserve_two_pseudo );

        THEN( "each pseudo item is its own occurrence" ) {
            CHECK( u.has_intrinsic_quality( qual_TEST_RESERVE_A, 1, 2 ) );
        }

        THEN( "a third occurrence is not invented" ) {
            CHECK_FALSE( u.has_intrinsic_quality( qual_TEST_RESERVE_A, 1, 3 ) );
        }

    }

    GIVEN( "a bionic whose weapon supplies a quality" ) {
        u.add_bionic( test_bio_reserve_weapon );

        THEN( "it supplies one occurrence while unpowered" ) {
            CHECK( u.has_intrinsic_quality( qual_TEST_RESERVE_B, 1, 1 ) );
        }

        THEN( "the direct and pseudo routes are not counted twice" ) {
            CHECK_FALSE( u.has_intrinsic_quality( qual_TEST_RESERVE_B, 1, 2 ) );
        }

        WHEN( "the bionic is powered" ) {
            bionic &bio = u.bionic_at_index( u.get_bionics().size() - 1 );
            bio.powered = true;

            THEN( "the exposed weapon is still the occurrence it already was" ) {
                CHECK( u.has_intrinsic_quality( qual_TEST_RESERVE_B, 1, 1 ) );
                CHECK_FALSE( u.has_intrinsic_quality( qual_TEST_RESERVE_B, 1, 2 ) );
            }
        }
    }

    GIVEN( "a bionic that exposes its pseudo tool only while it is on" ) {
        u.add_bionic( test_bio_reserve_toggled_pseudo );
        bionic &bio = u.bionic_at_index( u.get_bionics().size() - 1 );

        WHEN( "the bionic is off" ) {
            REQUIRE_FALSE( bio.powered );

            THEN( "it supplies no occurrence" ) {
                CHECK_FALSE( u.has_intrinsic_quality( qual_TEST_RESERVE_A, 1, 1 ) );
            }
        }

        WHEN( "the bionic is on" ) {
            bio.powered = true;

            THEN( "the pseudo tool counts" ) {
                CHECK( u.has_intrinsic_quality( qual_TEST_RESERVE_A, 1, 1 ) );
            }
        }
    }

    GIVEN( "both digging traits at once" ) {
        u.set_mutation( trait_BURROW );
        u.set_mutation( trait_BURROWLARGE );
        // Of the innate pair only the shovel carries a DIG quality; the pickaxe digs
        // through its use action.  One occurrence is therefore the whole supply.
        THEN( "the innate pair is granted once, not once per trait" ) {
            CHECK( u.has_intrinsic_quality( qual_DIG, 1, 1 ) );
            CHECK_FALSE( u.has_intrinsic_quality( qual_DIG, 1, 2 ) );
        }
    }
}

TEST_CASE( "reservation_index_expiry_decides_visibility", "[craft][attention][reservation]" )
{
    clear_avatar();
    clear_map();
    craft_reservation_index &idx = get_craft_reservations();

    // calendar::turn is never mutated; leases are set relative to it instead.
    const time_point now = calendar::turn;
    const time_point live_until = now + 1_hours;
    const time_point lapsed_at = now - 1_minutes;

    GIVEN( "a live record claiming an item" ) {
        idx.set( make_item_record( 100, 4242, live_until ) );

        THEN( "the item reads as reserved" ) {
            CHECK( idx.is_reserved_uid( 4242 ) );
        }

        WHEN( "the record is erased" ) {
            idx.erase( 100 );

            THEN( "the item is free again" ) {
                CHECK_FALSE( idx.is_reserved_uid( 4242 ) );
            }
        }
    }

    GIVEN( "a record whose lease has already lapsed" ) {
        idx.set( make_item_record( 100, 4242, lapsed_at ) );

        THEN( "it claims nothing, because an expired record is stored but never indexed" ) {
            CHECK_FALSE( idx.is_reserved_uid( 4242 ) );
        }

        THEN( "it is still findable, so a later refresh and the sweep can both reach it" ) {
            CHECK( idx.find( 100 ) != nullptr );
        }
    }

    GIVEN( "an expired incumbent that the sweep has not yet removed" ) {
        idx.set( make_item_record( 100, 4242, live_until ) );
        REQUIRE( idx.is_reserved_uid( 4242 ) );
        // Nothing runs at the deadline, so the mapping survives until the sweep.
        idx.set( make_item_record( 100, 4242, lapsed_at ) );
        REQUIRE_FALSE( idx.is_reserved_uid( 4242 ) );

        WHEN( "another craft binds the same item" ) {
            idx.set( make_item_record( 200, 4242, live_until ) );

            THEN( "the newcomer owns it, rather than being blocked by a dead mapping" ) {
                CHECK( idx.is_reserved_uid( 4242 ) );
                const craft_reservation_index::record *owner = idx.record_for_item_uid( 4242 );
                REQUIRE( owner != nullptr );
                CHECK( owner->craft_uid == 200 );
            }

            THEN( "the expired craft cannot claim it back" ) {
                CHECK( idx.item_claimed_by_other( 4242, 100 ) );
            }

            THEN( "the new owner does not consider itself blocked" ) {
                CHECK_FALSE( idx.item_claimed_by_other( 4242, 200 ) );
            }
        }
    }

    GIVEN( "a lapsed craft A and a live craft B holding the same item" ) {
        idx.set( make_item_record( 100, 4242, lapsed_at ) );
        idx.set( make_item_record( 200, 4242, live_until ) );
        REQUIRE( idx.record_for_item_uid( 4242 )->craft_uid == 200 );

        WHEN( "A is released" ) {
            idx.erase( 100 );

            THEN( "B still owns the item, since removal is conditional on ownership" ) {
                CHECK( idx.is_reserved_uid( 4242 ) );
                CHECK( idx.record_for_item_uid( 4242 )->craft_uid == 200 );
            }
        }
    }
}

TEST_CASE( "reservation_index_generation_tracks_visibility_only",
           "[craft][attention][reservation][perf]" )
{
    clear_avatar();
    clear_map();
    craft_reservation_index &idx = get_craft_reservations();

    const time_point now = calendar::turn;
    const time_point live_until = now + 1_hours;
    const time_point later = now + 2_hours;
    const time_point lapsed_at = now - 1_minutes;

    GIVEN( "a live record" ) {
        idx.set( make_item_record( 100, 4242, live_until ) );
        const uint64_t after_acquire = idx.generation();

        WHEN( "an identical record is rebuilt" ) {
            idx.set( make_item_record( 100, 4242, live_until ) );

            THEN( "the generation does not move" ) {
                CHECK( idx.generation() == after_acquire );
            }
        }

        WHEN( "only the lease is refreshed" ) {
            idx.set( make_item_record( 100, 4242, later ) );

            THEN( "the generation does not move, since nothing became visible or invisible" ) {
                CHECK( idx.generation() == after_acquire );
            }
        }

        WHEN( "the record is released" ) {
            idx.erase( 100 );

            THEN( "the generation moves" ) {
                CHECK( idx.generation() > after_acquire );
            }
        }
    }

    GIVEN( "a record that has expired" ) {
        idx.set( make_item_record( 100, 4242, lapsed_at ) );
        const uint64_t while_expired = idx.generation();

        WHEN( "it is refreshed back to live" ) {
            idx.set( make_item_record( 100, 4242, live_until ) );

            THEN( "the generation moves, because that is a not-reserved to reserved change" ) {
                CHECK( idx.generation() > while_expired );
            }
        }

        WHEN( "the expired records are swept" ) {
            idx.sweep_expired_records();

            THEN( "the generation does not move, since the sweep is pure bookkeeping" ) {
                CHECK( idx.generation() == while_expired );
            }

            THEN( "the record is physically gone" ) {
                CHECK( idx.find( 100 ) == nullptr );
            }
        }
    }
}

TEST_CASE( "reservation_tile_locks_are_tracked_separately",
           "[craft][attention][reservation][tile]" )
{
    clear_avatar();
    clear_map();
    map &here = get_map();
    craft_reservation_index &idx = get_craft_reservations();

    const time_point live_until = calendar::turn + 1_hours;
    const tripoint_abs_ms tile = here.get_abs( tripoint_bub_ms( 60, 60, 0 ) );

    GIVEN( "two crafts sharing one craft site" ) {
        craft_reservation_index::record first;
        first.craft_uid = 100;
        first.craft_tile = tile;
        first.expires_at = live_until;
        idx.set( first );

        craft_reservation_index::record second;
        second.craft_uid = 200;
        second.craft_tile = tile;
        second.expires_at = live_until;
        idx.set( second );

        REQUIRE( idx.craft_site_reserved( tile ) );

        WHEN( "one of them is released" ) {
            idx.erase( 100 );

            THEN( "the other still holds the tile, since the lock is a set of owners" ) {
                CHECK( idx.craft_site_reserved( tile ) );
            }
        }

        WHEN( "both are released" ) {
            idx.erase( 100 );
            idx.erase( 200 );

            THEN( "the tile is free" ) {
                CHECK_FALSE( idx.craft_site_reserved( tile ) );
            }
        }
    }

    GIVEN( "a craft site lock and no provider lock on the same tile" ) {
        craft_reservation_index::record rec;
        rec.craft_uid = 100;
        rec.craft_tile = tile;
        rec.expires_at = live_until;
        idx.set( rec );

        THEN( "provider-tile queries do not see it, so pseudo-tool filtering stays separate" ) {
            CHECK( idx.craft_site_reserved( tile ) );
            CHECK_FALSE( idx.provider_tile_reserved( tile ) );
        }
    }
}

TEST_CASE( "reservation_predicates_differ_on_ancestry", "[craft][attention][reservation]" )
{
    clear_avatar();
    clear_map();
    map &here = get_map();
    craft_reservation_index &idx = get_craft_reservations();

    const tripoint_bub_ms origin( 60, 60, 0 );
    item backpack( itype_backpack );
    item tool( itype_test_reserve_tool_a );
    REQUIRE( backpack.put_in( tool, pocket_type::CONTAINER ).success() );
    item &on_map = here.add_item( origin, backpack );
    REQUIRE( on_map.num_item_stacks() == 1 );

    item &nested = *on_map.all_items_top( pocket_type::CONTAINER ).front();
    const int64_t nested_uid = nested.uid().get_value();
    REQUIRE( nested_uid != 0 );

    GIVEN( "a reserved item nested inside a free container" ) {
        craft_reservation_index::record rec;
        rec.craft_uid = 100;
        rec.provider_item_uids.push_back( nested_uid );
        rec.expires_at = calendar::turn + 1_hours;
        idx.set( rec );

        THEN( "the ancestry predicate hides the whole container" ) {
            CHECK( craft_reservation::contains_reserved( on_map ) );
        }
    }
}

TEST_CASE( "craft_data_persists_reservation_fields", "[craft][attention][reservation][persist]" )
{
    clear_avatar();
    clear_map();
    map &here = get_map();

    item ingredient( itype_water, calendar::turn );
    item built( &recipe_water_clean_test_unattended_boil.obj(), 1, ingredient );
    REQUIRE( built.is_craft() );

    const tripoint_abs_ms tile = here.get_abs( tripoint_bub_ms( 60, 60, 0 ) );
    const time_point expiry = calendar::turn + 1_hours;

    craft_reservation::binding item_binding;
    item_binding.group_index = 0;
    item_binding.alternative_index = 0;
    item_binding.req = craft_reservation::requirement_kind::quality;
    item_binding.qual = qual_BOIL;
    item_binding.level = 1;
    item_binding.group_count = 1;
    item_binding.kind = craft_reservation::provider_kind::item;
    item_binding.provider_uid = 4242;

    craft_reservation::binding intrinsic_binding;
    intrinsic_binding.group_index = 0;
    intrinsic_binding.alternative_index = 0;
    intrinsic_binding.req = craft_reservation::requirement_kind::quality;
    intrinsic_binding.qual = qual_BOIL;
    intrinsic_binding.level = 1;
    intrinsic_binding.group_count = 1;
    intrinsic_binding.kind = craft_reservation::provider_kind::intrinsic;
    intrinsic_binding.intrinsic_owner = get_avatar().getID();
    intrinsic_binding.occurrence_slot = 0;

    built.set_reservations( { item_binding, intrinsic_binding } );
    built.set_reserved_tile( tile );
    built.set_reservation_expiry( expiry );
    built.set_reservation_search_attempts( 3 );
    built.set_reservation_pool_fingerprint( 0x1234abcdULL );
    built.set_reservation_pause_reason( 2 );
    const int64_t token = built.reservation_owner_token();
    REQUIRE( token != 0 );

    std::ostringstream ss;
    JsonOut jsout( ss );
    built.serialize( jsout );

    item restored;
    restored.deserialize( json_loader::from_string( ss.str() ).get_object() );
    REQUIRE( restored.is_craft() );

    CHECK( restored.peek_reservation_owner_token() == token );
    CHECK( restored.get_reserved_tile() == tile );
    CHECK( restored.get_reservation_expiry() == expiry );
    CHECK( restored.get_reservation_search_attempts() == 3 );
    CHECK( restored.get_reservation_pool_fingerprint() == 0x1234abcdULL );
    CHECK( restored.get_reservation_pause_reason() == 2 );

    REQUIRE( restored.get_reservations().size() == 2 );
    const craft_reservation::binding &r_item = restored.get_reservations()[0];
    CHECK( r_item.kind == craft_reservation::provider_kind::item );
    CHECK( r_item.provider_uid == 4242 );
    CHECK( r_item.qual == qual_BOIL );
    CHECK( r_item.level == 1 );
    CHECK( r_item.occurrence_slot == -1 );

    const craft_reservation::binding &r_intrinsic = restored.get_reservations()[1];
    CHECK( r_intrinsic.kind == craft_reservation::provider_kind::intrinsic );
    CHECK( r_intrinsic.intrinsic_owner == get_avatar().getID() );
    CHECK( r_intrinsic.occurrence_slot == 0 );
    CHECK( r_intrinsic.pseudo_type.is_null() );
}

TEST_CASE( "craft_data_omits_unset_reservation_fields", "[craft][attention][reservation][persist]" )
{
    item ingredient( itype_water, calendar::turn );
    item built( &recipe_water_clean_test_unattended_boil.obj(), 1, ingredient );
    REQUIRE( built.is_craft() );

    std::ostringstream ss;
    JsonOut jsout( ss );
    built.serialize( jsout );
    const std::string out = ss.str();

    CHECK( out.find( "reservations" ) == std::string::npos );
    CHECK( out.find( "reserved_tile" ) == std::string::npos );
    CHECK( out.find( "reservation_owner" ) == std::string::npos );
    CHECK( out.find( "reservation_expires_at" ) == std::string::npos );
}

TEST_CASE( "craft_data_reservation_owner_token_is_stable_and_unique",
           "[craft][attention][reservation][persist]" )
{
    item ingredient( itype_water, calendar::turn );
    item first( &recipe_water_clean_test_unattended_boil.obj(), 1, ingredient );
    item second( &recipe_water_clean_test_unattended_boil.obj(), 1, ingredient );
    REQUIRE( first.is_craft() );
    REQUIRE( second.is_craft() );

    GIVEN( "a craft that has been asked for its token" ) {
        const int64_t token = first.reservation_owner_token();

        THEN( "asking again returns the same value" ) {
            CHECK( first.reservation_owner_token() == token );
        }

        THEN( "another craft gets a different one" ) {
            CHECK( second.reservation_owner_token() != token );
        }

        THEN( "the token survives the copy that picking a craft up performs" ) {
            // The copy is the subject: item_uid regenerates, the token must not.
            // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
            item copied( first );
            CHECK( copied.peek_reservation_owner_token() == token );
            CHECK( copied.uid().get_value() != first.uid().get_value() );
        }
    }

    GIVEN( "a craft nobody has claimed" ) {
        THEN( "peeking does not allocate" ) {
            CHECK( second.peek_reservation_owner_token() == 0 );
        }
    }
}

TEST_CASE( "reservation_index_rebuilds_from_craft_state",
           "[craft][attention][reservation][persist]" )
{
    clear_avatar();
    clear_map();
    map &here = get_map();
    craft_reservation_index &idx = get_craft_reservations();

    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_abs_ms abs_origin = here.get_abs( origin );
    const tripoint_abs_ms provider_tile = here.get_abs( tripoint_bub_ms( 61, 60, 0 ) );

    item ingredient( itype_water, calendar::turn );
    item placed( &recipe_water_clean_test_unattended_boil.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );

    craft_reservation::binding item_binding;
    item_binding.kind = craft_reservation::provider_kind::item;
    item_binding.provider_uid = 4242;

    // Two bindings, one provider: release is reference counted.
    craft_reservation::binding same_provider = item_binding;

    craft_reservation::binding furn_binding;
    furn_binding.kind = craft_reservation::provider_kind::furniture;
    furn_binding.tile = provider_tile;

    craft_reservation::binding shared_binding;
    shared_binding.kind = craft_reservation::provider_kind::environment;
    shared_binding.occurrence_slot = 0;

    on_map.set_reservations( { item_binding, same_provider, furn_binding, shared_binding } );
    on_map.set_reserved_tile( abs_origin );
    on_map.set_reservation_expiry( calendar::turn + 1_hours );
    const int64_t token = on_map.reservation_owner_token();

    item_location loc( map_cursor( abs_origin ), &on_map );
    idx.rebuild_for_craft( loc );

    THEN( "indexed providers are claimed" ) {
        CHECK( idx.is_reserved_uid( 4242 ) );
        CHECK( idx.provider_tile_reserved( provider_tile ) );
        CHECK( idx.craft_site_reserved( abs_origin ) );
    }

    THEN( "the duplicate binding collapses to one entry" ) {
        const craft_reservation_index::record *rec = idx.find( token );
        REQUIRE( rec != nullptr );
        CHECK( rec->provider_item_uids.size() == 1 );
    }

    THEN( "shared providers are recorded on the craft but never indexed" ) {
        const craft_reservation_index::record *rec = idx.find( token );
        REQUIRE( rec != nullptr );
        CHECK( rec->provider_part_uids.empty() );
        CHECK( on_map.get_reservations().size() == 4 );
    }

    THEN( "the record copies the craft's lease rather than minting one" ) {
        const craft_reservation_index::record *rec = idx.find( token );
        REQUIRE( rec != nullptr );
        CHECK( rec->expires_at == on_map.get_reservation_expiry() );
    }
}

TEST_CASE( "reservation_index_rebuild_honours_a_lapsed_lease",
           "[craft][attention][reservation][persist]" )
{
    clear_avatar();
    clear_map();
    map &here = get_map();
    craft_reservation_index &idx = get_craft_reservations();

    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_abs_ms abs_origin = here.get_abs( origin );

    item ingredient( itype_water, calendar::turn );
    item placed( &recipe_water_clean_test_unattended_boil.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );

    craft_reservation::binding b;
    b.kind = craft_reservation::provider_kind::item;
    b.provider_uid = 4242;
    on_map.set_reservations( { b } );
    on_map.set_reservation_expiry( calendar::turn - 1_minutes );
    on_map.reservation_owner_token();

    item_location loc( map_cursor( abs_origin ), &on_map );
    idx.rebuild_for_craft( loc );

    THEN( "the craft reclaims nothing, so load order cannot decide ownership" ) {
        CHECK_FALSE( idx.is_reserved_uid( 4242 ) );
    }
}

TEST_CASE( "reservation_index_retakes_a_claim_the_incumbent_released",
           "[craft][attention][reservation][lifecycle]" )
{
    clear_avatar();
    clear_map();

    const int64_t provider = 987654321;
    craft_reservation_index &idx = get_craft_reservations();

    craft_reservation_index::record incumbent;
    incumbent.craft_uid = 111;
    incumbent.expires_at = calendar::turn + 1_hours;
    incumbent.provider_item_uids.push_back( provider );

    craft_reservation_index::record loser;
    loser.craft_uid = 222;
    loser.expires_at = calendar::turn + 1_hours;
    loser.provider_item_uids.push_back( provider );

    GIVEN( "two live records naming one provider" ) {
        idx.set( incumbent );
        idx.set( loser );
        REQUIRE( idx.record_for_item_uid( provider ) != nullptr );
        REQUIRE( idx.record_for_item_uid( provider )->craft_uid == 111 );

        WHEN( "the incumbent releases and the loser refreshes unchanged" ) {
            idx.erase( 111 );
            REQUIRE_FALSE( idx.is_reserved_uid( provider ) );
            idx.set( loser );

            THEN( "the loser takes the claim rather than leaving it free" ) {
                CHECK( idx.is_reserved_uid( provider ) );
            }
        }
    }
}

TEST_CASE( "reservation_keeps_claims_a_shift_did_not_walk",
           "[craft][attention][reservation][lifecycle]" )
{
    clear_avatar();
    clear_map();
    map &here = get_map();

    GIVEN( "a claim held by a craft this bubble does not hold" ) {
        craft_reservation_index::record held;
        held.craft_uid = 424242;
        held.expires_at = calendar::turn + 1_hours;
        held.provider_item_uids.push_back( 987654321 );
        get_craft_reservations().set( held );
        REQUIRE( get_craft_reservations().is_reserved_uid( 987654321 ) );

        WHEN( "the map shifts" ) {
            here.shift( point_rel_sm::east );

            THEN( "the claim survives, since the shift walked a bubble without it" ) {
                CHECK( get_craft_reservations().is_reserved_uid( 987654321 ) );
            }
        }
    }
}

TEST_CASE( "craft_bindings_survive_a_batched_reload", "[craft][attention][reservation][persist]" )
{
    clear_avatar();
    clear_map();

    // operator* scales components and tool charges, not qualities.
    item ingredient( itype_water, calendar::turn );
    item built( &recipe_water_clean_test_unattended_boil.obj(), 4, ingredient );
    REQUIRE( built.is_craft() );

    const std::vector<std::vector<quality_requirement>> &quals =
                recipe_water_clean_test_unattended_boil.obj().steps()[0].requirements.get_qualities();
    REQUIRE( quals.size() == 1 );
    REQUIRE( quals[0][0].count == 1 );

    craft_reservation::binding b;
    b.group_index = 0;
    b.alternative_index = 0;
    b.req = craft_reservation::requirement_kind::quality;
    b.qual = quals[0][0].type;
    b.level = quals[0][0].level;
    b.group_count = quals[0][0].count;
    b.kind = craft_reservation::provider_kind::item;
    b.provider_uid = 4242;
    built.set_reservations( { b } );
    built.set_reservation_expiry( calendar::turn + 1_hours );

    std::ostringstream ss;
    JsonOut jsout( ss );
    built.serialize( jsout );

    item restored;
    restored.deserialize( json_loader::from_string( ss.str() ).get_object() );
    REQUIRE( restored.is_craft() );

    THEN( "the binding is kept rather than dropped as stale" ) {
        CHECK( restored.get_reservations().size() == 1 );
    }

    THEN( "the recorded count is the unscaled one" ) {
        REQUIRE( restored.get_reservations().size() == 1 );
        CHECK( restored.get_reservations()[0].group_count == 1 );
    }
}

TEST_CASE( "reserved_items_are_marked_in_use", "[craft][attention][reservation][ui]" )
{
    clear_avatar();
    clear_map();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );

    item &reserved = here.add_item( origin, item( itype_pot ) );
    item &spare = here.add_item( origin, item( itype_pot ) );

    craft_reservation_index::record rec;
    rec.craft_uid = 4242;
    rec.provider_item_uids.push_back( reserved.uid().get_value() );
    rec.expires_at = calendar::turn + 1_hours;
    get_craft_reservations().set( rec );

    THEN( "the reserved item is marked" ) {
        CHECK( reserved.tname().find( "in use" ) != std::string::npos );
    }

    THEN( "an identical free one beside it is not" ) {
        CHECK( spare.tname().find( "in use" ) == std::string::npos );
    }
}

TEST_CASE( "reserved_tiles_refuse_construction_from_a_record",
           "[craft][attention][reservation][tile]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms site_pos( 60, 60, 0 );
    const tripoint_bub_ms provider_pos( 61, 60, 0 );
    const tripoint_bub_ms free_pos( 63, 60, 0 );
    // check_empty refuses a tile a creature stands on, so keep the avatar off all three.
    u.setpos( here, tripoint_bub_ms( 55, 55, 0 ) );

    for( const tripoint_bub_ms &p : {
             site_pos, provider_pos, free_pos
         } ) {
        here.ter_set( p, ter_t_dirt );
    }
    const construction &con = construction_test_constr_pit_shallow.obj();
    REQUIRE( can_construct( con, site_pos ) );

    craft_reservation_index::record rec;
    rec.craft_uid = 4242;
    rec.craft_tile = here.get_abs( site_pos );
    rec.provider_tiles.push_back( here.get_abs( provider_pos ) );
    rec.expires_at = calendar::turn + 1_hours;
    get_craft_reservations().set( rec );

    THEN( "the craft site is refused" ) {
        CHECK_FALSE( can_construct( con, site_pos ) );
    }

    THEN( "a tile supplying a provider is refused too" ) {
        CHECK_FALSE( can_construct( con, provider_pos ) );
    }

    THEN( "an unrelated tile is still buildable" ) {
        CHECK( can_construct( con, free_pos ) );
    }
}

TEST_CASE( "craft_relocation_rekeys_its_schedule_and_site_lock",
           "[craft][attention][reservation][lifecycle]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms landing( 62, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_only_unattended.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );
    // Without storage, obtain falls through i_add to wield, which carries a re-key hook
    // of its own; every obtain below would then pass whatever the wrapper does.
    u.wear_item( item( itype_debug_backpack ) );

    craft_stamp_passive_entry( on_map, u, calendar::turn, loc );
    REQUIRE( on_map.get_reservations().empty() );
    REQUIRE( get_craft_reservations().craft_site_reserved( here.get_abs( origin ) ) );

    GIVEN( "the craft is carried to another tile and put down" ) {
        item_location carried = loc.obtain( u );
        REQUIRE( carried );
        // Not wielded, so the wield hook is not what re-keyed it.
        REQUIRE( u.get_wielded_item().get_item() != carried.get_item() );
        item_location dropped = here.add_item_or_charges_ret_loc( landing, *carried );
        carried.remove_item();
        REQUIRE( dropped );
        craft_relocated( dropped );

        THEN( "the tile it left is buildable again" ) {
            CHECK_FALSE( get_craft_reservations().craft_site_reserved( here.get_abs( origin ) ) );
        }

        THEN( "the tile it landed on is locked" ) {
            CHECK( dropped->get_reserved_tile() == here.get_abs( landing ) );
            CHECK( get_craft_reservations().craft_site_reserved( here.get_abs( landing ) ) );
        }

        THEN( "it is still polling under its new identity" ) {
            CHECK( get_item_wakeups().is_scheduled( dropped->uid().get_value(),
                                                    item_wakeup_kind::env_check ) );
        }
    }

    GIVEN( "the craft is dropped through the drop activity" ) {
        const tripoint_bub_ms drop_pos( 61, 61, 0 );
        item_location carried = loc.obtain( u );
        REQUIRE( carried );
        REQUIRE_FALSE( carried->get_reserved_tile().has_value() );
        const std::list<item> to_drop{ *carried };
        carried.remove_item();
        const std::vector<item_location> dropped =
            drop_on_map( u, item_drop_reason::deliberate, to_drop, &here, drop_pos );
        REQUIRE( dropped.size() == 1 );

        THEN( "the landing site re-keys itself with no explicit call" ) {
            CHECK( get_item_wakeups().is_scheduled( dropped.front()->uid().get_value(),
                                                    item_wakeup_kind::env_check ) );
            CHECK( dropped.front()->get_reserved_tile() == here.get_abs( drop_pos ) );
            CHECK( get_craft_reservations().craft_site_reserved( here.get_abs( drop_pos ) ) );
        }
    }

    GIVEN( "the craft is inserted into a container on the ground" ) {
        const tripoint_bub_ms crate_pos( 61, 61, 0 );
        item &crate = here.add_item( crate_pos, item( itype_backpack ) );
        item_location holster( map_cursor( here.get_abs( crate_pos ) ), &crate );
        item_location carried = loc.obtain( u );
        REQUIRE( carried );
        drop_locations to_insert;
        to_insert.emplace_back( carried, 1 );
        insert_item_activity_actor actor( holster, to_insert );
        player_activity act;
        actor.finish( act, u );

        item *inside = nullptr;
        for( item *held : crate.all_items_top( pocket_type::CONTAINER ) ) {
            if( held->is_craft() ) {
                inside = held;
            }
        }
        REQUIRE( inside != nullptr );

        THEN( "the site lock names the container's tile, not the character" ) {
            CHECK( get_item_wakeups().is_scheduled( inside->uid().get_value(),
                                                    item_wakeup_kind::env_check ) );
            CHECK( inside->get_reserved_tile() == here.get_abs( crate_pos ) );
        }
    }

    GIVEN( "the craft is thrown to a clear tile" ) {
        const tripoint_bub_ms target( 60, 63, 0 );
        item_location carried = loc.obtain( u );
        REQUIRE( carried );
        const item thrown = *carried;
        carried.remove_item();
        u.set_str_bonus( 10 );
        u.throw_item( target, thrown );

        item *landed = nullptr;
        tripoint_bub_ms landing_tile;
        for( const tripoint_bub_ms &p : here.points_in_radius( target, 3 ) ) {
            for( item &ground : here.i_at( p ) ) {
                if( ground.is_craft() ) {
                    landed = &ground;
                    landing_tile = p;
                }
            }
        }
        REQUIRE( landed != nullptr );

        THEN( "the landing re-keys the schedule and the site lock" ) {
            CHECK( get_item_wakeups().is_scheduled( landed->uid().get_value(),
                                                    item_wakeup_kind::env_check ) );
            CHECK( landed->get_reserved_tile() == here.get_abs( landing_tile ) );
        }
    }

    GIVEN( "the craft rides in vehicle cargo" ) {
        const tripoint_bub_ms cart_pos( 65, 60, 0 );
        vehicle *cart = here.add_vehicle( vehicle_prototype_test_shopping_cart, cart_pos,
                                          0_degrees, 0, veh_spawn_status::UNDAMAGED );
        REQUIRE( cart != nullptr );
        std::optional<vpart_reference> cargo = here.veh_at( here.get_abs( cart_pos ) ).cargo();
        REQUIRE( cargo );
        item_location carried = loc.obtain( u );
        REQUIRE( carried );
        std::optional<vehicle_stack::iterator> in_cargo =
            cargo->vehicle().add_item( here, cargo->part(), *carried );
        carried.remove_item();
        REQUIRE( in_cargo );
        item_location cargo_loc( vehicle_cursor( cargo->vehicle(), cargo->part_index() ),
                                 & **in_cargo );
        // The vehicle add copies; re-key once the way the stow hook does, so the GIVENs
        // below start from a coherent schedule.
        craft_relocated( cargo_loc );
        REQUIRE( get_item_wakeups().is_scheduled( cargo_loc->uid().get_value(),
                 item_wakeup_kind::env_check ) );
        REQUIRE( cargo_loc->get_reserved_tile() == here.get_abs( cart_pos ) );

        WHEN( "it is obtained out of the cargo" ) {
            item_location taken = cargo_loc.obtain( u );
            REQUIRE( taken );
            REQUIRE( u.get_wielded_item().get_item() != taken.get_item() );

            THEN( "it polls under its new identity and holds no site lock" ) {
                CHECK( get_item_wakeups().is_scheduled( taken->uid().get_value(),
                                                        item_wakeup_kind::env_check ) );
                CHECK_FALSE( taken->get_reserved_tile().has_value() );
            }
        }

        WHEN( "the vehicle moves without the craft being touched" ) {
            const int64_t uid_before = cargo_loc->uid().get_value();
            const tripoint_abs_ms old_abs = cargo_loc.pos_abs();
            REQUIRE( here.displace_vehicle( *cart, tripoint_rel_ms( 2, 0, 0 ) ) );
            item *riding = nullptr;
            for( item &it : cargo->items() ) {
                if( it.is_craft() ) {
                    riding = &it;
                }
            }
            REQUIRE( riding != nullptr );
            item_location moved_loc( vehicle_cursor( cargo->vehicle(),
                                     cargo->part_index() ), riding );
            const tripoint_abs_ms moved_abs = moved_loc.pos_abs();
            REQUIRE( moved_abs != old_abs );

            THEN( "nothing was copied, and the lock lags on the tile it left" ) {
                CHECK( riding->uid().get_value() == uid_before );
                CHECK( get_item_wakeups().is_scheduled( uid_before,
                                                        item_wakeup_kind::env_check ) );
                CHECK( get_craft_reservations().craft_site_reserved( old_abs ) );
                CHECK_FALSE( get_craft_reservations().craft_site_reserved( moved_abs ) );
            }

            WHEN( "the next poll runs" ) {
                craft_actualize_scheduled( *riding, item_wakeup_kind::env_check,
                                           calendar::turn + 1_minutes, moved_loc );

                THEN( "the site lock follows to the tile the craft now occupies" ) {
                    CHECK( riding->get_reserved_tile() == moved_abs );
                    CHECK( get_craft_reservations().craft_site_reserved( moved_abs ) );
                    CHECK_FALSE( get_craft_reservations().craft_site_reserved( old_abs ) );
                }
            }
        }
    }

    GIVEN( "the hook is handed a location beneath the moved root" ) {
        // A craft type has no container pocket, so obtain can never return a location
        // whose parent is the craft itself; the below-root contract is exercised on
        // the helper directly, with the craft a sibling of the handed child.
        const tripoint_bub_ms bag_pos( 63, 63, 0 );
        item bag( itype_backpack );
        item_location carried = loc.obtain( u );
        REQUIRE( carried );
        REQUIRE( bag.put_in( *carried, pocket_type::CONTAINER ).success() );
        carried.remove_item();
        REQUIRE( bag.put_in( item( itype_2x4, calendar::turn ), pocket_type::CONTAINER ).success() );
        item &grounded_bag = here.add_item( bag_pos, bag );
        item_location bag_loc( map_cursor( here.get_abs( bag_pos ) ), &grounded_bag );

        item *plank = nullptr;
        item *nested_craft = nullptr;
        for( item *held : grounded_bag.all_items_top( pocket_type::CONTAINER ) ) {
            if( held->is_craft() ) {
                nested_craft = held;
            } else {
                plank = held;
            }
        }
        REQUIRE( plank != nullptr );
        REQUIRE( nested_craft != nullptr );
        REQUIRE_FALSE( get_item_wakeups().is_scheduled( nested_craft->uid().get_value(),
                       item_wakeup_kind::env_check ) );

        WHEN( "a sibling's location is what reaches the hook" ) {
            craft_relocated( item_location( bag_loc, plank ) );

            THEN( "the craft is found through the ascent and re-keyed" ) {
                CHECK( get_item_wakeups().is_scheduled( nested_craft->uid().get_value(),
                                                        item_wakeup_kind::env_check ) );
                CHECK( nested_craft->get_reserved_tile() == here.get_abs( bag_pos ) );
            }
        }
    }
    GIVEN( "a live craft whose step holds a bound provider" ) {
        const tripoint_bub_ms boil_pos( 64, 64, 0 );
        const tripoint_bub_ms pot_pos( 65, 64, 0 );
        item ingredient( itype_water, calendar::turn );
        item boil_placed( &recipe_water_clean_test_unattended_boil.obj(), 1, ingredient );
        item &boiling = here.add_item( boil_pos, boil_placed );
        REQUIRE( boiling.is_craft() );
        boiling.set_current_step( 0 );
        boiling.set_crafter_id( u.getID() );
        boiling.set_step_plans( std::vector<attention_plan>( 1 ) );
        item_location boil_loc( map_cursor( here.get_abs( boil_pos ) ), &boiling );
        here.add_item( pot_pos, item( itype_pot ) );
        craft_stamp_passive_entry( boiling, u, calendar::turn, boil_loc );
        REQUIRE( !boiling.get_reservations().empty() );
        const int64_t bound_token = boiling.peek_reservation_owner_token();
        const int64_t bound_uid = boiling.uid().get_value();
        REQUIRE( get_item_wakeups().is_scheduled( bound_uid, item_wakeup_kind::env_check ) );
        REQUIRE( get_craft_reservations().craft_site_reserved( here.get_abs( boil_pos ) ) );

        WHEN( "it is picked up" ) {
            item_location carried = boil_loc.obtain( u );
            REQUIRE( carried );
            REQUIRE( carried->is_craft() );

            THEN( "it polls under its new identity" ) {
                CHECK( carried->uid().get_value() != bound_uid );
                CHECK( get_item_wakeups().is_scheduled( carried->uid().get_value(),
                                                        item_wakeup_kind::env_check ) );
            }

            THEN( "it keeps its owner token, so the index record is not orphaned" ) {
                CHECK( carried->peek_reservation_owner_token() == bound_token );
            }

            THEN( "a carried craft holds no site lock" ) {
                CHECK_FALSE( carried->get_reserved_tile().has_value() );
                CHECK_FALSE( get_craft_reservations().craft_site_reserved(
                                 here.get_abs( boil_pos ) ) );
            }

            WHEN( "it is put back down somewhere else" ) {
                const tripoint_bub_ms boil_landing( 66, 64, 0 );
                item_location dropped =
                    here.add_item_or_charges_ret_loc( boil_landing, *carried );
                carried.remove_item();
                REQUIRE( dropped );
                craft_relocated( dropped );

                THEN( "the site lock follows it to the tile it landed on" ) {
                    CHECK( dropped->get_reserved_tile() == here.get_abs( boil_landing ) );
                    CHECK( get_craft_reservations().craft_site_reserved(
                               here.get_abs( boil_landing ) ) );
                    CHECK_FALSE( get_craft_reservations().craft_site_reserved(
                                     here.get_abs( boil_pos ) ) );
                }

                THEN( "it polls under the new identity" ) {
                    CHECK( get_item_wakeups().is_scheduled( dropped->uid().get_value(),
                                                            item_wakeup_kind::env_check ) );
                }
            }
        }

        WHEN( "it is stored into a worn container" ) {
            item worn_backpack( itype_backpack );
            u.worn.wear_item( u, worn_backpack, false, false );
            item &worn_pack = u.worn.front();
            item craft_copy = boiling;
            boil_loc.remove_item();
            item_location put_loc = u.i_add( craft_copy );
            REQUIRE( put_loc );
            u.Character::store( worn_pack, *put_loc, false, 0 );

            THEN( "the nested craft is re-keyed even though the hook saw the container" ) {
                item *nested = nullptr;
                worn_pack.visit_items( [&nested]( item * node, item * ) {
                    if( node->is_craft() ) {
                        nested = node;
                        return VisitResponse::ABORT;
                    }
                    return VisitResponse::NEXT;
                } );
                REQUIRE( nested != nullptr );
                CHECK( get_item_wakeups().is_scheduled( nested->uid().get_value(),
                                                        item_wakeup_kind::env_check ) );
            }

            THEN( "a craft in a carried container holds no site lock" ) {
                CHECK_FALSE( get_craft_reservations().craft_site_reserved(
                                 here.get_abs( boil_pos ) ) );
            }
        }

        WHEN( "the container holding it is wielded" ) {
            item holder( itype_backpack );
            holder.put_in( boiling, pocket_type::CONTAINER );
            boil_loc.remove_item();
            item_location carried = u.i_add( holder );
            REQUIRE( carried );
            REQUIRE( u.wield( *carried ) );
            item_location wielded = u.get_wielded_item();
            REQUIRE( wielded );

            THEN( "the craft inside polls under its new identity" ) {
                item *nested = nullptr;
                wielded->visit_items( [&nested]( item * node, item * ) {
                    if( node->is_craft() ) {
                        nested = node;
                        return VisitResponse::ABORT;
                    }
                    return VisitResponse::NEXT;
                } );
                REQUIRE( nested != nullptr );
                CHECK( nested->peek_reservation_owner_token() == bound_token );
                CHECK( get_item_wakeups().is_scheduled( nested->uid().get_value(),
                                                        item_wakeup_kind::env_check ) );
            }
        }
    }
}

TEST_CASE( "reservation_keeps_polling_a_site_only_step",
           "[craft][attention][reservation][lifecycle]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_only_unattended.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    const time_point t0 = calendar::turn;

    GIVEN( "a grounded step holding only its craft site" ) {
        craft_stamp_passive_entry( on_map, u, t0, loc );
        REQUIRE( on_map.get_reservations().empty() );
        REQUIRE( on_map.get_reserved_tile().has_value() );
        REQUIRE( on_map.get_env_check_at() != calendar::before_time_starts );

        WHEN( "the first check runs" ) {
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "the poll is armed again so the lease keeps sliding" ) {
                CHECK( on_map.get_env_check_at() != calendar::before_time_starts );
            }

            THEN( "the lease itself advanced with the completed tick" ) {
                CHECK( on_map.get_reservation_expiry() == t0 + 1_minutes + 1_hours );
            }
        }
    }
}

TEST_CASE( "reservation_binds_a_quality_provider_on_step_entry",
           "[craft][attention][reservation][binding]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_water, calendar::turn );
    item placed( &recipe_water_clean_test_unattended_boil.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "one pot on the ground beside the craft" ) {
        item &pot = here.add_item( origin, item( itype_pot ) );
        const int64_t pot_uid = pot.uid().get_value();
        REQUIRE( pot_uid != 0 );

        WHEN( "the passive step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the step holds a binding naming that pot" ) {
                REQUIRE( on_map.get_reservations().size() == 1 );
                const craft_reservation::binding &b = on_map.get_reservations()[0];
                CHECK( b.kind == craft_reservation::provider_kind::item );
                CHECK( b.provider_uid == pot_uid );
            }

            THEN( "the binding carries the descriptor it was made against" ) {
                REQUIRE( on_map.get_reservations().size() == 1 );
                const craft_reservation::binding &b = on_map.get_reservations()[0];
                CHECK( b.req == craft_reservation::requirement_kind::quality );
                CHECK( b.qual == qual_BOIL );
                CHECK( b.level == 1 );
                CHECK( b.group_count == 1 );
            }

            THEN( "the index reports the pot reserved" ) {
                CHECK( get_craft_reservations().is_reserved_uid( pot_uid ) );
            }

            THEN( "the craft holds a live lease and a site lock" ) {
                CHECK( on_map.get_reservation_expiry() > calendar::turn );
                CHECK( on_map.get_reserved_tile() == here.get_abs( origin ) );
                CHECK( get_craft_reservations().craft_site_reserved( here.get_abs( origin ) ) );
            }

            THEN( "the poll is armed, so the binding will be revalidated" ) {
                CHECK( on_map.get_env_check_at() != calendar::before_time_starts );
            }
        }
    }

    GIVEN( "the only pot already reserved by another craft" ) {
        item &pot = here.add_item( origin, item( itype_pot ) );
        craft_reservation_index::record other;
        other.craft_uid = 999;
        other.provider_item_uids.push_back( pot.uid().get_value() );
        other.expires_at = calendar::turn + 1_hours;
        get_craft_reservations().set( other );
        REQUIRE( get_craft_reservations().is_reserved_uid( pot.uid().get_value() ) );

        WHEN( "the passive step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "it binds nothing and pauses" ) {
                CHECK( on_map.get_reservations().empty() );
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }
        }
    }

    GIVEN( "two equally good pots" ) {
        here.add_item( origin, item( itype_pot ) );
        here.add_item( origin, item( itype_pot ) );

        WHEN( "the passive step is entered twice from the same world" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );
            REQUIRE( on_map.get_reservations().size() == 1 );
            const int64_t first_choice = on_map.get_reservations()[0].provider_uid;

            THEN( "the choice is stable rather than enumeration-order dependent" ) {
                get_craft_reservations().erase( on_map.peek_reservation_owner_token() );
                on_map.set_reservations( {} );
                craft_stamp_passive_entry( on_map, u, calendar::turn, loc );
                REQUIRE( on_map.get_reservations().size() == 1 );
                CHECK( on_map.get_reservations()[0].provider_uid == first_choice );
            }
        }
    }
}

TEST_CASE( "reservation_binds_non_item_providers", "[craft][attention][reservation][binding]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms furn_pos( 61, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_furn_qual.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "furniture supplying the quality through a pseudo item" ) {
        here.furn_set( furn_pos, furn_test_f_reserve_qual );
        REQUIRE( here.furn( furn_pos ) == furn_test_f_reserve_qual );

        WHEN( "the passive step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the step binds the furniture by tile" ) {
                REQUIRE( on_map.get_reservations().size() == 1 );
                const craft_reservation::binding &b = on_map.get_reservations()[0];
                CHECK( b.kind == craft_reservation::provider_kind::furniture );
                CHECK( b.tile == here.get_abs( furn_pos ) );
                CHECK( b.furn == furn_test_f_reserve_qual );
            }

            THEN( "the provider tile is locked, and separately from the craft site" ) {
                CHECK( get_craft_reservations().provider_tile_reserved( here.get_abs( furn_pos ) ) );
                CHECK_FALSE( get_craft_reservations().provider_tile_reserved(
                                 here.get_abs( origin ) ) );
                CHECK( get_craft_reservations().craft_site_reserved( here.get_abs( origin ) ) );
            }
        }
    }

    GIVEN( "a loose tool and furniture both supplying the quality" ) {
        here.furn_set( furn_pos, furn_test_f_reserve_qual );
        here.add_item( origin, item( itype_test_reserve_tool_a ) );

        WHEN( "the passive step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the furniture is preferred, leaving the portable tool to the player" ) {
                REQUIRE( on_map.get_reservations().size() == 1 );
                CHECK( on_map.get_reservations()[0].kind ==
                       craft_reservation::provider_kind::furniture );
            }
        }
    }

    GIVEN( "the quality available only from the crafter's mutation" ) {
        u.set_mutation( trait_TEST_RESERVE_QUALITIES );
        REQUIRE( u.has_trait( trait_TEST_RESERVE_QUALITIES ) );

        WHEN( "the passive step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "an intrinsic binding covers the group" ) {
                REQUIRE( on_map.get_reservations().size() == 1 );
                const craft_reservation::binding &b = on_map.get_reservations()[0];
                CHECK( b.kind == craft_reservation::provider_kind::intrinsic );
                CHECK( b.intrinsic_owner == u.getID() );
                CHECK( b.occurrence_slot >= 0 );
            }

            THEN( "it carries no item type, being keyed by capability" ) {
                REQUIRE( on_map.get_reservations().size() == 1 );
                CHECK( on_map.get_reservations()[0].pseudo_type.is_null() );
            }

            THEN( "nothing is taken from anyone, so no index entry appears" ) {
                const craft_reservation_index::record *rec =
                    get_craft_reservations().find( on_map.peek_reservation_owner_token() );
                REQUIRE( rec != nullptr );
                CHECK( rec->provider_item_uids.empty() );
                CHECK( rec->provider_tiles.empty() );
            }
        }
    }
}

TEST_CASE( "reservation_pauses_when_a_bound_provider_disappears",
           "[craft][attention][reservation][validate]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    // The pot sits on its own tile so it can be removed without touching the craft.
    const tripoint_bub_ms pot_pos( 61, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_water, calendar::turn );
    item placed( &recipe_water_clean_test_unattended_boil.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    here.add_item( pot_pos, item( itype_pot ) );
    const time_point t0 = calendar::turn;
    craft_stamp_passive_entry( on_map, u, t0, loc );
    REQUIRE( on_map.get_reservations().size() == 1 );
    const int64_t bound_uid = on_map.get_reservations()[0].provider_uid;
    const time_point lease_at_entry = on_map.get_reservation_expiry();

    GIVEN( "the bound pot is still there" ) {
        WHEN( "the step polls" ) {
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "it keeps running and slides its lease" ) {
                CHECK( on_map.get_pause_started_at() == calendar::before_time_starts );
                CHECK( on_map.get_reservation_expiry() > lease_at_entry );
            }

            THEN( "it still holds the same provider" ) {
                REQUIRE( on_map.get_reservations().size() == 1 );
                CHECK( on_map.get_reservations()[0].provider_uid == bound_uid );
            }
        }
    }

    GIVEN( "the bound pot is destroyed" ) {
        here.i_clear( pot_pos );

        WHEN( "the step polls" ) {
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "it pauses rather than silently rebinding" ) {
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }

            THEN( "it keeps the binding, so the identity is not forgotten" ) {
                REQUIRE( on_map.get_reservations().size() == 1 );
                CHECK( on_map.get_reservations()[0].provider_uid == bound_uid );
            }

            THEN( "the lease is not slid, so a permanently paused craft still expires" ) {
                CHECK( on_map.get_reservation_expiry() == lease_at_entry );
            }
        }
    }

    GIVEN( "the bound pot goes out of range and comes back" ) {
        here.i_clear( pot_pos );
        craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );
        REQUIRE( on_map.get_pause_started_at() != calendar::before_time_starts );

        WHEN( "a different pot appears" ) {
            here.add_item( pot_pos, item( itype_pot ) );
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 2_minutes, loc );

            THEN( "the step stays paused, since the replacement is a different identity" ) {
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }
        }
    }
}

TEST_CASE( "reservation_ownership_is_first_come", "[craft][attention][reservation][validate]" )
{
    clear_avatar();
    clear_map();
    craft_reservation_index &idx = get_craft_reservations();
    const time_point live_until = calendar::turn + 1_hours;

    GIVEN( "a live craft holding a provider" ) {
        craft_reservation_index::record first;
        first.craft_uid = 100;
        first.provider_item_uids.push_back( 4242 );
        first.expires_at = live_until;
        idx.set( first );
        REQUIRE( idx.record_for_item_uid( 4242 )->craft_uid == 100 );

        WHEN( "another live craft records the same provider" ) {
            craft_reservation_index::record second;
            second.craft_uid = 200;
            second.provider_item_uids.push_back( 4242 );
            second.expires_at = live_until;
            idx.set( second );

            THEN( "the incumbent keeps it" ) {
                CHECK( idx.record_for_item_uid( 4242 )->craft_uid == 100 );
            }

            THEN( "the newcomer sees it as claimed by another" ) {
                CHECK( idx.item_claimed_by_other( 4242, 200 ) );
            }
        }
    }
}

TEST_CASE( "reservation_ready_path_revalidates_before_completing",
           "[craft][attention][reservation][validate]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms pot_pos( 61, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_water, calendar::turn );
    item placed( &recipe_water_clean_test_unattended_boil.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    here.add_item( pot_pos, item( itype_pot ) );
    const time_point t0 = calendar::turn;
    craft_stamp_passive_entry( on_map, u, t0, loc );
    REQUIRE( on_map.get_reservations().size() == 1 );

    GIVEN( "the bound pot is destroyed just before the step comes due" ) {
        here.i_clear( pot_pos );

        WHEN( "the ready dispatch fires" ) {
            craft_actualize_scheduled( on_map, item_wakeup_kind::ready_check,
                                       on_map.get_ready_at(), loc );

            THEN( "the step pauses rather than completing with a provider that is gone" ) {
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }
        }
    }
}

TEST_CASE( "reservation_revalidates_non_item_providers",
           "[craft][attention][reservation][validate]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms furn_pos( 61, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_furn_qual.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    const time_point t0 = calendar::turn;

    GIVEN( "a step bound to furniture" ) {
        here.furn_set( furn_pos, furn_test_f_reserve_qual );
        craft_stamp_passive_entry( on_map, u, t0, loc );
        REQUIRE( on_map.get_reservations().size() == 1 );
        REQUIRE( on_map.get_reservations()[0].kind ==
                 craft_reservation::provider_kind::furniture );

        WHEN( "the furniture is removed" ) {
            here.furn_set( furn_pos, furn_str_id::NULL_ID() );
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "the step pauses" ) {
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }
        }

        WHEN( "the furniture is still there" ) {
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "the step keeps running" ) {
                CHECK( on_map.get_pause_started_at() == calendar::before_time_starts );
            }
        }
    }

    GIVEN( "a step bound to the crafter's mutation" ) {
        u.set_mutation( trait_TEST_RESERVE_QUALITIES );
        craft_stamp_passive_entry( on_map, u, t0, loc );
        REQUIRE( on_map.get_reservations().size() == 1 );
        REQUIRE( on_map.get_reservations()[0].kind ==
                 craft_reservation::provider_kind::intrinsic );

        WHEN( "the crafter walks out of range" ) {
            u.setpos( here, tripoint_bub_ms( 60 + PICKUP_RANGE + 5, 60, 0 ) );
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "the step pauses, since an intrinsic source is admitted by range too" ) {
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }
        }

        WHEN( "the crafter loses the mutation while still in range" ) {
            u.unset_mutation( trait_TEST_RESERVE_QUALITIES );
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "the step pauses" ) {
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }
        }
    }
}

TEST_CASE( "reservation_item_info_names_the_crafter_holding_it",
           "[craft][attention][reservation][info]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms pot_pos( 61, 60, 0 );
    u.setpos( here, tripoint_bub_ms( 60, 60, 0 ) );

    item &pot = here.add_item( pot_pos, item( itype_pot ) );

    GIVEN( "a claim recorded against the avatar" ) {
        craft_reservation_index::record rec;
        rec.craft_uid = 4242;
        rec.crafter = u.getID();
        rec.provider_item_uids.push_back( pot.uid().get_value() );
        rec.expires_at = calendar::turn + 1_hours;
        get_craft_reservations().set( rec );
        REQUIRE( get_craft_reservations().is_reserved_uid( pot.uid().get_value() ) );

        THEN( "the description says whose craft holds it" ) {
            const std::string text = pot.info( true );
            CHECK( text.find( "a craft of yours" ) != std::string::npos );
        }
    }
}

TEST_CASE( "reservation_pause_reports_itself_once_per_transition",
           "[craft][attention][reservation][pause]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms pot_pos( 61, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_water, calendar::turn );
    item placed( &recipe_water_clean_test_unattended_boil.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    here.add_item( pot_pos, item( itype_pot ) );
    const time_point t0 = calendar::turn;
    craft_stamp_passive_entry( on_map, u, t0, loc );
    REQUIRE( on_map.get_reservations().size() == 1 );
    REQUIRE( on_map.get_reservation_pause_reason() == 0 );

    GIVEN( "the bound pot destroyed under a running step" ) {
        here.i_clear( pot_pos );

        // Each section clears the message log as it starts, so every tick whose output
        // is being counted has to run inside the section that counts it.
        THEN( "the pause explains itself" ) {
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            CHECK( on_map.get_reservation_pause_reason() != 0 );
            CHECK( Messages::size() == 1 );
        }

        THEN( "the next check fails the same way and stays quiet" ) {
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );
            REQUIRE( on_map.get_reservation_pause_reason() != 0 );
            Messages::clear_messages();
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 2_minutes, loc );

            CHECK( Messages::size() == 0 );
        }
    }
}

TEST_CASE( "reserved_tiles_refuse_construction", "[craft][attention][reservation][tile]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms furn_pos( 61, 60, 0 );
    const tripoint_bub_ms free_pos( 63, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_furn_qual.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    here.furn_set( furn_pos, furn_test_f_reserve_qual );
    craft_stamp_passive_entry( on_map, u, calendar::turn, loc );
    REQUIRE( on_map.get_reservations().size() == 1 );

    THEN( "the craft site is refused" ) {
        CHECK( get_craft_reservations().craft_site_reserved( here.get_abs( origin ) ) );
    }

    THEN( "the provider tile is refused" ) {
        CHECK( get_craft_reservations().provider_tile_reserved( here.get_abs( furn_pos ) ) );
    }

    THEN( "an unrelated tile is not" ) {
        CHECK_FALSE( get_craft_reservations().craft_site_reserved( here.get_abs( free_pos ) ) );
        CHECK_FALSE( get_craft_reservations().provider_tile_reserved( here.get_abs( free_pos ) ) );
    }

    THEN( "construction itself refuses the provider tile a binding claimed" ) {
        here.furn_set( furn_pos, furn_str_id::NULL_ID() );
        here.ter_set( furn_pos, ter_t_dirt );
        CHECK_FALSE( can_construct( construction_test_constr_pit_shallow.obj(), furn_pos ) );
    }
}

TEST_CASE( "reservation_leaves_the_charged_pool_feasible",
           "[craft][attention][reservation][charges]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_qual_and_charges.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "two charged tools, either of which could supply the quality" ) {
        item &first = here.add_item( origin, item( itype_test_reserve_charged_tool ) );
        item &second = here.add_item( origin, item( itype_test_reserve_charged_tool ) );
        first.ammo_set( itype_battery, 200 );
        second.ammo_set( itype_battery, 200 );

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "one is bound and the other is left drainable" ) {
                REQUIRE( on_map.get_reservations().size() == 1 );
                const int64_t bound = on_map.get_reservations()[0].provider_uid;
                int free_charges = 0;
                for( const item &it : here.i_at( origin ) ) {
                    if( it.typeId() == itype_test_reserve_charged_tool &&
                        it.uid().get_value() != bound ) {
                        free_charges += it.ammo_remaining();
                    }
                }
                CHECK( free_charges >= 50 );
            }
        }
    }
}

TEST_CASE( "reservation_preflight_counts_the_power_a_bound_tool_takes_with_it",
           "[craft][attention][reservation][charges]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_qual_and_charges.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );

    // The recipe names the magazine-fed tool; this draw is of the bionic-powered one,
    // whose charges sit outside the item and vanish along with it.
    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::player;
    alloc.sel.comp.type = itype_test_reserve_bionic_powered_tool;
    alloc.sel.comp.count = 50;
    alloc.step_count_units = 50;
    on_map.set_step_tool_allocs( { { alloc } } );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    u.set_max_power_level( units::from_kilojoule( 100 ) );
    u.set_power_level( units::from_kilojoule( 100 ) );

    GIVEN( "one carried tool covering both the quality and the charges" ) {
        REQUIRE( u.i_add( item( itype_test_reserve_bionic_powered_tool ) ) );
        u.invalidate_crafting_inventory();
        REQUIRE( u.charges_of( itype_test_reserve_bionic_powered_tool ) >= 50 );

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "nothing is bound, since hiding it would take the power with it" ) {
                CHECK( on_map.get_reservations().empty() );
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
                CHECK( u.get_power_level() == units::from_kilojoule( 100 ) );
            }
        }
    }

    GIVEN( "a second identical tool" ) {
        REQUIRE( u.i_add( item( itype_test_reserve_bionic_powered_tool ) ) );
        REQUIRE( u.i_add( item( itype_test_reserve_bionic_powered_tool ) ) );
        u.invalidate_crafting_inventory();

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "one is bound and the other still reaches the shared pool" ) {
                CHECK( on_map.get_reservations().size() == 1 );
                CHECK( on_map.get_pause_started_at() == calendar::before_time_starts );
            }
        }
    }

    GIVEN( "the only tool wielded and impossible to drop" ) {
        step_tool_alloc bolted = alloc;
        bolted.sel.comp.type = itype_test_reserve_bolted_powered_tool;
        on_map.set_step_tool_allocs( { { bolted } } );
        u.set_wielded_item( item( itype_test_reserve_bolted_powered_tool ) );
        REQUIRE( u.get_wielded_item() );
        REQUIRE( !u.can_drop( *u.get_wielded_item() ).success() );
        u.invalidate_crafting_inventory();
        REQUIRE( u.charges_of( itype_test_reserve_bolted_powered_tool ) >= 50 );

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "it is still counted as taking its own power with it" ) {
                CHECK( on_map.get_reservations().empty() );
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
                CHECK( u.get_power_level() == units::from_kilojoule( 100 ) );
            }
        }
    }
}

TEST_CASE( "reservation_discovery_admits_providers_beside_a_sealed_craft_tile",
           "[craft][attention][reservation][discovery]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms beside( 61, 60, 0 );
    u.setpos( here, beside );

    item ingredient( itype_water, calendar::turn );
    item placed( &recipe_water_clean_test_unattended_boil.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "a pot on an accessible tile and the craft on a sealed one" ) {
        item &pot = here.add_item( beside, item( itype_pot ) );
        const int64_t pot_uid = pot.uid().get_value();
        here.furn_set( origin, furn_f_plant_seed );
        REQUIRE( !here.accessible_items( origin ) );
        REQUIRE( here.accessible_items( beside ) );

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the pot is bound, since accessibility is judged per provider tile" ) {
                REQUIRE( on_map.get_reservations().size() == 1 );
                CHECK( on_map.get_reservations()[0].provider_uid == pot_uid );
            }
        }
    }
}

TEST_CASE( "reservation_binds_a_fire_with_an_occurrence_slot",
           "[craft][attention][reservation][binding][persist]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_fire_tool.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    on_map.set_step_tool_allocs( { { fire_presence_alloc() } } );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "a fire beside the craft" ) {
        here.add_field( tripoint_bub_ms( 61, 60, 0 ), field_fd_fire, 1 );
        REQUIRE( here.has_nearby_fire( origin, 1 ) );

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the fire is bound as an abstract provider holding a slot" ) {
                REQUIRE( on_map.get_reservations().size() == 1 );
                CHECK( on_map.get_reservations()[0].kind ==
                       craft_reservation::provider_kind::environment );
                CHECK( on_map.get_reservations()[0].occurrence_slot >= 0 );
            }

            THEN( "the binding survives a reload rather than reading as stale" ) {
                std::ostringstream ss;
                JsonOut jsout( ss );
                on_map.serialize( jsout );

                item restored;
                restored.deserialize( json_loader::from_string( ss.str() ).get_object() );
                REQUIRE( restored.is_craft() );
                CHECK( restored.get_reservations().size() == 1 );
            }
        }
    }
}

TEST_CASE( "reservation_lease_slides_when_a_restored_step_is_not_yet_ready",
           "[craft][attention][reservation][lifecycle]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms furn_pos( 61, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_furn_qual.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    const time_point t0 = calendar::turn;

    GIVEN( "a step that paused when its furniture went away and now has it back" ) {
        here.furn_set( furn_pos, furn_test_f_reserve_qual );
        craft_stamp_passive_entry( on_map, u, t0, loc );
        REQUIRE( on_map.get_reservations().size() == 1 );
        const time_point entry_expiry = on_map.get_reservation_expiry();

        here.furn_set( furn_pos, furn_str_id::NULL_ID() );
        craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );
        REQUIRE( on_map.get_pause_started_at() != calendar::before_time_starts );
        REQUIRE( on_map.get_reservation_expiry() == entry_expiry );

        here.furn_set( furn_pos, furn_test_f_reserve_qual );

        WHEN( "the ready path restores it and pushes the deadline out" ) {
            const time_point due = on_map.get_saved_ready_at();
            REQUIRE( due != calendar::before_time_starts );
            craft_actualize_scheduled( on_map, item_wakeup_kind::ready_check, due, loc );
            REQUIRE( on_map.get_pause_started_at() == calendar::before_time_starts );
            REQUIRE( due < on_map.get_ready_at() );

            THEN( "the lease slid, since the tick got past every gate" ) {
                CHECK( on_map.get_reservation_expiry() > entry_expiry );
            }
        }
    }
}

TEST_CASE( "reservation_revalidates_abstract_and_vehicle_providers",
           "[craft][attention][reservation][validate]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms beside( 61, 60, 0 );
    u.setpos( here, origin );

    const time_point t0 = calendar::turn;

    GIVEN( "a step bound to a nearby fire" ) {
        item ingredient( itype_2x4, calendar::turn );
        item placed( &recipe_cudgel_test_unattended_fire_tool.obj(), 1, ingredient );
        item &on_map = here.add_item( origin, placed );
        REQUIRE( on_map.is_craft() );
        on_map.set_current_step( 0 );
        on_map.set_crafter_id( u.getID() );
        on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
        on_map.set_step_tool_allocs( { { fire_presence_alloc() } } );
        item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

        here.add_field( beside, field_fd_fire, 1 );
        craft_stamp_passive_entry( on_map, u, t0, loc );
        REQUIRE( on_map.get_reservations().size() == 1 );
        REQUIRE( on_map.get_reservations()[0].kind ==
                 craft_reservation::provider_kind::environment );

        WHEN( "the fire goes out" ) {
            here.remove_field( beside, field_fd_fire );
            REQUIRE( !here.has_nearby_fire( origin, 1 ) );
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "the step pauses" ) {
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }
        }

        WHEN( "the fire is still burning" ) {
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "the step keeps running" ) {
                CHECK( on_map.get_pause_started_at() == calendar::before_time_starts );
            }
        }
    }

    GIVEN( "a step bound to a vehicle part's pseudo tool" ) {
        item ingredient( itype_2x4, calendar::turn );
        item placed( &recipe_cudgel_test_unattended_furn_qual.obj(), 1, ingredient );
        item &on_map = here.add_item( origin, placed );
        REQUIRE( on_map.is_craft() );
        on_map.set_current_step( 0 );
        on_map.set_crafter_id( u.getID() );
        on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
        item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

        vehicle *veh = here.add_vehicle( vehicle_prototype_none, beside, 0_degrees, 0,
                                         veh_spawn_status::UNDAMAGED );
        REQUIRE( veh != nullptr );
        REQUIRE( veh->install_part( here, point_rel_ms::zero, vpart_frame ) != -1 );
        const int rig_idx = veh->install_part( here, point_rel_ms::zero,
                                               vpart_test_vp_reserve_qual );
        REQUIRE( rig_idx != -1 );
        veh->refresh();
        here.add_vehicle_to_cache( veh );

        craft_stamp_passive_entry( on_map, u, t0, loc );
        REQUIRE( on_map.get_reservations().size() == 1 );
        REQUIRE( on_map.get_reservations()[0].kind ==
                 craft_reservation::provider_kind::vehicle_part );

        WHEN( "the part is removed" ) {
            veh->remove_part( veh->part( rig_idx ) );
            veh->refresh();
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "the step pauses" ) {
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }
        }

        WHEN( "the part is still installed" ) {
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "the step keeps running" ) {
                CHECK( on_map.get_pause_started_at() == calendar::before_time_starts );
            }
        }
    }
}

TEST_CASE( "reservation_pauses_when_an_abstract_multiset_shrinks",
           "[craft][attention][reservation][validate][multiset]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_two_of_a.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    const time_point t0 = calendar::turn;

    GIVEN( "two intrinsic occurrences covering a group that demands two" ) {
        u.set_mutation( trait_TEST_RESERVE_QUALITIES );
        u.set_mutation( trait_TEST_RESERVE_QUALITIES_2 );
        REQUIRE( u.has_intrinsic_quality( qual_TEST_RESERVE_A, 1, 2 ) );

        craft_stamp_passive_entry( on_map, u, t0, loc );
        REQUIRE( on_map.get_reservations().size() == 2 );
        REQUIRE( on_map.get_reservations()[0].occurrence_slot !=
                 on_map.get_reservations()[1].occurrence_slot );

        WHEN( "one occurrence goes away and a single one remains" ) {
            u.unset_mutation( trait_TEST_RESERVE_QUALITIES_2 );
            REQUIRE( u.has_intrinsic_quality( qual_TEST_RESERVE_A, 1, 1 ) );
            REQUIRE( !u.has_intrinsic_quality( qual_TEST_RESERVE_A, 1, 2 ) );
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "the step pauses, since two slots cannot share one source" ) {
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }
        }

        WHEN( "both occurrences remain" ) {
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "the step keeps running" ) {
                CHECK( on_map.get_pause_started_at() == calendar::before_time_starts );
            }
        }
    }
}

TEST_CASE( "reservation_binds_the_presence_tool_the_allocation_chose",
           "[craft][attention][reservation][binding]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    const auto presence = []( const itype_id & type, bool root ) -> step_tool_alloc {
        step_tool_alloc a;
        a.sel.use_from = usage_from::map;
        a.sel.comp.type = type;
        a.sel.comp.count = -1;
        a.root_derived = root;
        return a;
    };

    GIVEN( "an OR group whose allocation picked the second alternative" ) {
        item ingredient( itype_2x4, calendar::turn );
        item placed( &recipe_cudgel_test_unattended_presence_or.obj(), 1, ingredient );
        item &on_map = here.add_item( origin, placed );
        REQUIRE( on_map.is_craft() );
        on_map.set_current_step( 0 );
        on_map.set_crafter_id( u.getID() );
        on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
        on_map.set_step_tool_allocs( { { presence( itype_test_reserve_tool_a, false ) } } );
        item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

        here.add_item( origin, item( itype_hammer ) );
        item &chosen = here.add_item( origin, item( itype_test_reserve_tool_a ) );
        const int64_t chosen_uid = chosen.uid().get_value();

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the chosen alternative is bound and the other stays free" ) {
                REQUIRE( on_map.get_reservations().size() == 1 );
                CHECK( on_map.get_reservations()[0].tool_type == itype_test_reserve_tool_a );
                CHECK( on_map.get_reservations()[0].provider_uid == chosen_uid );
            }
        }
    }

    GIVEN( "a presence tool that reaches the step only through a root allocation" ) {
        item ingredient( itype_2x4, calendar::turn );
        item placed( &recipe_cudgel_test_unattended_root_presence.obj(), 1, ingredient );
        item &on_map = here.add_item( origin, placed );
        REQUIRE( on_map.is_craft() );
        on_map.set_current_step( 0 );
        on_map.set_crafter_id( u.getID() );
        on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
        on_map.set_step_tool_allocs( { { presence( itype_hammer, true ) } } );
        item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

        item &hammer = here.add_item( origin, item( itype_hammer ) );
        const int64_t hammer_uid = hammer.uid().get_value();

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the root-derived tool is bound too" ) {
                REQUIRE( on_map.get_reservations().size() == 1 );
                CHECK( on_map.get_reservations()[0].tool_type == itype_hammer );
                CHECK( on_map.get_reservations()[0].provider_uid == hammer_uid );
            }

            THEN( "the binding survives a reload" ) {
                std::ostringstream ss;
                JsonOut jsout( ss );
                on_map.serialize( jsout );

                item restored;
                restored.deserialize( json_loader::from_string( ss.str() ).get_object() );
                REQUIRE( restored.is_craft() );
                CHECK( restored.get_reservations().size() == 1 );
            }
        }
    }
}

TEST_CASE( "reservation_binds_a_terrain_pseudo_tool", "[craft][attention][reservation][binding]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms tree_pos( 61, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_tree_qual.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "a tree beside the craft supplying the quality" ) {
        here.ter_set( tree_pos, ter_t_tree );
        REQUIRE( here.ter( tree_pos )->has_flag( ter_furn_flag::TFLAG_TREE ) );

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the terrain is bound by tile" ) {
                REQUIRE( on_map.get_reservations().size() == 1 );
                const craft_reservation::binding &b = on_map.get_reservations()[0];
                CHECK( b.kind == craft_reservation::provider_kind::terrain );
                CHECK( b.tile == here.get_abs( tree_pos ) );
                CHECK( b.ter == ter_t_tree );
            }

            THEN( "the provider tile is locked" ) {
                CHECK( get_craft_reservations().provider_tile_reserved( here.get_abs( tree_pos ) ) );
            }
        }

        WHEN( "the tree is felled after the step bound it" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );
            REQUIRE( on_map.get_reservations().size() == 1 );
            here.ter_set( tree_pos, ter_t_dirt );
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check,
                                       calendar::turn + 1_minutes, loc );

            THEN( "the step pauses" ) {
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }
        }
    }
}

TEST_CASE( "reservation_preflight_models_the_charge_debit",
           "[craft][attention][reservation][charges]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    const auto charged = []( usage_from from, int units ) -> step_tool_alloc {
        step_tool_alloc alloc;
        alloc.sel.use_from = from;
        alloc.sel.comp.type = itype_test_reserve_charged_tool;
        alloc.sel.comp.count = 50;
        alloc.step_count_units = units;
        return alloc;
    };

    const auto place_craft = [&]( const step_tool_alloc & alloc ) -> item & {
        item ingredient( itype_2x4, calendar::turn );
        item placed( &recipe_cudgel_test_unattended_qual_and_charges.obj(), 1, ingredient );
        item &on_map = here.add_item( origin, placed );
        on_map.set_current_step( 0 );
        on_map.set_crafter_id( u.getID() );
        on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
        on_map.set_step_tool_allocs( { { alloc } } );
        return on_map;
    };

    GIVEN( "the only qualifying tool sits in a container holding the step's charges" ) {
        item bag( itype_backpack );
        bag.put_in( item( itype_test_reserve_tool_a ), pocket_type::CONTAINER );
        item powered( itype_test_reserve_charged_tool );
        powered.ammo_set( itype_battery, 100 );
        bag.put_in( powered, pocket_type::CONTAINER );
        here.add_item( origin, bag );

        item &on_map = place_craft( charged( usage_from::map, 50 ) );
        item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "nothing is bound, since binding hides the whole container" ) {
                CHECK( on_map.get_reservations().empty() );
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }
        }
    }

    GIVEN( "a batch-scaled allocation that outruns the charges on the floor" ) {
        here.add_item( origin, item( itype_test_reserve_tool_a ) );
        item powered( itype_test_reserve_charged_tool );
        powered.ammo_set( itype_battery, 100 );
        here.add_item( origin, powered );

        item &on_map = place_craft( charged( usage_from::map, 200 ) );
        item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the step pauses rather than committing a draw it cannot make" ) {
                CHECK( on_map.get_reservations().empty() );
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }
        }
    }

    GIVEN( "a player-sourced allocation whose charges are only on the ground" ) {
        u.worn.wear_item( u, item( itype_backpack ), false, false );
        u.i_add( item( itype_test_reserve_tool_a ) );
        item powered( itype_test_reserve_charged_tool );
        powered.ammo_set( itype_battery, 100 );
        here.add_item( origin, powered );

        item &on_map = place_craft( charged( usage_from::player, 50 ) );
        item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the step pauses, since the crafter carries none of them" ) {
                CHECK( on_map.get_reservations().empty() );
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }
        }
    }

    GIVEN( "the same allocation with the charges carried beside a wielded provider" ) {
        u.worn.wear_item( u, item( itype_backpack ), false, false );
        item tool_a( itype_test_reserve_tool_a );
        REQUIRE( u.wield( tool_a ) );
        item powered( itype_test_reserve_charged_tool );
        powered.ammo_set( itype_battery, 100 );
        u.i_add( powered );

        item &on_map = place_craft( charged( usage_from::player, 50 ) );
        item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the quality provider binds and the charges stay drawable" ) {
                REQUIRE( on_map.get_reservations().size() == 1 );
                CHECK( on_map.get_reservations()[0].qual == qual_TEST_RESERVE_A );
            }
        }
    }
}

TEST_CASE( "reservation_search_terminates_on_a_shared_abstract_provider",
           "[craft][attention][reservation][search]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_qual_and_charges.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );

    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::map;
    alloc.sel.comp.type = itype_test_reserve_charged_tool;
    alloc.sel.comp.count = 50;
    alloc.step_count_units = 50;
    on_map.set_step_tool_allocs( { { alloc } } );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "a shared intrinsic provider and a charge demand nothing can meet" ) {
        u.set_mutation( trait_TEST_RESERVE_QUALITIES );
        REQUIRE( u.has_intrinsic_quality( qual_TEST_RESERVE_A, 1, 1 ) );

        WHEN( "the step is entered" ) {
            craft_reservation::reset_search_expansions();
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the search gives up instead of retrying the shared provider forever" ) {
                CHECK( on_map.get_reservations().empty() );
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
                CHECK( craft_reservation::search_expansions_total() <
                       craft_reservation::search_budget_initial );
            }
        }
    }
}

TEST_CASE( "reservation_pool_and_root_claims_span_the_whole_unit",
           "[craft][attention][reservation][charges]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    const auto place_craft = [&]( const std::vector<step_tool_alloc> &allocs ) -> item & {
        item ingredient( itype_2x4, calendar::turn );
        item placed( &recipe_cudgel_test_unattended_qual_and_charges.obj(), 1, ingredient );
        item &on_map = here.add_item( origin, placed );
        on_map.set_current_step( 0 );
        on_map.set_crafter_id( u.getID() );
        on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
        on_map.set_step_tool_allocs( { allocs } );
        return on_map;
    };
    const auto charged = []( usage_from from, int units ) -> step_tool_alloc {
        step_tool_alloc alloc;
        alloc.sel.use_from = from;
        alloc.sel.comp.type = itype_test_reserve_charged_tool;
        alloc.sel.comp.count = units;
        alloc.step_count_units = units;
        return alloc;
    };

    GIVEN( "one carried stack owed to a player draw and a both draw at once" ) {
        u.worn.wear_item( u, item( itype_backpack ), false, false );
        item tool_a( itype_test_reserve_tool_a );
        REQUIRE( u.wield( tool_a ) );
        item powered( itype_test_reserve_charged_tool );
        powered.ammo_set( itype_battery, 100 );
        u.i_add( powered );

        item &on_map = place_craft( { charged( usage_from::player, 60 ),
                                      charged( usage_from::both, 60 ) } );
        item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the step pauses, since the two draws share one supply" ) {
                CHECK( on_map.get_reservations().empty() );
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }
        }
    }

    GIVEN( "a container whose only free tool sits beside another craft's provider" ) {
        item bag( itype_backpack );
        bag.put_in( item( itype_test_reserve_tool_a ), pocket_type::CONTAINER );
        bag.put_in( item( itype_test_reserve_tool_a ), pocket_type::CONTAINER );
        item &on_ground = here.add_item( origin, bag );
        item powered( itype_test_reserve_charged_tool );
        powered.ammo_set( itype_battery, 100 );
        here.add_item( origin, powered );

        std::vector<const item *> nested;
        on_ground.visit_items( [&nested]( const item * node, const item * parent ) {
            if( parent != nullptr ) {
                nested.push_back( node );
            }
            return VisitResponse::NEXT;
        } );
        REQUIRE( nested.size() == 2 );

        craft_reservation_index::record claim;
        claim.craft_uid = 987654;
        claim.expires_at = calendar::turn + 1_hours;
        claim.provider_item_uids.push_back( nested[0]->uid().get_value() );
        get_craft_reservations().set( claim );

        item &on_map = place_craft( { charged( usage_from::map, 50 ) } );
        item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the sibling is unavailable too, since the whole root is claimed" ) {
                CHECK( on_map.get_reservations().empty() );
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }
        }
    }
}

TEST_CASE( "reservation_reuses_one_provider_across_groups",
           "[craft][attention][reservation][binding]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_two_qualities.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "one multitool covering both quality groups" ) {
        item &multitool = here.add_item( origin, item( itype_test_reserve_multitool_ab ) );
        const int64_t multitool_uid = multitool.uid().get_value();

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "both groups bind it, since reuse across groups costs nothing" ) {
                REQUIRE( on_map.get_reservations().size() == 2 );
                CHECK( on_map.get_reservations()[0].provider_uid == multitool_uid );
                CHECK( on_map.get_reservations()[1].provider_uid == multitool_uid );
                CHECK( on_map.get_reservations()[0].group_index !=
                       on_map.get_reservations()[1].group_index );
            }
        }
    }

    GIVEN( "one multitool and one single-quality tool" ) {
        here.add_item( origin, item( itype_test_reserve_tool_a ) );
        item &multitool = here.add_item( origin, item( itype_test_reserve_multitool_ab ) );
        const int64_t multitool_uid = multitool.uid().get_value();

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the group only the multitool can serve still gets it" ) {
                REQUIRE( on_map.get_reservations().size() == 2 );
                CHECK( std::any_of( on_map.get_reservations().begin(),
                                    on_map.get_reservations().end(),
                [multitool_uid]( const craft_reservation::binding & b ) {
                    return b.provider_uid == multitool_uid;
                } ) );
            }
        }
    }
}

TEST_CASE( "reservation_measures_a_synthesized_provider_as_prepared",
           "[craft][attention][reservation][binding]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms beside( 61, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_powered_qual.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "a powered vehicle rig whose quality depends on its charges" ) {
        vehicle *veh = here.add_vehicle( vehicle_prototype_none, beside, 0_degrees, 0,
                                         veh_spawn_status::UNDAMAGED );
        REQUIRE( veh != nullptr );
        REQUIRE( veh->install_part( here, point_rel_ms::zero, vpart_frame ) != -1 );
        REQUIRE( veh->install_part( here, point_rel_ms::zero,
                                    vpart_small_storage_battery ) != -1 );
        REQUIRE( veh->install_part( here, point_rel_ms::zero,
                                    vpart_test_vp_reserve_qual ) != -1 );
        veh->refresh();
        here.add_vehicle_to_cache( veh );
        veh->charge_battery( here, 500 );

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the part binds, since the prepared tool carries the charges" ) {
                REQUIRE( on_map.get_reservations().size() == 1 );
                CHECK( on_map.get_reservations()[0].kind ==
                       craft_reservation::provider_kind::vehicle_part );
            }
        }
    }
}

TEST_CASE( "reservation_shares_one_abstract_source_across_groups",
           "[craft][attention][reservation][multiset]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_two_qualities.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "one mutation supplying both groups' qualities" ) {
        u.set_mutation( trait_TEST_RESERVE_QUALITIES );
        REQUIRE( u.has_intrinsic_quality( qual_TEST_RESERVE_A, 1, 1 ) );

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "both groups bind it without claiming a second occurrence" ) {
                REQUIRE( on_map.get_reservations().size() == 2 );
                CHECK( on_map.get_reservations()[0].kind ==
                       craft_reservation::provider_kind::intrinsic );
                CHECK( on_map.get_reservations()[1].kind ==
                       craft_reservation::provider_kind::intrinsic );
                CHECK( on_map.get_reservations()[0].group_index !=
                       on_map.get_reservations()[1].group_index );
            }
        }
    }
}

TEST_CASE( "reservation_discovery_skips_an_unreachable_provider",
           "[craft][attention][reservation][discovery]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms walled_in( 62, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_water, calendar::turn );
    item placed( &recipe_water_clean_test_unattended_boil.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "the only pot sealed inside a ring of walls" ) {
        here.add_item( walled_in, item( itype_pot ) );
        for( int x = 61; x <= 63; ++x ) {
            for( int y = 59; y <= 61; ++y ) {
                const tripoint_bub_ms wall( x, y, 0 );
                if( wall != walled_in ) {
                    here.ter_set( wall, ter_t_wall );
                }
            }
        }
        here.invalidate_map_cache( 0 );
        here.build_map_cache( 0, true );

        inventory reachable;
        reachable.form_from_map( &here, origin, PICKUP_RANGE, &u );
        REQUIRE_FALSE( reachable.has_quality( qual_BOIL, 1, 1 ) );

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "nothing binds, since the gate cannot reach it either" ) {
                CHECK( on_map.get_reservations().empty() );
            }
        }
    }
}

TEST_CASE( "reservation_binds_an_intrinsic_presence_tool",
           "[craft][attention][reservation][binding][intrinsic]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    GIVEN( "a step whose presence tool only a bionic supplies" ) {
        u.add_bionic( test_bio_reserve_two_pseudo );
        item component( itype_2x4 );
        item placed( &recipe_cudgel_test_unattended_intrinsic_tool.obj(), 1, component );
        item &on_map = here.add_item( origin, placed );
        REQUIRE( on_map.is_craft() );
        on_map.set_current_step( 0 );
        on_map.set_crafter_id( u.getID() );
        on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
        step_tool_alloc alloc;
        alloc.sel.use_from = usage_from::player;
        alloc.sel.comp.type = itype_test_reserve_bionic_tool_1;
        alloc.sel.comp.count = -1;
        on_map.set_step_tool_allocs( { { alloc } } );
        item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

        WHEN( "the passive step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the binding names the pseudo item type" ) {
                REQUIRE( on_map.get_reservations().size() == 1 );
                const craft_reservation::binding &b = on_map.get_reservations()[0];
                CHECK( b.kind == craft_reservation::provider_kind::intrinsic );
                CHECK( b.req == craft_reservation::requirement_kind::presence_tool );
                CHECK( b.pseudo_type == itype_test_reserve_bionic_tool_1 );
                CHECK( b.occurrence_slot >= 0 );
            }

            THEN( "the step is not paused for want of a provider" ) {
                CHECK( on_map.get_reservation_pause_reason() == 0 );
            }
        }
    }
}

TEST_CASE( "reservation_sizes_a_group_by_the_alternative_it_chose",
           "[craft][attention][reservation][binding]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_qual_or_counts.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "one provider of the alternative that asks for one" ) {
        here.add_item( origin, item( itype_test_reserve_tool_a ) );

        WHEN( "the passive step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the group is covered by that single provider" ) {
                REQUIRE( on_map.get_reservations().size() == 1 );
                CHECK( on_map.get_reservations()[0].qual == qual_TEST_RESERVE_A );
                CHECK( on_map.get_reservation_pause_reason() == 0 );
            }
        }
    }

    GIVEN( "one provider of the alternative that asks for two" ) {
        here.add_item( origin, item( itype_test_reserve_tool_b ) );

        WHEN( "the passive step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the step pauses rather than covering the group half way" ) {
                CHECK( on_map.get_reservations().empty() );
                CHECK( on_map.get_reservation_pause_reason() != 0 );
            }
        }
    }
}

TEST_CASE( "reservation_refuses_a_broken_provider", "[craft][attention][reservation][binding]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_furn_qual.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "the only candidate is broken" ) {
        item broken( itype_test_reserve_tool_a );
        broken.set_flag( flag_ITEM_BROKEN );
        here.add_item( origin, broken );

        WHEN( "the passive step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "nothing is bound, since a broken tool satisfies no requirement" ) {
                CHECK( on_map.get_reservations().empty() );
                CHECK( on_map.get_reservation_pause_reason() != 0 );
            }
        }
    }
}

TEST_CASE( "reservation_counts_liquid_providers_post_merge",
           "[craft][attention][reservation][binding][liquid]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms east( 61, 60, 0 );
    const tripoint_bub_ms west( 59, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_two_of_a.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "two kegs holding the same liquid" ) {
        here.furn_set( east, furn_f_standing_tank );
        here.furn_set( west, furn_f_standing_tank );
        here.add_item( east, item( itype_test_reserve_liquid, calendar::turn, 10 ) );
        here.add_item( west, item( itype_test_reserve_liquid, calendar::turn, 10 ) );
        REQUIRE( craft_reservation::merge_equivalent(
                     item( itype_test_reserve_liquid, calendar::turn, 10 ),
                     item( itype_test_reserve_liquid, calendar::turn, 10 ) ) );

        WHEN( "a step demanding two providers is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the generated rows are one provider and nothing is bound" ) {
                CHECK( on_map.get_reservations().empty() );
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }
        }
    }

    GIVEN( "a keg and a carried container holding the same liquid" ) {
        here.furn_set( east, furn_f_standing_tank );
        here.add_item( east, item( itype_test_reserve_liquid, calendar::turn, 10 ) );
        item canteen( itype_canteen, calendar::turn );
        REQUIRE( canteen.put_in( item( itype_test_reserve_liquid, calendar::turn, 10 ),
                                 pocket_type::CONTAINER ).success() );
        u.i_add( canteen );

        WHEN( "a step demanding two providers is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the merge class spans both kinds and nothing is bound" ) {
                CHECK( on_map.get_reservations().empty() );
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }
        }
    }

    GIVEN( "two carried liquids that do not merge" ) {
        item marked( itype_test_reserve_liquid, calendar::turn, 10 );
        marked.set_favorite( true );
        REQUIRE( !craft_reservation::merge_equivalent(
                     item( itype_test_reserve_liquid, calendar::turn, 10 ), marked ) );

        item first( itype_canteen, calendar::turn );
        REQUIRE( first.put_in( item( itype_test_reserve_liquid, calendar::turn, 10 ),
                               pocket_type::CONTAINER ).success() );
        item second( itype_canteen, calendar::turn );
        REQUIRE( second.put_in( marked, pocket_type::CONTAINER ).success() );
        u.i_add( first );
        u.i_add( second );

        WHEN( "a step demanding two providers is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "they are two providers and the step keeps running" ) {
                CHECK( on_map.get_reservations().size() == 2 );
                CHECK( on_map.get_pause_started_at() == calendar::before_time_starts );
            }
        }
    }
}

TEST_CASE( "reservation_pauses_when_two_bound_liquids_become_merge_equivalent",
           "[craft][attention][reservation][validate][liquid]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_two_of_a.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    const time_point t0 = calendar::turn;

    GIVEN( "two bound liquids kept apart by one being a favorite" ) {
        item marked( itype_test_reserve_liquid, calendar::turn, 10 );
        marked.set_favorite( true );
        item first( itype_canteen, calendar::turn );
        REQUIRE( first.put_in( item( itype_test_reserve_liquid, calendar::turn, 10 ),
                               pocket_type::CONTAINER ).success() );
        item second( itype_canteen, calendar::turn );
        REQUIRE( second.put_in( marked, pocket_type::CONTAINER ).success() );
        u.i_add( first );
        item_location held = u.i_add( second );

        craft_stamp_passive_entry( on_map, u, t0, loc );
        REQUIRE( on_map.get_reservations().size() == 2 );

        WHEN( "the two become merge-equivalent" ) {
            held->only_item().set_favorite( false );
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "the step pauses, since two bindings cannot share one entry" ) {
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }
        }

        WHEN( "they stay distinct" ) {
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "the step keeps running" ) {
                CHECK( on_map.get_pause_started_at() == calendar::before_time_starts );
            }
        }
    }
}

TEST_CASE( "reservation_prefers_a_free_member_of_a_liquid_merge_class",
           "[craft][attention][reservation][binding][liquid]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_furn_qual.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "two equivalent carried liquids, one claimed by another craft" ) {
        item first( itype_canteen, calendar::turn );
        REQUIRE( first.put_in( item( itype_test_reserve_liquid, calendar::turn, 10 ),
                               pocket_type::CONTAINER ).success() );
        item second( itype_canteen, calendar::turn );
        REQUIRE( second.put_in( item( itype_test_reserve_liquid, calendar::turn, 10 ),
                                pocket_type::CONTAINER ).success() );
        const item_location claimed = u.i_add( first );
        const item_location free_one = u.i_add( second );
        REQUIRE( craft_reservation::merge_equivalent( claimed->only_item(),
                 free_one->only_item() ) );

        craft_reservation_index::record rec;
        rec.craft_uid = 4242;
        rec.expires_at = calendar::turn + 1_hours;
        rec.provider_item_uids.push_back( claimed->only_item().uid().get_value() );
        get_craft_reservations().set( rec );

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the unclaimed member represents the class" ) {
                REQUIRE( on_map.get_reservations().size() == 1 );
                CHECK( on_map.get_reservations()[0].provider_uid ==
                       free_one->only_item().uid().get_value() );
            }
        }
    }
}

TEST_CASE( "reservation_lets_one_liquid_cover_two_groups",
           "[craft][attention][reservation][validate][liquid]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_liquid_reuse.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::player;
    alloc.sel.comp.type = itype_test_reserve_liquid;
    alloc.sel.comp.count = -1;
    on_map.set_step_tool_allocs( { { alloc } } );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    const time_point t0 = calendar::turn;

    GIVEN( "one carried liquid covering both the quality and the presence tool" ) {
        item canteen( itype_canteen, calendar::turn );
        REQUIRE( canteen.put_in( item( itype_test_reserve_liquid, calendar::turn, 10 ),
                                 pocket_type::CONTAINER ).success() );
        u.i_add( canteen );

        craft_stamp_passive_entry( on_map, u, t0, loc );
        REQUIRE( on_map.get_reservations().size() == 2 );
        REQUIRE( on_map.get_reservations()[0].provider_uid ==
                 on_map.get_reservations()[1].provider_uid );

        WHEN( "the step revalidates" ) {
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "two bindings naming one provider are one occupied class" ) {
                CHECK( on_map.get_pause_started_at() == calendar::before_time_starts );
            }
        }
    }
}

TEST_CASE( "reservation_keeps_group_slots_distinct_across_an_abstract_source",
           "[craft][attention][reservation][binding][multiset]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_two_groups_of_a.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "two intrinsic occurrences shared by a one-provider and a two-provider group" ) {
        u.set_mutation( trait_TEST_RESERVE_QUALITIES );
        u.set_mutation( trait_TEST_RESERVE_QUALITIES_2 );
        REQUIRE( u.has_intrinsic_quality( qual_TEST_RESERVE_A, 1, 2 ) );

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the two-provider group holds two distinct slots" ) {
                REQUIRE( on_map.get_reservations().size() == 3 );
                std::set<int> wide_slots;
                for( const craft_reservation::binding &b : on_map.get_reservations() ) {
                    if( b.group_count == 2 ) {
                        wide_slots.insert( b.occurrence_slot );
                    }
                }
                CHECK( wide_slots.size() == 2 );
            }
        }
    }

    GIVEN( "a single intrinsic occurrence and a group that demands two" ) {
        u.set_mutation( trait_TEST_RESERVE_QUALITIES );
        REQUIRE( !u.has_intrinsic_quality( qual_TEST_RESERVE_A, 1, 2 ) );

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the wide group is not covered by reusing the narrow group's slot" ) {
                CHECK( on_map.get_reservations().empty() );
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }
        }
    }
}

TEST_CASE( "reservation_offers_a_tried_provider_to_the_next_alternative",
           "[craft][attention][reservation][binding]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_qual_or_wide_first.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "one tool covering both alternatives, too few for the wide one" ) {
        here.add_item( origin, item( itype_test_reserve_multitool_ab, calendar::turn ) );

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the narrow alternative binds it" ) {
                REQUIRE( on_map.get_reservations().size() == 1 );
                CHECK( on_map.get_reservations()[0].qual == qual_TEST_RESERVE_B );
            }
        }
    }
}

TEST_CASE( "reservation_preflight_ignores_charges_another_craft_holds",
           "[craft][attention][reservation][binding][charges]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_qual_and_charges.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::player;
    alloc.sel.comp.type = itype_test_reserve_charged_tool;
    alloc.sel.comp.count = 50;
    alloc.step_count_units = 50;
    on_map.set_step_tool_allocs( { { alloc } } );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "the only charge source carried and claimed by another craft" ) {
        u.i_add( item( itype_test_reserve_tool_a, calendar::turn ) );
        item powered( itype_test_reserve_charged_tool, calendar::turn );
        powered.ammo_set( itype_battery, 100 );
        const item_location claimed = u.i_add( powered );

        craft_reservation_index::record rec;
        rec.craft_uid = 4242;
        rec.expires_at = calendar::turn + 1_hours;
        rec.provider_item_uids.push_back( claimed->uid().get_value() );
        get_craft_reservations().set( rec );

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the preflight refuses rather than committing against them" ) {
                CHECK( on_map.get_reservations().empty() );
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }
        }
    }
}

TEST_CASE( "reservation_search_escalates_then_defers_on_an_unchanged_pool",
           "[craft][attention][reservation][search]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms tool_pos( 61, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_three_of_a.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );

    // Enough charges left over for any two providers to be hidden and not for any three,
    // so every complete assignment fails and the search has to walk the whole space.
    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::map;
    alloc.sel.comp.type = itype_test_reserve_charged_tool;
    alloc.sel.comp.count = 2050;
    alloc.step_count_units = 2050;
    on_map.set_step_tool_allocs( { { alloc } } );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    // Distinct charge loads, so no two collapse into one equivalence class and the
    // branching factor is the whole pool rather than one representative.
    for( int i = 0; i < 21; ++i ) {
        item tool( itype_test_reserve_charged_tool );
        tool.ammo_set( itype_battery, 100 + i );
        here.add_item( tool_pos, tool );
    }
    u.invalidate_crafting_inventory();

    const time_point t0 = calendar::turn;

    GIVEN( "a pool wide enough to exhaust the search budget" ) {
        craft_reservation::reset_search_expansions();
        craft_stamp_passive_entry( on_map, u, t0, loc );

        THEN( "the first attempt spends its budget and concludes nothing" ) {
            CHECK( on_map.get_reservations().empty() );
            CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            CHECK( craft_reservation::search_expansions_total() ==
                   craft_reservation::search_budget_for_attempt( 0 ) );
            CHECK( on_map.get_reservation_search_attempts() == 1 );
            CHECK( on_map.get_reservation_pool_fingerprint() != 0 );
        }

        WHEN( "the next check runs against the same pool" ) {
            craft_reservation::reset_search_expansions();
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "the budget doubles rather than repeating the identical search" ) {
                CHECK( craft_reservation::search_expansions_total() ==
                       craft_reservation::search_budget_for_attempt( 1 ) );
                CHECK( on_map.get_reservation_search_attempts() == 2 );
            }
        }

        WHEN( "the craft sits at the escalation cap" ) {
            on_map.set_reservation_search_attempts( craft_reservation::search_escalation_cap );

            THEN( "an untouched pool costs no expansion at all" ) {
                craft_reservation::reset_search_expansions();
                Messages::clear_messages();
                craft_actualize_scheduled( on_map, item_wakeup_kind::env_check,
                                           t0 + 1_minutes, loc );
                CHECK( craft_reservation::search_expansions_total() == 0 );
                CHECK( on_map.get_reservation_search_attempts() ==
                       craft_reservation::search_escalation_cap );
                // No search ran and the world has not moved, so there is nothing new to
                // report and the stored reason stands.
                CHECK( Messages::size() == 0 );
            }

            THEN( "one more qualifying tool makes the next check search again and bind" ) {
                item extra( itype_test_reserve_charged_tool );
                extra.ammo_set( itype_battery, 150 );
                here.add_item( tool_pos, extra );
                u.invalidate_crafting_inventory();
                craft_reservation::reset_search_expansions();
                craft_actualize_scheduled( on_map, item_wakeup_kind::env_check,
                                           t0 + 1_minutes, loc );
                CHECK( craft_reservation::search_expansions_total() > 0 );
                CHECK( on_map.get_reservations().size() == 3 );
                CHECK( on_map.get_reservation_search_attempts() == 0 );
            }
        }

        WHEN( "the player resumes the craft by hand" ) {
            craft_apply_resume_replan( loc );

            THEN( "the next search runs regardless of what the last one concluded" ) {
                CHECK( on_map.get_reservation_search_attempts() == 0 );
                CHECK( on_map.get_reservation_pool_fingerprint() == 0 );
            }
        }
    }
}

TEST_CASE( "reservation_preflight_counts_one_shared_battery_once",
           "[craft][attention][reservation][charges]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms beside( 61, 60, 0 );
    u.setpos( here, origin );

    vehicle *veh = here.add_vehicle( vehicle_prototype_none, beside, 0_degrees, 0,
                                     veh_spawn_status::UNDAMAGED );
    REQUIRE( veh != nullptr );
    REQUIRE( veh->install_part( here, point_rel_ms::zero, vpart_frame ) != -1 );
    REQUIRE( veh->install_part( here, point_rel_ms::zero, vpart_small_storage_battery ) != -1 );
    veh->refresh();
    here.add_vehicle_to_cache( veh );
    veh->charge_battery( here, 100, false );
    REQUIRE( veh->battery_left( here ) == 100 );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_qual_and_charges.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );

    // More than the one battery holds and less than two tools would report between them.
    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::map;
    alloc.sel.comp.type = itype_test_reserve_cabled_tool;
    alloc.sel.comp.count = 150;
    alloc.step_count_units = 150;
    on_map.set_step_tool_allocs( { { alloc } } );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "three empty tools plugged into one vehicle battery" ) {
        for( int i = 0; i < 3; ++i ) {
            item &tool = here.add_item( origin, item( itype_test_reserve_cabled_tool ) );
            REQUIRE( tool.can_link_up() );
            REQUIRE( tool.link_to( here.veh_at( beside ), link_state::automatic ).success() );
            REQUIRE( tool.ammo_remaining() == 0 );
            REQUIRE( tool.ammo_remaining_linked( here ) == 100 );
        }
        u.invalidate_crafting_inventory();

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "the shared charge is not counted once per tool" ) {
                CHECK( on_map.get_reservations().empty() );
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
                CHECK( veh->battery_left( here ) == 100 );
            }
        }
    }
}

TEST_CASE( "reservation_fingerprint_reads_the_power_behind_a_carried_tool",
           "[craft][attention][reservation][search]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms tool_pos( 61, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_three_of_a.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );

    // Sized so two hidden providers still clear the draw and any three fall short,
    // which is what makes the search walk the whole space before refusing the step.
    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::both;
    alloc.sel.comp.type = itype_test_reserve_bionic_powered_tool;
    alloc.sel.comp.count = 3300;
    alloc.step_count_units = 3300;
    on_map.set_step_tool_allocs( { { alloc } } );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    // Distinct charge loads, so no two collapse into one class and the branching factor
    // is the whole pool.
    for( int i = 0; i < 30; ++i ) {
        item tool( itype_test_reserve_bionic_powered_tool );
        tool.ammo_set( itype_battery, 100 + i );
        here.add_item( tool_pos, tool );
    }
    // Empty and carried, so everything it contributes to the draw is bionic power.  That
    // term moves without any item moving, which no class key can see.
    u.worn.wear_item( u, item( itype_debug_backpack ), false, false );
    REQUIRE( u.i_add( item( itype_test_reserve_bionic_powered_tool ) ) );
    u.set_max_power_level( units::from_kilojoule( 300 ) );
    u.set_power_level( units::from_kilojoule( 150 ) );
    u.invalidate_crafting_inventory();

    const time_point t0 = calendar::turn;

    GIVEN( "a capped craft whose candidate set has stopped changing" ) {
        craft_stamp_passive_entry( on_map, u, t0, loc );
        on_map.set_reservation_search_attempts( craft_reservation::search_escalation_cap );
        craft_reservation::reset_search_expansions();
        craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );
        REQUIRE( craft_reservation::search_expansions_total() == 0 );

        WHEN( "the bionic reserve fills while every item stays put" ) {
            u.set_power_level( units::from_kilojoule( 250 ) );
            u.invalidate_crafting_inventory();
            craft_reservation::reset_search_expansions();
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 2_minutes, loc );

            THEN( "the next check searches again and clears the escalation" ) {
                CHECK( craft_reservation::search_expansions_total() > 0 );
                CHECK( on_map.get_reservation_search_attempts() == 0 );
            }
        }
    }
}

TEST_CASE( "reservation_released_when_an_edit_makes_the_step_attended",
           "[craft][attention][reservation][lifecycle]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms tool_pos( 61, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_presence_tool.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 1 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 2 ) );

    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::map;
    alloc.sel.comp.type = itype_hammer;
    alloc.sel.comp.count = -1;
    on_map.set_step_tool_allocs( { {}, { alloc } } );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    item &hammer = here.add_item( tool_pos, item( itype_hammer ) );
    const int64_t hammer_uid = hammer.uid().get_value();

    const time_point t0 = calendar::turn;

    GIVEN( "a live unattended step holding a bound tool" ) {
        craft_stamp_passive_entry( on_map, u, t0, loc );
        REQUIRE_FALSE( on_map.get_reservations().empty() );
        REQUIRE( get_craft_reservations().is_reserved_uid( hammer_uid ) );

        WHEN( "a recipe edit leaves the craft on a step that is now attended" ) {
            on_map.set_current_step( 0 );
            craft_actualize_scheduled( on_map, item_wakeup_kind::ready_check,
                                       on_map.get_ready_at(), loc );

            THEN( "the claims go with the step rather than waiting out the lease" ) {
                CHECK( on_map.get_reservations().empty() );
                CHECK_FALSE( get_craft_reservations().is_reserved_uid( hammer_uid ) );
            }
        }
    }
}

TEST_CASE( "reservation_reuses_a_held_provider_across_groups",
           "[craft][attention][reservation][binding]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms tool_pos( 61, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_two_qualities.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );

    // Met by hiding one multitool and not by hiding two.
    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::map;
    alloc.sel.comp.type = itype_test_reserve_multitool_ab_charged;
    alloc.sel.comp.count = 90;
    alloc.step_count_units = 90;
    on_map.set_step_tool_allocs( { { alloc } } );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    item &first = here.add_item( tool_pos, item( itype_test_reserve_multitool_ab_charged ) );
    item &second = here.add_item( tool_pos, item( itype_test_reserve_multitool_ab_charged ) );
    first.ammo_set( itype_battery, 100 );
    second.ammo_set( itype_battery, 100 );
    const int64_t second_uid = second.uid().get_value();
    u.invalidate_crafting_inventory();

    const time_point t0 = calendar::turn;

    GIVEN( "a craft holding one group on the second of two equivalent multitools" ) {
        craft_stamp_passive_entry( on_map, u, t0, loc );
        REQUIRE( on_map.get_reservations().size() == 2 );

        std::vector<craft_reservation::binding> held = on_map.get_reservations();
        held.erase( std::remove_if( held.begin(), held.end(),
        []( const craft_reservation::binding & b ) {
            return b.group_index != 0;
        } ), held.end() );
        REQUIRE( held.size() == 1 );
        held[0].provider_uid = second_uid;
        on_map.set_reservations( held );
        get_craft_reservations().rebuild_for_craft( loc );
        u.invalidate_crafting_inventory();

        WHEN( "the uncovered group is resolved on the next check" ) {
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "it reuses the held provider rather than opening one it cannot afford" ) {
                REQUIRE( on_map.get_reservations().size() == 2 );
                for( const craft_reservation::binding &b : on_map.get_reservations() ) {
                    CHECK( b.provider_uid == second_uid );
                }
            }
        }
    }
}

TEST_CASE( "reservation_fingerprint_reads_the_unit_spread_of_a_class",
           "[craft][attention][reservation][search]" )
{
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms tool_pos( 61, 60, 0 );

    // Four members of one class, spread over `spread` containers, and one search that
    // runs out of budget so a fingerprint is stored.
    const auto fingerprint_for = [&]( const std::vector<int> &spread ) -> uint64_t {
        clear_avatar();
        clear_map();
        u.setpos( here, origin );

        item ingredient( itype_2x4, calendar::turn );
        item placed( &recipe_cudgel_test_unattended_three_of_a_four_of_b.obj(), 1, ingredient );
        item &on_map = here.add_item( origin, placed );
        REQUIRE( on_map.is_craft() );
        on_map.set_current_step( 0 );
        on_map.set_crafter_id( u.getID() );
        on_map.set_step_plans( std::vector<attention_plan>( 1 ) );

        step_tool_alloc alloc;
        alloc.sel.use_from = usage_from::map;
        alloc.sel.comp.type = itype_test_reserve_charged_tool;
        alloc.sel.comp.count = 2050;
        alloc.step_count_units = 2050;
        on_map.set_step_tool_allocs( { { alloc } } );
        item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

        // Distinct charge loads, so the wide group branches over the whole pool and the
        // search runs out of budget rather than settling.
        for( int i = 0; i < 21; ++i )
        {
            item tool( itype_test_reserve_charged_tool );
            tool.ammo_set( itype_battery, 100 + i );
            here.add_item( tool_pos, tool );
        }
        // Chargeless, so how many share a container changes what taking them all hides
        // without changing any class key.
        for( const int members : spread )
        {
            item bag( itype_backpack );
            for( int i = 0; i < members; ++i ) {
                bag.put_in( item( itype_test_reserve_tool_b ), pocket_type::CONTAINER );
            }
            here.add_item( tool_pos, bag );
        }
        u.invalidate_crafting_inventory();

        craft_stamp_passive_entry( on_map, u, calendar::turn, loc );
        REQUIRE( on_map.get_reservations().empty() );
        REQUIRE( on_map.get_reservation_search_attempts() == 1 );
        const uint64_t hash = on_map.get_reservation_pool_fingerprint();
        REQUIRE( hash != 0 );
        return hash;
    };

    GIVEN( "four members of one class spread over containers two ways" ) {
        const uint64_t three_and_one = fingerprint_for( { 3, 1 } );
        const uint64_t two_and_two = fingerprint_for( { 2, 2 } );

        THEN( "the same total over a different spread hashes differently" ) {
            CHECK( three_and_one != two_and_two );
        }

        THEN( "rebuilding the same spread hashes the same, so the difference is the spread" ) {
            CHECK( fingerprint_for( { 3, 1 } ) == three_and_one );
        }
    }
}

TEST_CASE( "reservation_excludes_a_whole_provider_tile_across_both_kinds",
           "[craft][attention][reservation][search]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms loaded_tile( 61, 60, 0 );
    const tripoint_bub_ms bare_tile( 62, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_tree_qual.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );

    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::map;
    alloc.sel.comp.type = itype_test_reserve_pseudo_kiln_charged;
    alloc.sel.comp.count = 60;
    alloc.step_count_units = 60;
    on_map.set_step_tool_allocs( { { alloc } } );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "two qualifying trees, one sharing its tile with the only charge source" ) {
        here.ter_set( loaded_tile, ter_t_tree );
        here.ter_set( bare_tile, ter_t_tree );
        here.furn_set( loaded_tile, furn_test_f_reserve_charged );
        here.add_item( loaded_tile, item( itype_battery, calendar::turn, 100 ) );
        u.invalidate_crafting_inventory();

        WHEN( "the step is entered" ) {
            craft_stamp_passive_entry( on_map, u, calendar::turn, loc );

            THEN( "it binds the bare tile and keeps the charges it still needs" ) {
                REQUIRE( on_map.get_reservations().size() == 1 );
                CHECK( on_map.get_reservations()[0].tile == here.get_abs( bare_tile ) );
                CHECK( on_map.get_pause_started_at() == calendar::before_time_starts );
            }
        }
    }
}

TEST_CASE( "reservation_does_not_renew_a_lapsed_site_lease",
           "[craft][attention][reservation][lifecycle]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_only_unattended.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );

    // Charges nothing in range can pay, so every tick pauses and none of them completes.
    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::map;
    alloc.sel.comp.type = itype_test_reserve_charged_tool;
    alloc.sel.comp.count = 50;
    alloc.step_count_units = 50;
    on_map.set_step_tool_allocs( { { alloc } } );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    const time_point t0 = calendar::turn;

    GIVEN( "a site-only step that pauses on its entry debit" ) {
        craft_stamp_passive_entry( on_map, u, t0, loc );
        REQUIRE( on_map.get_reservations().empty() );
        REQUIRE( on_map.get_reserved_tile().has_value() );
        REQUIRE( on_map.get_pause_started_at() != calendar::before_time_starts );
        REQUIRE( on_map.get_reservation_expiry() == t0 + 1_hours );

        WHEN( "a check runs after the lease has lapsed" ) {
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 2_hours, loc );

            THEN( "the deadline stands rather than being minted again" ) {
                CHECK( on_map.get_reservation_expiry() == t0 + 1_hours );
            }

            THEN( "the index copy stands with it" ) {
                const craft_reservation_index::record *rec =
                    get_craft_reservations().find( on_map.peek_reservation_owner_token() );
                REQUIRE( rec != nullptr );
                CHECK( rec->expires_at == t0 + 1_hours );
            }
        }
    }
}

TEST_CASE( "reservation_counts_a_carried_held_provider_once",
           "[craft][attention][reservation][charges]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    u.worn.wear_item( u, item( itype_backpack ), false, false );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_two_qualities.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );

    // Met by hiding one multitool and not by hiding two.
    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::player;
    alloc.sel.comp.type = itype_test_reserve_multitool_ab_charged;
    alloc.sel.comp.count = 90;
    alloc.step_count_units = 90;
    on_map.set_step_tool_allocs( { { alloc } } );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    // Separate roots, or hiding either one hides the pack holding both.
    item_location packed = u.i_add( item( itype_test_reserve_multitool_ab_charged ) );
    REQUIRE( packed );
    packed->ammo_set( itype_battery, 100 );
    item wielded_tool( itype_test_reserve_multitool_ab_charged );
    wielded_tool.ammo_set( itype_battery, 100 );
    REQUIRE( u.wield( wielded_tool ) );
    item_location held = u.get_wielded_item();
    REQUIRE( held );
    const int64_t second_uid = held->uid().get_value();
    u.invalidate_crafting_inventory();

    const time_point t0 = calendar::turn;

    GIVEN( "a craft holding one carried group on the second of two multitools" ) {
        craft_stamp_passive_entry( on_map, u, t0, loc );
        REQUIRE( on_map.get_reservations().size() == 2 );

        std::vector<craft_reservation::binding> held = on_map.get_reservations();
        held.erase( std::remove_if( held.begin(), held.end(),
        []( const craft_reservation::binding & b ) {
            return b.group_index != 0;
        } ), held.end() );
        REQUIRE( held.size() == 1 );
        held[0].provider_uid = second_uid;
        on_map.set_reservations( held );
        get_craft_reservations().rebuild_for_craft( loc );
        u.invalidate_crafting_inventory();

        WHEN( "the uncovered group is resolved on the next check" ) {
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "the held provider is charged against the pool once, so the step runs" ) {
                REQUIRE( on_map.get_reservations().size() == 2 );
                for( const craft_reservation::binding &b : on_map.get_reservations() ) {
                    CHECK( b.provider_uid == second_uid );
                }
                CHECK( on_map.get_pause_started_at() == calendar::before_time_starts );
            }
        }
    }
}

TEST_CASE( "reservation_checks_the_pseudo_tool_a_provider_tile_supplies",
           "[craft][attention][reservation][validate]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms bench_pos( 61, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_furn_qual.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    here.furn_set( bench_pos, furn_test_f_reserve_qual );
    const time_point t0 = calendar::turn;

    GIVEN( "a step bound to a furniture pseudo tool" ) {
        craft_stamp_passive_entry( on_map, u, t0, loc );
        REQUIRE( on_map.get_reservations().size() == 1 );
        REQUIRE( on_map.get_reservations()[0].kind ==
                 craft_reservation::provider_kind::furniture );

        WHEN( "the tile keeps its furniture but supplies a different tool" ) {
            std::vector<craft_reservation::binding> bound = on_map.get_reservations();
            bound[0].pseudo_type = itype_test_reserve_pseudo_kiln_b;
            on_map.set_reservations( bound );
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "the step pauses rather than validating the tool it no longer has" ) {
                CHECK( on_map.get_pause_started_at() != calendar::before_time_starts );
            }
        }

        WHEN( "the tool is unchanged" ) {
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "the step keeps running" ) {
                CHECK( on_map.get_pause_started_at() == calendar::before_time_starts );
            }
        }
    }
}

TEST_CASE( "reserved_providers_are_hidden_from_crafting_inventory",
           "[craft][attention][reservation][enforcement]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms pot_pos( 61, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_water, calendar::turn );
    item placed( &recipe_water_clean_test_unattended_boil.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "a ground pot bound by a live step" ) {
        here.add_item( pot_pos, item( itype_pot ) );
        REQUIRE( u.crafting_inventory( origin, PICKUP_RANGE ).has_quality( qual_BOIL, 1, 1 ) );

        craft_stamp_passive_entry( on_map, u, calendar::turn, loc );
        REQUIRE( on_map.get_reservations().size() == 1 );

        THEN( "it is gone from the crafting inventory, including the owner's own" ) {
            u.invalidate_crafting_inventory();
            CHECK_FALSE( u.crafting_inventory( origin,
                                               PICKUP_RANGE ).has_quality( qual_BOIL, 1, 1 ) );
        }
    }

    GIVEN( "a carried pot bound by a live step" ) {
        u.i_add( item( itype_pot ) );
        u.invalidate_crafting_inventory();
        REQUIRE( u.crafting_inventory( origin, PICKUP_RANGE ).has_quality( qual_BOIL, 1, 1 ) );

        craft_stamp_passive_entry( on_map, u, calendar::turn, loc );
        REQUIRE( on_map.get_reservations().size() == 1 );

        THEN( "the crafter's own carried provider is hidden too" ) {
            u.invalidate_crafting_inventory();
            CHECK_FALSE( u.crafting_inventory( origin,
                                               PICKUP_RANGE ).has_quality( qual_BOIL, 1, 1 ) );
        }
    }

    GIVEN( "a bound pot inside a worn container" ) {
        item backpack( itype_backpack );
        REQUIRE( backpack.put_in( item( itype_pot ), pocket_type::CONTAINER ).success() );
        u.worn.wear_item( u, backpack, false, false );
        u.invalidate_crafting_inventory();
        REQUIRE( u.crafting_inventory( origin, PICKUP_RANGE ).has_quality( qual_BOIL, 1, 1 ) );

        craft_stamp_passive_entry( on_map, u, calendar::turn, loc );
        REQUIRE( on_map.get_reservations().size() == 1 );

        THEN( "the whole container is pruned, since inventory copies entire trees" ) {
            u.invalidate_crafting_inventory();
            CHECK_FALSE( u.crafting_inventory( origin,
                                               PICKUP_RANGE ).has_quality( qual_BOIL, 1, 1 ) );
        }
    }

    GIVEN( "two pots, one bound" ) {
        here.add_item( pot_pos, item( itype_pot ) );
        here.add_item( pot_pos, item( itype_pot ) );

        craft_stamp_passive_entry( on_map, u, calendar::turn, loc );
        REQUIRE( on_map.get_reservations().size() == 1 );

        THEN( "the free one is still offered" ) {
            u.invalidate_crafting_inventory();
            CHECK( u.crafting_inventory( origin, PICKUP_RANGE ).has_quality( qual_BOIL, 1, 1 ) );
        }
    }
}

TEST_CASE( "reserved_providers_are_hidden_from_automation_sources",
           "[craft][attention][reservation][enforcement]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms pot_pos( 61, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_water, calendar::turn );
    item placed( &recipe_water_clean_test_unattended_boil.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "a bound pot on the ground" ) {
        item &pot = here.add_item( pot_pos, item( itype_pot ) );
        const int64_t pot_uid = pot.uid().get_value();
        craft_stamp_passive_entry( on_map, u, calendar::turn, loc );
        REQUIRE( on_map.get_reservations().size() == 1 );
        REQUIRE( get_craft_reservations().is_reserved_uid( pot_uid ) );

        THEN( "zone sorting leaves it alone" ) {
            std::vector<item_location> no_activity_items;
            item &still_there = *here.i_at( pot_pos ).begin();
            CHECK( zone_sorting::sort_skip_item( u, &still_there, no_activity_items, false,
                                                 here.get_abs( pot_pos ), nullptr ) );
        }
    }

    GIVEN( "a live unattended craft in a sort zone" ) {
        here.add_item( pot_pos, item( itype_pot ) );
        craft_stamp_passive_entry( on_map, u, calendar::turn, loc );
        REQUIRE( on_map.get_passive_started_at() != calendar::before_time_starts );

        THEN( "the craft itself is not sorted away from its providers" ) {
            std::vector<item_location> no_activity_items;
            CHECK( zone_sorting::sort_skip_item( u, &on_map, no_activity_items, false,
                                                 here.get_abs( origin ), nullptr ) );
        }
    }
}

TEST_CASE( "automation_leaves_a_container_holding_a_live_craft",
           "[craft][attention][reservation][enforcement]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms bag_pos( 61, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item craft( &recipe_cudgel_test_only_unattended.obj(), 1, ingredient );
    REQUIRE( craft.is_craft() );
    craft.set_current_step( 0 );
    craft.set_crafter_id( u.getID() );
    craft.set_step_plans( std::vector<attention_plan>( 1 ) );

    item bag( itype_backpack );
    bag.put_in( craft, pocket_type::CONTAINER );
    item &bag_on_map = here.add_item( bag_pos, bag );
    item *nested = nullptr;
    bag_on_map.visit_items( [&nested]( item * node, item * ) {
        if( node->is_craft() ) {
            nested = node;
            return VisitResponse::ABORT;
        }
        return VisitResponse::NEXT;
    } );
    REQUIRE( nested != nullptr );
    item_location bag_loc( map_cursor( here.get_abs( bag_pos ) ), &bag_on_map );
    item_location craft_loc( bag_loc, nested );

    GIVEN( "a live site-only craft inside a container" ) {
        craft_stamp_passive_entry( *nested, u, calendar::turn, craft_loc );
        REQUIRE( nested->get_passive_started_at() != calendar::before_time_starts );
        // The site-only recipe binds nothing, so contains_reserved cannot stand in for
        // the craft check here.
        REQUIRE( nested->get_reservations().empty() );
        REQUIRE_FALSE( craft_reservation::contains_reserved( bag_on_map ) );

        THEN( "the container reads as holding a live craft" ) {
            CHECK( craft_reservation::contains_live_craft( bag_on_map ) );
        }

        THEN( "zone sorting leaves the container where it is" ) {
            std::vector<item_location> no_activity_items;
            CHECK( zone_sorting::sort_skip_item( u, &bag_on_map, no_activity_items, false,
                                                 here.get_abs( bag_pos ), nullptr ) );
        }
    }

    GIVEN( "the same container while the craft is not running" ) {
        REQUIRE( nested->get_passive_started_at() == calendar::before_time_starts );

        THEN( "nothing about it is off limits" ) {
            CHECK_FALSE( craft_reservation::contains_live_craft( bag_on_map ) );
            std::vector<item_location> no_activity_items;
            CHECK_FALSE( zone_sorting::sort_skip_item( u, &bag_on_map, no_activity_items, false,
                         here.get_abs( bag_pos ), nullptr ) );
        }
    }
}

TEST_CASE( "reservation_keeps_node_local_npc_selectors_off_a_bound_item",
           "[craft][attention][reservation][npc]" )
{
    g->faction_manager_ptr->create_if_needed();
    clear_avatar();
    clear_map();
    set_time_to_day();

    npc &guy = spawn_npc( point_bub_ms( 60, 62 ), "thug" );
    guy.clear_worn();
    guy.inv->clear();
    guy.remove_weapon();
    guy.wear_item( item( itype_debug_backpack ) );
    guy.set_attitude( NPCATT_NULL );

    const auto claim = [&]( const item_location & what ) {
        craft_reservation_index::record rec;
        rec.craft_uid = 909090 + what->uid().get_value();
        rec.provider_item_uids.push_back( what->uid().get_value() );
        rec.expires_at = calendar::turn + 1_hours;
        get_craft_reservations().set( rec );
        REQUIRE( get_craft_reservations().is_reserved_uid( what->uid().get_value() ) );
    };

    GIVEN( "a claimed food item and a free one in the same pack" ) {
        item_location claimed = guy.i_add( item( itype_test_vitfood ) );
        item_location free_meal = guy.i_add( item( itype_test_vitfood ) );
        REQUIRE( claimed );
        REQUIRE( free_meal );
        const int64_t claimed_uid = claimed->uid().get_value();
        const int64_t free_uid = free_meal->uid().get_value();
        claim( claimed );

        THEN( "the free meal is still offered, since eating is node-local" ) {
            std::vector<item_location> offered =
                guy.cache_get_items_with( "is_food", &item::is_food );
            offered.erase( std::remove_if( offered.begin(), offered.end(),
            []( const item_location & e ) {
                return !e || !craft_reservation::usable_by_automation( *e );
            } ), offered.end() );
            bool saw_free = false;
            bool saw_claimed = false;
            for( const item_location &e : offered ) {
                saw_free = saw_free || e->uid().get_value() == free_uid;
                saw_claimed = saw_claimed || e->uid().get_value() == claimed_uid;
            }
            CHECK( saw_free );
            CHECK_FALSE( saw_claimed );
        }
    }
}

TEST_CASE( "reservation_keeps_npc_pickup_off_a_bound_provider",
           "[craft][attention][reservation][npc][pickup]" )
{
    g->faction_manager_ptr->create_if_needed();
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms stack_pos( 61, 60, 0 );
    u.setpos( here, origin );

    npc &scavenger = spawn_npc( point_bub_ms( 60, 62 ), "thug" );
    scavenger.clear_worn();
    scavenger.inv->clear();
    scavenger.remove_weapon();
    scavenger.wear_item( item( itype_debug_backpack ) );
    scavenger.set_attitude( NPCATT_NULL );

    // A live craft on the map so its owner token is a real one, and a bound pot beside it.
    item ingredient( itype_water, calendar::turn );
    item placed( &recipe_water_clean_test_unattended_boil.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    item &reserved_pot = here.add_item( stack_pos, item( itype_pot ) );
    item &free_pot = here.add_item( stack_pos, item( itype_pot ) );
    const int64_t reserved_uid = reserved_pot.uid().get_value();
    const int64_t free_uid = free_pot.uid().get_value();
    craft_stamp_passive_entry( on_map, u, calendar::turn, loc );
    REQUIRE( on_map.get_reservations().size() == 1 );
    const int64_t bound_uid = on_map.get_reservations().front().provider_uid;
    // Binding picks one of the two; the other is the free instance for the whole case.
    const int64_t loose_uid = bound_uid == reserved_uid ? free_uid : reserved_uid;
    REQUIRE( get_craft_reservations().is_reserved_uid( bound_uid ) );
    REQUIRE_FALSE( get_craft_reservations().is_reserved_uid( loose_uid ) );

    GIVEN( "a stack holding one bound provider and one free item" ) {
        WHEN( "the executor sweeps the tile" ) {
            const std::list<item> taken = scavenger.pick_up_item_map( stack_pos );

            THEN( "the bound provider is left where it lies" ) {
                bool bound_still_there = false;
                for( const item &left : here.i_at( stack_pos ) ) {
                    bound_still_there = bound_still_there || left.uid().get_value() == bound_uid;
                }
                CHECK( bound_still_there );
                for( const item &got : taken ) {
                    CHECK( got.uid().get_value() != bound_uid );
                }
            }
        }
    }

    GIVEN( "the same stack in vehicle cargo" ) {
        const tripoint_bub_ms cart_pos( 62, 62, 0 );
        REQUIRE( here.add_vehicle( vehicle_prototype_test_shopping_cart, cart_pos, 0_degrees, 0,
                                   veh_spawn_status::UNDAMAGED ) != nullptr );
        std::optional<vpart_reference> cargo = here.veh_at( here.get_abs( cart_pos ) ).cargo();
        REQUIRE( cargo );
        item stowed_bound( itype_pot );
        const int64_t stowed_uid = stowed_bound.uid().get_value();
        REQUIRE( cargo->vehicle().add_item( here, cargo->part(), stowed_bound ) );
        item *in_cargo = nullptr;
        for( item &it : cargo->items() ) {
            in_cargo = &it;
        }
        REQUIRE( in_cargo != nullptr );
        craft_reservation_index::record rec;
        rec.craft_uid = on_map.peek_reservation_owner_token();
        rec.provider_item_uids.push_back( in_cargo->uid().get_value() );
        rec.expires_at = calendar::turn + 1_hours;
        get_craft_reservations().set( rec );
        const int64_t claimed_uid = in_cargo->uid().get_value();
        static_cast<void>( stowed_uid );
        REQUIRE( get_craft_reservations().is_reserved_uid( claimed_uid ) );

        WHEN( "the executor sweeps the cargo" ) {
            const std::list<item> taken =
                scavenger.pick_up_item_vehicle( cargo->vehicle(), cargo->part_index() );

            THEN( "the claimed item stays in the cargo space" ) {
                bool still_stowed = false;
                for( const item &left : cargo->items() ) {
                    still_stowed = still_stowed || left.uid().get_value() == claimed_uid;
                }
                CHECK( still_stowed );
                for( const item &got : taken ) {
                    CHECK( got.uid().get_value() != claimed_uid );
                }
            }
        }
    }

    GIVEN( "a container on the ground whose contents are claimed" ) {
        const tripoint_bub_ms bag_pos( 59, 60, 0 );
        item bag( itype_backpack );
        bag.put_in( item( itype_hammer ), pocket_type::CONTAINER );
        item &grounded = here.add_item( bag_pos, bag );
        item *inner = grounded.all_items_top( pocket_type::CONTAINER ).front();
        REQUIRE( inner != nullptr );
        craft_reservation_index::record rec;
        rec.craft_uid = on_map.peek_reservation_owner_token();
        rec.provider_item_uids.push_back( inner->uid().get_value() );
        rec.expires_at = calendar::turn + 1_hours;
        get_craft_reservations().set( rec );
        const int64_t bag_uid = grounded.uid().get_value();

        WHEN( "the executor sweeps that tile" ) {
            const std::list<item> taken = scavenger.pick_up_item_map( bag_pos );

            THEN( "the whole container is left alone, since taking it moves the child" ) {
                bool bag_still_there = false;
                for( const item &left : here.i_at( bag_pos ) ) {
                    bag_still_there = bag_still_there || left.uid().get_value() == bag_uid;
                }
                CHECK( bag_still_there );
                CHECK( taken.empty() );
            }
        }
    }

    GIVEN( "a live unattended craft lying on the ground" ) {
        WHEN( "the executor sweeps the craft's own tile" ) {
            const int64_t craft_uid = on_map.uid().get_value();
            const std::list<item> taken = scavenger.pick_up_item_map( origin );

            THEN( "the craft is not carried off under a dead identity" ) {
                bool craft_still_there = false;
                for( const item &left : here.i_at( origin ) ) {
                    craft_still_there = craft_still_there || left.uid().get_value() == craft_uid;
                }
                CHECK( craft_still_there );
            }
        }
    }
}

TEST_CASE( "reservation_blocks_npc_pickup_and_bashing",
           "[craft][attention][reservation][npc]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms pot_pos( 61, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_water, calendar::turn );
    item placed( &recipe_water_clean_test_unattended_boil.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    here.add_item( pot_pos, item( itype_pot ) );
    craft_stamp_passive_entry( on_map, u, calendar::turn, loc );
    REQUIRE( on_map.get_reservations().size() == 1 );

    GIVEN( "reserved furniture standing in an NPC's way" ) {
        const tripoint_bub_ms wall_pos( 62, 60, 0 );
        here.furn_set( wall_pos, furn_test_f_reserve_qual );
        craft_reservation_index::record rec;
        rec.craft_uid = on_map.peek_reservation_owner_token();
        rec.provider_tiles.push_back( here.get_abs( wall_pos ) );
        rec.expires_at = calendar::turn + 1_hours;
        get_craft_reservations().set( rec );
        REQUIRE( get_craft_reservations().provider_tile_reserved( here.get_abs( wall_pos ) ) );

        THEN( "breaking through it is refused" ) {
            CHECK( craft_reservation::bashing_would_break_reservation( here, u, wall_pos ) );
        }
    }

    GIVEN( "a reserved item lying on a bashable tile" ) {
        const tripoint_bub_ms blocked( 62, 61, 0 );
        here.furn_set( blocked, furn_test_f_reserve_qual );
        item &stashed = here.add_item( blocked, item( itype_hammer ) );
        craft_reservation_index::record rec;
        rec.craft_uid = on_map.peek_reservation_owner_token();
        rec.provider_item_uids.push_back( stashed.uid().get_value() );
        rec.expires_at = calendar::turn + 1_hours;
        get_craft_reservations().set( rec );
        REQUIRE( get_craft_reservations().is_reserved_uid( stashed.uid().get_value() ) );

        THEN( "breaking in to reach it is refused" ) {
            CHECK( craft_reservation::bashing_would_break_reservation( here, u, blocked ) );
        }
    }

    GIVEN( "a craft site under a bashable tile" ) {
        const tripoint_bub_ms sited( 62, 62, 0 );
        here.furn_set( sited, furn_test_f_reserve_qual );
        craft_reservation_index::record rec;
        rec.craft_uid = on_map.peek_reservation_owner_token();
        rec.craft_tile = here.get_abs( sited );
        rec.expires_at = calendar::turn + 1_hours;
        get_craft_reservations().set( rec );
        REQUIRE( get_craft_reservations().craft_site_reserved( here.get_abs( sited ) ) );

        THEN( "smashing the craft itself is refused, so the union is what is checked" ) {
            CHECK( craft_reservation::bashing_would_break_reservation( here, u, sited ) );
        }
    }

    GIVEN( "an unreserved bashable tile" ) {
        const tripoint_bub_ms free_pos( 63, 60, 0 );
        here.furn_set( free_pos, furn_test_f_reserve_qual );

        THEN( "bashing is still allowed, so the guard did not disable it wholesale" ) {
            CHECK_FALSE( craft_reservation::bashing_would_break_reservation( here, u, free_pos ) );
        }
    }

    GIVEN( "a passable tile holding a reserved item" ) {
        THEN( "walking over it is fine, since only bashing destroys" ) {
            CHECK_FALSE( craft_reservation::bashing_would_break_reservation( here, u, pot_pos ) );
        }
    }

    GIVEN( "a claimed tile behind a closed door the character can open" ) {
        const tripoint_bub_ms door_pos( 60, 61, 0 );
        here.ter_set( door_pos, ter_t_door_c );
        craft_reservation_index::record rec;
        rec.craft_uid = on_map.peek_reservation_owner_token();
        rec.provider_tiles.push_back( here.get_abs( door_pos ) );
        rec.expires_at = calendar::turn + 1_hours;
        get_craft_reservations().set( rec );
        REQUIRE( get_craft_reservations().provider_tile_reserved( here.get_abs( door_pos ) ) );

        THEN( "the door is routable, since opening it breaks nothing" ) {
            CHECK_FALSE( craft_reservation::bashing_would_break_reservation( here, u, door_pos ) );
        }

        WHEN( "the same door is locked" ) {
            here.ter_set( door_pos, ter_t_door_locked );

            THEN( "it reads as bash-required again" ) {
                CHECK( craft_reservation::bashing_would_break_reservation( here, u, door_pos ) );
            }
        }
    }
}

TEST_CASE( "reserved_carried_tools_keep_their_charges",
           "[craft][attention][reservation][enforcement][charges]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item_location reserved = u.i_add( item( itype_test_reserve_charged_tool ) );
    item_location spare = u.i_add( item( itype_test_reserve_charged_tool ) );
    REQUIRE( reserved );
    REQUIRE( spare );
    reserved->ammo_set( itype_battery, 100 );
    spare->ammo_set( itype_battery, 100 );

    craft_reservation_index::record rec;
    rec.craft_uid = 4242;
    rec.provider_item_uids.push_back( reserved->uid().get_value() );
    rec.expires_at = calendar::turn + 1_hours;
    get_craft_reservations().set( rec );
    REQUIRE( get_craft_reservations().is_reserved_uid( reserved->uid().get_value() ) );
    u.invalidate_crafting_inventory();

    GIVEN( "another craft debits that tool type from the crafter" ) {
        comp_selection<tool_comp> sel;
        sel.use_from = usage_from::player;
        sel.comp = tool_comp( itype_test_reserve_charged_tool, 40 );

        WHEN( "the charges are consumed" ) {
            u.consume_tools( sel, 1 );

            THEN( "the reserved tool is untouched" ) {
                CHECK( reserved->ammo_remaining( ) == 100 );
            }

            THEN( "the free one paid instead" ) {
                int reserved_left = -1;
                int spare_left = -1;
                for( const item_location &e : u.all_items_loc() ) {
                    if( e->typeId() != itype_test_reserve_charged_tool ) {
                        continue;
                    }
                    const bool is_reserved =
                        get_craft_reservations().is_reserved_uid( e->uid().get_value() );
                    ( is_reserved ? reserved_left : spare_left ) = e->ammo_remaining();
                }
                CAPTURE( reserved_left );
                CAPTURE( spare_left );
                CHECK( spare_left < 100 );
            }
        }
    }
}

TEST_CASE( "reserved_components_are_not_consumed_by_type",
           "[craft][attention][reservation][enforcement][components]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );

    item &reserved = here.add_item( origin, item( itype_2x4 ) );
    item &spare = here.add_item( origin, item( itype_2x4 ) );
    const int64_t reserved_uid = reserved.uid().get_value();
    const int64_t spare_uid = spare.uid().get_value();

    craft_reservation_index::record rec;
    rec.craft_uid = 4242;
    rec.provider_item_uids.push_back( reserved_uid );
    rec.expires_at = calendar::turn + 1_hours;
    get_craft_reservations().set( rec );

    GIVEN( "a reserved and a free instance of one component type" ) {
        WHEN( "one is consumed from the map by amount" ) {
            int qty = 1;
            const std::list<item> used = here.use_amount( origin, 1, itype_2x4, qty );

            THEN( "the free one is taken" ) {
                CHECK( used.size() == 1 );
                CHECK( qty == 0 );
            }

            THEN( "the reserved one survives" ) {
                bool reserved_still_there = false;
                for( const item &it : here.i_at( origin ) ) {
                    if( it.uid().get_value() == reserved_uid ) {
                        reserved_still_there = true;
                    }
                }
                CHECK( reserved_still_there );
                static_cast<void>( spare_uid );
            }
        }
    }
}

TEST_CASE( "reservation_keeps_farm_actors_off_a_bound_consumable",
           "[craft][attention][reservation][farm]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms plant_pos( 61, 60, 0 );
    u.setpos( here, origin );
    u.worn.wear_item( u, item( itype_backpack ), false, false );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_presence_or.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );

    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::player;
    alloc.sel.comp.type = itype_test_reserve_tool_a;
    alloc.sel.comp.count = -1;
    on_map.set_step_tool_allocs( { { alloc } } );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    const auto plant_a_seed = [&]() {
        item seed( itype_seed_hops, calendar::turn );
        seed.set_flag( json_flag_HIDDEN_ITEM );
        here.add_item( plant_pos, seed );
        here.furn_set( plant_pos, furn_f_plant_seed );
    };

    GIVEN( "the only fertilizer claimed by a live craft" ) {
        item_location claimed = u.i_add( item( itype_fertilizer ) );
        REQUIRE( claimed );
        const int64_t claimed_uid = claimed->uid().get_value();

        craft_reservation_index::record held;
        held.craft_uid = 424242;
        held.expires_at = calendar::turn + 1_hours;
        held.provider_item_uids.push_back( claimed_uid );
        get_craft_reservations().set( held );
        REQUIRE( get_craft_reservations().is_reserved_uid( claimed_uid ) );

        plant_a_seed();

        WHEN( "the fertilize actor finishes" ) {
            player_activity act;
            fertilize_plant_activity_actor actor( 1_seconds, plant_pos, itype_fertilizer );
            actor.finish( act, u );

            THEN( "it consumes nothing rather than taking the claimed one" ) {
                CHECK( u.has_amount( itype_fertilizer, 1 ) );
            }
        }
    }

    GIVEN( "an unclaimed fertilizer" ) {
        item_location free_one = u.i_add( item( itype_fertilizer ) );
        REQUIRE( free_one );

        plant_a_seed();

        WHEN( "the fertilize actor finishes" ) {
            player_activity act;
            fertilize_plant_activity_actor actor( 1_seconds, plant_pos, itype_fertilizer );
            actor.finish( act, u );

            THEN( "it is consumed, so the filter did not over-block" ) {
                CHECK_FALSE( u.has_amount( itype_fertilizer, 1 ) );
            }
        }
    }
}

TEST_CASE( "reservation_keeps_direct_quality_selection_off_a_bound_tool",
           "[craft][attention][reservation][quality]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    u.setpos( here, origin );
    u.worn.wear_item( u, item( itype_backpack ), false, false );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_furn_qual.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    GIVEN( "a bound high-level provider beside a free lower-level one" ) {
        item better( itype_test_reserve_tool_a_good );
        REQUIRE( u.wield( better ) );
        item_location worse = u.i_add( item( itype_test_reserve_tool_a ) );
        REQUIRE( worse );

        craft_stamp_passive_entry( on_map, u, calendar::turn, loc );
        REQUIRE( on_map.get_reservations().size() == 1 );
        REQUIRE( get_craft_reservations().is_reserved_uid(
                     on_map.get_reservations()[0].provider_uid ) );

        THEN( "planning still passes, on the free one" ) {
            CHECK( u.has_unreserved_quality( qual_TEST_RESERVE_A, 1, 1 ) );
        }

        THEN( "selection returns the free one rather than the bound better one" ) {
            const item &chosen = u.best_unreserved_item_with_quality( qual_TEST_RESERVE_A );
            REQUIRE_FALSE( chosen.is_null() );
            CHECK( chosen.typeId() == itype_test_reserve_tool_a );
        }

        THEN( "the unfiltered pair still offers the bound one, so its owner can use it" ) {
            CHECK( u.has_quality( qual_TEST_RESERVE_A, 3, 1 ) );
        }
    }

    GIVEN( "only the bound provider" ) {
        item only( itype_test_reserve_tool_a );
        REQUIRE( u.wield( only ) );
        craft_stamp_passive_entry( on_map, u, calendar::turn, loc );
        REQUIRE( on_map.get_reservations().size() == 1 );

        THEN( "planning declines rather than selection returning nothing" ) {
            CHECK_FALSE( u.has_unreserved_quality( qual_TEST_RESERVE_A, 1, 1 ) );
        }
    }
}

TEST_CASE( "reservation_keeps_npc_selectors_off_a_bound_provider",
           "[craft][attention][reservation][npc]" )
{
    g->faction_manager_ptr->create_if_needed();
    clear_map_without_vision();
    clear_avatar();
    set_time_to_day();

    Character &player_character = get_player_character();
    const point five_tiles_south( 0, 5 );
    npc &hostile = spawn_npc( player_character.pos_bub().xy() + five_tiles_south, "thug" );
    hostile.clear_worn();
    hostile.invalidate_crafting_inventory();
    hostile.inv->clear();
    hostile.remove_weapon();
    hostile.clear_mutations();
    hostile.set_body();
    hostile.mutation_category_level.clear();
    hostile.clear_bionics();
    hostile.set_attitude( NPCATT_KILL );
    // Storage, or everything handed over below lands on the floor instead.
    hostile.wear_item( item( itype_debug_backpack ) );

    // The weapon selectors only run once the NPC thinks it is in danger.
    arm_shooter( player_character, itype_M24 );

    GIVEN( "an NPC carrying a reserved good weapon and a free worse one" ) {
        item_location good = hostile.i_add( item( itype_bat ) );
        hostile.i_add( item( itype_leather_belt ) );
        REQUIRE( good );
        const int64_t good_uid = good->uid().get_value();

        craft_reservation_index::record rec;
        rec.craft_uid = 4242;
        rec.provider_item_uids.push_back( good_uid );
        rec.expires_at = calendar::turn + 1_hours;
        get_craft_reservations().set( rec );
        REQUIRE( get_craft_reservations().is_reserved_uid( good_uid ) );

        hostile.regen_ai_cache();
        REQUIRE( hostile.danger_assessment() > 1.0f );

        THEN( "the reserved weapon is not the one it would take" ) {
            const item *best = hostile.evaluate_best_weapon();
            CHECK( ( best == nullptr || best->typeId() != itype_bat ) );
        }

        WHEN( "it wields the best weapon it will consider" ) {
            hostile.wield_better_weapon();

            THEN( "the reserved one keeps the identity its binding names" ) {
                bool still_held = false;
                for( const item_location &e : hostile.all_items_loc() ) {
                    still_held = still_held || e->uid().get_value() == good_uid;
                }
                CHECK( still_held );
            }
        }
    }

    GIVEN( "a free container in the NPC's pack whose contents are reserved" ) {
        item backpack( itype_backpack );
        backpack.force_insert_item( item( itype_bat ), pocket_type::CONTAINER );
        hostile.i_add( backpack );

        int64_t inner_uid = 0;
        for( const item_location &e : hostile.all_items_loc() ) {
            if( e->typeId() == itype_bat ) {
                inner_uid = e->uid().get_value();
            }
        }
        REQUIRE( inner_uid != 0 );

        craft_reservation_index::record rec;
        rec.craft_uid = 4243;
        rec.provider_item_uids.push_back( inner_uid );
        rec.expires_at = calendar::turn + 1_hours;
        get_craft_reservations().set( rec );
        REQUIRE( get_craft_reservations().is_reserved_uid( inner_uid ) );

        hostile.regen_ai_cache();
        REQUIRE( hostile.danger_assessment() > 1.0f );

        THEN( "the whole subtree is passed over rather than just the reserved item" ) {
            const item *best = hostile.evaluate_best_weapon();
            CHECK( ( best == nullptr || best->typeId() != itype_bat ) );
        }

        WHEN( "it wields the best weapon it will consider" ) {
            hostile.wield_better_weapon();

            THEN( "the reserved item inside keeps its identity, since wielding copies" ) {
                bool still_held = false;
                for( const item_location &e : hostile.all_items_loc() ) {
                    still_held = still_held || e->uid().get_value() == inner_uid;
                }
                CHECK( still_held );
            }
        }
    }

    GIVEN( "the same container with nothing reserved in it" ) {
        item backpack( itype_backpack );
        backpack.force_insert_item( item( itype_bat ), pocket_type::CONTAINER );
        hostile.i_add( backpack );
        hostile.regen_ai_cache();
        REQUIRE( hostile.danger_assessment() > 1.0f );

        THEN( "the nested weapon is still reachable, so the guard is what hid it" ) {
            const item *best = hostile.evaluate_best_weapon();
            REQUIRE( best != nullptr );
            CHECK( best->typeId() == itype_bat );
        }
    }
}

TEST_CASE( "reservation_hides_a_bound_vehicle_part_from_planning",
           "[craft][attention][reservation][enforcement][vehicle]" )
{
    clear_avatar();
    clear_map();
    clear_vehicles();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms beside( 61, 60, 0 );
    u.setpos( here, origin );

    const auto claim_part = []( int64_t part_uid ) {
        craft_reservation_index::record claim;
        claim.craft_uid = 424242;
        claim.expires_at = calendar::turn + 1_hours;
        claim.provider_part_uids.push_back( part_uid );
        get_craft_reservations().set( claim );
    };

    GIVEN( "a welding rig beside the work spot and no welder carried" ) {
        vehicle *veh = here.add_vehicle( vehicle_prototype_none, beside, 0_degrees, 0,
                                         veh_spawn_status::UNDAMAGED );
        REQUIRE( veh != nullptr );
        REQUIRE( veh->install_part( here, point_rel_ms::zero, vpart_frame ) != -1 );
        const int rig_idx = veh->install_part( here, point_rel_ms::zero,
                                               vpart_test_vp_reserve_welder );
        REQUIRE( rig_idx != -1 );
        veh->refresh();
        here.add_vehicle_to_cache( veh );
        const int64_t rig_uid = veh->part( rig_idx ).get_base().uid().get_value();

        const std::vector<tripoint_bub_ms> spots{ origin };

        WHEN( "nothing is reserved" ) {
            THEN( "planning fabricates the welder from the part" ) {
                CHECK( multi_activity_actor::are_requirements_nearby(
                           spots, requirement_data_test_reserve_vehicle_weld, u,
                           ACT_MULTIPLE_CONSTRUCTION, false, origin ) );
            }
        }

        WHEN( "the supplying part is claimed by a craft" ) {
            claim_part( rig_uid );

            THEN( "planning declines rather than offering a tool it cannot use" ) {
                CHECK_FALSE( multi_activity_actor::are_requirements_nearby(
                                 spots, requirement_data_test_reserve_vehicle_weld, u,
                                 ACT_MULTIPLE_CONSTRUCTION, false, origin ) );
            }
        }
    }

    GIVEN( "a faucet and a full tank on one vehicle" ) {
        vehicle *veh = here.add_vehicle( vehicle_prototype_none, beside, 0_degrees, 0,
                                         veh_spawn_status::UNDAMAGED );
        REQUIRE( veh != nullptr );
        REQUIRE( veh->install_part( here, point_rel_ms::zero, vpart_frame ) != -1 );
        const int faucet_idx = veh->install_part( here, point_rel_ms::zero, vpart_water_faucet );
        REQUIRE( faucet_idx != -1 );
        const int tank_idx = veh->install_part( here, point_rel_ms::zero, vpart_tank );
        REQUIRE( tank_idx != -1 );
        veh->part( tank_idx ).ammo_set( itype_water_clean, 100 );
        veh->refresh();
        here.add_vehicle_to_cache( veh );
        REQUIRE( veh->fuel_left( here, itype_water_clean ) == 100 );
        const int64_t faucet_uid = veh->part( faucet_idx ).get_base().uid().get_value();

        const optional_vpart_position ovp = here.veh_at( beside );
        REQUIRE( ovp.has_value() );

        WHEN( "nothing is reserved" ) {
            inventory inv;
            ovp->form_inventory( here, inv );

            THEN( "the tank's water is reachable through the faucet" ) {
                CHECK( inv.charges_of( itype_water_clean ) == 100 );
            }
        }

        WHEN( "the faucet part is claimed by a craft" ) {
            claim_part( faucet_uid );
            inventory inv;
            ovp->form_inventory( here, inv );

            THEN( "the tank's water goes with it" ) {
                CHECK( inv.charges_of( itype_water_clean ) == 0 );
            }
        }
    }
}

TEST_CASE( "reservation_keeps_zone_planning_off_a_bound_tool",
           "[craft][attention][reservation][enforcement][automation]" )
{
    clear_avatar();
    clear_map();
    zone_manager::get_manager().clear();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms work_pos( 61, 60, 0 );
    u.setpos( here, origin );
    u.worn.wear_item( u, item( itype_backpack ), false, false );

    const auto claim = []( const item_location & held ) {
        REQUIRE( held );
        craft_reservation_index::record rec;
        rec.craft_uid = 424242;
        rec.expires_at = calendar::turn + 1_hours;
        rec.provider_item_uids.push_back( held->uid().get_value() );
        get_craft_reservations().set( rec );
        REQUIRE( get_craft_reservations().is_reserved_uid( held->uid().get_value() ) );
    };

    const auto add_farm_zone = [&]( const std::string & seed ) {
        shared_ptr_fast<zone_options> options = zone_options::create( zone_type_FARM_PLOT );
        std::ostringstream ss;
        ss << R"({"mark":")" << seed << R"(","seed":")" << seed << R"(","fertilizer":""})";
        options->deserialize( json_loader::from_string( ss.str() ).get_object() );
        zone_manager::get_manager().add( "test farm", zone_type_FARM_PLOT, u.get_faction()->id,
                                         false, true, here.get_abs( work_pos ),
                                         here.get_abs( work_pos ), options );
    };

    GIVEN( "a spill to mop and one mop in the pack" ) {
        here.add_item( work_pos, item( itype_water, calendar::turn, 1 ) );
        REQUIRE( here.terrain_moppable( work_pos ) );
        item_location mop = u.i_add( item( itype_mop ) );

        WHEN( "the mop is free" ) {
            multi_mop_activity_actor actor;

            THEN( "planning takes the job" ) {
                CHECK( actor.multi_activity_can_do( u, work_pos ).can_do );
            }
        }

        WHEN( "a craft holds the mop" ) {
            claim( mop );
            multi_mop_activity_actor actor;

            THEN( "planning declines rather than trusting the flag cache" ) {
                CHECK_FALSE( actor.multi_activity_can_do( u, work_pos ).can_do );
            }
        }
    }

    GIVEN( "a plant that needs cutting and one grass-cutting tool in the pack" ) {
        add_farm_zone( "seed_hops" );
        item ripe( itype_seed_hops, calendar::turn );
        ripe.set_flag( flag_CUT_HARVEST );
        here.add_item( work_pos, ripe );
        here.furn_set( work_pos, furn_f_plant_harvest );
        REQUIRE( here.has_flag_furn( ter_furn_flag::TFLAG_GROWTH_HARVEST, work_pos ) );
        item_location sickle = u.i_add( item( itype_sickle ) );

        WHEN( "the tool is free" ) {
            multi_farm_activity_actor actor;

            THEN( "planning takes the job" ) {
                CHECK( actor.multi_activity_can_do( u, work_pos ).can_do );
            }
        }

        WHEN( "a craft holds the tool" ) {
            claim( sickle );
            multi_farm_activity_actor actor;

            THEN( "planning declines" ) {
                CHECK_FALSE( actor.multi_activity_can_do( u, work_pos ).can_do );
            }
        }
    }

    GIVEN( "a tilled plot and one seed of the zone's kind in the pack" ) {
        add_farm_zone( "seed_hops" );
        // Greenhouse rather than a mound, so the season's forecast cannot refuse the
        // planting before the seed is ever looked at.
        here.ter_set( work_pos, ter_t_greenhouse_tilled );
        item_location seed = u.i_add( item( itype_seed_hops, calendar::turn ) );

        WHEN( "the seed is free" ) {
            multi_farm_activity_actor actor;

            THEN( "planning takes the job" ) {
                CHECK( actor.multi_activity_can_do( u, work_pos ).can_do );
            }
        }

        WHEN( "a craft holds the seed" ) {
            claim( seed );
            multi_farm_activity_actor actor;

            THEN( "planning declines" ) {
                CHECK_FALSE( actor.multi_activity_can_do( u, work_pos ).can_do );
            }
        }
    }

    GIVEN( "fishable water and a rod the character has to power" ) {
        here.ter_set( work_pos, ter_t_water_sh );
        REQUIRE( here.has_flag( ter_furn_flag::TFLAG_FISHABLE, work_pos ) );
        u.i_add( item( itype_test_reserve_bionic_rod ) );
        u.set_max_power_level( 10_kJ );

        WHEN( "the character has power for it" ) {
            u.set_power_level( 10_kJ );
            multi_fish_activity_actor actor;

            THEN( "planning takes the job, judged on that character's power" ) {
                CHECK( actor.multi_activity_can_do( u, work_pos ).can_do );
            }
        }

        WHEN( "the character has no power" ) {
            u.set_power_level( 0_kJ );
            multi_fish_activity_actor actor;

            THEN( "planning declines" ) {
                CHECK_FALSE( actor.multi_activity_can_do( u, work_pos ).can_do );
            }
        }
    }
}

TEST_CASE( "reservation_keeps_a_lapsed_binding_out_of_its_own_debit",
           "[craft][attention][reservation][charges]" )
{
    clear_avatar();
    clear_map();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms tool_pos( 61, 60, 0 );
    u.setpos( here, origin );

    item ingredient( itype_2x4, calendar::turn );
    item placed( &recipe_cudgel_test_unattended_qual_and_charges.obj(), 1, ingredient );
    item &on_map = here.add_item( origin, placed );
    REQUIRE( on_map.is_craft() );
    on_map.set_current_step( 0 );
    on_map.set_crafter_id( u.getID() );
    on_map.set_step_plans( std::vector<attention_plan>( 1 ) );

    step_tool_alloc alloc;
    alloc.sel.use_from = usage_from::map;
    alloc.sel.comp.type = itype_test_reserve_charged_tool;
    alloc.sel.comp.count = 50;
    alloc.step_count_units = 50;
    on_map.set_step_tool_allocs( { { alloc } } );
    item_location loc( map_cursor( here.get_abs( origin ) ), &on_map );

    item bound_tool( itype_test_reserve_charged_tool );
    bound_tool.ammo_set( itype_battery, 100 );
    item &first = here.add_item( tool_pos, bound_tool );
    item drainable( itype_test_reserve_charged_tool );
    drainable.ammo_set( itype_battery, 100 );
    item &second = here.add_item( tool_pos, drainable );
    u.invalidate_crafting_inventory();

    const time_point t0 = calendar::turn;

    GIVEN( "a step whose quality binding and charge draw share a tool type" ) {
        craft_stamp_passive_entry( on_map, u, t0, loc );
        REQUIRE( on_map.get_reservations().size() == 1 );
        const int64_t bound_uid = on_map.get_reservations()[0].provider_uid;
        item &bound = first.uid().get_value() == bound_uid ? first : second;
        item &free_one = first.uid().get_value() == bound_uid ? second : first;
        const int bound_charges = bound.ammo_remaining();

        WHEN( "the free tool is empty and the lease has lapsed" ) {
            free_one.ammo_set( itype_battery, 0 );
            on_map.set_reservation_expiry( t0 - 1_minutes );
            get_craft_reservations().rebuild_for_craft( loc );
            u.invalidate_crafting_inventory();
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "the step does not drain the provider its own binding names" ) {
                CHECK( bound.ammo_remaining() == bound_charges );
            }
        }

        WHEN( "the draw is sourced from both pools" ) {
            std::vector<std::vector<step_tool_alloc>> both_allocs = on_map.get_step_tool_allocs();
            both_allocs[0][0].sel.use_from = usage_from::both;
            on_map.set_step_tool_allocs( both_allocs );
            free_one.ammo_set( itype_battery, 0 );
            on_map.set_reservation_expiry( t0 - 1_minutes );
            get_craft_reservations().rebuild_for_craft( loc );
            const int paid = on_map.get_step_tool_allocs()[0][0].consumed_buckets;
            craft_actualize_scheduled( on_map, item_wakeup_kind::env_check, t0 + 1_minutes, loc );

            THEN( "the shortfall is seen rather than the bucket being recorded unpaid" ) {
                CHECK( bound.ammo_remaining() == bound_charges );
                CHECK( on_map.get_step_tool_allocs()[0][0].consumed_buckets == paid );
            }
        }
    }
}

TEST_CASE( "reservation_scans_past_a_claimed_colocated_vehicle_tool",
           "[craft][attention][reservation][enforcement][vehicle]" )
{
    clear_avatar();
    clear_map();
    clear_vehicles();
    avatar &u = get_avatar();
    map &here = get_map();
    const tripoint_bub_ms origin( 60, 60, 0 );
    const tripoint_bub_ms beside( 61, 60, 0 );
    u.setpos( here, origin );

    GIVEN( "two welding rigs sharing one mount" ) {
        vehicle *veh = here.add_vehicle( vehicle_prototype_none, beside, 0_degrees, 0,
                                         veh_spawn_status::UNDAMAGED );
        REQUIRE( veh != nullptr );
        REQUIRE( veh->install_part( here, point_rel_ms::zero, vpart_frame ) != -1 );
        REQUIRE( veh->install_part( here, point_rel_ms::zero,
                                    vpart_test_vp_reserve_welder ) != -1 );
        REQUIRE( veh->install_part( here, point_rel_ms::zero,
                                    vpart_test_vp_reserve_welder_b ) != -1 );
        veh->refresh();
        here.add_vehicle_to_cache( veh );

        const optional_vpart_position ovp = here.veh_at( beside );
        REQUIRE( ovp.has_value() );
        const std::optional<vpart_reference> first = ovp->part_with_tool( here, itype_welder );
        REQUIRE( first );

        const std::vector<tripoint_bub_ms> spots{ origin };

        WHEN( "the first of them is claimed by a craft" ) {
            craft_reservation_index::record rec;
            rec.craft_uid = 424242;
            rec.expires_at = calendar::turn + 1_hours;
            rec.provider_part_uids.push_back( first->part().get_base().uid().get_value() );
            get_craft_reservations().set( rec );

            THEN( "planning still reaches the free one beside it" ) {
                CHECK( multi_activity_actor::are_requirements_nearby(
                           spots, requirement_data_test_reserve_vehicle_weld, u,
                           ACT_MULTIPLE_CONSTRUCTION, false, origin ) );
            }
        }
    }
}

