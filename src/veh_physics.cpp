#include "veh_physics.h"
#include "game.h"
#include "vehicle.h"
#include "translations.h"

static const std::string part_location_structure("structure");

veh_physics::veh_physics()
{
    veh = NULL;
}

void veh_physics::init( vehicle *init_vehicle )
{
    if( init_vehicle != NULL ) {
        veh = init_vehicle;
    }
    if( veh == NULL ) {
        debugmsg("veh_physics::init does not have a vehicle to initalize from!");
        return;
    }
    mass = veh->cached_mass;
    int unused1;
    eng_pwr_cur = veh->engine_power(&eng_pwr_max, &unused1, &eng_pwr_alt);
    // TODO: Add gearboxes, vary engine power based on speed/gear
    engine_watt_avail = veh->engine_on ? double(veh->power_to_epower(eng_pwr_cur)) : 0.0;
    valid_wheel_config = veh->valid_wheel_config(); // Cache this
    tire_friction_coeff = 0.7;
    ground_res_coeff = 0.01;
}

/**
 * Called to process vehicle actions that occur while it's moving through the tile it's currently in.
 * If it returns false, it was unable to move and did not perform any actions.
 * We can afford to do some heavy calcs here, since this routine is only called on moving vehicles
 */
bool veh_physics::drive_one_tile()
{
    int debug = 5;
    const bool pl_ctrl = veh->player_in_control(&g->u);
    const int gx = veh->global_x();
    const int gy = veh->global_y();
    if (!g->m.inbounds(gx, gy)) {
        if (g->debugmon) { debugmsg ("stopping out-of-map vehicle. (x,y)=(%d,%d)", gx, gy); }
        veh->stop();
        veh->of_turn = 0;
        return false;
    }

    init(); // Recalc engine power/velocity/kinetic energy

    // Handle wheels. Sink in water, mess up ground, set off traps.
    int num_wheels, submerged_wheels;
    num_wheels = int(veh->wheels.size());
    submerged_wheels = 0;
    tire_friction_coeff = 0.0;
    ground_res_coeff = 0.0;
    double wheel_area_on_ground = 0.0;
    for (int w = 0; w < num_wheels; w++) {
        const int p = veh->wheels[w];
        const int px = gx + veh->parts[p].precalc_dx[0];
        const int py = gy + veh->parts[p].precalc_dy[0];
        // deep water
        if( g->m.ter_at(px, py).has_flag(TFLAG_DEEP_WATER) ) {
            submerged_wheels++;
            tire_friction_coeff += 0.005;
            ground_res_coeff += 2.0;
        } else {
            int tile_move_cost = g->m.move_cost_ter_furn(px, py);
            double tg = std::max( 0.1, 0.9 - tile_move_cost/10.0 ); // 0.7 - 0.1 grip on terrain
            double mtg = 0.3 + veh->part_health(p)*0.6; // 0.9 - 0.3 max tire grip
            tire_friction_coeff += std::min( tg, mtg );

//            wheel_area_on_ground += part_info(p).wheel_width * part_health(p); // 2(bicycle)-20(armored)

//            double rr = part_info(p).wheel_width / 9.0;
            // Riding on the car frame gives lower rres than tires, which are made for max friction
            if( !valid_wheel_config ) {
                tire_friction_coeff = 1.0 - tg;
            } else if( veh->skidding ) {
                tire_friction_coeff /= 2;
            }
//            rolling_res_coeff += rr;
            // TODO: Differentiate between grass/road etc 

            // In difficult terrain you roll a lot worse, plus, debris
            // Just add resistance from rolling res/debris hitting car body here
            ground_res_coeff += veh->part_info(p).wheel_width * tile_move_cost / 8000 + 0.01 * pow(tile_move_cost, 3) / 8 - 0.01;
        }

        if( one_in(2) ) {
            if( g->m.displace_water(px, py) && pl_ctrl ) {
                g->add_msg(_("You hear a splash!"));
            }
        }

        // now we're gonna handle traps we're standing on
        handle_trap( px, py, p );
    }
    tire_friction_coeff /= num_wheels;
    ground_res_coeff /= num_wheels;

    // submerged wheels threshold is 2/3.
    if (num_wheels && (float)submerged_wheels / num_wheels > .666) {
        g->add_msg(_("Your %s sank."), veh->name.c_str());
        // destroy vehicle (sank to nowhere)
        g->m.destroy_vehicle(veh);
        return false;
    }

    if( !valid_wheel_config ) {
        // if not enough wheels, mess up the ground a bit.
        std::vector<int> structural_indices = veh->all_parts_at_location(part_location_structure);
        for( size_t s = 0; s < structural_indices.size(); s++ ) {
            const int p = structural_indices[s];
            const int px = gx + veh->parts[p].precalc_dx[0];
            const int py = gy + veh->parts[p].precalc_dy[0];
            const ter_id &pter = g->m.ter(px, py);
            if (pter == t_dirt || pter == t_grass) {
                g->m.ter_set(px, py, t_dirtmound);
            }
        }
    }

    // Eventually send vehicle skidding if no-one is controlling it
    if( !veh->boarded_parts().size() && one_in(10) ) {
        veh->start_skid(0);
    }

    int ds = g->u.skillLevel("driving");
    if (pl_ctrl) {
        if (veh->skidding) {
            if (one_in(ds+1)) { // Might turn uncontrollably while skidding
                veh->turn(rng(-2, 2) * 15);
            }
            g->u.practice( g->turn, "driving", 1 ); // Skidding equals learning!
        } else if ( dice(3, ds*20+25)+ds*20 < veh->velocity/100 ) {
            g->add_msg(_("You fumble with the %s's controls."), veh->name.c_str());
            int r;
            // TODO: Add somthing that changes velocity for r == 0
            do { r= dice(3, 3) - 6; } while( r == 0 );
            veh->turn(r * 15);
        }
        if( veh->velocity > one_in(ds*ds*20000+40000) ) {
            g->u.practice( g->turn, "driving", 1 );
        }
    } else {
        // cruise control TODO: enable for NPC?
        veh->cruise_on = false;
    }

    if( !valid_wheel_config && veh->velocity==0 && pl_ctrl &&
            ( veh->cruise_on || veh->player_thrust!=0 ) ) {
        g->add_msg (_("The %s doesn't have enough wheels to move!"), veh->name.c_str());
        velocity_end = 0.0;
        engine_watt_average = 0.0;
        time_taken = TURN_TIME_S;
    } else if( veh->skidding ) {
        // TODO: Maybe add engine strain again
        calculate_movement( -2.0, 0.0 ); // Skidding equals brake to zero
    } else if( veh->cruise_on ) {
        calculate_movement( -2.0, veh->cruise_velocity / 223.6 ); // Try to match cruise_vel
    } else if( pl_ctrl ) {
        calculate_movement( veh->player_thrust / 100.0, 0.0 ); // Ignore velocity, just thrust
    }
    veh->velocity = int(velocity_end * 223.6 + 0.5); // m/s to 100*mph

    // Less fuel use/noise if not running at full engine output, assume min 1%
    double load = (fabs(engine_watt_average) + veh->power_to_epower(eng_pwr_alt)) /
                  veh->power_to_epower(eng_pwr_max);
    load = std::max( load, 0.01 );

    if( pl_ctrl && veh->has_pedals && g->u.has_bionic("bio_torsionratchet") ) {
        // Charge one power for every 60 turns pedaling all out
        if( g->turn.get_turn() % 60 == 0 && load * time_taken * 100 > rng(0, 100) ) {
            g->u.charge_power( 1 );
        }
    }

    if( pl_ctrl ) {
        veh->consume_fuel( load * time_taken / TURN_TIME_S ); // Consume fuel
        veh->noise_and_smoke( load, time_taken ); // Make smoke and noise
    }

    if (!veh->skidding && veh->velocity!=0 && fabs(veh->turn_delta_per_tile) > 0.1) {
        // c_n = kg * v^2 / r = kg * v^2 * tdpt / ( 180 * tilesize )
        double centrifugal_newton = mass * velocity_end * velocity_end *
                                    veh->turn_delta_per_tile / 180 / TILE_SIZE_M;
        double grip = ( wheel_newton_max / 4 + wheel_newton_avail ) * TILE_SIZE_M;
        if (centrifugal_newton > grip) {
            if( dice(3, ds*5+5) > int(centrifugal_newton / grip * 100)) {
                g->add_msg(_("The %s's wheels lose their grip momentarily, but you quickly adjust the steering to compensate."), veh->name.c_str());
                veh->turn_delta_per_tile = 0;
                veh->turn_dir = 0;
            } else {
                veh->start_skid( 0 );
            }
        } else {
            int td = int(veh->turn_delta_per_tile + rng_float(0.0, 1.0) );
            if (abs(td) >= abs(veh->turn_dir))
                td = veh->turn_dir; // We've turned as much as we wanted to, so stop here
            veh->turn_dir -= td;
            veh->face.init( veh->face.dir() + td );
            if (debug>=3) g->add_msg("M] Turned %d to %d.", int(td), veh->face.dir());
            if ( abs(veh->face.dir() - veh->move.dir()) > 10 )
                veh->move = veh->face;
        }
    } else if (veh->skidding && fabs(veh->turn_delta_per_tile) > 0.1) {
        // We're skidding, absorb turning energy. Estimate moments of inertia 1000-8000 from:
        // http://www.nhtsa.gov/Research/Vehicle+Research+&+Testing+%28VRTC%29/ci.Crash+Avoidance+Data.print
        double moi = mass * mass / 600; // inaccurate, kg*m^2
        double grip = ( wheel_newton_max / 40 + wheel_newton_avail ) * TILE_SIZE_M; // J
        double angular_momentum = veh->turn_dir / 360.0 / TURN_TIME_S; // rad/s
        // kg*m^2/s^2 = kg*m^2 * rad/s * rad/s
        double kin_rot = 0.5 * moi * angular_momentum * angular_momentum;
        if (grip > kin_rot) {
            kin_rot = 0.0;
        } else {
            kin_rot -= grip;
        }
        angular_momentum = sqrt( 2 * kin_rot / moi );
        if (debug>=1) g->add_msg("Skidding! Pre-skid: f=%d m=%d t=%d td=%d", veh->face.dir(), veh->move.dir(), veh->turn_dir, int(veh->turn_delta_per_tile));
        veh->turn_dir = int ( angular_momentum * 360 * TURN_TIME_S ) * ( veh->turn_dir < 0 ? -1 : 1 );
        veh->turn_delta_per_tile = veh->turn_dir / (TURN_TIME_S * velocity_end / TILE_SIZE_M);
    }
    if (debug>=3) g->add_msg("f=%d m=%d t=%d td=%d", veh->face.dir(), veh->move.dir(), veh->turn_dir, int(veh->turn_delta_per_tile));

    return true;
}

