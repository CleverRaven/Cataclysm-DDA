#include <cstdint>
#include <cstdlib>
#include <functional>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_utility.h"
#include "character.h"
#include "coordinates.h"
#include "debug.h"
#include "flexbuffer_json.h"
#include "item.h"
#include "item_location.h"
#include "json.h"
#include "json_loader.h"
#include "map.h"
#include "map_helpers.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "point.h"
#include "proficiency.h"
#include "recipe.h"
#include "requirements.h"
#include "translation.h"
#include "type_id.h"

static const activity_id ACT_CRAFT( "ACT_CRAFT" );

static const itype_id itype_2x4( "2x4" );
static const itype_id itype_cudgel( "cudgel" );
static const itype_id itype_knife_hunting( "knife_hunting" );

static const proficiency_id proficiency_prof_test_step_a( "prof_test_step_a" );
static const proficiency_id proficiency_prof_test_step_b( "prof_test_step_b" );
static const proficiency_id proficiency_prof_test_step_required( "prof_test_step_required" );

static const recipe_id recipe_cudgel_simple( "cudgel_simple" );
static const recipe_id recipe_cudgel_test_steps_basic( "cudgel_test_steps_basic" );
static const recipe_id recipe_cudgel_test_steps_required_prof(
    "cudgel_test_steps_required_prof" );
static const recipe_id recipe_cudgel_test_steps_shared_prof(
    "cudgel_test_steps_shared_prof" );
static const recipe_id recipe_flatbread( "flatbread" );

static const skill_id skill_cooking( "cooking" );
static const skill_id skill_fabrication( "fabrication" );

static const trait_id trait_DEBUG_CNF( "DEBUG_CNF" );

// ---- Data model tests ----

TEST_CASE( "step_recipe_loads_correctly", "[recipe][steps]" )
{
    const recipe &r = recipe_cudgel_test_steps_basic.obj();

    REQUIRE( r.has_steps() );
    const std::vector<recipe_step> &steps = r.steps();
    REQUIRE( steps.size() == 3 );

    CHECK( steps[0].name.translated() == "Shape" );
    CHECK( steps[1].name.translated() == "Sand" );
    CHECK( steps[2].name.translated() == "Finish" );

    CHECK( steps[0].time == to_moves<int64_t>( 10_minutes ) );
    CHECK( steps[1].time == to_moves<int64_t>( 5_minutes ) );
    CHECK( steps[2].time == to_moves<int64_t>( 5_minutes ) );
}

TEST_CASE( "step_recipe_aggregate_time", "[recipe][steps]" )
{
    const recipe &r = recipe_cudgel_test_steps_basic.obj();
    clear_avatar();
    Character &guy = get_player_character();
    guy.set_skill_level( skill_fabrication, 10 );

    const int64_t expected = to_moves<int64_t>( 20_minutes );
    CHECK( r.time_to_craft_moves( guy, recipe_time_flag::ignore_proficiencies ) == expected );
}

TEST_CASE( "step_recipe_root_components_preserved", "[recipe][steps]" )
{
    const recipe &r = recipe_cudgel_test_steps_basic.obj();
    const requirement_data &reqs = r.simple_requirements();

    bool has_2x4 = false;
    for( const std::vector<item_comp> &comp_group : reqs.get_components() ) {
        for( const item_comp &comp : comp_group ) {
            if( comp.type.str() == "2x4" ) {
                has_2x4 = true;
            }
        }
    }
    CHECK( has_2x4 );
}

TEST_CASE( "step_recipe_tools_merged", "[recipe][steps]" )
{
    const recipe &r = recipe_cudgel_test_steps_basic.obj();
    const requirement_data &reqs = r.simple_requirements();

    bool has_cut = false;
    for( const std::vector<quality_requirement> &qual_group : reqs.get_qualities() ) {
        for( const quality_requirement &qual : qual_group ) {
            if( qual.type.str() == "CUT" && qual.level >= 1 ) {
                has_cut = true;
            }
        }
    }
    CHECK( has_cut );
}

TEST_CASE( "step_recipe_deduped_requirements_craftable", "[recipe][steps]" )
{
    const recipe &r = recipe_cudgel_test_steps_basic.obj();
    const deduped_requirement_data &dreqs = r.deduped_requirements();

    REQUIRE( !dreqs.alternatives().empty() );

    clear_avatar();
    clear_map();
    Character &guy = get_player_character();
    guy.set_skill_level( skill_fabrication, 10 );
    guy.add_proficiency( proficiency_prof_test_step_a, true );
    guy.add_proficiency( proficiency_prof_test_step_b, true );

    guy.i_add( item( itype_2x4 ) );
    guy.i_add( item( itype_knife_hunting ) );
    guy.invalidate_crafting_inventory();

    CHECK( guy.can_start_craft( &r, recipe_filter_flags::none, 1 ) );
}

TEST_CASE( "step_recipe_get_proficiencies", "[recipe][steps]" )
{
    const recipe &r = recipe_cudgel_test_steps_basic.obj();
    const std::vector<recipe_proficiency> &profs = r.get_proficiencies();

    CHECK( profs.size() == 2 );

    bool has_a = false;
    bool has_b = false;
    for( const recipe_proficiency &p : profs ) {
        if( p.id == proficiency_prof_test_step_a ) {
            has_a = true;
            CHECK( p.time_multiplier == Approx( 2.0f ) );
            CHECK( p.skill_penalty == Approx( 0.5f ) );
        }
        if( p.id == proficiency_prof_test_step_b ) {
            has_b = true;
            CHECK( p.time_multiplier == Approx( 1.5f ) );
            CHECK( p.skill_penalty == Approx( 0.3f ) );
        }
    }
    CHECK( has_a );
    CHECK( has_b );
}

TEST_CASE( "step_recipe_can_start_craft_requires_all_step_tools", "[recipe][steps]" )
{
    clear_avatar();
    clear_map();
    Character &guy = get_player_character();
    guy.set_skill_level( skill_fabrication, 10 );
    guy.add_proficiency( proficiency_prof_test_step_a, true );
    guy.add_proficiency( proficiency_prof_test_step_b, true );

    const recipe &r = recipe_cudgel_test_steps_basic.obj();

    // Component present but CUT quality from Finish step is missing
    guy.i_add( item( itype_2x4 ) );
    guy.invalidate_crafting_inventory();

    CHECK_FALSE( guy.can_start_craft( &r, recipe_filter_flags::none, 1 ) );
}

// ---- Proficiency time math ----

