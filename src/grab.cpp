#include "game.h" // IWYU pragma: associated

#include "map.h"
#include "messages.h"
#include "output.h"
#include "player.h"
#include "sounds.h"
#include "vehicle.h"
#include "vpart_position.h"

bool game::grabbed_veh_move( const tripoint &dp )
{
    const optional_vpart_position grabbed_vehicle_vp = m.veh_at( u.pos() + u.grab_point );
    if( !grabbed_vehicle_vp ) {
        add_msg( m_info, _( "No vehicle at grabbed point." ) );
        u.grab( OBJECT_NONE );
        return false;
    }
    vehicle *grabbed_vehicle = &grabbed_vehicle_vp->vehicle();
    const int grabbed_part = grabbed_vehicle_vp->part_index();

    const vehicle *veh_under_player = veh_pointer_or_null( m.veh_at( u.pos() ) );
    if( grabbed_vehicle == veh_under_player ) {
        u.grab_point = -dp;
        return false;
    }

    tripoint dp_veh = -u.grab_point;
    tripoint prev_grab = u.grab_point;
    tripoint next_grab = u.grab_point;

    bool zigzag = false;

    if( dp == prev_grab ) {
        // We are pushing in the direction of vehicle
        dp_veh = dp;
    } else if( abs( dp.x + dp_veh.x ) != 2 && abs( dp.y + dp_veh.y ) != 2 ) {
        // Not actually moving the vehicle, don't do the checks
        u.grab_point = -( dp + dp_veh );
        return false;
    } else if( ( dp.x == prev_grab.x || dp.y == prev_grab.y ) &&
               next_grab.x != 0 && next_grab.y != 0 ) {
        // Zig-zag (or semi-zig-zag) pull: player is diagonal to vehicle
        // and moves away from it, but not directly away
        dp_veh.x = ( dp.x == -dp_veh.x ) ? 0 : dp_veh.x;
        dp_veh.y = ( dp.y == -dp_veh.y ) ? 0 : dp_veh.y;

        next_grab = -dp_veh;
        zigzag = true;
    } else {
        // We are pulling the vehicle
        next_grab = -dp;
    }

    // Make sure the mass and pivot point are correct
    grabbed_vehicle->invalidate_mass();

    //vehicle movement: strength check
    int mc = 0;
    int str_req = ( grabbed_vehicle->total_mass() / 25_kilogram ); //strength required to move vehicle.

    //if vehicle is rollable we modify str_req based on a function of movecost per wheel.

    // Vehicle just too big to grab & move; 41-45 lets folks have a bit of a window
    // (Roughly 1.1K kg = danger zone; cube vans are about the max)
    if( str_req > 45 ) {
        add_msg( m_info, _( "The %s is too bulky for you to move by hand." ),
                 grabbed_vehicle->name );
        return true; // No shoving around an RV.
    }

    const auto &wheel_indices = grabbed_vehicle->wheelcache;
    if( grabbed_vehicle->valid_wheel_config() ) {
        //determine movecost for terrain touching wheels
        const tripoint vehpos = grabbed_vehicle->global_pos3();
        for( int p : wheel_indices ) {
            const tripoint wheel_pos = vehpos + grabbed_vehicle->parts[p].precalc[0];
            const int mapcost = m.move_cost( wheel_pos, grabbed_vehicle );
            mc += ( str_req / wheel_indices.size() ) * mapcost;
        }
        //set strength check threshold
        //if vehicle has many or only one wheel (shopping cart), it is as if it had four.
        if( wheel_indices.size() > 4 || wheel_indices.size() == 1 ) {
            str_req = mc / 4 + 1;
        } else {
            str_req = mc / wheel_indices.size() + 1;
        }
    } else {
        str_req++;
        //if vehicle has no wheels str_req make a noise.
        if( str_req <= u.get_str() ) {
            sounds::sound( grabbed_vehicle->global_pos3(), str_req * 2, sounds::sound_t::movement,
                           _( "a scraping noise." ) );
        }
    }

    //final strength check and outcomes
    ///\EFFECT_STR determines ability to drag vehicles
    if( str_req <= u.get_str() ) {
        //calculate exertion factor and movement penalty
        ///\EFFECT_STR increases speed of dragging vehicles
        u.moves -= 100 * str_req / std::max( 1, u.get_str() );
        int ex = dice( 1, 3 ) - 1 + str_req;
        if( ex > u.get_str() ) {
            add_msg( m_bad, _( "You strain yourself to move the %s!" ), grabbed_vehicle->name.c_str() );
            u.moves -= 200;
            u.mod_pain( 1 );
        } else if( ex == u.get_str() ) {
            u.moves -= 200;
            add_msg( _( "It takes some time to move the %s." ), grabbed_vehicle->name.c_str() );
        }
    } else {
        u.moves -= 100;
        add_msg( m_bad, _( "You lack the strength to move the %s" ), grabbed_vehicle->name.c_str() );
        return true;
    }

    std::string blocker_name = _( "errors in movement code" );
    const auto get_move_dir = [&]( const tripoint & dir, const tripoint & from ) {
        tileray mdir;

        mdir.init( dir.x, dir.y );
        grabbed_vehicle->turn( mdir.dir() - grabbed_vehicle->face.dir() );
        grabbed_vehicle->face = grabbed_vehicle->turn_dir;
        grabbed_vehicle->precalc_mounts( 1, mdir.dir(), grabbed_vehicle->pivot_point() );

        // Grabbed part has to stay at distance 1 to the player
        // and in roughly the same direction.
        const tripoint new_part_pos = grabbed_vehicle->global_pos3() +
                                      grabbed_vehicle->parts[ grabbed_part ].precalc[ 1 ];
        const tripoint expected_pos = u.pos() + dp + from;
        const tripoint actual_dir = expected_pos - new_part_pos;

        // Set player location to illegal value so it can't collide with vehicle.
        const tripoint player_prev = u.pos();
        u.setpos( tripoint_zero );
        std::vector<veh_collision> colls;
        bool failed = grabbed_vehicle->collision( colls, actual_dir, true );
        u.setpos( player_prev );
        if( !colls.empty() ) {
            blocker_name = colls.front().target_name;
        }
        return failed ? tripoint_zero : actual_dir;
    };

    // First try the move as intended
    // But if that fails and the move is a zig-zag, try to recover:
    // Try to place the vehicle in the position player just left rather than "flattening" the zig-zag
    tripoint final_dp_veh = get_move_dir( dp_veh, next_grab );
    if( final_dp_veh == tripoint_zero && zigzag ) {
        final_dp_veh = get_move_dir( -prev_grab, -dp );
        next_grab = -dp;
    }

    if( final_dp_veh == tripoint_zero ) {
        add_msg( _( "The %s collides with %s." ), grabbed_vehicle->name.c_str(), blocker_name.c_str() );
        u.grab_point = prev_grab;
        return true;
    }

    u.grab_point = next_grab;

    tripoint gp = grabbed_vehicle->global_pos3();
    grabbed_vehicle = m.displace_vehicle( gp, final_dp_veh );

    if( grabbed_vehicle == nullptr ) {
        debugmsg( "Grabbed vehicle disappeared" );
        return false;
    }

    for( int p : wheel_indices ) {
        if( one_in( 2 ) ) {
            tripoint wheel_p = grabbed_vehicle->global_part_pos3( grabbed_part );
            grabbed_vehicle->handle_trap( wheel_p, p );
        }
    }

    return false;

}