void veh_physics::handle_trap (int x, int y, int part)
{
    int pwh = veh->part_with_feature( part, VPFLAG_WHEEL );
    if (pwh < 0) {
        return;
    }
    trap_id t = g->m.tr_at(x, y);
    if (t == tr_null) {
        return;
    }
    int noise = 0;
    int chance = 100;
    int expl = 0;
    int shrap = 0;
    bool wreckit = false;
    std::string msg (_("The %s's %s runs over %s."));
    std::string snd;
    // todo; make trapfuncv?

    if ( t == tr_bubblewrap ) {
        noise = 18;
        snd = _("Pop!");
    } else if ( t == tr_beartrap ||
                t == tr_beartrap_buried ) {
        noise = 8;
        snd = _("SNAP!");
        wreckit = true;
        g->m.remove_trap(x, y);
        g->m.spawn_item(x, y, "beartrap");
    } else if ( t == tr_nailboard ) {
        wreckit = true;
    } else if ( t == tr_blade ) {
        noise = 1;
        snd = _("Swinnng!");
        wreckit = true;
    } else if ( t == tr_crossbow ) {
        chance = 30;
        noise = 1;
        snd = _("Clank!");
        wreckit = true;
        g->m.remove_trap(x, y);
        g->m.spawn_item(x, y, "crossbow");
        g->m.spawn_item(x, y, "string_6");
        if (!one_in(10)) {
            g->m.spawn_item(x, y, "bolt_steel");
        }
    } else if ( t == tr_shotgun_2 ||
                t == tr_shotgun_1 ) {
        noise = 60;
        snd = _("Bang!");
        chance = 70;
        wreckit = true;
        if (t == tr_shotgun_2) {
            g->m.add_trap(x, y, tr_shotgun_1);
        } else {
            g->m.remove_trap(x, y);
            g->m.spawn_item(x, y, "shotgun_sawn");
            g->m.spawn_item(x, y, "string_6");
        }
    } else if ( t == tr_landmine_buried ||
                t == tr_landmine ) {
        expl = 10;
        shrap = 8;
        g->m.remove_trap(x, y);
    } else if ( t == tr_boobytrap ) {
        expl = 18;
        shrap = 12;
    } else if ( t == tr_dissector ) {
        noise = 10;
        snd = _("BRZZZAP!");
        wreckit = true;
    } else if ( t == tr_sinkhole ||
                t == tr_pit ||
                t == tr_spike_pit ||
                t == tr_ledge ) {
        wreckit = true;
    } else if ( t == tr_goo ||
                t == tr_portal ||
                t == tr_telepad ||
                t == tr_temple_flood ||
                t == tr_temple_toggle ) {
        msg.clear();
    }
    if (!msg.empty() && g->u_see(x, y)) {
        g->add_msg( msg.c_str(), veh->name.c_str(), veh->part_info(part).name.c_str(),
                    g->traps[t]->name.c_str() );
    }
    if (noise > 0) {
        g->sound(x, y, noise, snd);
    }
    if (wreckit && chance >= rng (1, 100)) {
        veh->damage( part, 500 );
    }
    if (expl > 0) {
        g->explosion(x, y, expl, shrap, false);
    }
}

