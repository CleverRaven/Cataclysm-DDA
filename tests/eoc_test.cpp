#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character_martial_arts.h"
#include "effect_on_condition.h"
#include "game.h"
#include "make_static.h"
#include "map_helpers.h"
#include "mutation.h"
#include "timed_event.h"
#include "player_helpers.h"
#include "point.h"

static const activity_id ACT_ADD_VARIABLE_COMPLETE( "ACT_ADD_VARIABLE_COMPLETE" );
static const activity_id ACT_ADD_VARIABLE_DURING( "ACT_ADD_VARIABLE_DURING" );
static const activity_id ACT_GENERIC_EOC( "ACT_GENERIC_EOC" );

static const effect_on_condition_id
effect_on_condition_EOC_TEST_TRANSFORM_LINE( "EOC_TEST_TRANSFORM_LINE" );
static const effect_on_condition_id
effect_on_condition_EOC_TEST_TRANSFORM_RADIUS( "EOC_TEST_TRANSFORM_RADIUS" );
static const effect_on_condition_id
effect_on_condition_EOC_activate_mutation_to_start_test( "EOC_activate_mutation_to_start_test" );
static const effect_on_condition_id effect_on_condition_EOC_alive_test( "EOC_alive_test" );
static const effect_on_condition_id
effect_on_condition_EOC_armor_math_test( "EOC_armor_math_test" );
static const effect_on_condition_id effect_on_condition_EOC_attack_test( "EOC_attack_test" );
static const effect_on_condition_id
effect_on_condition_EOC_combat_mutator_test( "EOC_combat_mutator_test" );
static const effect_on_condition_id
effect_on_condition_EOC_increment_var_var( "EOC_increment_var_var" );
static const effect_on_condition_id
effect_on_condition_EOC_item_activate_test( "EOC_item_activate_test" );
static const effect_on_condition_id
effect_on_condition_EOC_item_flag_test( "EOC_item_flag_test" );
static const effect_on_condition_id
effect_on_condition_EOC_item_math_test( "EOC_item_math_test" );
static const effect_on_condition_id
effect_on_condition_EOC_item_teleport_test( "EOC_item_teleport_test" );
static const effect_on_condition_id
effect_on_condition_EOC_jmath_test( "EOC_jmath_test" );
static const effect_on_condition_id effect_on_condition_EOC_map_test( "EOC_map_test" );
static const effect_on_condition_id
effect_on_condition_EOC_martial_art_test_1( "EOC_martial_art_test_1" );
static const effect_on_condition_id
effect_on_condition_EOC_martial_art_test_2( "EOC_martial_art_test_2" );
static const effect_on_condition_id
effect_on_condition_EOC_math_addiction( "EOC_math_addiction" );
static const effect_on_condition_id
effect_on_condition_EOC_math_armor( "EOC_math_armor" );
static const effect_on_condition_id
effect_on_condition_EOC_math_diag_assign( "EOC_math_diag_assign" );
static const effect_on_condition_id
effect_on_condition_EOC_math_diag_w_vars( "EOC_math_diag_w_vars" );
static const effect_on_condition_id effect_on_condition_EOC_math_duration( "EOC_math_duration" );
static const effect_on_condition_id
effect_on_condition_EOC_math_field( "EOC_math_field" );
static const effect_on_condition_id
effect_on_condition_EOC_math_item_count( "EOC_math_item_count" );
static const effect_on_condition_id
effect_on_condition_EOC_math_proficiency( "EOC_math_proficiency" );
static const effect_on_condition_id
effect_on_condition_EOC_math_spell( "EOC_math_spell" );
static const effect_on_condition_id
effect_on_condition_EOC_math_spell_xp( "EOC_math_spell_xp" );
static const effect_on_condition_id
effect_on_condition_EOC_math_switch_math( "EOC_math_switch_math" );
static const effect_on_condition_id
effect_on_condition_EOC_math_test_context( "EOC_math_test_context" );
static const effect_on_condition_id
effect_on_condition_EOC_math_test_equals_assign( "EOC_math_test_equals_assign" );
static const effect_on_condition_id
effect_on_condition_EOC_math_test_greater_increment( "EOC_math_test_greater_increment" );
static const effect_on_condition_id
effect_on_condition_EOC_math_test_inline_condition( "EOC_math_test_inline_condition" );
static const effect_on_condition_id
effect_on_condition_EOC_math_weighted_list( "EOC_math_weighted_list" );
static const effect_on_condition_id
effect_on_condition_EOC_meta_test_message( "EOC_meta_test_message" );
static const effect_on_condition_id
effect_on_condition_EOC_meta_test_talker_type( "EOC_meta_test_talker_type" );
static const effect_on_condition_id
effect_on_condition_EOC_mon_nearby_test( "EOC_mon_nearby_test" );
static const effect_on_condition_id effect_on_condition_EOC_mutator_test( "EOC_mutator_test" );
static const effect_on_condition_id effect_on_condition_EOC_options_tests( "EOC_options_tests" );
static const effect_on_condition_id effect_on_condition_EOC_recipe_test_1( "EOC_recipe_test_1" );
static const effect_on_condition_id effect_on_condition_EOC_recipe_test_2( "EOC_recipe_test_2" );
static const effect_on_condition_id
effect_on_condition_EOC_run_inv_prepare( "EOC_run_inv_prepare" );
static const effect_on_condition_id effect_on_condition_EOC_run_inv_test1( "EOC_run_inv_test1" );
static const effect_on_condition_id effect_on_condition_EOC_run_inv_test2( "EOC_run_inv_test2" );
static const effect_on_condition_id effect_on_condition_EOC_run_inv_test3( "EOC_run_inv_test3" );
static const effect_on_condition_id effect_on_condition_EOC_run_inv_test4( "EOC_run_inv_test4" );
static const effect_on_condition_id effect_on_condition_EOC_run_inv_test5( "EOC_run_inv_test5" );
static const effect_on_condition_id effect_on_condition_EOC_run_until_test( "EOC_run_until_test" );
static const effect_on_condition_id effect_on_condition_EOC_run_with_test( "EOC_run_with_test" );
static const effect_on_condition_id
effect_on_condition_EOC_run_with_test_expects_fail( "EOC_run_with_test_expects_fail" );
static const effect_on_condition_id
effect_on_condition_EOC_run_with_test_expects_pass( "EOC_run_with_test_expects_pass" );
static const effect_on_condition_id
effect_on_condition_EOC_run_with_test_queued( "EOC_run_with_test_queued" );
static const effect_on_condition_id
effect_on_condition_EOC_stored_condition_test( "EOC_stored_condition_test" );
static const effect_on_condition_id
effect_on_condition_EOC_string_test( "EOC_string_test" );
static const effect_on_condition_id
effect_on_condition_EOC_string_test_nest( "EOC_string_test_nest" );
static const effect_on_condition_id
effect_on_condition_EOC_string_var_var( "EOC_string_var_var" );
static const effect_on_condition_id effect_on_condition_EOC_teleport_test( "EOC_teleport_test" );
static const effect_on_condition_id
effect_on_condition_EOC_test_weapon_damage( "EOC_test_weapon_damage" );
static const effect_on_condition_id effect_on_condition_EOC_try_kill( "EOC_try_kill" );