TEST_CASE( "step_recipe_proficiency_malus_clean_miss", "[recipe][steps]" )
{
    clear_avatar();
    Character &guy = get_player_character();
    guy.set_skill_level( skill_fabrication, 10 );

    const recipe &r = recipe_cudgel_test_steps_basic.obj();
    const int64_t raw_time = to_moves<int64_t>( 20_minutes );

    // Shape: 10m * 2.0, Sand: 5m * 1.0, Finish: 5m * 1.5 => 32.5m total
    const int64_t with_malus = r.time_to_craft_moves( guy );
    const double expected = 10.0 * to_moves<int64_t>( 1_minutes ) * 2.0 +
                            5.0 * to_moves<int64_t>( 1_minutes ) * 1.0 +
                            5.0 * to_moves<int64_t>( 1_minutes ) * 1.5;
    CHECK( with_malus == static_cast<int64_t>( expected ) );
    CHECK( with_malus > raw_time );
    CHECK( with_malus < raw_time * 2 );
}

TEST_CASE( "step_recipe_proficiency_grant_removes_step_penalty", "[recipe][steps]" )
{
    clear_avatar();
    Character &guy = get_player_character();
    guy.set_skill_level( skill_fabrication, 10 );

    const recipe &r = recipe_cudgel_test_steps_basic.obj();

    const int64_t full_malus_time = r.time_to_craft_moves( guy );

    guy.add_proficiency( proficiency_prof_test_step_a, true );
    const int64_t no_a_malus_time = r.time_to_craft_moves( guy );

    CHECK( full_malus_time > no_a_malus_time );

    // Difference is exactly the Shape step penalty: 10m * (2.0 - 1.0)
    const int64_t diff = full_malus_time - no_a_malus_time;
    CHECK( diff == to_moves<int64_t>( 10_minutes ) );
}

TEST_CASE( "step_recipe_partial_proficiency_sigmoid_mitigation", "[recipe][steps]" )
{
    clear_avatar();
    Character &guy = get_player_character();
    guy.set_skill_level( skill_fabrication, 10 );

    const recipe &r = recipe_cudgel_test_steps_basic.obj();

    const int64_t full_malus = r.time_to_craft_moves( guy );

    guy.add_proficiency( proficiency_prof_test_step_a, true );
    const int64_t no_malus = r.time_to_craft_moves( guy );

    // Train to ~75%, well past the sigmoid midpoint
    guy.lose_proficiency( proficiency_prof_test_step_a, true );
    const time_duration three_quarter = proficiency_prof_test_step_a->time_to_learn() * 3 / 4;
    guy.practice_proficiency( proficiency_prof_test_step_a, three_quarter );
    REQUIRE( !guy.has_proficiency( proficiency_prof_test_step_a ) );
    const int64_t partial_malus = r.time_to_craft_moves( guy );

    CHECK( partial_malus < full_malus );
    CHECK( partial_malus > no_malus );
    // At 75% the sigmoid should have mitigated most of the penalty
    CHECK( partial_malus < ( full_malus + no_malus ) / 2 );
}

TEST_CASE( "step_recipe_same_prof_in_two_steps", "[recipe][steps]" )
{
    const recipe &r = recipe_cudgel_test_steps_shared_prof.obj();

    REQUIRE( r.has_steps() );
    REQUIRE( r.steps().size() == 2 );

    // Aggregate should deduplicate to one entry with max time_multiplier
    const std::vector<recipe_proficiency> &profs = r.get_proficiencies();
    CHECK( profs.size() == 1 );
    CHECK( profs[0].id == proficiency_prof_test_step_a );
    CHECK( profs[0].time_multiplier == Approx( 2.0f ) );

    // Both steps penalized independently: 10m * 2.0 + 10m * 2.0 = 40m
    clear_avatar();
    Character &guy = get_player_character();
    guy.set_skill_level( skill_fabrication, 10 );

    const int64_t with_malus = r.time_to_craft_moves( guy );
    const double expected = 20.0 * to_moves<int64_t>( 1_minutes ) * 2.0;
    CHECK( with_malus == static_cast<int64_t>( expected ) );
}

// ---- Proficiency display ----

TEST_CASE( "step_recipe_missing_proficiencies_string", "[recipe][steps]" )
{
    clear_avatar();
    Character &guy = get_player_character();
    guy.set_skill_level( skill_fabrication, 10 );

    const recipe &r = recipe_cudgel_test_steps_basic.obj();
    const std::string missing = r.missing_proficiencies_string( &guy );

    CHECK( missing.find( "Test Step Proficiency A" ) != std::string::npos );
    CHECK( missing.find( "Test Step Proficiency B" ) != std::string::npos );
    CHECK( missing.find( "time" ) != std::string::npos );
}

TEST_CASE( "step_recipe_display_malus_is_step_derived", "[recipe][steps]" )
{
    clear_avatar();
    Character &guy = get_player_character();
    guy.set_skill_level( skill_fabrication, 10 );

    const recipe &r = recipe_cudgel_test_steps_basic.obj();

    const float display_malus = r.proficiency_time_maluses( guy );
    const int64_t raw_time = to_moves<int64_t>( 20_minutes );
    const int64_t actual_time = r.time_to_craft_moves( guy );

    // Display ratio should match actual_time / raw_time
    CHECK( display_malus == Approx( static_cast<float>( actual_time ) /
                                    static_cast<float>( raw_time ) ).margin( 0.01f ) );

    // Should NOT be the raw product of aggregate profs (2.0 * 1.5 = 3.0)
    CHECK( display_malus < 3.0f );
}

// ---- Batch ----

TEST_CASE( "step_recipe_per_step_batch_savings", "[recipe][steps]" )
{
    clear_avatar();
    Character &guy = get_player_character();
    guy.set_skill_level( skill_fabrication, 10 );
    guy.add_proficiency( proficiency_prof_test_step_a, true );
    guy.add_proficiency( proficiency_prof_test_step_b, true );

    const recipe &r = recipe_cudgel_test_steps_basic.obj();
    const int batch = 4;

    const int64_t batch4_time = r.batch_time( guy, batch, 1.0f, 0 );
    const int64_t single_time = r.batch_time( guy, 1, 1.0f, 0 );

    CHECK( batch4_time < single_time * 4 );
    CHECK( batch4_time > single_time );

    // Verify against manual per-step calculation
    double manual_batch = 0.0;
    for( const recipe_step &s : r.steps() ) {
        manual_batch += s.batch_info.apply( static_cast<double>( s.time ), batch );
    }
    CHECK( batch4_time == static_cast<int64_t>( manual_batch ) );
}