double veh_physics::calculate_movement( double engine_output, double target_speed )
{
    char buf[100];
    if( engine_output > 1.5 ) { // Special acceleration mode
        engine_watt_avail = double(veh->power_to_epower(eng_pwr_cur));
        if( engine_output > 2.5 ) {
            engine_watt_avail = double(veh->power_to_epower(eng_pwr_max));
        }
        if( engine_watt_avail < 1 ) {
            return 0.0;
        }
        double total_time = 0.0, tile_time;
        int nr_calcs = 0;
        int old_velocity = veh->velocity;
        veh->velocity = 0;
        do {
            tile_time = calculate_time_to_move_one_tile( 1.0 );
            veh->velocity = int( velocity_end * 223.6 + 0.5 );
            if( veh->velocity == 0 ) {
                veh->velocity = old_velocity;
                return 0.0;
            }
            total_time += tile_time;
        } while ( nr_calcs++ < 999 && velocity_end < target_speed );
        // Subtract time used for velocity above target_speed
        total_time += -tile_time + tile_time *
                      (target_speed - velocity_start) /
                      (velocity_end - velocity_start);
        veh->velocity = old_velocity;
        return ( nr_calcs < 999 ? total_time : 0.0 );
    }
    if( engine_output >= -1.0 ) { // Normal thrusting mode, no cruise control
        return calculate_time_to_move_one_tile( engine_output );
    }
    double eng_est = -1.0, eng_step = 2.0;
    int nr_calcs = 0;
    bool last_vel_too_high = false;
    do { // -2.0 engine_output, cruise control mode
        calculate_time_to_move_one_tile( eng_est );
        sprintf(buf, "Power %f got speed %f, want %f. step = %f", eng_est, velocity_end, target_speed, eng_step);
//        debugmsg(buf);
        if( velocity_end > target_speed ) {
            if( eng_est <= -1.0 )
                break; // Max engine power is not enough
            if( !last_vel_too_high ) {
                last_vel_too_high = true;
                eng_step = eng_step / 4;
            }
            eng_est = std::max( eng_est - eng_step, -1.0 );
        } else {
            if( eng_est >= 1.0 )
                break; // Max engine power is not enough
            if( last_vel_too_high ) {
                last_vel_too_high = false;
                eng_step = eng_step / 4;
            }
            eng_est = std::min( eng_est + eng_step, 1.0 );
        }
    } while( nr_calcs++ < 999 && fabs(velocity_end - target_speed) > 0.01 );
//    debugmsg( "Exiting function calc_move" );
    if( nr_calcs >= 999 )
        debugmsg("More than 1000 speed calculations trying to cruise control!");
    return time_taken;
}