static const flag_id json_flag_FILTHY( "FILTHY" );

static const furn_str_id furn_f_cardboard_box( "f_cardboard_box" );
static const furn_str_id furn_test_f_eoc( "test_f_eoc" );

static const itype_id itype_backpack( "backpack" );
static const itype_id itype_sword_wood( "sword_wood" );
static const itype_id itype_test_glock( "test_glock" );
static const itype_id itype_test_knife_combat( "test_knife_combat" );

static const matype_id style_aikido( "style_aikido" );
static const matype_id style_none( "style_none" );

static const mtype_id mon_triffid( "mon_triffid" );
static const mtype_id mon_zombie( "mon_zombie" );
static const mtype_id mon_zombie_smoker( "mon_zombie_smoker" );
static const mtype_id mon_zombie_tough( "mon_zombie_tough" );

static const recipe_id recipe_cattail_jelly( "cattail_jelly" );

static const skill_id skill_survival( "survival" );

static const spell_id spell_test_eoc_spell( "test_eoc_spell" );

static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_grass( "t_grass" );

static const trait_id trait_process_mutation( "process_mutation" );
static const trait_id trait_process_mutation_two( "process_mutation_two" );

namespace
{
void complete_activity( Character &u )
{
    while( !u.activity.is_null() ) {
        u.set_moves( u.get_speed() );
        u.activity.do_turn( u );
    }
}

void check_ter_in_radius( tripoint_abs_ms const &center, int range, ter_id const &ter )
{
    map tm;
    tm.load( project_to<coords::sm>( center - point{ range, range } ), false, false );
    tripoint_bub_ms const center_local = tm.bub_from_abs( center );
    for( tripoint_bub_ms p : tm.points_in_radius( center_local, range ) ) {
        if( trig_dist( center_local, p ) <= range ) {
            REQUIRE( tm.ter( p ) == ter );
        }
    }
}

void check_ter_in_line( tripoint_abs_ms const &first, tripoint_abs_ms const &second,
                        ter_id const &ter )
{
    map tm;
    tripoint_abs_ms const orig = coord_min( first, second );
    tm.load( project_to<coords::sm>( orig ), false, false );
    for( tripoint_abs_ms p : line_to( first, second ) ) {
        REQUIRE( tm.ter( tm.getlocal( p ) ) == ter );
    }
}

} // namespace

TEST_CASE( "EOC_teleport", "[eoc]" )
{
    clear_avatar();
    clear_map();
    tripoint_abs_ms before = get_avatar().get_location();
    dialogue newDialog( get_talker_for( get_avatar() ), nullptr );
    effect_on_condition_EOC_teleport_test->activate( newDialog );
    tripoint_abs_ms after = get_avatar().get_location();

    CHECK( before + tripoint_south_east == after );
}

