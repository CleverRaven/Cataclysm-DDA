#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character_id.h"
#include "coordinates.h"
#include "crafting.h"
#include "crafting_enums.h"
#include "flexbuffer_json.h"
#include "item.h"
#include "item_components.h"
#include "item_location.h"
#include "json.h"
#include "json_loader.h"
#include "map.h"
#include "map_helpers.h"
#include "map_selector.h"
#include "point.h"
#include "recipe.h"
#include "requirements.h"
#include "type_id.h"

static const itype_id itype_2x4( "2x4" );

static const recipe_id recipe_cudgel_test_consecutive_unattended(
    "cudgel_test_consecutive_unattended" );
static const recipe_id recipe_cudgel_test_first_step_unattended(
    "cudgel_test_first_step_unattended" );
static const recipe_id recipe_cudgel_test_only_unattended(
    "cudgel_test_only_unattended" );
static const recipe_id recipe_cudgel_test_steps_basic(
    "cudgel_test_steps_basic" );
static const recipe_id recipe_cudgel_test_timeout_recipe(
    "cudgel_test_timeout_recipe" );
static const recipe_id recipe_cudgel_test_unattended_simple(
    "cudgel_test_unattended_simple" );
static const recipe_id recipe_cudgel_test_unattended_with_qual(
    "cudgel_test_unattended_with_qual" );

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