double veh_physics::calculate_time_to_move_one_tile( double fdir )
{
    int debug = 5;
    if (debug>=1) g->add_msg("calc_time: %d", int(fdir*1000));
    distance_traveled = TILE_SIZE_M;
    double t = TURN_TIME_S;
    velocity_start = veh->velocity / 223.6; // Convert to m/s
    // joules = kg*m^2/s^2 = kg * m/s * m/s;
    v_dir = velocity_start < 0 ? -1 : 1;
    if( veh->velocity == 0 ) {
        v_dir = fdir < 0.0 ? -1 : 1;
    }
    kinetic_start = 0.5 * mass * velocity_start * velocity_start * v_dir;

    int nr_calcs = 0;
    do {
        time_taken = t;
        // TODO: engine_newton_avail should depend on speed (and lots of other stuff)
        engine_newton_avail = std::max(0.0, engine_watt_avail * t / TILE_SIZE_M);
        if( debug>=5 ) g->add_msg("eng_N = %d", int(engine_newton_avail));
        // Get forces at initial speed (start of tile)
        double newton_start = calculate_forces( velocity_start, fdir );
        // note, newton, kinetic, and velocity can all be negative
        // Calculate speed at end if drag etc at start of tile were kept constant
        double kin_end = kinetic_start + newton_start * TILE_SIZE_M;
        double v_end = sqrt( 2 * fabs(kin_end) / mass );
        if( kin_end < 0 ) {
            v_end *= -1;
        }
        // Now we can calculate drag etc using v_end (end of tile)
        double newton_end = calculate_forces( v_end, fdir );
        // Get the average force working on the vehicle through the tile
        newton_average = ( newton_start + newton_end ) / 2;
        // Calculate the correct kinetic value, using average force from tile start til tile end
        kinetic_end = kinetic_start + newton_average * TILE_SIZE_M;
        // And there we have an estimate of the speed
        velocity_end = sqrt( 2 * fabs(kinetic_end) / mass );
        if( kinetic_end < 0 ) {
            velocity_end *= -1;
        }
        if (  !(v_dir > 0.0 ^ velocity_end < 0.0) ) { // XAND
            // Don't go directly from forward to reverse
            kinetic_end = 0.0;
            velocity_end = 0.0;
        }
        // Update some other values as well now that we know speed etc
        velocity_average = (velocity_start + velocity_end) / 2;
        engine_watt_average = newton_average * velocity_average;
        distance_traveled = fabs(velocity_average * t); // No negatives
        // More closely match t to actual time taken, for use in next iteration
        t = t * TILE_SIZE_M / distance_traveled;
        if( fabs(velocity_end) < 0.1 )
            velocity_end = 0.0;
        if( distance_traveled < 0.001 || t >= TURN_TIME_S || t <= 0.001 || velocity_end == 0.0 ) {
            time_taken = TURN_TIME_S;
            break; // For gameplay purposes you can always travel 1 tile in a turn
        }
    } while( fabs( distance_traveled - TILE_SIZE_M ) > 0.0001 && nr_calcs++ < 999);
    if( nr_calcs >= 1000 )
        debugmsg( "Nr of calcs in calculate_time_ more than 1000!" );
    if( time_taken > TURN_TIME_S )
        time_taken = TURN_TIME_S;
    return time_taken;
}

double veh_physics::calculate_forces( double v, double fdir )
{
    int debug = 5;
    if (debug>=1) g->add_msg("calc_forces: %d", int(v*1000));
    static double air_density = 1.204; // in kg/m^3  TODO: Vary it based on altitude/temperature
    static double gravity = 9.80665;
    // Slow down vehicle due to air resistance.
    // Don't need to consider grip here, since the force acts on car chassis and not tires
    // http://en.wikipedia.org/wiki/Drag_%28physics%29 -> Fd = 1/2 p v^2 Cd A
    // kg*m/s^2 = kg/m^3 * m/s * m/s * m^2 
    drag_newton = air_density * v * v * veh->drag_coeff;
    if (debug>=4) g->add_msg("drag_N = %d", int(drag_newton));

    // http://en.wikipedia.org/wiki/Downforce
    // kg*m/s^2 = kg/m^3 * m/s * m/s * m^2
    downforce_newton = 0.5 * air_density * v * v * veh->downforce;
    if (debug>=5) g->add_msg("downF_N = %d", int(downforce_newton));
    // Add gravity to downforce
    // kg*m/s^2 = 9.81 m/s^2(earth gravity) * kg
    downforce_newton += (mass * gravity);

    wheel_newton_max = tire_friction_coeff * downforce_newton;
    if (debug>=5) g->add_msg("max_wheel_N = %d", int(wheel_newton_max));
    // Allow more slowdown than wheels can handle, could be terrain or car frame slowing down
    ground_res_newton = ground_res_coeff * downforce_newton;
    if (debug>=4) g->add_msg("groundres_N = %d", int(ground_res_newton));

    // Find max force tires can handle. Don't make it go negative from rres
    wheel_newton_avail = std::max( wheel_newton_max - ground_res_newton, 0.0 );
    if (debug>=5) g->add_msg("avail_wheel_N = %d", int(wheel_newton_avail));

    if( v_dir < 0 )
        engine_newton_avail *= 0.6; // Less power available in reverse;
    is_thrusting = ( veh->velocity == 0 ||
                ( veh->velocity > 0 ^ fdir < 0 ) ); // XOR operator
    if( is_thrusting ) {
        engine_newton = std::min(fabs(engine_newton_avail * fdir), wheel_newton_avail);
        user_applied_newton = engine_newton * v_dir;
    } else {
        engine_newton = 0.0;
        user_applied_newton = wheel_newton_avail * fdir;
    }
    if( !valid_wheel_config )
        user_applied_newton = 0.0;
    if (debug>=3) g->add_msg("applied_N = %d", int(user_applied_newton));

    // Subtract drag/ground res if driving forwards, add them if driving backwards
    newton_total = user_applied_newton - (ground_res_newton + drag_newton) * v_dir;
    if (debug>=2) g->add_msg("total_N = %d", int(newton_total));

    return newton_total;
}

// Helper sort function for vehicle::collision
bool _compare_veh_collisions_mass (const veh_collision& first, const veh_collision& second)
{
    return (first.mass < second.mass);
}

/**
 * Calculate all the collisions a vehicle is involved in at once.
 * Called from map. If just_detect==true, return true the moment it finds any collition.
 * Otherwise stores all collisions in two vectors, one for vehicles and one for the rest.
 * Then starts processing non-vehicles in order from least mass til most until it's done
 * them all, runs out of velocity, or is destroyed.
 * Vehicles are then processed, which can add velocity to this vehicle again.
 * After all that it returns true, and map will look at its new heading/velocity and
 * move it into the right tile if needed. This repeats until no collisions occur or
 * vehicle loses all velocity and stops (or vehicle is destroyed).
 * Uses precalc_dx/y which shows the positions the car parts would be
 * in if they were allowed to move as expected.
 */