TEST_CASE( "EOC_beta_elevate", "[eoc]" )
{
    clear_avatar();
    clear_map();
    npc &n = spawn_npc( get_avatar().pos().xy() + point_south, "thug" );

    REQUIRE( n.hp_percentage() > 0 );

    dialogue newDialog( get_talker_for( get_avatar() ), nullptr );
    effect_on_condition_EOC_try_kill->activate( newDialog );

    CHECK( n.hp_percentage() == 0 );
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity): false positive
TEST_CASE( "EOC_math_integration", "[eoc][math_parser]" )
{
    clear_avatar();
    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();
    REQUIRE( globvars.get_global_value( "npctalk_var_math_test" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_math_test_result" ).empty() );
    calendar::turn = calendar::start_of_cataclysm;

    CHECK_FALSE( effect_on_condition_EOC_math_test_greater_increment->test_condition( d ) );
    effect_on_condition_EOC_math_test_greater_increment->activate( d );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_math_test" ) ) == Approx( -1 ) );
    effect_on_condition_EOC_math_switch_math->activate( d );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_math_test_result" ) ) == Approx( 1 ) );
    CHECK( effect_on_condition_EOC_math_duration->recurrence.evaluate( d ) == 1_turns );
    calendar::turn += 1_days;

    CHECK_FALSE( effect_on_condition_EOC_math_test_equals_assign->test_condition( d ) );
    CHECK_FALSE( effect_on_condition_EOC_math_test_inline_condition->test_condition( d ) );
    get_avatar().set_stamina( 500 );
    CHECK( effect_on_condition_EOC_math_test_equals_assign->test_condition( d ) );
    CHECK( effect_on_condition_EOC_math_test_inline_condition->test_condition( d ) );
    effect_on_condition_EOC_math_test_equals_assign->activate( d );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_math_test" ) ) == Approx( 9 ) );
    effect_on_condition_EOC_math_switch_math->activate( d );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_math_test_result" ) ) == Approx( 2 ) );
    CHECK( effect_on_condition_EOC_math_duration->recurrence.evaluate( d ) == 2_turns );

    CHECK( effect_on_condition_EOC_math_test_greater_increment->test_condition( d ) );
    effect_on_condition_EOC_math_test_greater_increment->activate( d );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_math_test" ) ) == Approx( 10 ) );
    effect_on_condition_EOC_math_switch_math->activate( d );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_math_test_result" ) ) == Approx( 3 ) );
    CHECK( effect_on_condition_EOC_math_duration->recurrence.evaluate( d ) == 3_turns );

    int const stam_pre = get_avatar().get_stamina();
    effect_on_condition_EOC_math_diag_assign->activate( d );
    CHECK( get_avatar().get_stamina() == stam_pre / 2 );

    get_avatar().set_pain( 0 );
    get_avatar().set_stamina( 9000 );
    effect_on_condition_EOC_math_weighted_list->activate( d );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_weighted_var" ) ) == Approx( -999 ) );
    get_avatar().set_pain( 9000 );
    get_avatar().set_stamina( 0 );
    effect_on_condition_EOC_math_weighted_list->activate( d );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_weighted_var" ) ) == Approx( 1 ) );
}

TEST_CASE( "EOC_jmath", "[eoc][math_parser]" )
{
    global_variables &globvars = get_globals();
    globvars.clear_global_values();
    REQUIRE( globvars.get_global_value( "npctalk_var_blorgy" ).empty() );
    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    effect_on_condition_EOC_jmath_test->activate( d );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_blorgy" ) ) == Approx( 7 ) );
}

TEST_CASE( "EOC_diag_with_vars", "[eoc][math_parser]" )
{
    global_variables &globvars = get_globals();
    globvars.clear_global_values();
    REQUIRE( globvars.get_global_value( "npctalk_var_myskill_math" ).empty() );
    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    effect_on_condition_EOC_math_diag_w_vars->activate( d );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_myskill_math" ) ) == Approx( 0 ) );
    get_avatar().set_skill_level( skill_survival, 3 );
    effect_on_condition_EOC_math_diag_w_vars->activate( d );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_myskill_math" ) ) == Approx( 3 ) );
}

TEST_CASE( "EOC_transform_radius", "[eoc][timed_event]" )
{
    // no introspection :(
    constexpr int eoc_range = 5;
    constexpr time_duration delay = 30_seconds;
    clear_avatar();
    clear_map();
    tripoint_abs_ms const start = get_avatar().get_location();
    dialogue newDialog( get_talker_for( get_avatar() ), nullptr );
    check_ter_in_radius( start, eoc_range, ter_t_grass );
    effect_on_condition_EOC_TEST_TRANSFORM_RADIUS->activate( newDialog );
    check_ter_in_radius( start, eoc_range, ter_t_dirt );

    g->place_player_overmap( project_to<coords::omt>( start ) + point{ 60, 60 } );
    REQUIRE( !get_map().inbounds( start ) );

    calendar::turn += delay - 1_seconds;
    get_timed_events().process();
    check_ter_in_radius( start, eoc_range, ter_t_dirt );
    calendar::turn += 2_seconds;
    get_timed_events().process();
    check_ter_in_radius( start, eoc_range, ter_t_grass );
}

TEST_CASE( "EOC_transform_line", "[eoc][timed_event]" )
{
    clear_avatar();
    clear_map();
    standard_npc npc( "Mr. Testerman" );
    std::optional<tripoint> const dest = random_point( get_map(), []( tripoint const & p ) {
        return p.xy() != get_avatar().pos().xy();
    } );
    REQUIRE( dest.has_value() );
    npc.setpos( { dest.value().xy(), get_avatar().pos().z } );

    tripoint_abs_ms const start = get_avatar().get_location();
    tripoint_abs_ms const end = npc.get_location();
    dialogue newDialog( get_talker_for( get_avatar() ), get_talker_for( npc ) );
    check_ter_in_line( start, end, ter_t_grass );
    effect_on_condition_EOC_TEST_TRANSFORM_LINE->activate( newDialog );
    check_ter_in_line( start, end, ter_t_dirt );
}

TEST_CASE( "EOC_activity_finish", "[eoc][timed_event]" )
{
    clear_avatar();
    clear_map();
    get_avatar().assign_activity( ACT_ADD_VARIABLE_COMPLETE, 10 );

    complete_activity( get_avatar() );

    CHECK( stoi( get_avatar().get_value( "npctalk_var_activitiy_incrementer" ) ) == 1 );
}

TEST_CASE( "EOC_combat_mutator_test", "[eoc]" )
{
    clear_avatar();
    clear_map();
    item weapon( itype_test_knife_combat );
    get_avatar().set_wielded_item( weapon );
    npc &n = spawn_npc( get_avatar().pos().xy() + point_south, "thug" );

    dialogue d( get_talker_for( get_avatar() ), get_talker_for( n ) );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_key1" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key2" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key3" ).empty() );
    CHECK( effect_on_condition_EOC_combat_mutator_test->activate( d ) );
    CHECK( globvars.get_global_value( "npctalk_var_key1" ) == "RAPID_TEST" );
    CHECK( globvars.get_global_value( "npctalk_var_key2" ) == "Rapid Strike Test" );
    CHECK( globvars.get_global_value( "npctalk_var_key3" ) == "50% moves, 66% damage" );
}

TEST_CASE( "EOC_alive_test", "[eoc]" )
{
    clear_avatar();
    clear_map();

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_key1" ).empty() );
    CHECK( effect_on_condition_EOC_alive_test->activate( d ) );
    CHECK( globvars.get_global_value( "npctalk_var_key1" ) == "alive" );
}

TEST_CASE( "EOC_attack_test", "[eoc]" )
{
    clear_avatar();
    clear_map();
    npc &n = spawn_npc( get_avatar().pos().xy() + point_south, "thug" );

    dialogue newDialog( get_talker_for( get_avatar() ), get_talker_for( n ) );
    CHECK( effect_on_condition_EOC_attack_test->activate( newDialog ) );
}

TEST_CASE( "EOC_context_test", "[eoc][math_parser]" )
{
    clear_avatar();
    clear_map();

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_simple_global" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_nested_simple_global" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_non_nested_simple_global" ).empty() );
    CHECK( effect_on_condition_EOC_math_test_context->activate( d ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_simple_global" ) ) == Approx( 12 ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_nested_simple_global" ) ) == Approx(
               7 ) );
    // shouldn't be passed back up
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_non_nested_simple_global" ) ) == Approx(
               0 ) );

    // value shouldn't exist in the original dialogue
    CHECK( d.get_value( "npctalk_var_simple" ).empty() );
}

TEST_CASE( "EOC_option_test", "[eoc][math_parser]" )
{
    clear_avatar();
    clear_map();

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();

    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_key1" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key2" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key3" ).empty() );
    CHECK( effect_on_condition_EOC_options_tests->activate( d ) );
    CHECK( globvars.get_global_value( "npctalk_var_key1" ) == "ALWAYS" );
    CHECK( globvars.get_global_value( "npctalk_var_key2" ) == "4" );
    CHECK( globvars.get_global_value( "npctalk_var_key3" ) == "1" );
}

TEST_CASE( "EOC_mutator_test", "[eoc][math_parser]" )
{
    clear_avatar();
    clear_map();

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_key1" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key2" ).empty() );
    CHECK( effect_on_condition_EOC_mutator_test->activate( d ) );
    CHECK( globvars.get_global_value( "npctalk_var_key1" ) == "zombie" );
    CHECK( globvars.get_global_value( "npctalk_var_key2" ) == "zombie" );
}

TEST_CASE( "EOC_math_addiction", "[eoc][math_parser]" )
{
    clear_avatar();
    clear_map();

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_key_add_intensity" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key_add_turn" ).empty() );
    CHECK( effect_on_condition_EOC_math_addiction->activate( d ) );
    CHECK( globvars.get_global_value( "npctalk_var_key_add_intensity" ) == "1" );
    CHECK( globvars.get_global_value( "npctalk_var_key_add_turn" ) == "3600" );
}

