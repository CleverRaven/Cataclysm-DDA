#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "effect_on_condition.h"
#include "game.h"
#include "map_helpers.h"
#include "timed_event.h"
#include "player_helpers.h"
#include "point.h"


static const activity_id ACT_ADD_VARIABLE_COMPLETE( "ACT_ADD_VARIABLE_COMPLETE" );
static const activity_id ACT_ADD_VARIABLE_DURING( "ACT_ADD_VARIABLE_DURING" );

static const effect_on_condition_id
effect_on_condition_EOC_TEST_TRANSFORM_LINE( "EOC_TEST_TRANSFORM_LINE" );
static const effect_on_condition_id
effect_on_condition_EOC_TEST_TRANSFORM_RADIUS( "EOC_TEST_TRANSFORM_RADIUS" );
static const effect_on_condition_id
effect_on_condition_EOC_math_diag_assign( "EOC_math_diag_assign" );
static const effect_on_condition_id effect_on_condition_EOC_math_duration( "EOC_math_duration" );
static const effect_on_condition_id
effect_on_condition_EOC_math_switch_math( "EOC_math_switch_math" );
static const effect_on_condition_id
effect_on_condition_EOC_math_test_equals_assign( "EOC_math_test_equals_assign" );
static const effect_on_condition_id
effect_on_condition_EOC_math_test_greater_increment( "EOC_math_test_greater_increment" );
static const effect_on_condition_id
effect_on_condition_EOC_math_var( "EOC_math_var" );
static const effect_on_condition_id
effect_on_condition_EOC_math_weighted_list( "EOC_math_weighted_list" );
static const effect_on_condition_id effect_on_condition_EOC_teleport_test( "EOC_teleport_test" );
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
    get_avatar().set_stamina( 500 );
    CHECK( effect_on_condition_EOC_math_test_equals_assign->test_condition( d ) );
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

TEST_CASE( "EOC_activity_ongoing", "[eoc][timed_event]" )
{
    clear_avatar();
    clear_map();
    get_avatar().assign_activity( ACT_ADD_VARIABLE_DURING, 300 );

    complete_activity( get_avatar() );

    // been going for 3 whole seconds should have incremented 3 times
    CHECK( stoi( get_avatar().get_value( "npctalk_var_activitiy_incrementer" ) ) == 3 );
}
