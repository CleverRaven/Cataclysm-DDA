#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "effect_on_condition.h"
#include "game.h"
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
static const effect_on_condition_id effect_on_condition_EOC_attack_test( "EOC_attack_test" );
static const effect_on_condition_id
effect_on_condition_EOC_combat_mutator_test( "EOC_combat_mutator_test" );
static const effect_on_condition_id
effect_on_condition_EOC_increment_var_var( "EOC_increment_var_var" );
static const effect_on_condition_id
effect_on_condition_EOC_jmath_test( "EOC_jmath_test" );
static const effect_on_condition_id
effect_on_condition_EOC_math_armor( "EOC_math_armor" );
static const effect_on_condition_id
effect_on_condition_EOC_math_diag_assign( "EOC_math_diag_assign" );
static const effect_on_condition_id
effect_on_condition_EOC_math_diag_w_vars( "EOC_math_diag_w_vars" );
static const effect_on_condition_id effect_on_condition_EOC_math_duration( "EOC_math_duration" );
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
effect_on_condition_EOC_math_var( "EOC_math_var" );
static const effect_on_condition_id
effect_on_condition_EOC_math_weighted_list( "EOC_math_weighted_list" );
static const effect_on_condition_id
effect_on_condition_EOC_mon_nearby_test( "EOC_mon_nearby_test" );
static const effect_on_condition_id effect_on_condition_EOC_mutator_test( "EOC_mutator_test" );
static const effect_on_condition_id effect_on_condition_EOC_options_tests( "EOC_options_tests" );
static const effect_on_condition_id effect_on_condition_EOC_recipe_test_1( "EOC_recipe_test_1" );
static const effect_on_condition_id effect_on_condition_EOC_recipe_test_2( "EOC_recipe_test_2" );
static const effect_on_condition_id effect_on_condition_EOC_run_inv_test1( "EOC_run_inv_test1" );
static const effect_on_condition_id effect_on_condition_EOC_run_inv_test2( "EOC_run_inv_test2" );
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
effect_on_condition_EOC_string_var_var( "EOC_string_var_var" );
static const effect_on_condition_id effect_on_condition_EOC_teleport_test( "EOC_teleport_test" );
static const effect_on_condition_id effect_on_condition_EOC_try_kill( "EOC_try_kill" );

static const itype_id itype_backpack( "backpack" );
static const itype_id itype_test_knife_combat( "test_knife_combat" );

static const mtype_id mon_zombie( "mon_zombie" );
static const mtype_id mon_zombie_smoker( "mon_zombie_smoker" );
static const mtype_id mon_zombie_tough( "mon_zombie_tough" );

static const recipe_id recipe_cattail_jelly( "cattail_jelly" );

static const skill_id skill_survival( "survival" );

static const spell_id spell_test_eoc_spell( "test_eoc_spell" );

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
    CHECK( effect_on_condition_EOC_math_var->test_condition( d ) );
    calendar::turn = calendar::start_of_cataclysm;

    CHECK_FALSE( effect_on_condition_EOC_math_test_greater_increment->test_condition( d ) );
    effect_on_condition_EOC_math_test_greater_increment->activate( d );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_math_test" ) ) == Approx( -1 ) );
    effect_on_condition_EOC_math_switch_math->activate( d );
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_math_test_result" ) ) == Approx( 1 ) );
    CHECK( effect_on_condition_EOC_math_duration->recurrence.evaluate( d ) == 1_turns );
    CHECK_FALSE( effect_on_condition_EOC_math_var->test_condition( d ) );
    calendar::turn += 1_days;
    CHECK( effect_on_condition_EOC_math_var->test_condition( d ) );

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
    check_ter_in_radius( start, eoc_range, t_grass );
    effect_on_condition_EOC_TEST_TRANSFORM_RADIUS->activate( newDialog );
    check_ter_in_radius( start, eoc_range, t_dirt );

    g->place_player_overmap( project_to<coords::omt>( start ) + point{ 60, 60 } );
    REQUIRE( !get_map().inbounds( start ) );

    calendar::turn += delay - 1_seconds;
    get_timed_events().process();
    check_ter_in_radius( start, eoc_range, t_dirt );
    calendar::turn += 2_seconds;
    get_timed_events().process();
    check_ter_in_radius( start, eoc_range, t_grass );
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
    check_ter_in_line( start, end, t_grass );
    effect_on_condition_EOC_TEST_TRANSFORM_LINE->activate( newDialog );
    check_ter_in_line( start, end, t_dirt );
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
    g->place_critter_at( mon_zombie, a.pos() + tripoint{ 2, 0, 0 } );
    g->place_critter_at( mon_zombie_tough, a.pos() + tripoint_north );
    g->place_critter_at( mon_zombie_tough, a.pos() + tripoint{ 0, 2, 0 } );
    g->place_critter_at( mon_zombie_tough, a.pos() + tripoint{ 0, 3, 0 } );
    g->place_critter_at( mon_zombie_smoker, a.pos() + tripoint{ 10, 0, 0 } );
    g->place_critter_at( mon_zombie_smoker, a.pos() + tripoint{ 11, 0, 0 } );

    REQUIRE( globvars.get_global_value( "npctalk_var_mons" ).empty() );
    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    REQUIRE( effect_on_condition_EOC_mon_nearby_test->activate( d ) );

    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_mons" ) ) == 7 );
    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_zombs" ) ) == 2 );
    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_zplust" ) ) == 5 );
    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_zplust_adj" ) ) == 2 );
    CHECK( std::stoi( globvars.get_global_value( "npctalk_var_smoks" ) ) == 1 );
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

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
    global_variables &globvars = get_globals();
    globvars.clear_global_values();

    REQUIRE( globvars.get_global_value( "npctalk_var_key1" ).empty() );
    REQUIRE( globvars.get_global_value( "npctalk_var_key2" ).empty() );

    CHECK( effect_on_condition_EOC_string_var_var->activate( d ) );
    CHECK( globvars.get_global_value( "npctalk_var_key1" ) == "Works" );
    CHECK( globvars.get_global_value( "npctalk_var_key2" ) == "Works" );
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
    CHECK( std::stod( globvars.get_global_value( "npctalk_var_key1" ) ) == Approx( 10 ) );
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

    item weapon( itype_test_knife_combat );
    item backpack( itype_backpack );
    get_avatar().set_wielded_item( weapon );
    get_avatar().worn.wear_item( get_avatar(), backpack, false, false );
    get_avatar().i_add( item( itype_test_knife_combat ) );
    get_avatar().i_add( item( itype_backpack ) );

    dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );

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

    // Worn backpack only
    CHECK( effect_on_condition_EOC_run_inv_test2->activate( d ) );

    items_after = get_avatar().items_with( []( const item & it ) {
        return it.get_var( "npctalk_var_general_run_inv_test_key2" ) == "yes";
    } );

    CHECK( items_after.size() == 1 );
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

    CHECK( globvars.get_global_value( "npctalk_var_key1" ) == "45" );
    CHECK( globvars.get_global_value( "npctalk_var_key2" ) == "100" );
    CHECK( globvars.get_global_value( "npctalk_var_key3" ) == "5" );

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