// ---- Backwards compatibility ----

TEST_CASE( "stepless_recipe_unchanged", "[recipe][steps]" )
{
    const recipe &r = recipe_flatbread.obj();

    CHECK_FALSE( r.has_steps() );

    clear_avatar();
    Character &guy = get_player_character();
    guy.set_skill_level( skill_cooking, 10 );

    CHECK( r.time_to_craft_moves( guy, recipe_time_flag::ignore_proficiencies ) ==
           to_moves<int64_t>( 15_minutes ) );
    CHECK_FALSE( r.simple_requirements().get_components().empty() );
    CHECK_FALSE( r.get_proficiencies().empty() );
}

// ---- Schema rejection (load-time throws) ----

static void check_step_recipe_throws( const std::string &json )
{
    JsonObject jo = json_loader::from_string( json );
    jo.allow_omitted_members();
    recipe r;
    CHECK_THROWS_AS( r.load( jo, "dda" ), JsonError );
}

TEST_CASE( "step_recipe_schema_rejection", "[recipe][steps]" )
{
    // Verify a well-formed step recipe loads without throwing
    const std::string base = R"({
        "type": "recipe",
        "result": "cudgel",
        "id_suffix": "test_schema",
        "category": "CC_WEAPON",
        "subcategory": "CSC_WEAPON_BASHING",
        "skill_used": "fabrication",
        "difficulty": 1,
        "components": [[ [ "2x4", 1 ] ]],
        "steps": [
            { "name": "Work", "time": "10 m", "activity_level": "MODERATE_EXERCISE" }
        ]
    })";
    {
        JsonObject jo = json_loader::from_string( base );
        recipe r;
        CHECK_NOTHROW( r.load( jo, "dda" ) );
    }

    SECTION( "empty steps array" ) {
        check_step_recipe_throws( R"({
            "type": "recipe", "result": "cudgel", "id_suffix": "test_empty",
            "category": "CC_WEAPON", "subcategory": "CSC_WEAPON_BASHING",
            "skill_used": "fabrication", "difficulty": 1,
            "components": [[ [ "2x4", 1 ] ]],
            "steps": []
        })" );
    }

    SECTION( "zero-time step" ) {
        check_step_recipe_throws( R"({
            "type": "recipe", "result": "cudgel", "id_suffix": "test_zero",
            "category": "CC_WEAPON", "subcategory": "CSC_WEAPON_BASHING",
            "skill_used": "fabrication", "difficulty": 1,
            "components": [[ [ "2x4", 1 ] ]],
            "steps": [
                { "name": "Bad", "time": "0 m", "activity_level": "NO_EXERCISE" }
            ]
        })" );
    }

    SECTION( "step with components" ) {
        check_step_recipe_throws( R"({
            "type": "recipe", "result": "cudgel", "id_suffix": "test_comp",
            "category": "CC_WEAPON", "subcategory": "CSC_WEAPON_BASHING",
            "skill_used": "fabrication", "difficulty": 1,
            "components": [[ [ "2x4", 1 ] ]],
            "steps": [
                { "name": "Bad", "time": "5 m", "activity_level": "NO_EXERCISE",
                  "components": [[ [ "2x4", 1 ] ]] }
            ]
        })" );
    }

    SECTION( "root-level tools" ) {
        check_step_recipe_throws( R"({
            "type": "recipe", "result": "cudgel", "id_suffix": "test_tools",
            "category": "CC_WEAPON", "subcategory": "CSC_WEAPON_BASHING",
            "skill_used": "fabrication", "difficulty": 1,
            "components": [[ [ "2x4", 1 ] ]],
            "tools": [[ [ "hammer", -1 ] ]],
            "steps": [
                { "name": "Work", "time": "5 m", "activity_level": "NO_EXERCISE" }
            ]
        })" );
    }

    SECTION( "root-level proficiencies" ) {
        check_step_recipe_throws( R"({
            "type": "recipe", "result": "cudgel", "id_suffix": "test_prof",
            "category": "CC_WEAPON", "subcategory": "CSC_WEAPON_BASHING",
            "skill_used": "fabrication", "difficulty": 1,
            "components": [[ [ "2x4", 1 ] ]],
            "proficiencies": [ { "proficiency": "prof_test" } ],
            "steps": [
                { "name": "Work", "time": "5 m", "activity_level": "NO_EXERCISE" }
            ]
        })" );
    }

    SECTION( "root-level time" ) {
        check_step_recipe_throws( R"({
            "type": "recipe", "result": "cudgel", "id_suffix": "test_time",
            "category": "CC_WEAPON", "subcategory": "CSC_WEAPON_BASHING",
            "skill_used": "fabrication", "difficulty": 1,
            "time": "10 m",
            "components": [[ [ "2x4", 1 ] ]],
            "steps": [
                { "name": "Work", "time": "5 m", "activity_level": "NO_EXERCISE" }
            ]
        })" );
    }

    SECTION( "root-level batch_time_factors" ) {
        check_step_recipe_throws( R"({
            "type": "recipe", "result": "cudgel", "id_suffix": "test_batch",
            "category": "CC_WEAPON", "subcategory": "CSC_WEAPON_BASHING",
            "skill_used": "fabrication", "difficulty": 1,
            "batch_time_factors": [ 50, 4 ],
            "components": [[ [ "2x4", 1 ] ]],
            "steps": [
                { "name": "Work", "time": "5 m", "activity_level": "NO_EXERCISE" }
            ]
        })" );
    }

    SECTION( "root-level activity_level" ) {
        check_step_recipe_throws( R"({
            "type": "recipe", "result": "cudgel", "id_suffix": "test_exertion",
            "category": "CC_WEAPON", "subcategory": "CSC_WEAPON_BASHING",
            "skill_used": "fabrication", "difficulty": 1,
            "activity_level": "MODERATE_EXERCISE",
            "components": [[ [ "2x4", 1 ] ]],
            "steps": [
                { "name": "Work", "time": "5 m", "activity_level": "NO_EXERCISE" }
            ]
        })" );
    }

    SECTION( "root-level using" ) {
        check_step_recipe_throws( R"({
            "type": "recipe", "result": "cudgel", "id_suffix": "test_using",
            "category": "CC_WEAPON", "subcategory": "CSC_WEAPON_BASHING",
            "skill_used": "fabrication", "difficulty": 1,
            "using": [ [ "sewing_standard", 1 ] ],
            "components": [[ [ "2x4", 1 ] ]],
            "steps": [
                { "name": "Work", "time": "5 m", "activity_level": "NO_EXERCISE" }
            ]
        })" );
    }

    SECTION( "abstract step recipe" ) {
        check_step_recipe_throws( R"({
            "abstract": "test_abstract_step",
            "type": "recipe",
            "steps": [
                { "name": "Work", "time": "5 m", "activity_level": "NO_EXERCISE" }
            ]
        })" );
    }

    SECTION( "copy-from on step recipe" ) {
        check_step_recipe_throws( R"({
            "type": "recipe",
            "result": "cudgel",
            "id_suffix": "test_copyfrom",
            "copy-from": "cudgel",
            "category": "CC_WEAPON",
            "subcategory": "CSC_WEAPON_BASHING",
            "skill_used": "fabrication",
            "difficulty": 1,
            "components": [[ [ "2x4", 1 ] ]],
            "steps": [
                { "name": "Work", "time": "5 m", "activity_level": "NO_EXERCISE" }
            ]
        })" );
    }
}