TEST_CASE( "EOC_math_armor", "[eoc][math_parser]" )
{
    clear_avatar();
    clear_map();
    avatar &a = get_avatar();
    a.worn.wear_item( a, item( "test_eoc_armor_suit" ), false, true, true );

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_key1" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key2" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key3" ).empty() );
    CHECK( effect_on_condition_EOC_math_armor->activate( d ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_key1" ) ) == Approx( 4 ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_key2" ) ) == Approx( 9 ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_key3" ) ) == Approx( 0 ) );
}

TEST_CASE( "EOC_math_field", "[eoc][math_parser]" )
{
    clear_avatar();
    clear_map();

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    get_map().add_field( get_avatar().pos(), fd_blood, 3 );
    get_map().add_field( get_avatar().pos() + point_south, fd_blood_insect, 3 );

    REQUIRE( globvars.get_global_value( "npctalk_var_key_field_strength" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key_field_strength_north" ).empty() );
    CHECK( effect_on_condition_EOC_math_field->activate( d ) );
    CHECK( globvars.get_global_value( "npctalk_var_key_field_strength" ) == "3" );
    CHECK( globvars.get_global_value( "npctalk_var_key_field_strength_north" ) == "3" );
}

TEST_CASE( "EOC_math_item", "[eoc][math_parser]" )
{
    clear_avatar();
    clear_map();

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_key_item_count" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key_charge_count" ).empty() );
    CHECK( effect_on_condition_EOC_math_item_count->activate( d ) );
    CHECK( globvars.get_global_value( "npctalk_var_key_item_count" ) == "2" );
    CHECK( globvars.get_global_value( "npctalk_var_key_charge_count" ) == "300" );
}

TEST_CASE( "EOC_math_proficiency", "[eoc][math_parser]" )
{
    clear_avatar();
    clear_map();

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_key_total_time_required" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key_time_spent_50" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key_percent_50" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key_percent_50_turn" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key_permille_50" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key_permille_50_turn" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key_time_left_50" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key_time_left_50_turn" ).empty() );
    CHECK( effect_on_condition_EOC_math_proficiency->activate( d ) );
    CHECK( globvars.get_global_value( "npctalk_var_key_total_time_required" ) == "86400" );
    CHECK( globvars.get_global_value( "npctalk_var_key_time_spent_50" ) == "50" );
    CHECK( globvars.get_global_value( "npctalk_var_key_percent_50" ) == "50" );
    CHECK( globvars.get_global_value( "npctalk_var_key_percent_50_turn" ) == "43200" );
    CHECK( globvars.get_global_value( "npctalk_var_key_permille_50" ) == "50" );
    CHECK( globvars.get_global_value( "npctalk_var_key_permille_50_turn" ) == "4320" );
    CHECK( globvars.get_global_value( "npctalk_var_key_time_left_50" ) == "50" );
    CHECK( globvars.get_global_value( "npctalk_var_key_time_left_50_turn" ) == "86350" );
}

TEST_CASE( "EOC_math_spell", "[eoc][math_parser]" )
{
    clear_avatar();
    clear_map();

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_key_spell_level" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key_highest_spell_level" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key_school_level_test_trait" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key_spell_count" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key_spell_count_test_trait" ).empty() );
    CHECK( effect_on_condition_EOC_math_spell->activate( d ) );
    CHECK( globvars.get_global_value( "npctalk_var_key_spell_level" ) == "1" );
    CHECK( globvars.get_global_value( "npctalk_var_key_highest_spell_level" ) == "10" );
    CHECK( globvars.get_global_value( "npctalk_var_key_school_level_test_trait" ) == "1" );
    CHECK( globvars.get_global_value( "npctalk_var_key_spell_count" ) == "2" );
    CHECK( globvars.get_global_value( "npctalk_var_key_spell_count_test_trait" ) == "1" );

    get_avatar().magic->evaluate_opens_spellbook_data();

    CHECK( get_avatar().magic->get_spell( spell_test_eoc_spell ).get_effective_level() == 4 );
}

TEST_CASE( "EOC_mutation_test", "[eoc][mutations]" )
{
    clear_avatar();
    clear_map();

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();

    avatar &me = get_avatar();

    // test activation
    globvars.clear_global_values();
    me.toggle_trait( trait_process_mutation_two );
    me.activate_mutation( trait_process_mutation_two );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_test_val" ) ) == Approx(
               1 ) );
    CHECK( globvars.get_global_value( "npctalk_var_context_test" ) == "process_mutation_two" );

    // test process
    globvars.clear_global_values();
    me.suffer();
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_test_val" ) ) == Approx(
               1 ) );
    CHECK( globvars.get_global_value( "npctalk_var_context_test" ) == "process_mutation_two" );

    // test deactivate
    globvars.clear_global_values();
    me.deactivate_mutation( trait_process_mutation_two );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_test_val" ) ) == Approx(
               1 ) );
    CHECK( globvars.get_global_value( "npctalk_var_context_test" ) == "process_mutation_two" );

    // more complex test
    globvars.clear_global_values();
    me.toggle_trait( trait_process_mutation );
    effect_on_condition_EOC_activate_mutation_to_start_test->activate( d );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_test_val" ) ) == Approx(
               1 ) );
    CHECK( globvars.get_global_value( "npctalk_var_context_test" ) == "process_mutation" );
}

TEST_CASE( "EOC_monsters_nearby", "[eoc][math_parser]" )
{
    clear_avatar();
    clear_map();
    avatar &a = get_avatar();
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    g->place_critter_at( mon_zombie, a.pos() + tripoint_east );
    monster *friendo = g->place_critter_at( mon_zombie, a.pos() + tripoint{ 2, 0, 0 } );
    g->place_critter_at( mon_triffid, a.pos() + tripoint{ 3, 0, 0 } );
    g->place_critter_at( mon_zombie_tough, a.pos() + tripoint_north );
    g->place_critter_at( mon_zombie_tough, a.pos() + tripoint{ 0, 2, 0 } );
    g->place_critter_at( mon_zombie_tough, a.pos() + tripoint{ 0, 3, 0 } );
    g->place_critter_at( mon_zombie_smoker, a.pos() + tripoint{ 10, 0, 0 } );
    g->place_critter_at( mon_zombie_smoker, a.pos() + tripoint{ 11, 0, 0 } );

    REQUIRE( globvars.get_global_value( "npctalk_var_mons" ).empty() );
    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    REQUIRE( effect_on_condition_EOC_mon_nearby_test->activate( d ) );

    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_mons" ) ) == 8 );
    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_triffs" ) ) == 1 );
    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_group" ) ) == 4 );
    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_zombs" ) ) == 2 );
    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_zombs_friends" ) ) == 0 );
    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_zombs_both" ) ) == 2 );
    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_zplust" ) ) == 5 );
    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_zplust_adj" ) ) == 2 );
    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_smoks" ) ) == 1 );

    friendo->make_friendly();
    REQUIRE( effect_on_condition_EOC_mon_nearby_test->activate( d ) );
    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_zombs" ) ) == 1 );
    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_zombs_friends" ) ) == 1 );
    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_zombs_both" ) ) == 2 );
}

TEST_CASE( "EOC_activity_ongoing", "[eoc][timed_event]" )
{
    clear_avatar();
    clear_map();
    get_avatar().assign_activity( ACT_ADD_VARIABLE_DURING, 300 );

    complete_activity( get_avatar() );

    // been going for 3 whole seconds should have incremented 3 times
    CHECK( stoi( get_avatar().get_value( "npctalk_var_activitiy_incrementer" ) ) == 3 );
}

