#include <array>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "map_scale_constants.h"
#include "monster.h"
#include "point.h"
#include "tileray.h"
#include "type_id.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

static const mtype_id pseudo_debug_mon( "pseudo_debug_mon" );

static void clear_game_and_set_ramp( const int transit_x, bool use_ramp, bool up )
{
    // Set to turn 0 to prevent solars from producing power
    calendar::turn = calendar::turn_zero;
    clear_map();
    clear_vehicles();

    Character &player_character = get_player_character();
    // Move player somewhere safe
    REQUIRE_FALSE( player_character.in_vehicle );

    map &here = get_map();
    build_test_map( ter_id( "t_pavement" ) );
    if( use_ramp ) {
        const int upper_zlevel = up ? 1 : 0;
        const int lower_zlevel = up - 1;
        const int highx = transit_x + ( up ? 0 : 1 );
        const int lowx = transit_x + ( up ? 1 : 0 );

        // up   z1    ......  rdh  rDl
        //      z0            rUh  rul .................
        // down z0            rDl  rdh .................
        //      z-1   ......  rdl  rUh
        //                    60   61
        for( int y = 0; y < SEEY * MAPSIZE; y++ ) {
            for( int x = 0; x < transit_x; x++ ) {
                const int mid = up ? upper_zlevel : lower_zlevel;
                here.ter_set( tripoint_bub_ms( x, y, mid - 2 ), ter_id( "t_rock" ) );
                here.ter_set( tripoint_bub_ms( x, y, mid - 1 ), ter_id( "t_rock" ) );
                here.ter_set( tripoint_bub_ms( x, y, mid ), ter_id( "t_pavement" ) );
                here.ter_set( tripoint_bub_ms( x, y, mid + 1 ), ter_id( "t_open_air" ) );
                here.ter_set( tripoint_bub_ms( x, y, mid + 2 ), ter_id( "t_open_air" ) );
            }
            const tripoint_bub_ms ramp_up_low = tripoint_bub_ms( lowx, y, lower_zlevel );
            const tripoint_bub_ms ramp_up_high = tripoint_bub_ms( highx, y, lower_zlevel );
            const tripoint_bub_ms ramp_down_low = tripoint_bub_ms( lowx, y, upper_zlevel );
            const tripoint_bub_ms ramp_down_high = tripoint_bub_ms( highx, y, upper_zlevel );
            here.ter_set( ramp_up_low, ter_id( "t_ramp_up_low" ) );
            here.ter_set( ramp_up_high, ter_id( "t_ramp_up_high" ) );
            here.ter_set( ramp_down_low, ter_id( "t_ramp_down_low" ) );
            here.ter_set( ramp_down_high, ter_id( "t_ramp_down_high" ) );
            for( int x = transit_x + 2; x < SEEX * MAPSIZE; x++ ) {
                here.ter_set( tripoint_bub_ms( x, y, 1 ), ter_id( "t_open_air" ) );
                here.ter_set( tripoint_bub_ms( x, y, 0 ), ter_id( "t_pavement" ) );
                here.ter_set( tripoint_bub_ms( x, y, -1 ), ter_id( "t_rock" ) );
            }
        }
    }
    here.invalidate_map_cache( 0 );
    here.build_map_cache( 0, true );
}