bool veh_physics::collision (int dx, int dy, bool just_detect)
{
    first_collision_name = "";
    std::list<veh_collision> veh_veh_colls;
    std::list<veh_collision> veh_misc_colls;
    std::vector<int> structural_indices = veh->all_parts_at_location(part_location_structure);
    for (int i = 0; i < structural_indices.size(); i++)
    {
        const int p = structural_indices[i];
        // coords of where part will go due to movement (dx/dy)
        // and turning (precalc_dx/dy [1])
        const int x = veh->parts[p].precalc_dx[1];
        const int y = veh->parts[p].precalc_dy[1];
        const int dsx = veh->global_x() + dx + x;
        const int dsy = veh->global_y() + dy + y;
        veh_collision coll = get_point_collision(dsx, dsy);  // TODO: Unsafe, fix
        coll.precalc_x = x;
        coll.precalc_y = y;
        if( coll.type != veh_coll_nothing ) {
            if( first_collision_name == "" ) {
                first_collision_name = coll.target_name;
            }
            if( just_detect ) {
                return true;
            }
        } else if (coll.type == veh_coll_veh) {
            veh_veh_colls.push_back(coll);
        } else if (coll.type != veh_coll_nothing) { //run over someone?
            veh_misc_colls.push_back(coll);
        }
    }
    if (veh_veh_colls.empty() && veh_misc_colls.empty())
        return false;
    veh_misc_colls.sort(_compare_veh_collisions_mass);
    for (std::list<veh_collision>::iterator i = veh_misc_colls.begin(); i != veh_misc_colls.end(); i++) {
        process_collision(*i);
    }
    
    return true;
}

veh_collision veh_physics::get_point_collision (int x, int y)
{
    veh_collision ret;
    ret.x = x;
    ret.y = y;
    ret.target_name = g->m.name(x, y).c_str();
    int target_part; // Not used here
    vehicle *oveh = g->m.veh_at(x, y, target_part);

    // Is it a vehicle collision?
    if (oveh && (oveh->posx != veh->posx || oveh->posy != veh->posy))
    {
       ret.type = veh_coll_veh;
       ret.target = oveh;
       ret.target_name = oveh->name.c_str();
       return ret;
    }

    // Is it a monster collision?
    int mondex = g->mon_at(x, y);
    if (mondex >= 0)
    {
        ret.type = veh_coll_body; // body
        monster *z = &g->zombie(mondex);
        ret.target = z;
        ret.target_name = z->name().c_str();
        ret.elasticity = 0.30f;
        ret.density = 15;
        switch (z->type->size)
        {
            case MS_TINY:    // Rodent
                ret.mass = 1;
                break;
            case MS_SMALL:   // Half human
                ret.mass = 41;
                break;
            default:
            case MS_MEDIUM:  // Human
                ret.mass = 82;
                break;
            case MS_LARGE:   // Cow
                ret.mass = 120;
                break;
            case MS_HUGE:     // TAAAANK
                ret.mass = 200;
                break;
            return ret;
        }
    }

    // Is it a player/npc collision?
    int npcind = g->npc_at(x, y);
    bool u_here = x == g->u.posx && y == g->u.posy && !g->u.in_vehicle;
    player *ph = (npcind >= 0 ? g->active_npc[npcind] : (u_here ? &g->u : 0));
    // if player is in a vehicle at this point (s)he's inside the vehicle we're checking for collisions
    if (ph && !ph->in_vehicle)
    {
        ret.type = veh_coll_body; // body
        ret.target = ph;
        ret.target_name = ph->name;
        ret.elasticity = 0.30f;
        ret.density = 15;
        ret.mass = 82; // player or NPC
        return ret;
    }

    // In order, starting with the worst thing to collide with
    ret.type = veh_coll_nothing;
    if (g->m.has_flag_ter_or_furn ("TREE", x, y)) {
        ret.type = veh_coll_destructable;
        ret.mass = std::max(dice(2, 9)-7, 2) * std::max(dice(2, 9)-7, 2) * std::max(dice(2, 9)-7, 2) * 100;
        ret.elasticity = 0.40f;
        ret.density = 60;
    } else if (g->m.move_cost_ter_furn(x, y) == 0) {
        if(g->m.is_destructable_ter_furn(x, y)) {
            ret.type = veh_coll_destructable; // destructible (wall)
            ret.mass = 200;
            ret.elasticity = 0.30f;
            ret.density = 60;
        } else {
            ret.type = veh_coll_other; // not destructible
            ret.mass = 1000;
            ret.elasticity = 0.10f;
            ret.density = 80;
        }
    } else if (g->m.has_flag_ter_or_furn ("YOUNG", x, y)) {
        ret.type = veh_coll_bashable;
        ret.mass = (dice(3, 7)-1) * 40;
        ret.elasticity = 0.60f;
        ret.density = 40;
    } else if (g->m.has_flag_ter_or_furn ("THIN_OBSTACLE", x, y)) {
        // if all above fails, go for terrain which might obstruct moving
        ret.type = veh_coll_thin_obstacle; // some fence
        ret.mass = 10;
        ret.elasticity = 0.30f;
        ret.density = 20;
    } else if (g->m.has_flag_ter_or_furn("BASHABLE", x, y)) {
        ret.type = veh_coll_bashable; // (door, window)
        ret.mass = 50;
        ret.elasticity = 0.30f;
        ret.density = 20;
    }
    if( ret.type != veh_coll_nothing ) {
        ret.target_name = g->m.ter_at(x, y).name;
    }
    return ret;
}

