#include "avatar.h"
#include "cata_catch.h"
#include "effect_on_condition.h"
#include "game.h"
#include "map_helpers.h"
#include "timed_event.h"
#include "player_helpers.h"
#include "point.h"

static const effect_on_condition_id
effect_on_condition_EOC_TEST_TRANSFORM_LINE( "EOC_TEST_TRANSFORM_LINE" );
static const effect_on_condition_id
effect_on_condition_EOC_TEST_TRANSFORM_RADIUS( "EOC_TEST_TRANSFORM_RADIUS" );
static const effect_on_condition_id effect_on_condition_EOC_teleport_test( "EOC_teleport_test" );
namespace
{
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

TEST_CASE( "EOC_transform_radius", "[eoc][timed_event]" )
{
    // FIXME: remove once the overlap warning in `map::load()` has been fully solved
    restore_on_out_of_scope<bool> restore_test_mode( test_mode );
    test_mode = false;

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
    // FIXME: remove once the overlap warning in `map::load()` has been fully solved
    restore_on_out_of_scope<bool> restore_test_mode( test_mode );
    test_mode = false;

    clear_avatar();
    clear_map();
    standard_npc npc( "Mr. Testerman" );
    cata::optional<tripoint> const dest = random_point( get_map(), []( tripoint const & p ) {
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