// Algorithm goes as follows:
// Clear map and create a ramp
// Spawn a vehicle
// Drive it over the ramp, and confirm that the vehicle changes z-levels
static void ramp_transition_angled( const vproto_id &veh_id, const units::angle angle,
                                    const int transition_x, bool use_ramp, bool up )
{
    map &here = get_map();
    clear_game_and_set_ramp( transition_x, use_ramp, up );

    const tripoint_bub_ms map_starting_point( transition_x + 4, 60, 0 );
    REQUIRE( here.ter( map_starting_point ) == ter_id( "t_pavement" ) );
    if( here.ter( map_starting_point ) != ter_id( "t_pavement" ) ) {
        return;
    }
    vehicle *veh_ptr = here.add_vehicle( veh_id, map_starting_point, angle, 1, 0 );

    REQUIRE( veh_ptr != nullptr );
    if( veh_ptr == nullptr ) {
        return;
    }

    vehicle &veh = *veh_ptr;
    veh.check_falling_or_floating();

    REQUIRE( !veh.is_in_water() );

    veh.tags.insert( "IN_CONTROL_OVERRIDE" );
    veh.engine_on = true;
    Character &player_character = get_player_character();
    player_character.setpos( here, map_starting_point );

    REQUIRE( player_character.pos_bub( here ) == map_starting_point );
    if( player_character.pos_bub( here ) != map_starting_point ) {
        return;
    }
    get_map().board_vehicle( map_starting_point, &player_character );
    REQUIRE( player_character.pos_bub( here ) == map_starting_point );
    if( player_character.pos_bub( here ) != map_starting_point ) {
        return;
    }
    const int transition_cycle = 3;
    veh.cruise_velocity = 0;
    veh.velocity = 0;
    here.vehmove();

    const int target_velocity = 400;
    veh.cruise_velocity = target_velocity;
    veh.velocity = target_velocity;
    CHECK( veh.safe_velocity( here ) > 0 );
    int cycles = 0;
    const int target_z = use_ramp ? ( up ? 1 : -1 ) : 0;

    std::set<tripoint_abs_ms> vpts = veh.get_points();
    while( veh.engine_on && veh.safe_velocity( here ) > 0 && cycles < 10 ) {
        clear_creatures();
        CAPTURE( cycles );
        for( const tripoint_abs_ms &checkpt : vpts ) {
            int partnum = 0;
            vehicle *check_veh = here.veh_at_internal( here.get_bub( checkpt ), partnum );
            CAPTURE( veh_ptr->pos_bub( here ) );
            CAPTURE( veh_ptr->face.dir() );
            CAPTURE( checkpt );
            CHECK( check_veh == veh_ptr );
        }
        vpts.clear();
        here.vehmove();
        CHECK( veh.velocity == target_velocity );
        // If the vehicle starts skidding, the effects become random and test is RUINED
        REQUIRE( !veh.skidding );
        for( const tripoint_abs_ms &pos : veh.get_points() ) {
            REQUIRE( here.ter( here.get_bub( pos ) ) );
        }
        for( const vpart_reference &vp : veh.get_all_parts() ) {
            if( vp.info().location != "structure" ) {
                continue;
            }
            const point_rel_ms &pmount = vp.mount_pos();
            CAPTURE( pmount );
            const tripoint_bub_ms &ppos = vp.pos_bub( here );
            CAPTURE( ppos );
            if( cycles > ( transition_cycle - pmount.x() ) ) {
                CHECK( ppos.z() == target_z );
            } else {
                CHECK( ppos.z() == 0 );
            }
            if( pmount.x() == 0 && pmount.y() == 0 ) {
                CHECK( player_character.pos_bub() == ppos );
            }
        }
        vpts = veh.get_points();
        cycles++;
    }
    const std::optional<vpart_reference> vp = here.veh_at(
                player_character.pos_bub() ).part_with_feature(
                VPFLAG_BOARDABLE, true );
    REQUIRE( vp );
    if( vp ) {
        const int z_change = map_starting_point.z() - player_character.posz();
        here.unboard_vehicle( *vp, &player_character, false );
        here.ter_set( map_starting_point, ter_id( "t_pavement" ) );
        player_character.setpos( here, map_starting_point );
        if( z_change ) {
            g->vertical_move( z_change, true );
        }
    }
}

static void test_ramp( const std::string &type, const int transition_x )
{
    CAPTURE( type );
    SECTION( "no ramp" ) {
        ramp_transition_angled( vproto_id( type ), 180_degrees, transition_x, false, false );
    }
    SECTION( "ramp up" ) {
        ramp_transition_angled( vproto_id( type ), 180_degrees, transition_x, true, true );
    }
    SECTION( "ramp down" ) {
        ramp_transition_angled( vproto_id( type ), 180_degrees, transition_x, true, false );
    }
    SECTION( "angled no ramp" ) {
        ramp_transition_angled( vproto_id( type ), 225_degrees, transition_x, false, false );
    }
    SECTION( "angled ramp down" ) {
        ramp_transition_angled( vproto_id( type ), 225_degrees, transition_x, true, false );
    }
    SECTION( "angled ramp up" ) {
        ramp_transition_angled( vproto_id( type ), 225_degrees, transition_x, true, true );
    }
}

static std::vector<std::string> ramp_vehs_to_test = {{
        "motorcycle",
    }
};