// ---- End-to-end craft ----

static void setup_step_craft( const recipe_id &rid )
{
    clear_avatar();
    clear_map();
    map &here = get_map();
    avatar &guy = get_avatar();
    const tripoint_bub_ms origin( 60, 60, 0 );
    guy.setpos( here, origin );
    guy.toggle_trait( trait_DEBUG_CNF );

    const recipe &r = rid.obj();
    guy.set_skill_level( r.skill_used, r.difficulty + 1 );
    for( const recipe_proficiency &p : r.get_proficiencies() ) {
        guy.add_proficiency( p.id, true );
    }

    for( const std::vector<item_comp> &comp_group : r.simple_requirements().get_components() ) {
        if( !comp_group.empty() ) {
            here.add_item( origin, item( comp_group.front().type, calendar::turn,
                                         comp_group.front().count ) );
        }
    }
    for( const std::vector<quality_requirement> &qual_group :
         r.simple_requirements().get_qualities() ) {
        for( const quality_requirement &qual : qual_group ) {
            if( qual.type.str() == "CUT" ) {
                here.add_item( origin, item( itype_knife_hunting ) );
            }
        }
    }
    guy.invalidate_crafting_inventory();
    guy.learn_recipe( &r );

    set_time( calendar::turn_zero + 12_hours );
    guy.remove_weapon();
    guy.make_craft( rid, 1 );
    REQUIRE( guy.activity );
    REQUIRE( guy.activity.id() == ACT_CRAFT );
}

static int run_craft_to_completion( avatar &guy )
{
    int turns = 0;
    while( guy.activity.id() == ACT_CRAFT ) {
        ++turns;
        guy.set_moves( 100 );
        guy.activity.do_turn( guy );
        if( turns % 60 == 0 ) {
            guy.update_mental_focus();
        }
        if( turns > 100000 ) {
            FAIL( "craft did not complete within 100000 turns" );
            break;
        }
    }
    return turns;
}

TEST_CASE( "step_recipe_end_to_end_craft", "[recipe][steps][crafting]" )
{
    setup_step_craft( recipe_cudgel_test_steps_basic );
    avatar &guy = get_avatar();
    const recipe &r = recipe_cudgel_test_steps_basic.obj();

    const int expected_moves = r.batch_time( guy, 1, 1.0f, 0 );
    const int expected_turns = divide_round_up( expected_moves, 100 );
    REQUIRE( expected_turns > 2 );

    const int actual_turns = run_craft_to_completion( guy );

    CHECK( std::abs( actual_turns - expected_turns ) <= 1 );

    bool has_cudgel = false;
    for( const item *it : guy.inv_dump() ) {
        if( it->typeId() == itype_cudgel ) {
            has_cudgel = true;
        }
    }
    if( !has_cudgel && guy.get_wielded_item() &&
        guy.get_wielded_item()->typeId() == itype_cudgel ) {
        has_cudgel = true;
    }
    CHECK( has_cudgel );
}

TEST_CASE( "step_recipe_interrupt_and_resume", "[recipe][steps][crafting]" )
{
    setup_step_craft( recipe_cudgel_test_steps_basic );
    avatar &guy = get_avatar();
    const recipe &r = recipe_cudgel_test_steps_basic.obj();

    const int expected_moves = r.batch_time( guy, 1, 1.0f, 0 );
    const int expected_turns = divide_round_up( expected_moves, 100 );
    REQUIRE( expected_turns > 10 );

    int first_turns = 0;
    while( guy.activity.id() == ACT_CRAFT && first_turns < 5 ) {
        ++first_turns;
        guy.set_moves( 100 );
        guy.activity.do_turn( guy );
    }
    REQUIRE( first_turns == 5 );

    // Kill light to force interrupt
    set_time( calendar::turn_zero + 0_hours );
    guy.set_moves( 100 );
    guy.activity.do_turn( guy );
    const bool craft_stopped = !guy.activity || guy.activity.id() != ACT_CRAFT;
    REQUIRE( craft_stopped );

    // Find in-progress craft and resume
    std::vector<item *> crafts = guy.items_with( []( const item & itm ) {
        return itm.is_craft();
    } );
    REQUIRE( crafts.size() == 1 );
    item *craft = crafts.front();

    set_time( calendar::turn_zero + 12_hours );
    REQUIRE( !guy.activity );
    guy.use( guy.get_item_position( craft ) );
    REQUIRE( guy.activity );
    REQUIRE( guy.activity.id() == ACT_CRAFT );

    const int resume_turns = run_craft_to_completion( guy );

    const int total_turns = first_turns + resume_turns;
    CHECK( std::abs( total_turns - expected_turns ) <= 1 );
}

// ---- Step iteration tests ----

// Helper: setup step craft WITHOUT granting proficiencies
static void setup_step_craft_no_profs( const recipe_id &rid )
{
    setup_step_craft( rid );
    avatar &guy = get_avatar();
    const recipe &r = rid.obj();

    // Strip all step proficiencies (setup_step_craft grants them)
    for( const recipe_proficiency &p : r.get_proficiencies() ) {
        if( !p.required ) {
            guy.lose_proficiency( p.id, true );
        }
    }

    // Restart craft without profs, re-supplying consumed components
    guy.cancel_activity();
    guy.remove_weapon();
    map &here = get_map();
    for( const std::vector<item_comp> &comp_group : r.simple_requirements().get_components() ) {
        if( !comp_group.empty() ) {
            here.add_item( guy.pos_bub(), item( comp_group.front().type, calendar::turn,
                                                comp_group.front().count ) );
        }
    }
    guy.invalidate_crafting_inventory();
    guy.set_focus( 100 );
    guy.make_craft( rid, 1 );
    REQUIRE( guy.activity );
    REQUIRE( guy.activity.id() == ACT_CRAFT );
}