TEST_CASE( "EOC_stored_condition_test", "[eoc]" )
{
    clear_avatar();
    clear_map();

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_key1" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key2" ).empty() );

    d.set_value( "npctalk_var_context", "0" );

    // running with a value of 0 will have the conditional evaluate to 0
    CHECK( effect_on_condition_EOC_stored_condition_test->activate( d ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_key1" ) ) == Approx( 0 ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_key2" ) ) == Approx( 0 ) );
    CHECK( std::stod( d.get_value( "npctalk_var_context" ) ) == Approx( 0 ) );

    // try again with a different value
    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_key1" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key2" ).empty() );

    d.set_value( "npctalk_var_context", "10" );

    // running with a value greater than 1 will have the conditional evaluate to 1
    CHECK( effect_on_condition_EOC_stored_condition_test->activate( d ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_key1" ) ) == Approx( 1 ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_key2" ) ) == Approx( 1 ) );
    CHECK( std::stod( d.get_value( "npctalk_var_context" ) ) == Approx( 10 ) );

}

TEST_CASE( "dialogue_copy", "[eoc]" )
{
    standard_npc dude;
    dialogue d( get_talker_for( get_avatar() ), get_talker_for( &dude ) );
    dialogue d_copy( d );
    d_copy.set_value( "suppress", "1" );
    CHECK( d_copy.actor( false )->get_character() != nullptr );
    CHECK( d_copy.actor( true )->get_character() != nullptr );

    item hammer( "hammer" ) ;
    item_location hloc( map_cursor( tripoint_zero ), &hammer );
    computer comp( "test_computer", 0, tripoint_zero );
    dialogue d2( get_talker_for( hloc ), get_talker_for( comp ) );
    dialogue d2_copy( d2 );
    d2_copy.set_value( "suppress", "1" );
    CHECK( d2_copy.actor( false )->get_item() != nullptr );
    CHECK( d2_copy.actor( true )->get_computer() != nullptr );

    monster zombie( mon_zombie );
    dialogue d3( get_talker_for( zombie ), std::make_unique<talker>() );
    dialogue d3_copy( d3 );
    d3_copy.set_value( "suppress", "1" );
    CHECK( d3_copy.actor( false )->get_monster() != nullptr );
    CHECK( d3_copy.actor( true )->get_character() == nullptr );
}

TEST_CASE( "EOC_meta_test", "[eoc]" )
{
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    standard_npc dude;
    monster zombie( mon_zombie );
    item hammer( "hammer" ) ;
    item_location hloc( map_cursor( tripoint_zero ), &hammer );
    computer comp( "test_computer", 0, tripoint_zero );

    dialogue d_empty( std::make_unique<talker>(), std::make_unique<talker>() );
    dialogue d_avatar( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    dialogue d_npc( get_talker_for( dude ), std::make_unique<talker>() );
    dialogue d_monster( get_talker_for( zombie ), std::make_unique<talker>() );
    dialogue d_item( get_talker_for( hloc ), std::make_unique<talker>() );
    dialogue d_furniture( get_talker_for( comp ), std::make_unique<talker>() );

    CHECK( effect_on_condition_EOC_meta_test_message->activate( d_empty ) );

    std::vector<std::pair<std::string, std::string>> messages = Messages::recent_messages( 0 );

    REQUIRE( !messages.empty() );
    CHECK( messages.back().second == "message ok." );

    globvars.clear_global_values();

    CHECK( effect_on_condition_EOC_meta_test_talker_type->activate( d_avatar ) );
    CHECK( globvars.get_global_value( "npctalk_var_key_avatar" ) == "yes" );
    CHECK( globvars.get_global_value( "npctalk_var_key_npc" ).empty() );
    CHECK( globvars.get_global_value( "npctalk_var_key_character" ) == "yes" );
    CHECK( globvars.get_global_value( "npctalk_var_key_monster" ).empty() );
    CHECK( globvars.get_global_value( "npctalk_var_key_item" ).empty() );
    CHECK( globvars.get_global_value( "npctalk_var_key_furniture" ).empty() );

    globvars.clear_global_values();

    CHECK( effect_on_condition_EOC_meta_test_talker_type->activate( d_npc ) );
    CHECK( globvars.get_global_value( "npctalk_var_key_avatar" ).empty() );
    CHECK( globvars.get_global_value( "npctalk_var_key_npc" ) == "yes" );
    CHECK( globvars.get_global_value( "npctalk_var_key_character" ) == "yes" );
    CHECK( globvars.get_global_value( "npctalk_var_key_monster" ).empty() );
    CHECK( globvars.get_global_value( "npctalk_var_key_item" ).empty() );
    CHECK( globvars.get_global_value( "npctalk_var_key_furniture" ).empty() );

    globvars.clear_global_values();

    CHECK( effect_on_condition_EOC_meta_test_talker_type->activate( d_monster ) );
    CHECK( globvars.get_global_value( "npctalk_var_key_avatar" ).empty() );
    CHECK( globvars.get_global_value( "npctalk_var_key_npc" ).empty() );
    CHECK( globvars.get_global_value( "npctalk_var_key_character" ).empty() );
    CHECK( globvars.get_global_value( "npctalk_var_key_monster" ) == "yes" );
    CHECK( globvars.get_global_value( "npctalk_var_key_item" ).empty() );
    CHECK( globvars.get_global_value( "npctalk_var_key_furniture" ).empty() );

    globvars.clear_global_values();

    CHECK( effect_on_condition_EOC_meta_test_talker_type->activate( d_item ) );
    CHECK( globvars.get_global_value( "npctalk_var_key_avatar" ).empty() );
    CHECK( globvars.get_global_value( "npctalk_var_key_npc" ).empty() );
    CHECK( globvars.get_global_value( "npctalk_var_key_character" ).empty() );
    CHECK( globvars.get_global_value( "npctalk_var_key_monster" ).empty() );
    CHECK( globvars.get_global_value( "npctalk_var_key_item" ) == "yes" );
    CHECK( globvars.get_global_value( "npctalk_var_key_furniture" ).empty() );

    globvars.clear_global_values();

    CHECK( effect_on_condition_EOC_meta_test_talker_type->activate( d_furniture ) );
    CHECK( globvars.get_global_value( "npctalk_var_key_avatar" ).empty() );
    CHECK( globvars.get_global_value( "npctalk_var_key_npc" ).empty() );
    CHECK( globvars.get_global_value( "npctalk_var_key_character" ).empty() );
    CHECK( globvars.get_global_value( "npctalk_var_key_monster" ).empty() );
    CHECK( globvars.get_global_value( "npctalk_var_key_item" ).empty() );
    CHECK( globvars.get_global_value( "npctalk_var_key_furniture" ) == "yes" );
}

TEST_CASE( "EOC_increment_var_var", "[eoc]" )
{
    clear_avatar();
    clear_map();

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_key1" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key2" ).empty() );

    CHECK( effect_on_condition_EOC_increment_var_var->activate( d ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_key1" ) ) == Approx( 5 ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_key2" ) ) == Approx( 10 ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_global_u" ) ) == Approx( 6 ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_global_context" ) ) == Approx( 4 ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_global_nested" ) ) == Approx( 2 ) );
}

