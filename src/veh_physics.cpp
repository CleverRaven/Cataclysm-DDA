#include "veh_physics.h"
#include "game.h"
#include "vehicle.h"
#include "translations.h"

double veh_physics::calculate_top_speed( bool max_power )
{
    engine_watt_avail = double(veh->power_to_epower(eng_pwr_cur));
    if( max_power ) {
        engine_watt_avail = double(veh->power_to_epower(eng_pwr_max));
    }
    int old_velocity = veh->velocity;
    int nr_calcs = 0;
    int speed_step = 128;
    veh->velocity = 0;
    int new_vel = 0;
    do {
        veh->velocity += speed_step;
        calculate_time_to_move_one_tile( 1.0 );
        new_vel = int( velocity_end * 223.6 + 0.5 );
        if( new_vel == veh->velocity ) {
            veh->velocity = old_velocity;
            return velocity_end;
        }
        if( new_vel < veh->velocity ) {
            veh->velocity -= speed_step;
            speed_step /= 2;
            if( speed_step < 1 ) {
                veh->velocity = old_velocity;
                return velocity_end;
            }
        }
    } while( nr_calcs++ < 999 );
    veh->velocity = old_velocity;
    return velocity_end;
}

double veh_physics::calculate_acceleration( double target_speed, bool max_power )
{
    engine_watt_avail = double(veh->power_to_epower(eng_pwr_cur));
    if( max_power ) {
        engine_watt_avail = double(veh->power_to_epower(eng_pwr_max));
    }
    if( engine_watt_avail < 1 ) {
        return 0.0;
    }
    double total_time = 0.0;
    double tile_time = 0.0;
    int nr_calcs = 0;
    int old_velocity = veh->velocity;
    veh->velocity = 0;
    do {
        tile_time = calculate_time_to_move_one_tile( 1.0 );
        veh->velocity = int( velocity_end * 223.6 + 0.5 ); /* m/s to 100 x mph */
        if( veh->velocity == 0 ) {
            veh->velocity = old_velocity;
            return 0.0;
        }
        total_time += tile_time;
        if( total_time >= 10.0 ) {
            /* If we've accelerated for 10 secs without reaching target_speed,
               just return the speed reached at 10 sec (remove speed for >10 sec) */
            double speed;
            speed = velocity_end -
                    (velocity_end - velocity_start) / tile_time *
                    (total_time - 10.0);
            veh->velocity = old_velocity;
            return -speed;
        }
        if( ++nr_calcs > 999 ) {
            /* We've driven across 1000 tiles in less than 10 secs and still haven't
               reached target_speed, it must be set far too high, return 0 */
            veh->velocity = old_velocity;
            return 0.0;
        }
    } while( nr_calcs++ < 999 && velocity_end < target_speed );
    /* Subtract time used for velocity above target_speed */
    total_time += -tile_time + tile_time *
                  (target_speed - velocity_start) /
                  (velocity_end - velocity_start);
    veh->velocity = old_velocity;
    return ( nr_calcs < 999 ? total_time : 0.0 );
}

double veh_physics::calculate_cruise_control( double target_speed )
{
    double eng_est = -1.0;
    double eng_step = 2.0;
    int nr_calcs = 0;
    bool last_vel_too_high = false;
    do {
        calculate_time_to_move_one_tile( eng_est );
        if( velocity_end > target_speed ) {
            if( eng_est <= -1.0 )
                break; /* Max engine power is not enough */
            if( !last_vel_too_high ) {
                last_vel_too_high = true;
                eng_step = eng_step / 4;
            }
            eng_est = std::max( eng_est - eng_step, -1.0 );
        } else {
            if( eng_est >= 1.0 )
                break; /* Max engine power is not enough */
            if( last_vel_too_high ) {
                last_vel_too_high = false;
                eng_step = eng_step / 4;
            }
            eng_est = std::min( eng_est + eng_step, 1.0 );
        }
    } while( nr_calcs++ < 999 && fabs(velocity_end - target_speed) > 0.01 );
    if( nr_calcs >= 999 ) {
        debugmsg("More than 1000 speed calculations trying to cruise control!");
    }
    return time_taken;
}