// Helper: advance N turns, with periodic focus updates
static void advance_turns( avatar &guy, int n )
{
    for( int i = 0; i < n && guy.activity.id() == ACT_CRAFT; ++i ) {
        guy.set_moves( 100 );
        guy.activity.do_turn( guy );
        if( i % 60 == 0 ) {
            guy.update_mental_focus();
        }
    }
}

// Helper: advance until craft reaches a given step index
static void advance_until_step( avatar &guy, int target_step )
{
    int safety = 0;
    while( guy.activity.id() == ACT_CRAFT && safety < 100000 ) {
        // Find the in-progress craft item
        item *craft = nullptr;
        for( item *it : guy.items_with( []( const item & itm ) {
        return itm.is_craft();
        } ) ) {
            craft = it;
            break;
        }
        if( !craft ) {
            // Craft is wielded
            if( guy.get_wielded_item() && guy.get_wielded_item()->is_craft() ) {
                craft = &*guy.get_wielded_item();
            }
        }
        if( craft && craft->get_current_step() >= target_step ) {
            break;
        }
        guy.set_moves( 100 );
        guy.activity.do_turn( guy );
        ++safety;
        if( safety % 60 == 0 ) {
            guy.update_mental_focus();
        }
    }
    REQUIRE( safety < 100000 );
}

// Helper: find the in-progress craft item on the avatar
static item *find_craft_item( avatar &guy )
{
    for( item *it : guy.items_with( []( const item & itm ) {
    return itm.is_craft();
    } ) ) {
        return it;
    }
    if( guy.get_wielded_item() && guy.get_wielded_item()->is_craft() ) {
        return &*guy.get_wielded_item();
    }
    return nullptr;
}

// step_budget_moves helper returns correct values
TEST_CASE( "step_budget_moves_direct", "[recipe][steps]" )
{
    clear_avatar();
    avatar &guy = get_avatar();
    guy.set_skill_level( skill_fabrication, 10 );
    guy.add_proficiency( proficiency_prof_test_step_a, true );
    guy.add_proficiency( proficiency_prof_test_step_b, true );

    const recipe &r = recipe_cudgel_test_steps_basic.obj();
    REQUIRE( r.has_steps() );
    REQUIRE( r.steps().size() == 3 );

    SECTION( "all profs known, batch 1" ) {
        CHECK( r.step_budget_moves( guy, 0, 1 ) == Approx( 60000.0 ) );
        CHECK( r.step_budget_moves( guy, 1, 1 ) == Approx( 30000.0 ) );
        CHECK( r.step_budget_moves( guy, 2, 1 ) == Approx( 30000.0 ) );

        double sum = r.step_budget_moves( guy, 0, 1 ) +
                     r.step_budget_moves( guy, 1, 1 ) +
                     r.step_budget_moves( guy, 2, 1 );
        CHECK( std::abs( sum - r.batch_time( guy, 1, 1.0f, 0 ) ) <= 1.0 );
    }

    SECTION( "prof_A missing, batch 1" ) {
        guy.lose_proficiency( proficiency_prof_test_step_a, true );
        // Step 0 has prof_A with 2x malus -> doubled
        CHECK( r.step_budget_moves( guy, 0, 1 ) == Approx( 120000.0 ) );
        // Other steps unaffected
        CHECK( r.step_budget_moves( guy, 1, 1 ) == Approx( 30000.0 ) );
        CHECK( r.step_budget_moves( guy, 2, 1 ) == Approx( 30000.0 ) );
    }

    SECTION( "all profs known, batch 4" ) {
        double sum = 0.0;
        for( size_t i = 0; i < r.steps().size(); ++i ) {
            sum += r.step_budget_moves( guy, i, 4 );
        }
        CHECK( std::abs( sum - r.batch_time( guy, 4, 1.0f, 0 ) ) <= 1.0 );
    }
}

// step transitions at exact boundaries
TEST_CASE( "step_transitions_exact_boundaries", "[recipe][steps][crafting]" )
{
    setup_step_craft( recipe_cudgel_test_steps_basic );
    avatar &guy = get_avatar();
    const recipe &r = recipe_cudgel_test_steps_basic.obj();

    // With all profs and batch=1, step budgets are 600, 300, 300 turns
    const int step_0_turns = static_cast<int>( r.step_budget_moves( guy, 0, 1 ) / 100 );
    const int step_1_turns = static_cast<int>( r.step_budget_moves( guy, 1, 1 ) / 100 );
    REQUIRE( step_0_turns == 600 );
    REQUIRE( step_1_turns == 300 );

    item *craft = find_craft_item( guy );
    REQUIRE( craft );

    // Just before step 0 boundary
    advance_turns( guy, step_0_turns - 1 );
    CHECK( craft->get_current_step() == 0 );

    // Cross step 0 boundary
    advance_turns( guy, 1 );
    CHECK( craft->get_current_step() == 1 );
    // Remainder should be small (at most one turn's worth of work)
    CHECK( craft->get_step_progress() >= 0.0 );
    CHECK( craft->get_step_progress() < 200.0 );

    // Cross step 1 boundary
    advance_turns( guy, step_1_turns );
    CHECK( craft->get_current_step() == 2 );

    // Complete
    run_craft_to_completion( guy );
}

// multiple step boundaries crossed in single turn
TEST_CASE( "step_multi_boundary_single_turn", "[recipe][steps][crafting]" )
{
    setup_step_craft( recipe_cudgel_test_steps_basic );
    avatar &guy = get_avatar();

    item *craft = find_craft_item( guy );
    REQUIRE( craft );

    // Step budgets: 60000 + 30000 + 30000 = 120000mv.
    // Give 100000 moves -> crosses step 0 and step 1, lands in step 2.
    guy.set_moves( 100000 );
    guy.activity.do_turn( guy );

    CHECK( craft->get_current_step() == 2 );
    CHECK( craft->get_step_progress() == Approx( 100000.0 - 60000.0 - 30000.0 ).margin( 100.0 ) );
    // Should NOT have completed (need 120000 total)
    CHECK( guy.activity.id() == ACT_CRAFT );
}

