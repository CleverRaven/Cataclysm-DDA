#include <algorithm>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character_id.h"
#include "coordinates.h"
#include "craft_command.h"
#include "crafting.h"
#include "crafting_enums.h"
#include "flexbuffer_json.h"
#include "game_constants.h"
#include "item.h"
#include "item_components.h"
#include "item_location.h"
#include "item_uid.h"
#include "item_wakeup.h"
#include "json.h"
#include "json_loader.h"
#include "map.h"
#include "map_helpers.h"
#include "map_selector.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "point.h"
#include "recipe.h"
#include "requirements.h"
#include "type_id.h"

static const itype_id itype_2x4( "2x4" );
static const itype_id itype_cudgel( "cudgel" );
static const itype_id itype_hammer( "hammer" );
static const itype_id itype_microwave( "microwave" );
static const itype_id itype_soldering_iron_portable( "soldering_iron_portable" );
static const itype_id itype_water( "water" );

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
static const recipe_id recipe_cudgel_test_unattended_with_qual(
    "cudgel_test_unattended_with_qual" );
static const recipe_id recipe_water_clean_test_unattended_liquid(
    "water_clean_test_unattended_liquid" );

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

TEST_CASE( "craft_stamp_skips_env_check_when_step_has_no_env_requirements",
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
    CHECK( on_map.get_env_check_at() == calendar::before_time_starts );
    CHECK_FALSE( get_item_wakeups().is_scheduled( on_map.uid().get_value(),
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
    // same iteration map::reconcile_item_wakeups uses for character inventory
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