double veh_physics::calculate_time_to_move_one_tile( double fdir )
{
    distance_traveled = TILE_SIZE_M;
    double t = TURN_TIME_S;
    velocity_start = veh->velocity / 223.6; /* Convert to m/s */
    /* joules = kg*m^2/s^2 = kg * m/s * m/s; */
    v_dir = velocity_start < 0 ? -1 : 1;
    if( veh->velocity == 0 ) {
        v_dir = fdir < 0.0 ? -1 : 1;
    }
    kinetic_start = 0.5 * mass * velocity_start * velocity_start * v_dir;

    int nr_calcs = 0;
    do {
        time_taken = t;
        /* TODO: engine_newton_avail should depend on speed (and lots of other stuff) */
        engine_newton_avail = std::max(0.0, engine_watt_avail * t / TILE_SIZE_M);
        /* Get forces at initial speed (start of tile) */
        double newton_start = calculate_forces( velocity_start, fdir );
        /* note, newton, kinetic, and velocity can all be negative */
        /* Calculate speed at end if drag etc at start of tile were kept constant */
        double kin_end = kinetic_start + newton_start * TILE_SIZE_M;
        double v_end = sqrt( 2 * fabs(kin_end) / mass );
        if( kin_end < 0 ) {
            v_end *= -1;
        }
        /* Now we can calculate drag etc using v_end (end of tile) */
        double newton_end = calculate_forces( v_end, fdir );
        /* Get the average force working on the vehicle through the tile */
        newton_average = ( newton_start + newton_end ) / 2;
        /* Calculate the correct kinetic value, using average force from tile start til tile end */
        kinetic_end = kinetic_start + newton_average * TILE_SIZE_M;
        /* And there we have an estimate of the speed */
        velocity_end = sqrt( 2 * fabs(kinetic_end) / mass );
        if( kinetic_end < 0 ) {
            velocity_end *= -1;
        }
        if (  !((v_dir > 0.0) ^ (velocity_end < 0.0)) ) { /* XAND */
            /* Don't go directly from forward to reverse */
            kinetic_end = 0.0;
            velocity_end = 0.0;
        }
        /* Update some other values as well now that we know speed etc */
        velocity_average = (velocity_start + velocity_end) / 2;
        engine_watt_average = newton_average * velocity_average;
        distance_traveled = fabs(velocity_average * t); /* No negatives */
        /* More closely match t to actual time taken, for use in next iteration */
        t = t * TILE_SIZE_M / distance_traveled;
        if( fabs(velocity_end) < 0.1 )
            velocity_end = 0.0;
        if( distance_traveled < 0.001 || t >= TURN_TIME_S || t <= 0.001 || velocity_end == 0.0 ) {
            time_taken = TURN_TIME_S;
            break; /* For gameplay purposes you can always travel 1 tile in a turn */
            /* TODO: Lower the 1 tile/turn minimum speed (or remove it) before doing a proper pull request */
        }
    } while( fabs( distance_traveled - TILE_SIZE_M ) > 0.0001 && nr_calcs++ < 999);
    if( nr_calcs >= 1000 ) {
        debugmsg( "Nr of calcs in calculate_time more than 1000!" );
    }
    if( time_taken > TURN_TIME_S ) {
        time_taken = TURN_TIME_S;
    }
    return time_taken;
}

double veh_physics::calculate_forces( double v, double fdir )
{
    /* Slow down vehicle due to air resistance.
       Don't need to consider grip here since the force acts on car chassis and not tires
       http://en.wikipedia.org/wiki/Drag_%28physics%29 -> Fd = 1/2 p v^2 Cd A
       kg*m/s^2 = kg/m^3 * m/s * m/s * m^2 */
    drag_newton = air_density * v * v * veh->drag_coeff;

    /* http://en.wikipedia.org/wiki/Downforce
       kg*m/s^2 = kg/m^3 * m/s * m/s * m^2 */
    downforce_newton = 0.5 * air_density * v * v * veh->downforce;

    /* Add gravity to downforce
       kg*m/s^2 = 9.81 m/s^2(earth gravity) * kg */
    downforce_newton += (mass * gravity);

    wheel_newton_max = tire_friction_coeff * downforce_newton;
    /* Allow more slowdown than wheels can handle, could be terrain or car frame slowing down */
    ground_res_newton = ground_res_coeff * downforce_newton;

    /* Find max force tires can handle. Don't make it go negative from rres */
    wheel_newton_avail = std::max( wheel_newton_max - ground_res_newton, 0.0 );

    if( v_dir < 0 ) {
        engine_newton_avail *= 0.6; /* Less power available in reverse */
    }
    is_thrusting = ( veh->velocity == 0 || ( (veh->velocity > 0) ^ (fdir < 0) ) );
    if( is_thrusting ) {
        engine_newton = std::min(fabs(engine_newton_avail * fdir), wheel_newton_avail);
        user_applied_newton = engine_newton * v_dir;
    } else {
        engine_newton = 0.0;
        user_applied_newton = wheel_newton_avail * fdir;
    }
    if( !valid_wheel_config ) {
        user_applied_newton = 0.0;
    }

    /* Subtract drag/ground res if driving forwards, add them if driving backwards */
    newton_total = user_applied_newton - (ground_res_newton + drag_newton) * v_dir;

    return newton_total;
}