// exertion changes per step
TEST_CASE( "step_exertion_per_step", "[recipe][steps][crafting]" )
{
    setup_step_craft( recipe_cudgel_test_steps_basic );
    avatar &guy = get_avatar();

    const recipe &r = recipe_cudgel_test_steps_basic.obj();
    // Step 0: MODERATE_EXERCISE
    CHECK( guy.activity.exertion_level() == Approx( r.steps()[0].exertion ) );

    // Advance to step 1 (LIGHT_EXERCISE)
    advance_until_step( guy, 1 );
    // Need one more do_turn for exertion_level to read the updated step
    guy.set_moves( 100 );
    guy.activity.do_turn( guy );
    CHECK( guy.activity.exertion_level() == Approx( r.steps()[1].exertion ) );

    // Advance to step 2 (NO_EXERCISE)
    advance_until_step( guy, 2 );
    guy.set_moves( 100 );
    guy.activity.do_turn( guy );
    CHECK( guy.activity.exertion_level() == Approx( r.steps()[2].exertion ) );
}

// exertion correct on resume
TEST_CASE( "step_exertion_correct_on_resume", "[recipe][steps][crafting]" )
{
    setup_step_craft( recipe_cudgel_test_steps_basic );
    avatar &guy = get_avatar();

    // Advance into step 1
    advance_until_step( guy, 1 );

    // Interrupt (kill light)
    set_time( calendar::turn_zero + 0_hours );
    guy.set_moves( 100 );
    guy.activity.do_turn( guy );
    {
        const bool craft_stopped = !guy.activity || guy.activity.id() != ACT_CRAFT;
        REQUIRE( craft_stopped );
    }

    // Resume
    item *craft = find_craft_item( guy );
    REQUIRE( craft );
    REQUIRE( craft->get_current_step() == 1 );

    set_time( calendar::turn_zero + 12_hours );
    guy.use( guy.get_item_position( craft ) );
    REQUIRE( guy.activity );
    REQUIRE( guy.activity.id() == ACT_CRAFT );

    // Exertion should be step 1's value (LIGHT_EXERCISE), not step 0 or average
    const recipe &r = recipe_cudgel_test_steps_basic.obj();
    CHECK( guy.activity.exertion_level() == Approx( r.steps()[1].exertion ) );
}

// proficiency learning is targeted to current step
TEST_CASE( "step_proficiency_learning_targeted", "[recipe][steps][crafting]" )
{
    setup_step_craft_no_profs( recipe_cudgel_test_steps_basic );
    avatar &guy = get_avatar();

    REQUIRE( !guy.has_proficiency( proficiency_prof_test_step_a ) );
    REQUIRE( !guy.has_proficiency( proficiency_prof_test_step_b ) );

    item *craft = find_craft_item( guy );
    REQUIRE( craft );

    // Run through step 0 into step 1. Step 0 has prof_A.
    advance_until_step( guy, 1 );

    // At this point, all step 0 learning is done. Snapshot.
    float prof_a_at_step1_entry = guy.get_proficiency_practice( proficiency_prof_test_step_a );
    float prof_b_at_step1_entry = guy.get_proficiency_practice( proficiency_prof_test_step_b );
    CHECK( prof_a_at_step1_entry > 0.0f );
    CHECK( prof_b_at_step1_entry == 0.0f );

    // Run in step 1 (Sand, no profs) past a 5% tick boundary.
    const int counter_at_step1_entry = craft->item_counter;
    while( craft->get_current_step() == 1 &&
           craft->item_counter / 500000 == counter_at_step1_entry / 500000 ) {
        guy.set_moves( 100 );
        guy.activity.do_turn( guy );
        if( !guy.activity || guy.activity.id() != ACT_CRAFT ) {
            break;
        }
    }
    // Verify we crossed a 5% tick while in step 1
    REQUIRE( craft->item_counter / 500000 > counter_at_step1_entry / 500000 );

    // prof_A should not have changed (Sand has no profs)
    CHECK( guy.get_proficiency_practice( proficiency_prof_test_step_a ) ==
           Approx( prof_a_at_step1_entry ) );
    CHECK( guy.get_proficiency_practice( proficiency_prof_test_step_b ) == 0.0f );

    // Advance into step 2 (Finish, prof_B). Run past a 5% tick.
    advance_until_step( guy, 2 );
    const int counter_at_step2_entry = craft->item_counter;
    while( craft->get_current_step() == 2 &&
           craft->item_counter / 500000 == counter_at_step2_entry / 500000 ) {
        guy.set_moves( 100 );
        guy.activity.do_turn( guy );
        if( !guy.activity || guy.activity.id() != ACT_CRAFT ) {
            break;
        }
    }
    CHECK( guy.get_proficiency_practice( proficiency_prof_test_step_b ) > 0.0f );
}

// retroactive repricing when proficiency learned mid-step
TEST_CASE( "step_retroactive_repricing", "[recipe][steps][crafting]" )
{
    setup_step_craft_no_profs( recipe_cudgel_test_steps_basic );
    avatar &guy = get_avatar();

    item *craft = find_craft_item( guy );
    REQUIRE( craft );

    // Step 0 budget with missing prof_A (2x) = 120000mv = 1200 turns.
    // Run 800 turns (2/3 of doubled budget).
    advance_turns( guy, 800 );
    REQUIRE( guy.activity.id() == ACT_CRAFT );
    CHECK( craft->get_current_step() == 0 );

    // Grant prof_A. Budget drops to 60000mv. step_progress (~80000) > new budget.
    guy.add_proficiency( proficiency_prof_test_step_a, true );

    // Next turn should transition past step 0.
    guy.set_moves( 100 );
    guy.activity.do_turn( guy );
    CHECK( craft->get_current_step() == 1 );
}

// progress message contains step name
TEST_CASE( "step_progress_message_contains_step_name", "[recipe][steps][crafting]" )
{
    setup_step_craft( recipe_cudgel_test_steps_basic );
    avatar &guy = get_avatar();

    std::optional<std::string> msg = guy.activity.get_progress_message( guy );
    REQUIRE( msg.has_value() );
    CHECK( msg->find( "Shape" ) != std::string::npos );

    advance_until_step( guy, 1 );
    // Need one turn to update the progress message path
    guy.set_moves( 100 );
    guy.activity.do_turn( guy );
    msg = guy.activity.get_progress_message( guy );
    REQUIRE( msg.has_value() );
    CHECK( msg->find( "Sand" ) != std::string::npos );

    advance_until_step( guy, 2 );
    guy.set_moves( 100 );
    guy.activity.do_turn( guy );
    msg = guy.activity.get_progress_message( guy );
    REQUIRE( msg.has_value() );
    CHECK( msg->find( "Finish" ) != std::string::npos );
}

