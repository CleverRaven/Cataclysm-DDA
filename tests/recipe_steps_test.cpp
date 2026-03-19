#include <cstdint>
#include <cstdlib>
#include <functional>
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

// ---- XP distribution (documents phase-1 approximate behavior) ----

TEST_CASE( "step_recipe_xp_distribution", "[recipe][steps][crafting]" )
{
    setup_step_craft( recipe_cudgel_test_steps_basic );
    avatar &guy = get_avatar();

    guy.lose_proficiency( proficiency_prof_test_step_a, true );
    guy.lose_proficiency( proficiency_prof_test_step_b, true );
    REQUIRE( !guy.has_proficiency( proficiency_prof_test_step_a ) );
    REQUIRE( !guy.has_proficiency( proficiency_prof_test_step_b ) );

    // Restart craft without profs, re-supplying consumed components
    guy.cancel_activity();
    guy.remove_weapon();
    const recipe &r = recipe_cudgel_test_steps_basic.obj();
    map &here = get_map();
    for( const std::vector<item_comp> &comp_group : r.simple_requirements().get_components() ) {
        if( !comp_group.empty() ) {
            here.add_item( guy.pos_bub(), item( comp_group.front().type, calendar::turn,
                                                comp_group.front().count ) );
        }
    }
    guy.invalidate_crafting_inventory();
    guy.set_focus( 100 );
    guy.make_craft( recipe_cudgel_test_steps_basic, 1 );
    REQUIRE( guy.activity );
    REQUIRE( guy.activity.id() == ACT_CRAFT );

    run_craft_to_completion( guy );

    // Both step proficiencies should have received non-zero practice.
    // Phase 1 distributes XP across all aggregate profs uniformly.
    CHECK( guy.get_proficiency_practice( proficiency_prof_test_step_a ) > 0.0f );
    CHECK( guy.get_proficiency_practice( proficiency_prof_test_step_b ) > 0.0f );
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
