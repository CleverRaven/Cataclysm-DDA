#include "game.h" // IWYU pragma: associated

#include <algorithm>
#include <cstdlib>

#include "avatar.h"
#include "debug.h"
#include "map.h"
#include "messages.h"
#include "monster.h"
#include "point.h"
#include "rng.h"
#include "sounds.h"
#include "tileray.h"
#include "translations.h"
#include "units.h"
#include "units_utility.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

bool game::grabbed_veh_move( const tripoint_rel_ms &dp )
{
    map &here = get_map();

    const optional_vpart_position grabbed_vehicle_vp = here.veh_at( u.pos_bub( here ) + u.grab_point );
    if( !grabbed_vehicle_vp ) {
        add_msg( m_info, _( "No vehicle at grabbed point." ) );
        u.grab( object_type::NONE );
        return false;
    }
    vehicle *grabbed_vehicle = &grabbed_vehicle_vp->vehicle();
    if( !grabbed_vehicle ||
        !grabbed_vehicle->handle_potential_theft( get_avatar() ) ) {
        return false;
    }
    const int grabbed_part = grabbed_vehicle_vp->part_index();
    if( monster *mon = grabbed_vehicle->get_harnessed_animal( here ) ) {
        add_msg( m_info, _( "You cannot move this vehicle whilst your %s is harnessed!" ),
                 mon->get_name() );
        u.grab( object_type::NONE );
        return false;
    }
    const vehicle *veh_under_player = veh_pointer_or_null( here.veh_at( u.pos_bub( here ) ) );
    if( grabbed_vehicle == veh_under_player ) {
        u.grab_point = - dp;
        return false;
    }

    tripoint_rel_ms dp_veh = - u.grab_point;
    const tripoint_rel_ms prev_grab = u.grab_point;
    tripoint_rel_ms next_grab = u.grab_point;
    const tileray initial_veh_face = grabbed_vehicle->face;

    const bool veh_has_solid = !empty( grabbed_vehicle->get_avail_parts( VPFLAG_OBSTACLE ) );
    const bool veh_single_tile = grabbed_vehicle->get_points().size() == 1;
    bool zigzag = false;
    bool pushing = false;
    bool pulling = false;

    if( dp == prev_grab ) {
        // We are pushing in the direction of vehicle
        dp_veh = dp;
        pushing = true;
    } else if( std::abs( dp.x() + dp_veh.x() ) != 2 && std::abs( dp.y() + dp_veh.y() ) != 2 ) {
        // Not actually moving the vehicle, don't do the checks
        u.grab_point = - ( dp + dp_veh );
        return false;
    } else if( ( dp.x() == prev_grab.x() || dp.y() == prev_grab.y() ) &&
               next_grab.x() != 0 && next_grab.y() != 0 ) {
        // Zig-zag (or semi-zig-zag) pull: player is diagonal to vehicle
        // and moves away from it, but not directly away
        dp_veh.x() = dp.x() == -dp_veh.x() ? 0 : dp_veh.x();
        dp_veh.y() = dp.y() == -dp_veh.y() ? 0 : dp_veh.y();

        next_grab = - dp_veh;
        zigzag = true;
    } else {
        // We are pulling the vehicle
        next_grab = - dp;
        pulling = true;
    }

    // Make sure the mass and pivot point are correct
    grabbed_vehicle->invalidate_mass();

    //vehicle movement: strength check. very strong humans can move about 2,000 kg in a wheelbarrow.
    int mc = 0;
    // worst case scenario strength required to move vehicle.
    const int max_str_req = grabbed_vehicle->total_mass( here ) / 10_kilogram;
    // actual strength required to move vehicle.
    int str_req = 0;
    // ARM_STR governs dragging heavy things
    int str = u.get_arm_str();

    bool bad_veh_angle = false;
    bool invalid_veh_turndir = false;
    bool invalid_veh_face = false;
    bool back_of_vehicle = false;
    bool valid_wheels = false;

    //if vehicle is rollable we modify str_req based on a function of movecost per wheel.
    const auto &wheel_indices = grabbed_vehicle->wheelcache;
    valid_wheels = grabbed_vehicle->valid_wheel_config( here );
    if( valid_wheels ) {
        //check for bad push/pull angle
        if( veh_has_solid && !veh_single_tile && grabbed_vehicle->steering_effectiveness( here ) > 0 ) {
            tileray my_dir;
            my_dir.init( dp.xy() );
            units::angle face_delta = angle_delta( grabbed_vehicle->face.dir(), my_dir.dir() );

            tileray my_pos_dir;
            tripoint_rel_ms my_angle = u.pos_bub( here ) - grabbed_vehicle->pos_bub( here );
            my_pos_dir.init( my_angle.xy() );
            back_of_vehicle = ( angle_delta( grabbed_vehicle->face.dir(), my_pos_dir.dir() ) > 90_degrees );
            invalid_veh_face = ( face_delta > vehicles::steer_increment * 2 - 1_degrees &&
                                 face_delta < 180_degrees - vehicles::steer_increment * 2 + 1_degrees );
            invalid_veh_turndir = ( normalize( angle_delta( grabbed_vehicle->turn_dir,
                                               grabbed_vehicle->face.dir() ), 180_degrees ) > vehicles::steer_increment * 4 - 1_degrees );
            bad_veh_angle = invalid_veh_face || invalid_veh_turndir;
            if( bad_veh_angle ) {
                str_req = max_str_req;
            }
        } else {
            str_req = max_str_req / 10;
            //determine movecost for terrain touching wheels
            const tripoint_bub_ms vehpos = grabbed_vehicle->pos_bub( here );
            for( int p : wheel_indices ) {
                const tripoint_bub_ms wheel_pos = vehpos + grabbed_vehicle->part( p ).precalc[0];
                const int mapcost = here.move_cost( wheel_pos, grabbed_vehicle );
                mc += str_req * mapcost / wheel_indices.size();
            }
            //set strength check threshold
            //if vehicle has many or only one wheel (shopping cart), it is as if it had four.
            if( wheel_indices.size() > 4 || wheel_indices.size() == 1 ) {
                str_req = mc / 4 + 1;
            } else {
                str_req = mc / wheel_indices.size() + 1;
            }
            //finally, adjust by the off-road coefficient (always 1.0 on a road, as low as 0.1 off road.)
            str_req /= grabbed_vehicle->k_traction( here, here.vehicle_wheel_traction( *grabbed_vehicle ) );
            // If it would be easier not to use the wheels, don't use the wheels.
            str_req = std::min( str_req, max_str_req );
        }
    } else {
        str_req = max_str_req;
    }

    //final strength check and outcomes
    ///\ARM_STR determines ability to drag vehicles
    if( str_req <= str ) {
        if( str_req == max_str_req ) {
            //if vehicle has no wheels, make a noise.
            sounds::sound( grabbed_vehicle->pos_bub( here ), str_req * 2, sounds::sound_t::movement,
                           _( "a scraping noise." ), true, "misc", "scraping" );
        }
        //calculate exertion factor and movement penalty
        ///\EFFECT_STR increases speed of dragging vehicles
        u.mod_moves( -to_moves<int>( 4_seconds )  * str_req / std::max( 1, str ) );
        ///\EFFECT_STR decreases stamina cost of dragging vehicles
        u.burn_energy_all( -200 * str_req / std::max( 1, str ) );
        const int ex = dice( 1, 6 ) - 1 + str_req;
        if( ex > str + 1 ) {
            // Pain and movement penalty if exertion exceeds character strength
            add_msg( m_bad, _( "You strain yourself to move the %s!" ), grabbed_vehicle->name );
            u.mod_moves( -to_moves<int>( 2_seconds ) );
            u.mod_pain( 1 );
        } else if( ex >= str ) {
            // Movement is slow if exertion nearly equals character strength
            add_msg( _( "It takes some time to move the %s." ), grabbed_vehicle->name );
            u.mod_moves( -to_moves<int>( 2_seconds ) );
        }
    } else {
        if( invalid_veh_face ) {
            add_msg( m_bad, _( "The %s is at too sharp an angle to move like this!" ), grabbed_vehicle->name );
        }
        if( invalid_veh_turndir ) {
            add_msg( m_bad, _( "The %s is steered too far to move like this!" ), grabbed_vehicle->name );
        }
        if( !bad_veh_angle ) {
            add_msg( m_bad, _( "You lack the strength to move the %s." ), grabbed_vehicle->name );
        }
        u.mod_moves( -to_moves<int>( 1_seconds ) );
        return true;
    }

    std::string blocker_name = _( "errors in movement code" );
    const auto get_move_dir = [&]( const tripoint_rel_ms & md_dp_veh,
    const tripoint_rel_ms & md_next_grab ) {
        tileray mdir;

        mdir.init( md_dp_veh.xy() );
        units::angle turn = normalize( mdir.dir() - grabbed_vehicle->face.dir() );
        if( grabbed_vehicle->is_on_ramp && turn == 180_degrees ) {
            add_msg( m_bad, _( "The %s can't be turned around while on a ramp." ), grabbed_vehicle->name );
            return tripoint_rel_ms::zero;
        }
        units::angle precalc_dir = grabbed_vehicle->face.dir();
        if( veh_has_solid && !bad_veh_angle && !veh_single_tile ) {
            //get rotation direction
            units::angle abs_turn_delta = angle_delta( grabbed_vehicle->face.dir(), grabbed_vehicle->turn_dir );
            if( abs_turn_delta != 0_degrees ) {
                //angle values lose exact precision during calculation
                int clockwise = std::abs( normalize( grabbed_vehicle->face.dir() + abs_turn_delta ).value() -
                                          normalize( grabbed_vehicle->turn_dir ).value() ) < 0.1 ? 1 : -1;
                units::angle turn_delta = abs_turn_delta * clockwise;
                // mirror turn for given cases
                if( ( pushing && !back_of_vehicle ) || ( pulling && back_of_vehicle ) ) {
                    turn_delta *= -1;
                }
                grabbed_vehicle->turn_dir = normalize( grabbed_vehicle->face.dir() + turn_delta );
                grabbed_vehicle->face = tileray( grabbed_vehicle->turn_dir );
                precalc_dir = grabbed_vehicle->face.dir();
            }
        }
        //bad angle only applies to solid vehicles
        else if( !veh_has_solid || veh_single_tile ) {
            grabbed_vehicle->turn( turn );
            grabbed_vehicle->face = tileray( grabbed_vehicle->turn_dir );
            precalc_dir = mdir.dir();
        }
        grabbed_vehicle->precalc_mounts( 1, precalc_dir, grabbed_vehicle->pivot_point( here ) );
        grabbed_vehicle->pos -= grabbed_vehicle->pivot_displacement().raw();

        // Grabbed part has to stay at distance 1 to the player
        // and in roughly the same direction.
        const tripoint_bub_ms new_part_pos = grabbed_vehicle->pos_bub( here ) +
                                             grabbed_vehicle->part( grabbed_part ).precalc[1];
        const tripoint_bub_ms expected_pos = u.pos_bub( here ) + dp + md_next_grab;
        tripoint_rel_ms actual_dir = tripoint_rel_ms( ( expected_pos - new_part_pos ).xy(), 0 );

        bool failed = false;
        bool actual_diff = false;
        tripoint_rel_ms skip = pushing ? u.grab_point : dp;
        if( veh_has_solid ) {
            //avoid player collision from vehicle turning
            bool no_player_collision = false;
            while( !no_player_collision ) {
                no_player_collision = true;
                for( const vpart_reference &vp : grabbed_vehicle->get_all_parts() ) {
                    if( grabbed_vehicle->pos_bub( here ) +
                        vp.part().precalc[1] + actual_dir == u.pos_bub( here ) + skip ) {
                        no_player_collision = false;
                        break;
                    }
                }
                if( !no_player_collision ) {
                    actual_dir += u.grab_point;
                    no_player_collision = false;
                    actual_diff = true;
                }
            }
            if( actual_diff ) {
                add_msg( _( "You let go of the %s as it turns." ), grabbed_vehicle->disp_name() );
                u.grab( object_type::NONE );
                u.grab_point = tripoint_rel_ms::zero;
            }
        }
        // Set player location to illegal value so it can't collide with vehicle.
        const tripoint_abs_ms player_prev = u.pos_abs( );
        u.setpos( here, tripoint_bub_ms::zero, false );
        std::vector<veh_collision> colls;
        failed = grabbed_vehicle->collision( here, colls, actual_dir, true );
        u.setpos( player_prev );
        if( !colls.empty() ) {
            blocker_name = colls.front().target_name;
        }

        return failed ? tripoint_rel_ms::invalid : actual_dir;
    };

    // First try the move as intended
    // But if that fails and the move is a zig-zag, try to recover:
    // Try to place the vehicle in the position player just left rather than "flattening" the zig-zag
    tripoint_rel_ms final_dp_veh = get_move_dir( dp_veh, next_grab );
    if( final_dp_veh == tripoint_rel_ms::invalid && zigzag ) {
        final_dp_veh = get_move_dir( - prev_grab, - dp );
        next_grab = - dp;
    }

    if( final_dp_veh == tripoint_rel_ms::invalid ) {
        add_msg( _( "The %s collides with %s." ), grabbed_vehicle->name, blocker_name );
        u.grab_point = prev_grab;
        grabbed_vehicle->face = initial_veh_face;
        return true;
    }

    //if grab was already released, do not set grab again
    if( u.grab_point != tripoint_rel_ms::zero ) {
        u.grab_point = next_grab;
    }

    here.displace_vehicle( *grabbed_vehicle, final_dp_veh );
    here.rebuild_vehicle_level_caches();

    if( grabbed_vehicle ) {
        here.level_vehicle( *grabbed_vehicle );
        grabbed_vehicle->check_falling_or_floating();
        if( grabbed_vehicle->is_falling ) {
            add_msg( _( "You let go of the %1$s as it starts to fall." ), grabbed_vehicle->disp_name() );
            u.grab( object_type::NONE );
            u.grab_point = tripoint_rel_ms::zero;
            here.set_seen_cache_dirty( grabbed_vehicle->pos_bub( here ) );
            return true;
        }
    } else {
        debugmsg( "Grabbed vehicle disappeared" );
        return false;
    }

    for( int p : wheel_indices ) {
        if( one_in( 2 ) ) {
            vehicle_part &vp_wheel = grabbed_vehicle->part( p );
            tripoint_bub_ms wheel_p = grabbed_vehicle->bub_part_pos( here, vp_wheel );
            grabbed_vehicle->handle_trap( &here, wheel_p, vp_wheel );
        }
    }

    return false;

}