/*
        bool veh_veh_coll_flag = false;
        // Used to calculate the epicenter of the collision.
        point epicenter1(0, 0);
        point epicenter2(0, 0);
    
        if (veh_veh_colls.size()) { // we have dynamic crap!
            // effects of colliding with another vehicle:
            // transfers of momentum, skidding,
            // parts are damaged/broken on both sides,
            // remaining times are normalized,
            veh_veh_coll_flag = true;
            veh_collision c = veh_veh_colls[0]; // TODO: Should not ignore collision with any vehicle other than the one in veh_veh_colls[0]
            vehicle* veh2 = (vehicle*) c.target;
            g->add_msg(_("The %1$s's %2$s collides with the %3$s's %4$s."),
                       veh->name.c_str(),  veh->part_info(c.part).name.c_str(),
                       veh2->name.c_str(), veh2->part_info(c.target_part).name.c_str());
    
            // for reference, a cargo truck weighs ~25300, a bicycle 690,
            //  and 38mph is 3800 'velocity'
            rl_vec2d velo_veh1 = veh->velo_vec();
            rl_vec2d velo_veh2 = veh2->velo_vec();
            float m1 = veh->total_mass();
            float m2 = veh2->total_mass();
            //Energy of vehicle1 annd vehicle2 before collision
            float E = 0.5 * m1 * velo_veh1.norm() * velo_veh1.norm() +
                0.5 * m2 * velo_veh2.norm() * velo_veh2.norm();
    
            //collision_axis
            int x_cof1 = 0, y_cof1 = 0, x_cof2 = 0, y_cof2 = 0;
            veh ->center_of_mass(x_cof1, y_cof1);
            veh2->center_of_mass(x_cof2, y_cof2);
            rl_vec2d collision_axis_y;
    
            collision_axis_y.x = ( veh->global_x() + x_cof1 ) -  ( veh2->global_x() + x_cof2 );
            collision_axis_y.y = ( veh->global_y() + y_cof1 ) -  ( veh2->global_y() + y_cof2 );
            collision_axis_y = collision_axis_y.normalized();
            rl_vec2d collision_axis_x = collision_axis_y.get_vertical();
            // imp? & delta? & final? reworked:
            // newvel1 =( vel1 * ( mass1 - mass2 ) + ( 2 * mass2 * vel2 ) ) / ( mass1 + mass2 )
            // as per http://en.wikipedia.org/wiki/Elastic_collision
            //velocity of veh1 before collision in the direction of collision_axis_y
            float vel1_y = collision_axis_y.dot_product(velo_veh1);
            float vel1_x = collision_axis_x.dot_product(velo_veh1);
            //velocity of veh2 before collision in the direction of collision_axis_y
            float vel2_y = collision_axis_y.dot_product(velo_veh2);
            float vel2_x = collision_axis_x.dot_product(velo_veh2);
            // e = 0 -> inelastic collision
            // e = 1 -> elastic collision
            float e = get_collision_factor(vel1_y/100 - vel2_y/100);
    
            //velocity after collision
            float vel1_x_a = vel1_x;
            // vel1_x_a = vel1_x, because in x-direction we have no transmission of force
            float vel2_x_a = vel2_x;
            //transmission of force only in direction of collision_axix_y
            //equation: partially elastic collision
            float vel1_y_a = ( m2 * vel2_y * ( 1 + e ) + vel1_y * ( m1 - m2 * e) ) / ( m1 + m2);
            //equation: partially elastic collision
            float vel2_y_a = ( m1 * vel1_y * ( 1 + e ) + vel2_y * ( m2 - m1 * e) ) / ( m1 + m2);
            //add both components; Note: collision_axis is normalized
            rl_vec2d final1 = collision_axis_y * vel1_y_a + collision_axis_x * vel1_x_a;
            //add both components; Note: collision_axis is normalized
            rl_vec2d final2 = collision_axis_y * vel2_y_a + collision_axis_x * vel2_x_a;
    
            //Energy after collision
            float E_a = 0.5 * m1 * final1.norm() * final1.norm() +
                0.5 * m2 * final2.norm() * final2.norm();
            float d_E = E - E_a;  //Lost energy at collision -> deformation energy
            float dmg = abs( d_E / 1000 / 2000 );  //adjust to balance damage
            float dmg_veh1 = dmg * 0.5;
            float dmg_veh2 = dmg * 0.5;
    
            int coll_parts_cnt = 0; //quantity of colliding parts between veh1 and veh2
            for(int i = 0; i < veh_veh_colls.size(); i++) {
                veh_collision tmp_c = veh_veh_colls[i];
                if(veh2 == (vehicle*) tmp_c.target) { coll_parts_cnt++; }
            }
    
            float dmg1_part = dmg_veh1 / coll_parts_cnt;
            float dmg2_part = dmg_veh2 / coll_parts_cnt;
    
            //damage colliding parts (only veh1 and veh2 parts)
            for(int i = 0; i < veh_veh_colls.size(); i++) {
                veh_collision tmp_c = veh_veh_colls[i];
    
                if (veh2 == (vehicle*) tmp_c.target) { // Ignore all but the first colliding vehicle
                    
                    int parm1 = veh->part_with_feature (tmp_c.part, VPFLAG_ARMOR);
                    if (parm1 < 0) {
                        parm1 = tmp_c.part;
                    }
                    int parm2 = veh2->part_with_feature (tmp_c.target_part, VPFLAG_ARMOR);
                    if (parm2 < 0) {
                        parm2 = tmp_c.target_part;
                    }
                    epicenter1.x += veh->parts[parm1].mount_dx;
                    epicenter1.y += veh->parts[parm1].mount_dy;
                    veh->damage(parm1, dmg1_part, 1);
    
                    epicenter2.x += veh2->parts[parm2].mount_dx;
                    epicenter2.y += veh2->parts[parm2].mount_dy;
                    veh2->damage(parm2, dmg2_part, 1);
                }
            }
            epicenter1.x /= coll_parts_cnt;
            epicenter1.y /= coll_parts_cnt;
            epicenter2.x /= coll_parts_cnt;
            epicenter2.y /= coll_parts_cnt;
    
    
            if (dmg2_part > 100) {
                // shake veh because of collision
                veh2->damage_all(dmg2_part / 2, dmg2_part, 1, epicenter2);
            }
    
            dmg_1 += dmg1_part;
    
            veh->move.init (final1.x, final1.y);
            veh->velocity = final1.norm();
            // shrug it off if the change is less than 8mph.
            if(dmg_veh1 > 800) {
                veh->start_skid(0);
            }
            veh2->move.init(final2.x, final2.y);
            veh2->velocity = final2.norm();
            if(dmg_veh2 > 800) {
                veh2->start_skid(0);
            }
            //give veh2 the initiative to proceed next before veh1
            float avg_of_turn = (veh2->of_turn + veh->of_turn) / 2;
            if(avg_of_turn < .1f)
                avg_of_turn = .1f;
            veh->of_turn = avg_of_turn * .9;
            veh2->of_turn = avg_of_turn * 1.1;
        }
    
    
        int coll_turn = 0;
        if (dmg_1 > 0) {
            int vel1_a = veh->velocity / 100; //velocity of car after collision
            int d_vel = abs(vel1 - vel1_a);
    
            std::vector<int> ppl = veh->boarded_parts();
    
            for (int ps = 0; ps < ppl.size(); ps++) {
                player *psg = veh->get_passenger (ppl[ps]);
                if (!psg) {
                    debugmsg ("throw passenger: empty passenger at part %d", ppl[ps]);
                    continue;
                }
    
                bool throw_from_seat = 0;
                if (veh->part_with_feature (ppl[ps], VPFLAG_SEATBELT) == -1) {
                    throw_from_seat = d_vel * rng(80, 120) / 100 > (psg->str_cur * 1.5 + 5);
                }
    
                //damage passengers if d_vel is too high
                if(d_vel > 60* rng(50,100)/100 && !throw_from_seat) {
                    int dmg = d_vel/4*rng(70,100)/100;
                    psg->hurtall(dmg);
                    if (psg == &g->u) {
                        g->add_msg(_("You take %d damage by the power of the impact!"), dmg);
                    } else if (psg->name.length()) {
                        g->add_msg(_("%s takes %d damage by the power of the impact!"),
                                   psg->name.c_str(), dmg);
                    }
                }
    
                if (throw_from_seat) {
                    if (psg == &g->u) {
                        g->add_msg(_("You are hurled from the %s's seat by the power of the impact!"),
                                   veh->name.c_str());
                    } else if (psg->name.length()) {
                        g->add_msg(_("%s is hurled from the %s's seat by the power of the impact!"),
                                   psg->name.c_str(), veh->name.c_str());
                    }
                    unboard_vehicle(x + veh->parts[ppl[ps]].precalc_dx[0],
                                         y + veh->parts[ppl[ps]].precalc_dy[0]);
                    g->fling_player_or_monster(psg, 0, mdir.dir() + rng(0, 60) - 30,
                                               (vel1 - psg->str_cur < 10 ? 10 :
                                                vel1 - psg->str_cur));
                } else if (veh->part_with_feature (ppl[ps], "CONTROLS") >= 0) {
                    // FIXME: should actually check if passenger is in control,
                    // not just if there are controls there.
                    const int lose_ctrl_roll = rng (0, dmg_1);
                    if (lose_ctrl_roll > psg->dex_cur * 2 + psg->skillLevel("driving") * 3) {
                        if (psg == &g->u) {
                            g->add_msg(_("You lose control of the %s."), veh->name.c_str());
                        } else if (psg->name.length()) {
                            g->add_msg(_("%s loses control of the %s."), psg->name.c_str());
                        }
                        int turn_amount = (rng (1, 3) * sqrt((double)vel1_a) / 2) / 15;
                        if (turn_amount < 1) {
                            turn_amount = 1;
                        }
                        turn_amount *= 15;
                        if (turn_amount > 120) {
                            turn_amount = 120;
                        }
                        coll_turn = one_in (2)? turn_amount : -turn_amount;
                    }
                }
            }
        }
        if (coll_turn) {
            veh->start_skid(coll_turn);
        }
        if (veh_veh_coll_flag) return true;
    
    
*/
void veh_physics::process_collision (veh_collision coll)
{
    std::vector<int> structural_indices = veh->all_parts_at_location(part_location_structure);
    for (int i = 0; i < structural_indices.size(); i++)
    {
        const int p = structural_indices[i];
        if (coll.precalc_x == veh->parts[p].precalc_dx[1] && coll.precalc_y == veh->parts[p].precalc_dy[1]) {
            while (veh->parts[p].hp > 0 && apply_damage_from_collision_to_point(p, coll));
        }
    }
}