TEST_CASE( "EOC_string_var_var", "[eoc]" )
{
    clear_avatar();
    clear_map();
    standard_npc dude;
    dialogue d( get_talker_for( get_avatar() ), get_talker_for( &dude ) );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_key1" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key2" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key3" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key4" ).empty() );

    CHECK( effect_on_condition_EOC_string_var_var->activate( d ) );
    CHECK( globvars.get_global_value( "npctalk_var_key1" ) == "Works_global" );
    CHECK( globvars.get_global_value( "npctalk_var_key2" ) == "Works_context" );
    CHECK( globvars.get_global_value( "npctalk_var_key3" ) == "Works_u" );
    CHECK( globvars.get_global_value( "npctalk_var_key4" ) == "Works_npc" );
}

TEST_CASE( "EOC_run_with_test", "[eoc]" )
{
    clear_avatar();
    clear_map();

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_key1" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key2" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key3" ).empty() );

    CHECK( effect_on_condition_EOC_run_with_test->activate( d ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_key1" ) ) == Approx( 1 ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_key2" ) ) == Approx( 2 ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_key3" ) ) == Approx( 3 ) );

    // value shouldn't exist in the original dialogue
    CHECK( d.get_value( "npctalk_var_key" ).empty() );
    CHECK( d.get_value( "npctalk_var_key2" ).empty() );
    CHECK( d.get_value( "npctalk_var_key3" ).empty() );
}

TEST_CASE( "EOC_run_until_test", "[eoc]" )
{
    clear_avatar();
    clear_map();

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_key1" ).empty() );

    CHECK( effect_on_condition_EOC_run_until_test->activate( d ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_key1" ) ) == Approx( 10000 ) );
}

TEST_CASE( "EOC_run_with_test_expects", "[eoc]" )
{
    clear_avatar();
    clear_map();

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_key1" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key2" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key3" ).empty() );

    CHECK( capture_debugmsg_during( [&]() {
        effect_on_condition_EOC_run_with_test_expects_fail->activate( d );
    } ) == "Missing required variables: key1, key2, key3, " );
    CHECK( globvars.get_global_value( "npctalk_var_key1" ).empty() );
    CHECK( globvars.get_global_value( "npctalk_var_key2" ).empty() );
    CHECK( globvars.get_global_value( "npctalk_var_key3" ).empty() );

    globvars.clear_global_values();

    effect_on_condition_EOC_run_with_test_expects_pass->activate( d );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_key1" ) ) == Approx( 1 ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_key2" ) ) == Approx( 2 ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_key3" ) ) == Approx( 3 ) );
}

TEST_CASE( "EOC_run_with_test_queue", "[eoc]" )
{
    clear_avatar();
    clear_map();

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_key1" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key2" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key3" ).empty() );

    CHECK( effect_on_condition_EOC_run_with_test_queued->activate( d ) );

    set_time( calendar::turn + 2_seconds );
    effect_on_conditions::process_effect_on_conditions( get_avatar() );

    CHECK( std::stod( globvars.get_global_value( "npctalk_var_key1" ) ) == Approx( 1 ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_key2" ) ) == Approx( 2 ) );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_key3" ) ) == Approx( 3 ) );

    // value shouldn't exist in the original dialogue
    CHECK( d.get_value( "npctalk_var_key" ).empty() );
    CHECK( d.get_value( "npctalk_var_key2" ).empty() );
    CHECK( d.get_value( "npctalk_var_key3" ).empty() );
}

TEST_CASE( "EOC_run_inv_test", "[eoc]" )
{
    clear_avatar();
    clear_map();

    tripoint_abs_ms pos_before = get_avatar().get_location();
    tripoint_abs_ms pos_after = pos_before + tripoint_south_east;

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );

    effect_on_condition_EOC_run_inv_prepare->activate( d );
    std::vector<item *> items_before = get_avatar().items_with( []( const item & it ) {
        return it.get_var( "npctalk_var_general_run_inv_test_key1" ).empty();
    } );

    REQUIRE( items_before.size() == 4 );

    // All items
    CHECK( effect_on_condition_EOC_run_inv_test1->activate( d ) );

    std::vector<item *> items_after = get_avatar().items_with( []( const item & it ) {
        return it.get_var( "npctalk_var_general_run_inv_test_key1" ) == "yes";
    } );

    CHECK( items_after.size() == 4 );

    // Test search_data: id, worn_only
    CHECK( effect_on_condition_EOC_run_inv_test2->activate( d ) );

    items_after = get_avatar().items_with( []( const item & it ) {
        return it.get_var( "npctalk_var_general_run_inv_test_key2" ) == "yes";
    } );

    CHECK( items_after.size() == 1 );

    // Test search_data: material, wielded_only
    CHECK( effect_on_condition_EOC_run_inv_test3->activate( d ) );

    items_after = get_avatar().items_with( []( const item & it ) {
        return it.get_var( "npctalk_var_general_run_inv_test_key3" ) == "yes";
    } );

    CHECK( items_after.empty() );

    get_avatar().get_wielded_item().remove_item();
    item weapon_wood( itype_sword_wood );
    get_avatar().set_wielded_item( weapon_wood );

    CHECK( effect_on_condition_EOC_run_inv_test3->activate( d ) );

    items_after = get_avatar().items_with( []( const item & it ) {
        return it.get_var( "npctalk_var_general_run_inv_test_key3" ) == "yes";
    } );

    CHECK( items_after.size() == 1 );

    // Test search_data: category, flags
    CHECK( effect_on_condition_EOC_run_inv_test4->activate( d ) );

    items_after = get_avatar().items_with( []( const item & it ) {
        return it.get_var( "npctalk_var_general_run_inv_test_key4" ) == "yes";
    } );

    CHECK( items_after.size() == 1 );

    // Test search_data: excluded flags
    CHECK( effect_on_condition_EOC_run_inv_test5->activate( d ) );

    items_after = get_avatar().items_with( []( const item & it ) {
        return it.get_var( "npctalk_var_general_run_inv_test_key5" ) == "yes";
    } );

    CHECK( items_after.size() == 3 );

    // Flag test for item
    CHECK( effect_on_condition_EOC_item_flag_test->activate( d ) );

    items_after = get_avatar().items_with( []( const item & it ) {
        return it.get_var( "npctalk_var_general_run_inv_test_is_filthy" ) == "yes";
    } );

    CHECK( items_after.size() == 1 );

    items_after = get_avatar().items_with( []( const item & it ) {
        return it.has_flag( json_flag_FILTHY );
    } );

    CHECK( items_after.empty() );

    // Math function test for item
    items_before = get_avatar().items_with( []( const item & it ) {
        return it.damage() == 0;
    } );

    REQUIRE( items_before.size() == 4 );
    CHECK( effect_on_condition_EOC_item_math_test->activate( d ) );

    items_after = get_avatar().items_with( []( const item & it ) {
        return it.damage() == it.max_damage() - 100;
    } );

    CHECK( items_after.size() == 4 );

    // Activate test for item
    CHECK( effect_on_condition_EOC_item_activate_test->activate( d ) );
    CHECK( get_map().furn( get_map().getlocal( pos_after ) ) == furn_f_cardboard_box );

    // Teleport test for item
    CHECK( effect_on_condition_EOC_item_teleport_test->activate( d ) );
    CHECK( get_map().i_at( get_map().getlocal( pos_after ) ).size() == 3 );

    // Math function test for armor
    CHECK( effect_on_condition_EOC_armor_math_test->activate( d ) );

    const item &check_item = get_avatar().worn.i_at( 0 );

    REQUIRE( check_item.typeId() == itype_backpack );
    CHECK( std::stod( get_avatar().get_value( "npctalk_var_key1" ) ) == Approx( 27 ) );
    CHECK( std::stod( get_avatar().get_value( "npctalk_var_key2" ) ) == Approx( 2 ) );
    CHECK( std::stod( check_item.get_var( "npctalk_var_key1" ) ) == Approx( 27 ) );
    CHECK( std::stod( check_item.get_var( "npctalk_var_key2" ) ) == Approx( 2 ) );
}

