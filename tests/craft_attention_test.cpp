#include <algorithm>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <list>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "activity_actor_definitions.h"
#include "activity_handlers.h"
#include "avatar.h"
#include "bionics.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_utility.h"
#include "character_id.h"
#include "construction.h"
#include "coordinates.h"
#include "craft_command.h"
#include "craft_reservation.h"
#include "crafting.h"
#include "crafting_enums.h"
#include "enums.h"
#include "flexbuffer_json.h"
#include "game_constants.h"
#include "inventory.h"
#include "item.h"
#include "item_components.h"
#include "item_location.h"
#include "item_uid.h"
#include "item_wakeup.h"
#include "json.h"
#include "json_loader.h"
#include "map.h"
#include "map_helpers.h"
#include "map_iterator.h"
#include "map_selector.h"
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
#include "vpart_position.h"

static const bionic_id test_bio_reserve_toggled_pseudo( "test_bio_reserve_toggled_pseudo" );
static const bionic_id test_bio_reserve_two_pseudo( "test_bio_reserve_two_pseudo" );
static const bionic_id test_bio_reserve_weapon( "test_bio_reserve_weapon" );

static const construction_str_id
construction_test_constr_pit_shallow( "test_constr_pit_shallow" );

static const itype_id itype_2x4( "2x4" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_cudgel( "cudgel" );
static const itype_id itype_debug_backpack( "debug_backpack" );
static const itype_id itype_hammer( "hammer" );
static const itype_id itype_microwave( "microwave" );
static const itype_id itype_pot( "pot" );
static const itype_id itype_soldering_iron_portable( "soldering_iron_portable" );
static const itype_id itype_test_reserve_charge_stack( "test_reserve_charge_stack" );
static const itype_id itype_test_reserve_tool_a( "test_reserve_tool_a" );
static const itype_id itype_water( "water" );

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
static const recipe_id recipe_cudgel_test_unattended_simple(
    "cudgel_test_unattended_simple" );
static const recipe_id recipe_cudgel_test_unattended_two_of_a(
    "cudgel_test_unattended_two_of_a" );
static const recipe_id recipe_cudgel_test_unattended_with_qual(
    "cudgel_test_unattended_with_qual" );
static const recipe_id recipe_water_clean_test_unattended_boil(
    "water_clean_test_unattended_boil" );
static const recipe_id recipe_water_clean_test_unattended_liquid(
    "water_clean_test_unattended_liquid" );

static const ter_str_id ter_t_dirt( "t_dirt" );

static const trait_id trait_BURROW( "BURROW" );
static const trait_id trait_BURROWLARGE( "BURROWLARGE" );

static const vproto_id vehicle_prototype_test_shopping_cart( "test_shopping_cart" );

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