bool veh_physics::apply_damage_from_collision_to_point(int part, veh_collision coll)
{
    int mondex = g->mon_at(coll.x, coll.y);
    int npcind = g->npc_at(coll.x, coll.y);
    bool u_here = coll.x == g->u.posx && coll.y == g->u.posy && !g->u.in_vehicle;
    monster *z = mondex >= 0 ? &g->zombie(mondex) : NULL;
    player *ph = (npcind >= 0 ? g->active_npc[npcind] : (u_here? &g->u : 0));
                        
    float e = coll.elasticity;
    int x = coll.x, y = coll.y;
    bool pl_ctrl = veh->player_in_control( &g->u );

    // Damage armor before damaging any other parts
    int parm = veh->part_with_feature( part, VPFLAG_ARMOR );
    if (parm < 0)
        parm = part; // No armor found, use frame instead

    vpart_info vpi = veh->part_info(part);
    int dmg_mod = vpi.dmg_mod;
    int degree = rng (70, 100);

    // Calculate damage resulting from d_E
    material_type* vpart_item_mat1 = material_type::find_material(itypes[vpi.item]->m1);
    material_type* vpart_item_mat2 = material_type::find_material(itypes[vpi.item]->m2);
    int vpart_dens;
    if (vpart_item_mat2->ident() == "null") {
        vpart_dens = vpart_item_mat1->density();
    } else {
        vpart_dens = (vpart_item_mat1->density() + vpart_item_mat2->density())/2; //average
    }

    // k=100 -> 100% damage on part, k=0 -> 100% damage on obj
    float material_factor = (coll.density - vpart_dens)*0.5;
    if (material_factor >= 25) material_factor = 25; //saturation
    if (material_factor < -25) material_factor = -25;
    float weight_factor;
    // factor = -25 if mass is much greater than coll.mass
    if (mass >= coll.mass) weight_factor = -25 * ( log(mass) - log(coll.mass) ) / log(mass);
    // factor = +25 if coll.mass is much greater than mass
    else weight_factor = 25 * ( log(coll.mass) - log(mass) ) / log(coll.mass) ;

    float k = 50 + material_factor + weight_factor;
    if (k > 90) k = 90;  // saturation
    if (k < 10) k = 10;

    bool smashed = true;
    std::string snd;
    float part_dmg = 0.0;
    float dmg = 0.0;
    //Calculate Impulse of car
    const float prev_velocity = veh->velocity / 100;
    int turns_stunned = 0;
    bool is_sharp = veh->part_flag(part, "SHARP");

    do {
        // Impulse of object
        const float vel1 = veh->velocity / 100;

        // Assumption: velocitiy of hit object = 0 mph
        const float vel2 = 0;
        // lost energy at collision -> deformation energy -> damage
        const float d_E = ((mass*coll.mass)*(1-e)*(1-e)*(vel1-vel2)*(vel1-vel2)) / (2*mass + 2*coll.mass);
        // velocity of car after collision
        const float vel1_a = (coll.mass*vel2*(1+e) + vel1*(mass - e*coll.mass)) / (mass + coll.mass);
        // velocity of object after collision
        const float vel2_a = (mass*vel1*(1+e) + vel2*(coll.mass - e*mass)) / (mass + coll.mass);

        // Damage calculation
        // damage dealt overall
        dmg += abs(d_E / k_mvel);
        //damage for vehicle-part - only if not a hallucination
        if(!z || !z->is_hallucination()) {
            part_dmg = dmg * k / 100;
        }
        // damage for object
        const float obj_dmg  = dmg * (100-k)/100;

        if (coll.type == veh_coll_bashable) {
            // something bashable -- use map::bash to determine outcome
            int absorb = -1;
            g->m.bash(x, y, obj_dmg, snd, &absorb);
            smashed = obj_dmg > absorb;
        } else if (coll.type >= veh_coll_thin_obstacle) {
            // some other terrain
            smashed = obj_dmg > coll.mass;
            if (smashed) {
                // destroy obstacle
                switch (coll.type) {
                case veh_coll_thin_obstacle:
                    if (g->m.has_furn(x, y)) {
                        g->m.furn_set(x, y, f_null);
                    } else {
                        g->m.ter_set(x, y, t_dirt);
                    }
                    break;
                case veh_coll_destructable:
                    g->m.destroy(x, y, false);
                    snd = _("crash!");
                    break;
                case veh_coll_other:
                    smashed = false;
                    break;
                default:;
                }
            }
        }
        if (coll.type == veh_coll_body) {
            int dam = obj_dmg*dmg_mod/100;
            if (z) {
                int z_armor = is_sharp ? z->type->armor_cut : z->type->armor_bash;
                if (z_armor < 0) {
                    z_armor = 0;
                }
                dam -= z_armor;
            }
            if (dam < 0) { dam = 0; }

            // No blood from hallucinations
            if (z && !z->is_hallucination()) {
                if (is_sharp) {
                    veh->parts[part].blood += (20 + dam) * 5;
                } else if (dam > rng (10, 30)) {
                    veh->parts[part].blood += (10 + dam / 2) * 5;
                }
                veh->check_environmental_effects = true;
            }

            turns_stunned = rng (0, dam) > 10? rng (1, 2) + (dam > 40? rng (1, 2) : 0) : 0;
            if (is_sharp) {
                turns_stunned = 0;
            }
            if (turns_stunned > 6) {
                turns_stunned = 6;
            }
            if (turns_stunned > 0 && z) {
                z->add_effect("stunned", turns_stunned);
            }

            int angle = (100 - degree) * 2 * (one_in(2)? 1 : -1);
            if (z) {
                z->hurt(dam);

                if (vel2_a > rng (10, 20)) {
                    g->fling_player_or_monster (0, z, veh->move.dir() + angle, vel2_a);
                }
                if (z->hp < 1 || z->is_hallucination()) {
                    g->kill_mon (mondex, pl_ctrl);
                }
            } else {
                ph->hitall (dam, 40);
                if (vel2_a > rng (10, 20)) {
                    g->fling_player_or_monster (ph, 0, veh->move.dir() + angle, vel2_a);
                }
            }
        }

        if (vel1_a*100 != veh->velocity) {
            veh->velocity = vel1_a*100;
        } else {
            veh->velocity = 0;
        }

    } while (!smashed && veh->velocity != 0);

    // Apply special effects from collision.
    if (!(coll.type == veh_coll_body)) {
        if (pl_ctrl) {
            if (snd.length() > 0) {
                g->add_msg (_("Your %s's %s rams into a %s with a %s"), veh->name.c_str(),
                            vpi.name.c_str(), coll.target_name.c_str(), snd.c_str());
            } else {
                g->add_msg (_("Your %s's %s rams into a %s."), veh->name.c_str(),
                            vpi.name.c_str(), coll.target_name.c_str());
            }
        } else if (snd.length() > 0) {
            g->add_msg (_("You hear a %s"), snd.c_str());
        }
        g->sound(x, y, smashed? 80 : 50, "");
    } else {
        std::string dname;
        if (z) {
            dname = z->name().c_str();
        } else {
            dname = ph->name;
        }
        if (pl_ctrl) {
            g->add_msg (_("Your %s's %s rams into %s%s!"),
                        veh->name.c_str(), vpi.name.c_str(), dname.c_str(),
                        turns_stunned > 0 && z ? _(" and stuns it") : "");
        }

        if (is_sharp) {
            g->m.adjust_field_strength(point(x, y), fd_blood, 1 );
        } else {
            g->sound(x, y, 20, "");
        }
    }

    if (smashed) {
        int turn_amount = rng (1, 3) * sqrt ((double)dmg);
        turn_amount /= 15;
        if (turn_amount < 1) {
            turn_amount = 1;
        }
        turn_amount *= 15;
        if (turn_amount > 120) {
            turn_amount = 120;
        }
        int turn_roll = rng (0, 100);
        //probability of skidding increases with higher delta_v
        if (turn_roll < abs(prev_velocity - (float)(veh->velocity / 100)) * 2 ) {
            //delta_v = vel1 - vel1_a
            //delta_v = 50 mph -> 100% probability of skidding
            //delta_v = 25 mph -> 50% probability of skidding
            veh->start_skid(one_in (2)? turn_amount : -turn_amount);
        }
    }
    veh->damage (parm, part_dmg, 1);
    point collision_point(veh->parts[part].mount_dx, veh->parts[part].mount_dy);
    // Shock damage
    veh->damage_all(part_dmg / 2, part_dmg, 1, collision_point);
    return false;
}