TEST_CASE( "math_weapon_damage", "[eoc]" )
{
    clear_avatar();
    item myweapon( GENERATE( true, false ) ? itype_test_knife_combat : itype_test_glock );
    get_avatar().set_wielded_item( myweapon );

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE( effect_on_condition_EOC_test_weapon_damage->activate( d ) );

    int total_damage{};
    for( damage_type const &dt : damage_type::get_all() ) {
        total_damage += myweapon.damage_melee( dt.id );
    }
    int const bash_damage = myweapon.damage_melee( STATIC( damage_type_id( "bash" ) ) );
    int const gun_damage = myweapon.gun_damage().total_damage();
    int const bullet_damage = myweapon.gun_damage().type_damage( STATIC( damage_type_id( "bullet" ) ) );

    CAPTURE( myweapon.typeId().c_str() );
    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_mymelee" ) ) ==  total_damage );
    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_mymelee_bash" ) ) == bash_damage );
    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_mygun" ) ) == gun_damage );
    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_mygun_bullet" ) ) == bullet_damage );
}

TEST_CASE( "EOC_event_test", "[eoc]" )
{
    clear_avatar();
    clear_map();

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_key1" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key2" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key3" ).empty() );

    // character_casts_spell
    spell temp_spell( spell_test_eoc_spell );
    temp_spell.set_level( get_avatar(), 5 );
    temp_spell.cast_all_effects( get_avatar(), tripoint() );

    CHECK( globvars.get_global_value( "npctalk_var_key1" ) == "test_eoc_spell" );
    CHECK( globvars.get_global_value( "npctalk_var_key2" ) == "test_trait" );
    CHECK( globvars.get_global_value( "npctalk_var_key3" ) == "5" );
    CHECK( globvars.get_global_value( "npctalk_var_key4" ) == "150" );
    CHECK( globvars.get_global_value( "npctalk_var_key5" ) == "100" );
    CHECK( globvars.get_global_value( "npctalk_var_key6" ) == "45" );

    // character_starts_activity
    globvars.clear_global_values();
    get_avatar().assign_activity( ACT_GENERIC_EOC, 1 );

    CHECK( globvars.get_global_value( "npctalk_var_key1" ) == "ACT_GENERIC_EOC" );
    CHECK( globvars.get_global_value( "npctalk_var_key2" ) == "0" );
    CHECK( globvars.get_global_value( "npctalk_var_key3" ) == "activity start" );

    // character_finished_activity
    get_avatar().cancel_activity();

    CHECK( globvars.get_global_value( "npctalk_var_key1" ) == "ACT_GENERIC_EOC" );
    CHECK( globvars.get_global_value( "npctalk_var_key2" ) == "1" );
    CHECK( globvars.get_global_value( "npctalk_var_key3" ) == "activity finished" );

    // character_wields_item
    item weapon_item( itype_test_knife_combat );
    get_avatar().wield( weapon_item );
    item_location weapon = get_avatar().get_wielded_item();

    CHECK( get_avatar().get_value( "npctalk_var_test_event_last_event" ) == "character_wields_item" );
    CHECK( weapon->get_var( "npctalk_var_test_event_last_event" ) == "character_wields_item" );

    // character_wears_item
    std::list<item>::iterator armor = *get_avatar().worn.wear_item( get_avatar(),
                                      item( itype_backpack ), false, true, true );

    CHECK( get_avatar().get_value( "npctalk_var_test_event_last_event" ) == "character_wears_item" );
    CHECK( armor->get_var( "npctalk_var_test_event_last_event" ) == "character_wears_item" );
}