// item round-trip preserves step state
TEST_CASE( "step_item_state_round_trip", "[recipe][steps][crafting]" )
{
    setup_step_craft( recipe_cudgel_test_steps_basic );
    avatar &guy = get_avatar();

    advance_until_step( guy, 1 );

    item *craft = find_craft_item( guy );
    REQUIRE( craft );
    REQUIRE( craft->get_current_step() == 1 );
    REQUIRE( craft->get_step_progress() >= 0.0 );

    // Advance a few more turns to accumulate some step_progress
    advance_turns( guy, 10 );

    const int saved_step = craft->get_current_step();
    const double saved_progress = craft->get_step_progress();
    const int saved_counter = craft->item_counter;

    // Serialize
    std::string serialized;
    {
        std::ostringstream os;
        JsonOut jsout( os );
        craft->serialize( jsout );
        serialized = os.str();
    }

    // Deserialize into fresh item
    item loaded_item;
    {
        std::istringstream is( serialized );
        JsonValue jsin = json_loader::from_string( serialized );
        loaded_item.deserialize( jsin );
    }

    CHECK( loaded_item.get_current_step() == saved_step );
    CHECK( loaded_item.get_step_progress() == Approx( saved_progress ) );
    CHECK( loaded_item.item_counter == saved_counter );
}

// interrupt and resume completes at expected time
TEST_CASE( "step_interrupt_resume_completes", "[recipe][steps][crafting]" )
{
    setup_step_craft( recipe_cudgel_test_steps_basic );
    avatar &guy = get_avatar();
    const recipe &r = recipe_cudgel_test_steps_basic.obj();

    const int expected_moves = r.batch_time( guy, 1, 1.0f, 0 );
    const int expected_turns = divide_round_up( expected_moves, 100 );

    // Advance into step 1
    advance_until_step( guy, 1 );

    // Count turns so far
    item *craft = find_craft_item( guy );
    REQUIRE( craft );
    int first_turns = static_cast<int>( craft->item_counter *
                                        static_cast<double>( expected_turns ) / 10000000.0 );

    // Interrupt
    set_time( calendar::turn_zero + 0_hours );
    guy.set_moves( 100 );
    guy.activity.do_turn( guy );
    {
        const bool craft_stopped = !guy.activity || guy.activity.id() != ACT_CRAFT;
        REQUIRE( craft_stopped );
    }

    // Resume
    craft = find_craft_item( guy );
    REQUIRE( craft );
    set_time( calendar::turn_zero + 12_hours );
    guy.use( guy.get_item_position( craft ) );
    REQUIRE( guy.activity.id() == ACT_CRAFT );

    const int resume_turns = run_craft_to_completion( guy );
    const int total_turns = first_turns + resume_turns;
    CHECK( std::abs( total_turns - expected_turns ) <= 2 );
}

// legacy migration from saves without step fields
// Strips a JSON key-value pair from a serialized string.
static void strip_json_field( std::string &json, const std::string &field_name )
{
    std::string quoted = "\"" + field_name + "\"";
    std::string::size_type pos = json.find( quoted );
    if( pos == std::string::npos ) {
        return;
    }
    std::string::size_type end = json.find_first_of( ",}", pos );
    if( end != std::string::npos && json[end] == ',' ) {
        json.erase( pos, end - pos + 1 );
    } else if( end != std::string::npos ) {
        std::string::size_type comma = json.rfind( ',', pos );
        if( comma != std::string::npos ) {
            json.erase( comma, end - comma );
        }
    }
}

TEST_CASE( "step_legacy_migration", "[recipe][steps][crafting]" )
{
    setup_step_craft( recipe_cudgel_test_steps_basic );
    avatar &guy = get_avatar();
    const recipe &r = recipe_cudgel_test_steps_basic.obj();

    // Advance to ~60% progress
    const int total_turns = divide_round_up(
                                static_cast<int>( r.batch_time( guy, 1, 1.0f, 0 ) ), 100 );
    advance_turns( guy, total_turns * 6 / 10 );
    REQUIRE( guy.activity.id() == ACT_CRAFT );

    item *craft = find_craft_item( guy );
    REQUIRE( craft );
    REQUIRE( craft->item_counter > 0 );

    // Interrupt the craft so we can swap the item
    set_time( calendar::turn_zero + 0_hours );
    guy.set_moves( 100 );
    guy.activity.do_turn( guy );
    {
        const bool craft_stopped = !guy.activity || guy.activity.id() != ACT_CRAFT;
        REQUIRE( craft_stopped );
    }

    // Find the interrupted craft item
    craft = find_craft_item( guy );
    REQUIRE( craft );
    const int saved_counter = craft->item_counter;
    REQUIRE( saved_counter > 0 );

    // Serialize it, strip the step fields to simulate a pre-step-iteration save
    std::string serialized;
    {
        std::ostringstream os;
        JsonOut jsout( os );
        craft->serialize( jsout );
        serialized = os.str();
    }
    strip_json_field( serialized, "current_step" );
    strip_json_field( serialized, "step_progress" );
    CHECK( serialized.find( "current_step" ) == std::string::npos );
    CHECK( serialized.find( "step_progress" ) == std::string::npos );

    // Deserialize into a legacy item (missing fields default to 0)
    item legacy_item;
    {
        JsonValue jsin = json_loader::from_string( serialized );
        legacy_item.deserialize( jsin );
    }
    REQUIRE( legacy_item.is_craft() );
    CHECK( legacy_item.get_current_step() == 0 );
    CHECK( legacy_item.get_step_progress() == 0.0 );
    CHECK( legacy_item.item_counter == saved_counter );

    // Swap: remove the current craft, add the legacy item
    guy.i_rem( craft );
    guy.i_add( legacy_item );
    guy.invalidate_crafting_inventory();

    // Resume the legacy craft
    set_time( calendar::turn_zero + 12_hours );
    item *legacy_craft = find_craft_item( guy );
    REQUIRE( legacy_craft );
    guy.use( guy.get_item_position( legacy_craft ) );
    REQUIRE( guy.activity );
    REQUIRE( guy.activity.id() == ACT_CRAFT );

    // Find the craft item again (may have moved due to wielding)
    legacy_craft = find_craft_item( guy );
    REQUIRE( legacy_craft );

    // Run one turn to trigger migration
    guy.set_moves( 100 );
    guy.activity.do_turn( guy );

    // At 60% progress: step 0 is 50% of total, so migration should land in step 1
    CHECK( legacy_craft->get_current_step() == 1 );
    CHECK( legacy_craft->get_step_progress() > 0.0 );
}