// I'd like to do this in a single loop, but that doesn't work for some reason
TEST_CASE( "vehicle_ramp_test_59", "[vehicle][ramp]" )
{
    for( const std::string &veh : ramp_vehs_to_test ) {
        test_ramp( veh, 59 );
    }
}
TEST_CASE( "vehicle_ramp_test_60", "[vehicle][ramp]" )
{
    for( const std::string &veh : ramp_vehs_to_test ) {
        test_ramp( veh, 60 );
    }
}
TEST_CASE( "vehicle_ramp_test_61", "[vehicle][ramp]" )
{
    for( const std::string &veh : ramp_vehs_to_test ) {
        test_ramp( veh, 61 );
    }
}

static void level_out( const vproto_id &veh_id, const bool drop_pos )
{
    map &here = get_map();
    clear_game_and_set_ramp( 75, drop_pos, false );
    const int start_z = drop_pos ? 1 : 0;

    const tripoint_bub_ms map_starting_point( 60, 60, start_z );

    // Make sure the avatar is out of the way
    Character &player_character = get_player_character();
    player_character.setpos( here, map_starting_point + point( 5, 5 ) );
    vehicle *veh_ptr = here.add_vehicle( veh_id, map_starting_point, 180_degrees, 1, 0 );

    REQUIRE( veh_ptr != nullptr );
    if( veh_ptr == nullptr ) {
        return;
    }
    vehicle &veh = *veh_ptr;
    veh.check_falling_or_floating();

    REQUIRE( !veh.is_in_water() );

    veh.tags.insert( "IN_CONTROL_OVERRIDE" );
    veh.engine_on = true;

    const int target_velocity = 800;
    veh.cruise_velocity = target_velocity;
    veh.velocity = target_velocity;
    CHECK( veh.safe_velocity( here ) > 0 );

    std::vector<vehicle_part *> all_parts;
    for( const tripoint_abs_ms &pos : veh.get_points() ) {
        for( vehicle_part *prt : veh.get_parts_at( pos, "", part_status_flag::any ) ) {
            all_parts.push_back( prt );
            if( drop_pos && prt->mount.x() < 0 ) {
                prt->precalc[0].z() = -1;
                prt->precalc[1].z() = -1;
            } else if( !drop_pos && prt->mount.x() > 1 ) {
                prt->precalc[0].z() = 1;
                prt->precalc[1].z() = 1;
            }
        }
    }
    std::set<int> z_span;
    for( vehicle_part *prt : all_parts ) {
        z_span.insert( veh.abs_part_pos( *prt ).z() );
    }
    REQUIRE( z_span.size() > 1 );

    creature_tracker &creatures = get_creature_tracker();
    Creature *existing_creature =
        creatures.creature_at<Creature>( map_starting_point );
    if( existing_creature ) {
        CAPTURE( existing_creature->as_character() );
        CAPTURE( existing_creature->as_avatar() );
        CAPTURE( existing_creature->is_monster() );
        REQUIRE( !existing_creature );
    }
    monster *dmon_p = g->place_critter_at( pseudo_debug_mon, map_starting_point );
    REQUIRE( dmon_p );
    monster &dmon = *dmon_p;

    for( int y = 0; y < SEEY * MAPSIZE; y++ ) {
        for( int x = 0; x < SEEX * MAPSIZE; x++ ) {
            here.ter_set( tripoint_bub_ms( x, y, 1 ), ter_id( "t_open_air" ) );
            here.ter_set( tripoint_bub_ms( x, y, 0 ), ter_id( "t_pavement" ) );
        }
    }

    here.vehmove();
    for( vehicle_part *prt : all_parts ) {
        CHECK( veh.abs_part_pos( *prt ).z() == 0 );
    }
    CHECK( dmon.posz() == 0 );
    CHECK( veh.pos_abs().z() == 0 );
}

static void test_leveling( const std::string &type )
{
    SECTION( type + " body drop" ) {
        level_out( vproto_id( type ), true );
    }
    SECTION( type + " edge drop" ) {
        level_out( vproto_id( type ), false );
    }
}

static std::vector<std::string> level_vehs_to_test = {{
        "beetle",
    }
};

TEST_CASE( "vehicle_level_test", "[vehicle][ramp]" )
{
    for( const std::string &veh : level_vehs_to_test ) {
        test_leveling( veh );
    }
}