TEST_CASE( "EOC_combat_event_test", "[eoc]" )
{
    size_t loop;
    global_variables &globvars = get_globals();
    globvars.clear_global_values();
    clear_avatar();
    clear_npcs();
    clear_map();

    // character_melee_attacks_character
    npc &npc_dst_melee = spawn_npc( get_avatar().pos().xy() + point_south, "thug" );
    item weapon_item( itype_test_knife_combat );
    get_avatar().wield( weapon_item );
    get_avatar().melee_attack( npc_dst_melee, false );

    CHECK( get_avatar().get_value( "npctalk_var_test_event_last_event" ) ==
           "character_melee_attacks_character" );
    CHECK( npc_dst_melee.get_value( "npctalk_var_test_event_last_event" ) ==
           "character_melee_attacks_character" );
    CHECK( globvars.get_global_value( "npctalk_var_weapon" ) == "test_knife_combat" );
    CHECK( globvars.get_global_value( "npctalk_var_victim_name" ) == npc_dst_melee.name );

    // character_melee_attacks_monster
    clear_map();
    monster &mon_dst_melee = spawn_test_monster( "mon_zombie", get_avatar().pos() + tripoint_east );
    get_avatar().melee_attack( mon_dst_melee, false );

    CHECK( get_avatar().get_value( "npctalk_var_test_event_last_event" ) ==
           "character_melee_attacks_monster" );
    CHECK( mon_dst_melee.get_value( "npctalk_var_test_event_last_event" ) ==
           "character_melee_attacks_monster" );
    CHECK( globvars.get_global_value( "npctalk_var_weapon" ) == "test_knife_combat" );
    CHECK( globvars.get_global_value( "npctalk_var_victim_type" ) == "mon_zombie" );

    // character_ranged_attacks_character
    const tripoint target_pos = get_avatar().pos() + point_east;
    clear_map();
    npc &npc_dst_ranged = spawn_npc( target_pos.xy(), "thug" );
    for( loop = 0; loop < 1000; loop++ ) {
        get_avatar().set_body();
        arm_shooter( get_avatar(), "shotgun_s" );
        get_avatar().recoil = 0;
        get_avatar().fire_gun( target_pos, 1, *get_avatar().get_wielded_item() );
        if( !npc_dst_ranged.get_value( "npctalk_var_test_event_last_event" ).empty() ) {
            break;
        }
    }

    CHECK( get_avatar().get_value( "npctalk_var_test_event_last_event" ) ==
           "character_ranged_attacks_character" );
    CHECK( npc_dst_ranged.get_value( "npctalk_var_test_event_last_event" ) ==
           "character_ranged_attacks_character" );
    CHECK( globvars.get_global_value( "npctalk_var_weapon" ) == "shotgun_s" );
    CHECK( globvars.get_global_value( "npctalk_var_victim_name" ) == npc_dst_ranged.name );

    // character_ranged_attacks_monster
    clear_map();
    monster &mon_dst_ranged = spawn_test_monster( "mon_zombie", target_pos );
    for( loop = 0; loop < 1000; loop++ ) {
        get_avatar().set_body();
        arm_shooter( get_avatar(), "shotgun_s" );
        get_avatar().recoil = 0;
        get_avatar().fire_gun( mon_dst_ranged.pos(), 1, *get_avatar().get_wielded_item() );
        if( !mon_dst_ranged.get_value( "npctalk_var_test_event_last_event" ).empty() ) {
            break;
        }
    }

    CHECK( get_avatar().get_value( "npctalk_var_test_event_last_event" ) ==
           "character_ranged_attacks_monster" );
    CHECK( mon_dst_ranged.get_value( "npctalk_var_test_event_last_event" ) ==
           "character_ranged_attacks_monster" );
    CHECK( globvars.get_global_value( "npctalk_var_weapon" ) == "shotgun_s" );
    CHECK( globvars.get_global_value( "npctalk_var_victim_type" ) == "mon_zombie" );

    // character_kills_monster
    clear_map();
    monster &victim = spawn_test_monster( "mon_zombie", target_pos );
    victim.die( &get_avatar() );

    CHECK( get_avatar().get_value( "npctalk_var_test_event_last_event" ) == "character_kills_monster" );
    CHECK( globvars.get_global_value( "npctalk_var_victim_type" ) == "mon_zombie" );
    CHECK( globvars.get_global_value( "npctalk_var_test_exp" ) == "4" );
}

TEST_CASE( "EOC_spell_exp", "[eoc]" )
{
    clear_avatar();
    clear_map();

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_key1" ).empty() );

    get_avatar().magic->learn_spell( "test_eoc_spell", get_avatar(), true );

    CHECK( effect_on_condition_EOC_math_spell_xp->activate( d ) );

    CHECK( globvars.get_global_value( "npctalk_var_key1" ) == "1000" );
}

TEST_CASE( "EOC_recipe_test", "[eoc]" )
{
    clear_avatar();
    const recipe *r = &recipe_cattail_jelly.obj();
    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE_FALSE( get_avatar().knows_recipe( r ) );
    REQUIRE( globvars.get_global_value( "fail_var" ).empty() );

    CHECK( effect_on_condition_EOC_recipe_test_1->activate( d ) );
    CHECK( globvars.get_global_value( "fail_var" ).empty() );
    CHECK( get_avatar().knows_recipe( r ) );

    CHECK( effect_on_condition_EOC_recipe_test_2->activate( d ) );
    CHECK( globvars.get_global_value( "fail_var" ).empty() );
    CHECK_FALSE( get_avatar().knows_recipe( r ) );
}

TEST_CASE( "EOC_map_test", "[eoc]" )
{
    global_variables &globvars = get_globals();
    globvars.clear_global_values();
    clear_avatar();
    clear_map();

    map &m = get_map();
    const tripoint_abs_ms start = get_avatar().get_location();
    const tripoint tgt = m.getlocal( start + tripoint_north );
    m.furn_set( tgt, furn_test_f_eoc );
    m.furn( tgt )->examine( get_avatar(), tgt );

    CHECK( globvars.get_global_value( "npctalk_var_this" ) == "test_f_eoc" );
    CHECK( globvars.get_global_value( "npctalk_var_pos" ) == m.getglobal( tgt ).to_string() );

    const tripoint target_pos = get_avatar().pos() + point_east * 10;
    npc &npc_dst = spawn_npc( target_pos.xy(), "thug" );
    dialogue d( get_talker_for( get_avatar() ), get_talker_for( npc_dst ) );

    CHECK( effect_on_condition_EOC_map_test->activate( d ) );
    CHECK( globvars.get_global_value( "npctalk_var_key_distance_loc" ) == "14" );
    CHECK( globvars.get_global_value( "npctalk_var_key_distance_npc" ) == "10" );
}

TEST_CASE( "EOC_martial_art_test", "[eoc]" )
{
    global_variables &globvars = get_globals();
    globvars.clear_global_values();
    clear_avatar();
    clear_map();

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );

    REQUIRE_FALSE( get_avatar().has_martialart( style_aikido ) );
    REQUIRE( globvars.get_global_value( "fail_var" ).empty() );

    CHECK( effect_on_condition_EOC_martial_art_test_1->activate( d ) );
    CHECK( globvars.get_global_value( "fail_var" ).empty() );
    CHECK( get_avatar().has_martialart( style_aikido ) );

    get_avatar().martial_arts_data->set_style( style_aikido );

    CHECK_FALSE( effect_on_condition_EOC_martial_art_test_2->activate( d ) );
    CHECK( get_avatar().has_martialart( style_aikido ) );

    get_avatar().martial_arts_data->set_style( style_none );

    CHECK( effect_on_condition_EOC_martial_art_test_2->activate( d ) );
    CHECK( !get_avatar().has_martialart( style_aikido ) );
}

TEST_CASE( "EOC_string_test", "[eoc]" )
{
    clear_avatar();
    clear_map();

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_key3" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key4" ).empty() );

    CHECK( effect_on_condition_EOC_string_test->activate( d ) );
    CHECK( globvars.get_global_value( "npctalk_var_key3" ) == "<global_val:key1> <global_val:key2>" );
    CHECK( globvars.get_global_value( "npctalk_var_key4" ) == "test1 test2" );

    CHECK( effect_on_condition_EOC_string_test_nest->activate( d ) );
    CHECK( get_avatar().get_value( "npctalk_var_key1" ) == "nest2" );
    CHECK( get_avatar().get_value( "npctalk_var_key2" ) == "nest3" );
    CHECK( get_avatar().get_value( "npctalk_var_key3" ) == "nest4" );
}