// stepless recipe is unaffected by step iteration
TEST_CASE( "stepless_recipe_step_state_unchanged", "[recipe][steps][crafting]" )
{
    // Use cudgel_simple from TEST_DATA -- a non-step recipe
    setup_step_craft( recipe_cudgel_simple );
    avatar &guy = get_avatar();

    const recipe &r = recipe_cudgel_simple.obj();
    REQUIRE_FALSE( r.has_steps() );

    item *craft = find_craft_item( guy );
    REQUIRE( craft );

    // Run partway
    advance_turns( guy, 50 );
    REQUIRE( guy.activity.id() == ACT_CRAFT );

    CHECK( craft->get_current_step() == 0 );
    CHECK( craft->get_step_progress() == 0.0 );

    // Exertion is recipe-level
    CHECK( guy.activity.exertion_level() == Approx( r.exertion_level() ) );

    // Progress message contains craft name but no step names
    std::optional<std::string> msg = guy.activity.get_progress_message( guy );
    REQUIRE( msg.has_value() );
    CHECK( msg->find( "Shape" ) == std::string::npos );
    CHECK( msg->find( "Sand" ) == std::string::npos );
    CHECK( msg->find( "Finish" ) == std::string::npos );
}

// ---- Edge case: inherited steps on non-step child ----

TEST_CASE( "step_recipe_inherited_steps_rejected", "[recipe][steps]" )
{
    // Load a step recipe to populate steps_, then load non-step JSON onto the
    // same object. This simulates a non-step child that copy-from'd a step base.
    recipe r;
    REQUIRE( r.steps().empty() );
    {
        const std::string step_json = R"({
            "type": "recipe",
            "result": "cudgel",
            "id_suffix": "test_inherit_base",
            "category": "CC_WEAPON",
            "subcategory": "CSC_WEAPON_BASHING",
            "skill_used": "fabrication",
            "difficulty": 1,
            "components": [[ [ "2x4", 1 ] ]],
            "steps": [
                { "name": "Work", "time": "10 m", "activity_level": "MODERATE_EXERCISE" }
            ]
        })";
        JsonObject jo = json_loader::from_string( step_json );
        jo.allow_omitted_members();
        r.load( jo, "dda" );
    }
    REQUIRE( r.has_steps() );

    const std::string child_json = R"({
        "type": "recipe",
        "result": "cudgel",
        "id_suffix": "test_inherit_child",
        "category": "CC_WEAPON",
        "subcategory": "CSC_WEAPON_BASHING",
        "skill_used": "fabrication",
        "difficulty": 1,
        "time": "10 m",
        "activity_level": "MODERATE_EXERCISE",
        "components": [[ [ "2x4", 1 ] ]]
    })";
    JsonObject jo2 = json_loader::from_string( child_json );
    jo2.allow_omitted_members();
    CHECK_THROWS_AS( r.load( jo2, "dda" ), JsonError );
}

// ---- Edge case: required/optional proficiency conflict across steps ----

TEST_CASE( "step_recipe_required_optional_prof_conflict", "[recipe][steps]" )
{
    const std::string json = R"({
        "type": "recipe",
        "result": "cudgel",
        "id_suffix": "test_prof_conflict",
        "category": "CC_WEAPON",
        "subcategory": "CSC_WEAPON_BASHING",
        "skill_used": "fabrication",
        "difficulty": 1,
        "components": [[ [ "2x4", 1 ] ]],
        "steps": [
            {
                "name": "Step A",
                "time": "5 m",
                "activity_level": "MODERATE_EXERCISE",
                "proficiencies": [ { "proficiency": "prof_test_step_required", "required": true } ]
            },
            {
                "name": "Step B",
                "time": "5 m",
                "activity_level": "LIGHT_EXERCISE",
                "proficiencies": [ { "proficiency": "prof_test_step_required" } ]
            }
        ]
    })";

    JsonObject jo = json_loader::from_string( json );
    jo.allow_omitted_members();
    recipe r;
    r.load( jo, "dda" );

    const std::string dmsg = capture_debugmsg_during( [&]() {
        r.finalize();
    } );
    CHECK( dmsg.find( "required in one step but optional" ) != std::string::npos );

    // Fallback: required wins, multipliers zeroed
    const std::vector<recipe_proficiency> &profs = r.get_proficiencies();
    REQUIRE( profs.size() == 1 );
    CHECK( profs[0].id == proficiency_prof_test_step_required );
    CHECK( profs[0].required == true );
    CHECK( profs[0].time_multiplier == 0.0f );
    CHECK( profs[0].skill_penalty == 0.0f );
}

// ---- Edge case: required prof zero-divisor guard in craft_proficiency_gain ----

TEST_CASE( "step_recipe_required_prof_no_crash_in_learning", "[recipe][steps][crafting]" )
{
    // This recipe has a required prof (zero time_multiplier in aggregate) and
    // an optional prof. Without the guard in craft_proficiency_gain, the
    // required prof would cause division by zero during learning.
    const recipe &r = recipe_cudgel_test_steps_required_prof.obj();

    bool has_zero_mult = false;
    for( const recipe_proficiency &p : r.get_proficiencies() ) {
        if( p.required && p.time_multiplier == 0.0f ) {
            has_zero_mult = true;
        }
    }
    REQUIRE( has_zero_mult );

    clear_avatar();
    clear_map();
    map &here = get_map();
    avatar &guy = get_avatar();
    const tripoint_bub_ms origin( 60, 60, 0 );
    guy.setpos( here, origin );
    guy.toggle_trait( trait_DEBUG_CNF );
    guy.set_skill_level( skill_fabrication, 10 );
    // Grant required prof (gating), but NOT the optional one (will be learned)
    guy.add_proficiency( proficiency_prof_test_step_required, true );
    REQUIRE( !guy.has_proficiency( proficiency_prof_test_step_a ) );

    here.add_item( origin, item( itype_2x4 ) );
    guy.invalidate_crafting_inventory();
    guy.learn_recipe( &r );

    set_time( calendar::turn_zero + 12_hours );
    guy.remove_weapon();
    guy.set_focus( 100 );
    guy.make_craft( recipe_cudgel_test_steps_required_prof, 1 );
    REQUIRE( guy.activity );
    REQUIRE( guy.activity.id() == ACT_CRAFT );

    // Without the zero-divisor guard this crashes with SIGFPE
    const int turns = run_craft_to_completion( guy );
    CHECK( turns > 0 );

    CHECK( guy.get_proficiency_practice( proficiency_prof_test_step_a ) > 0.0f );
}
