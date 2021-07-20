#include "vehicle.h" // IWYU pragma: associated

#include "character.h"
#include "coordinates.h"
#include "game.h"
#include "map.h"
#include "point.h"


std::vector<std::tuple<point, int, std::string>> vehicle::get_debug_overlay_data() const
{
    static const std::vector<std::string> debug_what = { "valid_position", "omt" };
    std::vector<std::tuple<point, int, std::string>> ret;

    const tripoint veh_pos = get_map().getabs( global_pos3() );
    if( autodrive_local_target != tripoint_zero ) {
        ret.emplace_back( ( autodrive_local_target - veh_pos ).xy(), catacurses::red, "T" );
    }
    for( const point &pt_elem : collision_check_points ) {
        ret.emplace_back( pt_elem - veh_pos.xy(), catacurses::yellow, "C" );
    }

    return ret;
}

autodrive_result vehicle::do_autodrive( Character &driver )
{
    if( !is_autodriving ) {
        return autodrive_result::abort;
    }
    std::vector<tripoint_abs_omt> &omt_path = driver.omt_path;
    if( omt_path.empty() ) {
        stop_autodriving();
        return autodrive_result::finished;
    }
    map &here = get_map();
    tripoint vehpos = global_pos3();
    // TODO: fix point types
    tripoint_abs_omt veh_omt_pos( ms_to_omt_copy( here.getabs( vehpos ) ) );
    // we're at or close to the waypoint, pop it out and look for the next one.
    if( ( is_autodriving && !omt_path.empty() ) &&
        veh_omt_pos == omt_path.back() ) {
        omt_path.pop_back();
    }
    if( omt_path.empty() ) {
        stop_autodriving();
        return autodrive_result::finished;
    }

    point_rel_omt omt_diff = omt_path.back().xy() - veh_omt_pos.xy();
    if( omt_diff.x() > 3 || omt_diff.x() < -3 || omt_diff.y() > 3 || omt_diff.y() < -3 ) {
        // we've gone walkabout somehow, call off the whole thing
        stop_autodriving();
        return autodrive_result::abort;
    }
    point side;
    if( omt_diff.x() > 0 ) {
        side.x = 2 * SEEX - 1;
    } else if( omt_diff.x() < 0 ) {
        side.x = 0;
    } else {
        side.x = SEEX;
    }
    if( omt_diff.y() > 0 ) {
        side.y = 2 * SEEY - 1;
    } else if( omt_diff.y() < 0 ) {
        side.y = 0;
    } else {
        side.y = SEEY;
    }
    // get the shared border mid-point of the next path omt
    tripoint_abs_ms global_a = project_to<coords::ms>( veh_omt_pos );
    // TODO: fix point types
    tripoint autodrive_temp_target = ( global_a.raw() + tripoint( side,
                                       sm_pos.z ) - here.getabs( vehpos ) ) + vehpos;
    autodrive_local_target = here.getabs( autodrive_temp_target );
    drive_to_local_target( autodrive_local_target, false );
    if( !is_autodriving ) {
        return autodrive_result::abort;
    }
    return autodrive_result::ok;
}

void vehicle::stop_autodriving()
{
    if( !is_autodriving && !is_patrolling && !is_following ) {
        return;
    }
    if( velocity > 0 ) {
        cruise_on = true;
        cruise_velocity = 0;
    }
    is_autodriving = false;
    is_patrolling = false;
    is_following = false;
    autopilot_on = false;
    autodrive_local_target = tripoint_zero;
    collision_check_points.clear();
}
