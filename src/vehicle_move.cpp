#include "vehicle.h" // IWYU pragma: associated

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <set>
#include <memory>
#include <ostream>

#include "avatar.h"
#include "debug.h"
#include "explosion.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "mapdata.h"
#include "material.h"
#include "messages.h"
#include "options.h"
#include "sounds.h"
#include "translations.h"
#include "trap.h"
#include "veh_type.h"
#include "bodypart.h"
#include "creature.h"
#include "math_defines.h"
#include "optional.h"
#include "player.h"
#include "rng.h"
#include "vpart_position.h"
#include "string_id.h"
#include "enums.h"
#include "int_id.h"
#include "monster.h"

static const std::string part_location_structure( "structure" );
static const itype_id fuel_type_muscle( "muscle" );
static const itype_id fuel_type_animal( "animal" );

const efftype_id effect_stunned( "stunned" );
const efftype_id effect_harnessed( "harnessed" );
const skill_id skill_driving( "driving" );

#define dbg(x) DebugLog((x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

// tile height in meters
static const float tile_height = 4;
// miles per hour to vehicle 100ths of miles per hour
static const int mi_to_vmi = 100;
// meters per second to miles per hour
static const float mps_to_miph = 2.23694f;

// convert m/s to vehicle 100ths of a mile per hour
int mps_to_vmiph( double mps )
{
    return mps * mps_to_miph * mi_to_vmi;
}

// convert vehicle 100ths of a mile per hour to m/s
double vmiph_to_mps( int vmiph )
{
    return vmiph / mps_to_miph / mi_to_vmi;
}

int cmps_to_vmiph( int cmps )
{
    return cmps * mps_to_miph;
}

int vmiph_to_cmps( int vmiph )
{
    return vmiph / mps_to_miph;
}

int vehicle::slowdown( int at_velocity ) const
{
    double mps = vmiph_to_mps( abs( at_velocity ) );

    // slowdown due to air resistance is proportional to square of speed
    double f_total_drag = coeff_air_drag() * mps * mps;
    if( is_watercraft() ) {
        // same with water resistance
        f_total_drag += coeff_water_drag() * mps * mps;
    } else if( !is_falling ) {
        // slowdown due to rolling resistance is proportional to speed
        double f_rolling_drag = coeff_rolling_drag() * ( vehicles::rolling_constant_to_variable + mps );
        // increase rolling resistance by up to 25x if the vehicle is skidding at right angle to facing
        const double skid_factor = 1 + 24 * std::abs( sin( DEGREES( face.dir() - move.dir() ) ) );
        f_total_drag += f_rolling_drag * skid_factor;
    }
    double accel_slowdown = f_total_drag / to_kilogram( total_mass() );
    // converting m/s^2 to vmiph/s
    int slowdown = mps_to_vmiph( accel_slowdown );
    if( slowdown < 0 ) {
        debugmsg( "vehicle %s has negative drag slowdown %d\n", name, slowdown );
    }
    add_msg( m_debug, "%s at %d vimph, f_drag %3.2f, drag accel %d vmiph - extra drag %d",
             name, at_velocity, f_total_drag, slowdown, static_drag() );
    // plows slow rolling vehicles, but not falling or floating vehicles
    if( !( is_falling || is_floating ) ) {
        slowdown -= static_drag();
    }

    return std::max( 1, slowdown );
}

void vehicle::thrust( int thd )
{
    //if vehicle is stopped, set target direction to forward.
    //ensure it is not skidding. Set turns used to 0.
    if( !is_moving() ) {
        turn_dir = face.dir();
        stop();
    }

    bool pl_ctrl = player_in_control( g->u );

    // No need to change velocity if there are no wheels
    if( in_water && can_float() ) {
        // we're good
    } else if( is_floating && !can_float() ) {
        if( pl_ctrl ) {
            add_msg( _( "The %s is too leaky!" ), name );
        }
        return;
    } else if( !valid_wheel_config() ) {
        if( pl_ctrl ) {
            add_msg( _( "The %s doesn't have enough wheels to move!" ), name );
        }
        return;
    }
    // Accelerate (true) or brake (false)
    bool thrusting = true;
    if( velocity ) {
        int sgn = ( velocity < 0 ) ? -1 : 1;
        thrusting = ( sgn == thd );
    }

    // TODO: Pass this as an argument to avoid recalculating
    float traction = k_traction( g->m.vehicle_wheel_traction( *this ) );
    int accel = current_acceleration() * traction;
    if( thrusting && accel == 0 ) {
        if( pl_ctrl ) {
            add_msg( _( "The %s is too heavy for its engine(s)!" ), name );
        }
        return;
    }
    int max_vel = traction * max_velocity();

    // Get braking power
    int brk = std::max( 1000, abs( max_vel ) * 3 / 10 );

    //pos or neg if accelerator or brake
    int vel_inc = ( ( thrusting ) ? accel : brk ) * thd;
    if( thd == -1 && thrusting ) {
        //accelerate 60% if going backward
        vel_inc = .6 * vel_inc;
    }

    //find power ratio used of engines max
    int load;
    // Keep exact cruise control speed
    if( cruise_on ) {
        int effective_cruise = std::min( cruise_velocity, max_vel );
        if( thd > 0 ) {
            vel_inc = std::min( vel_inc, effective_cruise - velocity );
            //find power ratio used of engines max
            load = 1000 * std::max( 0, vel_inc ) / std::max( ( thrusting ? accel : brk ), 1 );
        } else {
            vel_inc = std::max( vel_inc, effective_cruise - velocity );
            load = 1000 * std::min( 0, vel_inc ) / std::max( ( thrusting ? accel : brk ), 1 );
        }
    } else {
        load = ( thrusting ? 1000 : 0 );
    }

    // only consume resources if engine accelerating
    if( load >= 1 && thrusting ) {
        //abort if engines not operational
        if( total_power_w() <= 0 || !engine_on || accel == 0 ) {
            if( pl_ctrl ) {
                if( total_power_w( false ) <= 0 ) {
                    add_msg( m_info, _( "The %s doesn't have an engine!" ), name );
                } else if( has_engine_type( fuel_type_muscle, true ) ) {
                    add_msg( m_info, _( "The %s's mechanism is out of reach!" ), name );
                } else if( !engine_on ) {
                    add_msg( _( "The %s's engine isn't on!" ), name );
                } else if( traction < 0.01f ) {
                    add_msg( _( "The %s is stuck." ), name );
                } else {
                    add_msg( _( "The %s's engine emits a sneezing sound." ), name );
                }
            }
            cruise_velocity = 0;
            return;
        }

        //make noise and consume fuel
        noise_and_smoke( load );
        consume_fuel( load, 1 );

        //break the engines a bit, if going too fast.
        int strn = static_cast<int>( strain() * strain() * 100 );
        for( size_t e = 0; e < engines.size(); e++ ) {
            do_engine_damage( e, strn );
        }
    }

    //wheels aren't facing the right way to change velocity properly
    //lower down, since engines should be getting damaged anyway
    if( skidding ) {
        return;
    }

    //change vehicles velocity
    if( ( velocity > 0 && velocity + vel_inc < 0 ) || ( velocity < 0 && velocity + vel_inc > 0 ) ) {
        //velocity within braking distance of 0
        stop();
    } else {
        // Increase velocity up to max_vel or min_vel, but not above.
        const int min_vel = -max_vel / 4;
        if( vel_inc > 0 ) {
            // Don't allow braking by accelerating (could happen with damaged engines)
            velocity = std::max( velocity, std::min( velocity + vel_inc, max_vel ) );
        } else {
            velocity = std::min( velocity, std::max( velocity + vel_inc, min_vel ) );
        }
    }
    // If you are going faster than the animal can handle, harness is damaged
    // Animal may come free ( and possibly hit by vehicle )
    for( size_t e = 0; e < parts.size(); e++ ) {
        const vehicle_part &vp = parts[ e ];
        if( vp.info().fuel_type == fuel_type_animal && engines.size() != 1 ) {
            monster *mon = get_pet( e );
            if( mon != nullptr && mon->has_effect( effect_harnessed ) ) {
                if( velocity > mon->get_speed() * 12 ) {
                    add_msg( m_bad, _( "Your %s is not fast enough to keep up with the %s" ), mon->get_name(), name );
                    int dmg = rng( 0, 10 );
                    damage_direct( e, dmg );
                }
            }
        }
    }
}

void vehicle::cruise_thrust( int amount )
{
    if( amount == 0 ) {
        return;
    }
    int safe_vel = safe_velocity();
    int max_vel = max_velocity();
    int max_rev_vel = -max_vel / 4;

    //if the safe velocity is between the cruise velocity and its next value, set to safe velocity
    if( ( cruise_velocity < safe_vel && safe_vel < ( cruise_velocity + amount ) ) ||
        ( cruise_velocity > safe_vel && safe_vel > ( cruise_velocity + amount ) ) ) {
        cruise_velocity = safe_vel;
    } else {
        if( amount < 0 && ( cruise_velocity == safe_vel || cruise_velocity == max_vel ) ) {
            // If coming down from safe_velocity or max_velocity decrease by one so
            // the rounding below will drop velocity to a multiple of amount.
            cruise_velocity += -1;
        } else if( amount > 0 && cruise_velocity == max_rev_vel ) {
            // If increasing from max_rev_vel, do the opposite.
            cruise_velocity += 1;
        } else {
            // Otherwise just add the amount.
            cruise_velocity += amount;
        }
        // Integer round to lowest multiple of amount.
        // The result is always equal to the original or closer to zero,
        // even if negative
        cruise_velocity = ( cruise_velocity / abs( amount ) ) * abs( amount );
    }
    // Can't have a cruise speed faster than max speed
    // or reverse speed faster than max reverse speed.
    if( cruise_velocity > max_vel ) {
        cruise_velocity = max_vel;
    } else if( cruise_velocity < max_rev_vel ) {
        cruise_velocity = max_rev_vel;
    }
}

void vehicle::turn( int deg )
{
    if( deg == 0 ) {
        return;
    }
    if( velocity < 0 && !::get_option<bool>( "REVERSE_STEERING" ) ) {
        deg = -deg;
    }
    last_turn = deg;
    turn_dir += deg;
    if( turn_dir < 0 ) {
        turn_dir += 360;
    }
    if( turn_dir >= 360 ) {
        turn_dir -= 360;
    }
    // quick rounding the turn dir to a multiple of 15
    turn_dir = 15 * ( ( turn_dir * 2 + 15 ) / 30 );
}

void vehicle::stop( bool update_cache )
{
    velocity = 0;
    skidding = false;
    move = face;
    last_turn = 0;
    of_turn_carry = 0;
    if( !update_cache ) {
        return;
    }
    for( const tripoint &p : get_points() ) {
        g->m.set_memory_seen_cache_dirty( p );
    }
}

bool vehicle::collision( std::vector<veh_collision> &colls,
                         const tripoint &dp,
                         bool just_detect, bool bash_floor )
{

    /*
     * Big TODO:
     * Rewrite this function so that it has "pre-collision" phase (detection)
     *  and "post-collision" phase (applying damage).
     * Then invoke the functions cyclically (pre-post-pre-post-...) until
     *  velocity == 0 or no collision happens.
     * Make all post-collisions in a given phase use the same momentum.
     *
     * How it works right now: find the first obstacle, then ram it over and over
     *  until either the obstacle is removed or the vehicle stops.
     * Bug: when ramming a critter without enough force to send it flying,
     *  the vehicle will phase into it.
     */

    if( dp.z != 0 && ( dp.x != 0 || dp.y != 0 ) ) {
        // Split into horizontal + vertical
        return collision( colls, tripoint( dp.xy(), 0 ), just_detect, bash_floor ) ||
               collision( colls, tripoint( 0,    0,    dp.z ), just_detect, bash_floor );
    }

    if( dp.z == -1 && !bash_floor ) {
        // First check current level, then the one below if current had no collisions
        // Bash floors on the current one, but not on the one below.
        if( collision( colls, tripoint_zero, just_detect, true ) ) {
            return true;
        }
    }

    const bool vertical = bash_floor || dp.z != 0;
    const int &coll_velocity = vertical ? vertical_velocity : velocity;
    if( !just_detect && coll_velocity == 0 ) {
        debugmsg( "Collision check on stationary vehicle %s", name );
        just_detect = true;
    }

    const int velocity_before = coll_velocity;
    const int sign_before = sgn( velocity_before );
    bool empty = true;
    for( int p = 0; static_cast<size_t>( p ) < parts.size(); p++ ) {
        if( part_info( p ).location != part_location_structure || parts[ p ].removed ) {
            continue;
        }
        empty = false;
        // Coordinates of where part will go due to movement (dx/dy/dz)
        //  and turning (precalc[1])
        const tripoint dsp = global_pos3() + dp + parts[p].precalc[1];
        veh_collision coll = part_collision( p, dsp, just_detect, bash_floor );
        if( coll.type == veh_coll_nothing ) {
            continue;
        }

        colls.push_back( coll );

        if( just_detect ) {
            // DO insert the first collision so we can tell what was it
            return true;
        }

        const int velocity_after = coll_velocity;
        // A hack for falling vehicles: restore the velocity so that it hits at full force everywhere
        // TODO: Make this more elegant
        if( vertical ) {
            vertical_velocity = velocity_before;
        } else if( sgn( velocity_after ) != sign_before ) {
            // Sign of velocity inverted, collisions would be in wrong direction
            break;
        }
    }

    if( empty ) {
        // Hack for dirty vehicles that didn't yet get properly removed
        veh_collision fake_coll;
        fake_coll.type = veh_coll_other;
        colls.push_back( fake_coll );
        velocity = 0;
        vertical_velocity = 0;
        add_msg( m_debug, "Collision check on a dirty vehicle %s", name );
        return true;
    }

    return !colls.empty();
}

// A helper to make sure mass and density is always calculated the same way
static void terrain_collision_data( const tripoint &p, bool bash_floor,
                                    float &mass, float &density, float &elastic )
{
    elastic = 0.30;
    // Just a rough rescale for now to obtain approximately equal numbers
    const int bash_min = g->m.bash_resistance( p, bash_floor );
    const int bash_max = g->m.bash_strength( p, bash_floor );
    mass = ( bash_min + bash_max ) / 2.0;
    density = bash_min;
}

veh_collision vehicle::part_collision( int part, const tripoint &p,
                                       bool just_detect, bool bash_floor )
{
    // Vertical collisions need to be handled differently
    // All collisions have to be either fully vertical or fully horizontal for now
    const bool vert_coll = bash_floor || p.z != sm_pos.z;
    const bool pl_ctrl = player_in_control( g->u );
    Creature *critter = g->critter_at( p, true );
    player *ph = dynamic_cast<player *>( critter );

    Creature *driver = pl_ctrl ? &g->u : nullptr;

    // If in a vehicle assume it's this one
    if( ph != nullptr && ph->in_vehicle ) {
        critter = nullptr;
        ph = nullptr;
    }

    const optional_vpart_position ovp = g->m.veh_at( p );
    // Disable vehicle/critter collisions when bashing floor
    // TODO: More elegant code
    const bool is_veh_collision = !bash_floor && ovp && &ovp->vehicle() != this;
    const bool is_body_collision = !bash_floor && critter != nullptr;

    veh_collision ret;
    ret.type = veh_coll_nothing;
    ret.part = part;

    // Vehicle collisions are a special case. just return the collision.
    // The map takes care of the dynamic stuff.
    if( is_veh_collision ) {
        ret.type = veh_coll_veh;
        //"imp" is too simplistic for vehicle-vehicle collisions
        ret.target = &ovp->vehicle();
        ret.target_part = ovp->part_index();
        ret.target_name = ovp->vehicle().disp_name();
        return ret;
    }

    // Non-vehicle collisions can't happen when the vehicle is not moving
    int &coll_velocity = vert_coll ? vertical_velocity : velocity;
    if( !just_detect && coll_velocity == 0 ) {
        return ret;
    }

    if( is_body_collision ) {
        // critters on a BOARDABLE part in this vehicle aren't colliding
        if( ovp && ( &ovp->vehicle() == this ) && get_pet( ovp->part_index() ) ) {
            return ret;
        }
        // we just ran into a fish, so move it out of the way
        if( g->m.has_flag( "SWIMMABLE", critter->pos() ) ) {
            tripoint end_pos = critter->pos();
            tripoint start_pos;
            const int angle = move.dir() + 45 * ( parts[part].mount.x > pivot_point().x ? -1 : 1 );
            std::set<tripoint> &cur_points = get_points( true );
            // push the animal out of way until it's no longer in our vehicle and not in
            // anyone else's position
            while( g->critter_at( end_pos, true ) ||
                   cur_points.find( end_pos ) != cur_points.end() ) {
                start_pos = end_pos;
                calc_ray_end( angle, 2, start_pos, end_pos );
            }
            critter->setpos( end_pos );
            return ret;
        }
    }

    // Damage armor before damaging any other parts
    // Actually target, not just damage - spiked plating will "hit back", for example
    const int armor_part = part_with_feature( ret.part, VPFLAG_ARMOR, true );
    if( armor_part >= 0 ) {
        ret.part = armor_part;
    }

    int dmg_mod = part_info( ret.part ).dmg_mod;
    // Let's calculate type of collision & mass of object we hit
    float mass2 = 0;
    float e = 0.3; // e = 0 -> plastic collision
    // e = 1 -> inelastic collision
    float part_dens = 0; //part density

    if( is_body_collision ) {
        // Check any monster/NPC/player on the way
        ret.type = veh_coll_body; // body
        ret.target = critter;
        e = 0.30;
        part_dens = 15;
        mass2 = units::to_kilogram( critter->get_weight() );
        ret.target_name = critter->disp_name();
    } else if( ( bash_floor && g->m.is_bashable_ter_furn( p, true ) ) ||
               ( g->m.is_bashable_ter_furn( p, false ) && g->m.move_cost_ter_furn( p ) != 2 &&
                 // Don't collide with tiny things, like flowers, unless we have a wheel in our space.
                 ( part_with_feature( ret.part, VPFLAG_WHEEL, true ) >= 0 ||
                   !g->m.has_flag_ter_or_furn( "TINY", p ) ) &&
                 // Protrusions don't collide with short terrain.
                 // Tiny also doesn't, but it's already excluded unless there's a wheel present.
                 !( part_with_feature( ret.part, "PROTRUSION", true ) >= 0 &&
                    g->m.has_flag_ter_or_furn( "SHORT", p ) ) &&
                 // These are bashable, but don't interact with vehicles.
                 !g->m.has_flag_ter_or_furn( "NOCOLLIDE", p ) &&
                 // Do not collide with track tiles if we can use rails
                 !( g->m.has_flag_ter_or_furn( TFLAG_RAIL, p ) && this->can_use_rails() ) ) ) {
        // Movecost 2 indicates flat terrain like a floor, no collision there.
        ret.type = veh_coll_bashable;
        terrain_collision_data( p, bash_floor, mass2, part_dens, e );
        ret.target_name = g->m.disp_name( p );
    } else if( g->m.impassable_ter_furn( p ) ||
               ( bash_floor && !g->m.has_flag( TFLAG_NO_FLOOR, p ) ) ) {
        ret.type = veh_coll_other; // not destructible
        mass2 = 1000;
        e = 0.10;
        part_dens = 80;
        ret.target_name = g->m.disp_name( p );
    }

    if( ret.type == veh_coll_nothing || just_detect ) {
        // Hit nothing or we aren't actually hitting
        return ret;
    }

    // Calculate mass AFTER checking for collision
    //  because it involves iterating over all cargo
    const float mass = to_kilogram( total_mass() );

    //Calculate damage resulting from d_E
    const itype *type = item::find_type( part_info( ret.part ).item );
    const auto &mats = type->materials;
    float vpart_dens = 0;
    if( !mats.empty() ) {
        for( auto &mat_id : mats ) {
            vpart_dens += mat_id.obj().density();
        }
        vpart_dens /= mats.size(); // average
    }

    //k=100 -> 100% damage on part
    //k=0 -> 100% damage on obj
    float material_factor = ( part_dens - vpart_dens ) * 0.5;
    material_factor = std::max( -25.0f, std::min( 25.0f, material_factor ) );
    // factor = -25 if mass is much greater than mass2
    // factor = +25 if mass2 is much greater than mass
    const float weight_factor = mass >= mass2 ?
                                -25 * ( log( mass ) - log( mass2 ) ) / log( mass ) :
                                25 * ( log( mass2 ) - log( mass ) ) / log( mass2 );

    float k = 50 + material_factor + weight_factor;
    k = std::max( 10.0f, std::min( 90.0f, k ) );

    bool smashed = true;
    std::string snd = "Smash!"; // NOTE: Unused!
    float dmg = 0.0f;
    float part_dmg = 0.0f;
    // Calculate Impulse of car
    time_duration time_stunned = 0_turns;

    const int prev_velocity = coll_velocity;
    const int vel_sign = sgn( coll_velocity );
    // Velocity of the object we're hitting
    // Assuming it starts at 0, but we'll probably hit it many times
    // in one collision, so accumulate the velocity gain from each hit.
    float vel2 = 0.0f;
    do {
        smashed = false;
        // Impulse of vehicle
        const float vel1 = coll_velocity / 100.0f;
        // Velocity of car after collision
        const float vel1_a = ( mass * vel1 + mass2 * vel2 + e * mass2 * ( vel2 - vel1 ) ) /
                             ( mass + mass2 );
        // Velocity of object after collision
        const float vel2_a = ( mass * vel1 + mass2 * vel2 + e * mass * ( vel1 - vel2 ) ) / ( mass + mass2 );
        // Lost energy at collision -> deformation energy -> damage
        const float E_before = 0.5f * ( mass * vel1 * vel1 )     + 0.5f * ( mass2 * vel2 * vel2 );
        const float E_after  = 0.5f * ( mass * vel1_a * vel1_a ) + 0.5f * ( mass2 * vel2_a * vel2_a );
        const float d_E = E_before - E_after;
        if( d_E <= 0 ) {
            // Deformation energy is signed
            // If it's negative, it means something went wrong
            // But it still does happen sometimes...
            if( fabs( vel1_a ) < fabs( vel1 ) ) {
                // Lower vehicle's speed to prevent infinite loops
                coll_velocity = vel1_a * 90;
            }
            if( fabs( vel2_a ) > fabs( vel2 ) ) {
                vel2 = vel2_a;
            }
            if( mass2 == 0 ) { // this causes infinite loop
                mass2 = 1;
            }
            continue;
        }

        add_msg( m_debug, "Deformation energy: %.2f", d_E );
        // Damage calculation
        // Damage dealt overall
        dmg += d_E / 400;
        // Damage for vehicle-part
        // Always if no critters, otherwise if critter is real
        if( critter == nullptr || !critter->is_hallucination() ) {
            part_dmg = dmg * k / 100;
            add_msg( m_debug, "Part collision damage: %.2f", part_dmg );
        }
        // Damage for object
        const float obj_dmg = dmg * ( 100 - k ) / 100;

        if( ret.type == veh_coll_other ) {
        } else if( ret.type == veh_coll_bashable ) {
            // Something bashable -- use map::bash to determine outcome
            // NOTE: Floor bashing disabled for balance reasons
            //       Floor values are still used to set damage dealt to vehicle
            smashed = g->m.is_bashable_ter_furn( p, false ) &&
                      g->m.bash_resistance( p, bash_floor ) <= obj_dmg &&
                      g->m.bash( p, obj_dmg, false, false, false, this ).success;
            if( smashed ) {
                if( g->m.is_bashable_ter_furn( p, bash_floor ) ) {
                    // There's new terrain there to smash
                    smashed = false;
                    terrain_collision_data( p, bash_floor, mass2, part_dens, e );
                    ret.target_name = g->m.disp_name( p );
                } else if( g->m.impassable_ter_furn( p ) ) {
                    // There's new terrain there, but we can't smash it!
                    smashed = false;
                    ret.type = veh_coll_other;
                    mass2 = 1000;
                    e = 0.10;
                    part_dens = 80;
                    ret.target_name = g->m.disp_name( p );
                }
            }
        } else if( ret.type == veh_coll_body ) {
            int dam = obj_dmg * dmg_mod / 100;

            // No blood from hallucinations
            if( critter != nullptr && !critter->is_hallucination() ) {
                if( part_flag( ret.part, "SHARP" ) ) {
                    parts[ret.part].blood += ( 20 + dam ) * 5;
                } else if( dam > rng( 10, 30 ) ) {
                    parts[ret.part].blood += ( 10 + dam / 2 ) * 5;
                }

                check_environmental_effects = true;
            }

            time_stunned = time_duration::from_turns( ( rng( 0, dam ) > 10 ) + ( rng( 0, dam ) > 40 ) );
            if( time_stunned > 0_turns ) {
                critter->add_effect( effect_stunned, time_stunned );
            }

            if( ph != nullptr ) {
                ph->hitall( dam, 40, driver );
            } else {
                const int armor = part_flag( ret.part, "SHARP" ) ?
                                  critter->get_armor_cut( bp_torso ) :
                                  critter->get_armor_bash( bp_torso );
                dam = std::max( 0, dam - armor );
                critter->apply_damage( driver, bp_torso, dam );
                add_msg( m_debug, "Critter collision damage: %d", dam );
            }

            // Don't fling if vertical - critter got smashed into the ground
            if( !vert_coll ) {
                if( fabs( vel2_a ) > 10.0f ||
                    fabs( e * mass * vel1_a ) > fabs( mass2 * ( 10.0f - vel2_a ) ) ) {
                    const int angle = rng( -60, 60 );
                    // Also handle the weird case when we don't have enough force
                    // but still have to push (in such case compare momentum)
                    const float push_force = std::max<float>( fabs( vel2_a ), 10.1f );
                    // move.dir is where the vehicle is facing. If velocity is negative,
                    // we're moving backwards and have to adjust the angle accordingly.
                    const int angle_sum = angle + move.dir() + ( vel2_a > 0 ? 0 : 180 );
                    g->fling_creature( critter, angle_sum, push_force );
                } else if( fabs( vel2_a ) > fabs( vel2 ) ) {
                    vel2 = vel2_a;
                } else {
                    // Vehicle's momentum isn't big enough to push the critter
                    velocity = 0;
                    break;
                }

                if( critter->is_dead_state() ) {
                    smashed = true;
                } else if( critter != nullptr ) {
                    // Only count critter as pushed away if it actually changed position
                    smashed = critter->pos() != p;
                }
            }
        }

        if( critter == nullptr || !critter->is_hallucination() ) {
            coll_velocity = vel1_a * ( smashed ? 100 : 90 );
        }
        // Stop processing when sign inverts, not when we reach 0
    } while( !smashed && sgn( coll_velocity ) == vel_sign );

    // Apply special effects from collision.
    if( critter != nullptr ) {
        if( !critter->is_hallucination() ) {
            if( pl_ctrl ) {
                if( time_stunned > 0_turns ) {
                    //~ 1$s - vehicle name, 2$s - part name, 3$s - NPC or monster
                    add_msg( m_warning, _( "Your %1$s's %2$s rams into %3$s and stuns it!" ),
                             name, parts[ ret.part ].name(), ret.target_name );
                } else {
                    //~ 1$s - vehicle name, 2$s - part name, 3$s - NPC or monster
                    add_msg( m_warning, _( "Your %1$s's %2$s rams into %3$s!" ),
                             name, parts[ ret.part ].name(), ret.target_name );
                }
            }

            if( part_flag( ret.part, "SHARP" ) ) {
                critter->bleed();
            } else {
                sounds::sound( p, 20, sounds::sound_t::combat, snd, false, "smash_success", "hit_vehicle" );
            }
        }
    } else {
        if( pl_ctrl ) {
            if( snd.length() > 0 ) { // TODO: that is always false!
                //~ 1$s - vehicle name, 2$s - part name, 3$s - collision object name, 4$s - sound message
                add_msg( m_warning, _( "Your %1$s's %2$s rams into %3$s with a %4$s" ),
                         name, parts[ ret.part ].name(), ret.target_name, snd );
            } else {
                //~ 1$s - vehicle name, 2$s - part name, 3$s - collision object name
                add_msg( m_warning, _( "Your %1$s's %2$s rams into %3$s." ),
                         name, parts[ ret.part ].name(), ret.target_name );
            }
        }

        sounds::sound( p, smashed ? 80 : 50, sounds::sound_t::combat, snd, false, "smash_success",
                       "hit_vehicle" );
    }

    if( smashed && !vert_coll ) {
        int turn_amount = rng( 1, 3 ) * sqrt( static_cast<double>( part_dmg ) );
        turn_amount /= 15;
        if( turn_amount < 1 ) {
            turn_amount = 1;
        }
        turn_amount *= 15;
        if( turn_amount > 120 ) {
            turn_amount = 120;
        }
        int turn_roll = rng( 0, 100 );
        // Probability of skidding increases with higher delta_v
        if( turn_roll < std::abs( ( prev_velocity - coll_velocity ) / 100.0f * 2.0f ) ) {
            //delta_v = vel1 - vel1_a
            //delta_v = 50 mph -> 100% probability of skidding
            //delta_v = 25 mph -> 50% probability of skidding
            skidding = true;
            turn( one_in( 2 ) ? turn_amount : -turn_amount );
        }
    }

    ret.imp = part_dmg;
    return ret;
}

void vehicle::handle_trap( const tripoint &p, int part )
{
    int pwh = part_with_feature( part, VPFLAG_WHEEL, true );
    if( pwh < 0 ) {
        return;
    }
    const trap &tr = g->m.tr_at( p );
    const trap_id t = tr.loadid;

    if( t == tr_null ) {
        // If the trap doesn't exist, we can't interact with it, so just return
        return;
    }
    vehicle_handle_trap_data veh_data = tr.vehicle_data;

    if( veh_data.is_falling ) {
        return;
    }

    const bool seen = g->u.sees( p );
    const bool known = g->u.knows_trap( p );
    if( seen ) {
        if( known ) {
            //~ %1$s: name of the vehicle; %2$s: name of the related vehicle part; %3$s: trap name
            add_msg( m_bad, _( "The %1$s's %2$s runs over %3$s." ), name, parts[ part ].name(), tr.name() );
        } else {
            add_msg( m_bad, _( "The %1$s's %2$s runs over something." ), name, parts[ part ].name() );
        }
    }

    if( veh_data.chance >= rng( 1, 100 ) ) {
        if( veh_data.sound_volume > 0 ) {
            sounds::sound( p, veh_data.sound_volume, sounds::sound_t::combat, veh_data.sound, false,
                           veh_data.sound_type, veh_data.sound_variant );
        }
        if( veh_data.do_explosion ) {
            explosion_handler::explosion( p, veh_data.damage, 0.5f, false, veh_data.shrapnel );
        } else {
            // Hit the wheel directly since it ran right over the trap.
            damage_direct( pwh, veh_data.damage );
        }
        bool still_has_trap = true;
        if( veh_data.remove_trap || veh_data.do_explosion ) {
            g->m.remove_trap( p );
            still_has_trap = false;
        }
        for( const auto &it : veh_data.spawn_items ) {
            int cnt = roll_remainder( it.second );
            if( cnt > 0 ) {
                g->m.spawn_item( p, it.first, cnt );
            }
        }
        if( veh_data.set_trap ) {
            g->m.trap_set( p, veh_data.set_trap.id() );
            still_has_trap = true;
        }
        if( still_has_trap ) {
            const trap &tr = g->m.tr_at( p );
            if( seen || known ) {
                // known status has been reset by map::trap_set()
                g->u.add_known_trap( p, tr );
            }
            if( seen && !known ) {
                // hard to miss!
                const std::string direction = direction_name( direction_from( g->u.pos(), p ) );
                add_msg( _( "You've spotted a %1$s to the %2$s!" ), tr.name(), direction );
            }
        }
    }
}

void vehicle::autodrive( int x, int y )
{
    // for now, autodriving is only possible when pulled by an animal
    for( size_t e = 0; e < parts.size(); e++ ) {
        const vehicle_part &vp = parts[ e ];
        if( vp.info().fuel_type == fuel_type_animal ) {
            monster *mon = get_pet( e );
            if( !mon || !mon->has_effect( effect_harnessed ) ) {
                is_following = false;
            }
        }
    }
    int turn_delta = 15 * x;
    const float handling_diff = handling_difficulty();
    if( turn_delta != 0 ) {
        float eff = steering_effectiveness();
        if( eff == -2 ) {
            return;
        }

        if( eff < 0 ) {
            return;
        }

        if( eff == 0 ) {
            return;
        }
        turn( turn_delta );

    }

    if( y != 0 ) {
        int thr_amount = 100 * ( abs( velocity ) < 2000 ? 4 : 5 );
        if( cruise_on ) {
            cruise_thrust( -y * thr_amount );
        } else {
            thrust( -y );
        }
    }

    // TODO: Actually check if we're on land on water (or disable water-skidding)
    if( skidding && valid_wheel_config() ) {
        ///\EFFECT_DEX increases chance of regaining control of a vehicle

        ///\EFFECT_DRIVING increases chance of regaining control of a vehicle
        if( handling_diff * rng( 1, 10 ) < 15 ) {
            velocity = static_cast<int>( forward_velocity() );
            skidding = false;
            move.init( turn_dir );
        }
    }
}

void vehicle::pldrive( const point &p )
{
    player &u = g->u;
    int turn_delta = 15 * p.x;
    const float handling_diff = handling_difficulty();
    if( turn_delta != 0 ) {
        float eff = steering_effectiveness();
        if( eff == -2 ) {
            add_msg( m_info, _( "You cannot steer an animal-drawn vehicle with no animal harnessed." ) );
            return;
        }

        if( eff < 0 ) {
            add_msg( m_info, _( "This vehicle has no steering system installed, you can't turn it." ) );
            return;
        }

        if( eff == 0 ) {
            add_msg( m_bad, _( "The steering is completely broken!" ) );
            return;
        }

        // If you've got more moves than speed, it's most likely time stop
        // Let's get rid of that
        u.moves = std::min( u.moves, u.get_speed() );

        ///\EFFECT_DEX reduces chance of losing control of vehicle when turning

        ///\EFFECT_PER reduces chance of losing control of vehicle when turning

        ///\EFFECT_DRIVING reduces chance of losing control of vehicle when turning
        float skill = std::min( 10.0f,
                                u.get_skill_level( skill_driving ) + ( u.get_dex() + u.get_per() ) / 10.0f );
        float penalty = rng_float( 0.0f, handling_diff ) - skill;
        int cost;
        if( penalty > 0.0f ) {
            // At 10 penalty (rather hard to get), we're taking 4 turns per turn
            cost = 100 * ( 1.0f + penalty / 2.5f );
        } else {
            // At 10 skill, with a perfect vehicle, we could turn up to 3 times per turn
            cost = std::max( u.get_speed(), 100 ) * ( 1.0f - ( -penalty / 10.0f ) * 2 / 3 );
        }

        if( penalty > skill || cost > 400 ) {
            add_msg( m_warning, _( "You fumble with the %s's controls." ), name );
            // Anything from a wasted attempt to 2 turns in the intended direction
            turn_delta *= rng( 0, 2 );
            // Also wastes next turn
            cost = std::max( cost, u.moves + 100 );
        } else if( one_in( 10 ) ) {
            // Don't warn all the time or it gets spammy
            if( cost >= u.get_speed() * 2 ) {
                add_msg( m_warning, _( "It takes you a very long time to steer that vehicle!" ) );
            } else if( cost >= u.get_speed() * 1.5f ) {
                add_msg( m_warning, _( "It takes you a long time to steer that vehicle!" ) );
            }
        }

        turn( turn_delta );

        // At most 3 turns per turn, because otherwise it looks really weird and jumpy
        u.moves -= std::max( cost, u.get_speed() / 3 + 1 );
    }

    if( p.y != 0 ) {
        int thr_amount = 100 * ( abs( velocity ) < 2000 ? 4 : 5 );
        if( cruise_on ) {
            cruise_thrust( -p.y * thr_amount );
        } else {
            thrust( -p.y );
            u.moves = std::min( u.moves, 0 );
        }
    }

    // TODO: Actually check if we're on land on water (or disable water-skidding)
    if( skidding && valid_wheel_config() ) {
        ///\EFFECT_DEX increases chance of regaining control of a vehicle

        ///\EFFECT_DRIVING increases chance of regaining control of a vehicle
        if( handling_diff * rng( 1, 10 ) < u.dex_cur + u.get_skill_level( skill_driving ) * 2 ) {
            add_msg( _( "You regain control of the %s." ), name );
            u.practice( skill_driving, velocity / 5 );
            velocity = static_cast<int>( forward_velocity() );
            skidding = false;
            move.init( turn_dir );
        }
    }
}

// A chance to stop skidding if moving in roughly the faced direction
void vehicle::possibly_recover_from_skid()
{
    if( last_turn > 13 ) {
        // Turning on the initial skid is delayed, so move==face, initially. This filters out that case.
        return;
    }

    rl_vec2d mv = move_vec();
    rl_vec2d fv = face_vec();
    float dot = mv.dot_product( fv );
    // Threshold of recovery is Gaussianesque.

    if( fabs( dot ) * 100 > dice( 9, 20 ) ) {
        add_msg( _( "The %s recovers from its skid." ), name );
        skidding = false; // face_vec takes over.
        velocity *= dot; // Wheels absorb horizontal velocity.
        if( dot < -.8 ) {
            // Pointed backwards, velo-wise.
            velocity *= -1; // Move backwards.
        }

        move = face;
    }
}

// if not skidding, move_vec == face_vec, mv <dot> fv == 1, velocity*1 is returned.
float vehicle::forward_velocity() const
{
    rl_vec2d mv = move_vec();
    rl_vec2d fv = face_vec();
    float dot = mv.dot_product( fv );
    return velocity * dot;
}

rl_vec2d vehicle::velo_vec() const
{
    rl_vec2d ret;
    if( skidding ) {
        ret = move_vec();
    } else {
        ret = face_vec();
    }
    ret = ret.normalized();
    ret = ret * velocity;
    return ret;
}

inline rl_vec2d degree_to_vec( double degrees )
{
    return rl_vec2d( cos( degrees * M_PI / 180 ), sin( degrees * M_PI / 180 ) );
}

// normalized.
rl_vec2d vehicle::move_vec() const
{
    return degree_to_vec( move.dir() );
}

// normalized.
rl_vec2d vehicle::face_vec() const
{
    return degree_to_vec( face.dir() );
}

rl_vec2d vehicle::dir_vec() const
{
    return degree_to_vec( turn_dir );
}

float get_collision_factor( const float delta_v )
{
    if( std::abs( delta_v ) <= 31 ) {
        return ( 1 - ( 0.9 * std::abs( delta_v ) ) / 31 );
    } else {
        return 0.1;
    }
}

void vehicle::precalculate_vehicle_turning( int new_turn_dir, bool check_rail_direction,
        const ter_bitflags ter_flag_to_check, int &wheels_on_rail,
        int &turning_wheels_that_are_one_axis ) const
{
    tileray mdir; // The direction we're moving
    mdir.init( new_turn_dir ); // calculate direction after turn
    tripoint dp;
    bool is_diagonal_movement = new_turn_dir % 90 == 45;

    if( abs( velocity ) >= 20 ) {
        mdir.advance( velocity < 0 ? -1 : 1 );
        dp.x = mdir.dx();
        dp.y = mdir.dy();
    }

    // number of wheels that will land on rail
    wheels_on_rail = 0;

    // used to count wheels that will land on different axis
    int yVal = INT_MAX;
    /*
    number of wheels that are on one axis and will land on rail
    (not sometimes correct, for vehicle with 4 wheels, wheels_on_rail==3
    this can get 1 or 2 depending on position inaxis .wheelcache
    */
    turning_wheels_that_are_one_axis = 0;

    for( int part_index : wheelcache ) {
        const auto &wheel = parts[ part_index ];
        bool rails_ahead = true;
        point wheel_point;
        coord_translate( mdir.dir(), this->pivot_point(), wheel.mount,
                         wheel_point );

        tripoint wheel_tripoint = global_pos3() + wheel_point;

        // maximum number of incorrect tiles for this type of turn(diagonal or not)
        const int allowed_incorrect_tiles_diagonal = 1;
        const int allowed_incorrect_tiles_not_diagonal = 2;
        int incorrect_tiles_diagonal = 0;
        int incorrect_tiles_not_diagonal = 0;

        // check if terrain under the wheel and in direction of moving is rails
        for( int try_num = 0; try_num < 3; try_num++ ) {
            // advance precalculated wheel position 1 time in direction of moving
            wheel_tripoint += dp;

            if( !g->m.has_flag_ter_or_furn( ter_flag_to_check, wheel_tripoint ) ) {
                // this tile is not allowed, disallow turn
                rails_ahead = false;
                break;
            }

            // special case for rails
            if( check_rail_direction ) {
                ter_id terrain_at_wheel = g->m.ter( wheel_tripoint );
                // check is it correct tile to turn into
                if( !is_diagonal_movement &&
                    ( terrain_at_wheel == t_railroad_track_d || terrain_at_wheel == t_railroad_track_d1 ||
                      terrain_at_wheel == t_railroad_track_d2 || terrain_at_wheel == t_railroad_track_d_on_tie ) ) {
                    incorrect_tiles_not_diagonal++;
                } else if( is_diagonal_movement &&
                           ( terrain_at_wheel == t_railroad_track || terrain_at_wheel == t_railroad_track_on_tie ||
                             terrain_at_wheel == t_railroad_track_h || terrain_at_wheel == t_railroad_track_v ||
                             terrain_at_wheel == t_railroad_track_h_on_tie || terrain_at_wheel == t_railroad_track_v_on_tie ) ) {
                    incorrect_tiles_diagonal++;
                }
                if( incorrect_tiles_diagonal > allowed_incorrect_tiles_diagonal ||
                    incorrect_tiles_not_diagonal > allowed_incorrect_tiles_not_diagonal ) {
                    rails_ahead = false;
                    break;
                }
            }
        }
        if( rails_ahead ) { // found a wheel that turns correctly on rails
            if( yVal == INT_MAX ) { // if wheel that lands on rail still not found
                yVal = wheel.mount.y; // store mount point.y of wheel
            }
            if( yVal == wheel.mount.y ) {
                turning_wheels_that_are_one_axis++;
            }
            wheels_on_rail++;
        }
    }
}

// rounds turn_dir to 45*X degree, respecting face_dir
static int get_corrected_turn_dir( int turn_dir, int face_dir )
{
    int corrected_turn_dir = 0;

    // Driver turned vehicle, round angle to 45 deg
    if( turn_dir > face_dir && turn_dir < face_dir + 180 ) {
        corrected_turn_dir = face_dir + 45;
    } else if( turn_dir < face_dir || turn_dir > 270 ) {
        corrected_turn_dir = face_dir - 45;
    }
    if( corrected_turn_dir < 0 ) {
        corrected_turn_dir += 360;
    } else if( corrected_turn_dir >= 360 ) {
        corrected_turn_dir -= 360;
    }
    return corrected_turn_dir;
}

bool vehicle::allow_manual_turn_on_rails( int &corrected_turn_dir ) const
{
    bool allow_turn_on_rail = false;
    if( turn_dir != face.dir() ) { // driver tried to turn rails vehicle
        corrected_turn_dir = get_corrected_turn_dir( turn_dir, face.dir() );

        int wheels_on_rail, turning_wheels_that_are_one_axis;
        precalculate_vehicle_turning( corrected_turn_dir, true, TFLAG_RAIL, wheels_on_rail,
                                      turning_wheels_that_are_one_axis );
        if( is_wheel_state_correct_to_turn_on_rails( wheels_on_rail, rail_wheelcache.size(),
                turning_wheels_that_are_one_axis ) ) {
            allow_turn_on_rail = true;
        }
    }
    return allow_turn_on_rail;
}

bool vehicle::allow_auto_turn_on_rails( int &corrected_turn_dir ) const
{
    bool allow_turn_on_rail = false;
    if( turn_dir == face.dir() ) {  // check if autoturn is possible
        // precalculate wheels for every direction
        int straight_wheels_on_rail, straight_turning_wheels_that_are_one_axis;
        precalculate_vehicle_turning( face.dir(), true, TFLAG_RAIL, straight_wheels_on_rail,
                                      straight_turning_wheels_that_are_one_axis );

        int left_turn_dir = get_corrected_turn_dir( face.dir() - 45, face.dir() );
        int leftturn_wheels_on_rail, leftturn_turning_wheels_that_are_one_axis;
        precalculate_vehicle_turning( left_turn_dir, true, TFLAG_RAIL, leftturn_wheels_on_rail,
                                      leftturn_turning_wheels_that_are_one_axis );

        int right_turn_dir = get_corrected_turn_dir( face.dir() + 45, face.dir() );
        int rightturn_wheels_on_rail, rightturn_turning_wheels_that_are_one_axis;
        precalculate_vehicle_turning( right_turn_dir, true, TFLAG_RAIL, rightturn_wheels_on_rail,
                                      rightturn_turning_wheels_that_are_one_axis );

        // if bad terrain ahead (landing wheels num is low)
        if( straight_wheels_on_rail <= leftturn_wheels_on_rail &&
            is_wheel_state_correct_to_turn_on_rails( leftturn_wheels_on_rail, rail_wheelcache.size(),
                    leftturn_turning_wheels_that_are_one_axis ) ) {
            allow_turn_on_rail = true;
            corrected_turn_dir = left_turn_dir;
        } else if( straight_wheels_on_rail <= rightturn_wheels_on_rail &&
                   is_wheel_state_correct_to_turn_on_rails( rightturn_wheels_on_rail, rail_wheelcache.size(),
                           rightturn_turning_wheels_that_are_one_axis ) ) {
            allow_turn_on_rail = true;
            corrected_turn_dir = right_turn_dir;
        }
    }
    return allow_turn_on_rail;
}

bool vehicle::is_wheel_state_correct_to_turn_on_rails( int wheels_on_rail, int wheel_count,
        int turning_wheels_that_are_one_axis ) const
{
    return ( wheels_on_rail >= 2 || // minimum wheels to be able to turn (excluding one axis vehicles)
             ( wheels_on_rail == 1 && ( wheel_count == 1 ||
                                        all_wheels_on_one_axis ) ) ) // for bikes or 1 wheel vehicle
           && ( wheels_on_rail !=
                turning_wheels_that_are_one_axis // wheels that want to turn is not on same axis
                || all_wheels_on_one_axis ||
                ( abs( rail_wheel_bounding_box.p2.x - rail_wheel_bounding_box.p1.x ) < 4 && velocity < 0 ) );
    // allow turn for vehicles with wheel distance < 4 when moving backwards
}

vehicle *vehicle::act_on_map()
{
    const tripoint pt = global_pos3();
    if( !g->m.inbounds( pt ) ) {
        dbg( D_INFO ) << "stopping out-of-map vehicle. (x,y,z)=(" << pt.x << "," << pt.y << "," << pt.z <<
                      ")";
        stop( false );
        of_turn = 0;
        is_falling = false;
        return this;
    }

    const bool pl_ctrl = player_in_control( g->u );
    // TODO: Remove this hack, have vehicle sink a z-level
    if( is_floating && !can_float() ) {
        add_msg( m_bad, _( "Your %s sank." ), name );
        if( pl_ctrl ) {
            unboard_all();
        }
        if( g->remoteveh() == this ) {
            g->setremoteveh( nullptr );
        }

        g->m.on_vehicle_moved( sm_pos.z );
        // Destroy vehicle (sank to nowhere)
        g->m.destroy_vehicle( this );
        return nullptr;
    }

    // It needs to fall when it has no support OR was falling before
    //  so that vertical collisions happen.
    const bool should_fall = is_falling || vertical_velocity != 0;

    // TODO: Saner diagonal movement, so that you can jump off cliffs properly
    // The ratio of vertical to horizontal movement should be vertical_velocity/velocity
    //  for as long as of_turn doesn't run out.
    if( should_fall ) {
        const float g = 9.8f; // 9.8 m/s^2
        // Convert from 100*mph to m/s
        const float old_vel = vmiph_to_mps( vertical_velocity );
        // Formula is v_2 = sqrt( 2*d*g + v_1^2 )
        // Note: That drops the sign
        const float new_vel = -sqrt( 2 * tile_height * g + old_vel * old_vel );
        vertical_velocity = mps_to_vmiph( new_vel );
        is_falling = true;
    } else {
        // Not actually falling, was just marked for fall test
        is_falling = false;
    }

    // Low enough for bicycles to go in reverse.
    if( !should_fall && abs( velocity ) < 20 ) {
        stop();
        of_turn -= .321f;
        return this;
    }

    const float wheel_traction_area = g->m.vehicle_wheel_traction( *this );
    const float traction = k_traction( wheel_traction_area );
    if( traction < 0.001f ) {
        of_turn = 0;
        if( !should_fall ) {
            stop();
            if( floating.empty() ) {
                add_msg( m_info, _( "Your %s can't move on this terrain." ), name );
            } else {
                add_msg( m_info, _( "Your %s is beached." ), name );
            }
            return this;
        }
    }
    const float turn_cost = vehicles::vmiph_per_tile / std::max<float>( 0.0001f, abs( velocity ) );

    // Can't afford it this turn?
    // Low speed shouldn't prevent vehicle from falling, though
    bool falling_only = false;
    if( turn_cost >= of_turn ) {
        if( !should_fall ) {
            of_turn_carry = of_turn;
            of_turn = 0;
            return this;
        }
        falling_only = true;
    }

    // Decrease of_turn if falling+moving, but not when it's lower than move cost
    if( !falling_only ) {
        of_turn -= turn_cost;
    }

    bool can_use_rails = this->can_use_rails();
    if( one_in( 10 ) ) {
        bool controlled = false;
        // It can even be a NPC, but must be at the controls
        for( int boarded : boarded_parts() ) {
            if( part_with_feature( boarded, VPFLAG_CONTROLS, true ) >= 0 ) {
                controlled = true;
                player *passenger = get_passenger( boarded );
                if( passenger != nullptr ) {
                    passenger->practice( skill_driving, 1 );
                }
            }
        }

        // Eventually send it skidding if no control
        // But not if it's remotely controlled, is in water or can use rails
        if( !controlled && !pl_ctrl && !is_floating && !can_use_rails ) {
            skidding = true;
        }
    }

    if( skidding && one_in( 4 ) ) {
        // Might turn uncontrollably while skidding
        turn( one_in( 2 ) ? -15 : 15 );
    }

    if( should_fall ) {
        // TODO: Insert a (hard) driving test to stop this from happening
        skidding = true;
    }

    bool allow_turn_on_rail = false;
    if( can_use_rails && !falling_only ) {
        int corrected_turn_dir;
        allow_turn_on_rail = allow_manual_turn_on_rails( corrected_turn_dir );
        if( !allow_turn_on_rail ) {
            allow_turn_on_rail = allow_auto_turn_on_rails( corrected_turn_dir );
        }
        if( allow_turn_on_rail ) {
            turn_dir = corrected_turn_dir;
        }
    }

    tileray mdir; // The direction we're moving
    if( skidding || should_fall ) {
        // If skidding, it's the move vector
        // Same for falling - no air control
        mdir = move;
    } else if( turn_dir != face.dir() && ( !can_use_rails || allow_turn_on_rail ) ) {
        // Driver turned vehicle, get turn_dir
        mdir.init( turn_dir );
    } else {
        // Not turning, keep face.dir
        mdir = face;
    }

    tripoint dp;
    if( abs( velocity ) >= 20 && !falling_only ) {
        mdir.advance( velocity < 0 ? -1 : 1 );
        dp.x = mdir.dx();
        dp.y = mdir.dy();
    }

    if( should_fall ) {
        dp.z = -1;
    }

    return g->m.move_vehicle( *this, dp, mdir );
}

void vehicle::check_falling_or_floating()
{
    // TODO: Make the vehicle "slide" towards its center of weight
    //  when it's not properly supported
    const auto &pts = get_points( true );
    if( pts.empty() ) {
        // Dirty vehicle with no parts
        is_falling = false;
        is_floating = false;
        in_water = false;
        return;
    }

    is_falling = g->m.has_zlevels();

    size_t deep_water_tiles = 0;
    size_t water_tiles = 0;
    for( const tripoint &p : pts ) {
        if( is_falling ) {
            tripoint below( p.xy(), p.z - 1 );
            is_falling &= g->m.has_flag_ter_or_furn( TFLAG_NO_FLOOR, p ) && ( p.z > -OVERMAP_DEPTH ) &&
                          !g->m.supports_above( below );
        }
        deep_water_tiles += g->m.has_flag( TFLAG_DEEP_WATER, p ) ? 1 : 0;
        water_tiles += g->m.has_flag( TFLAG_SWIMMABLE, p ) ? 1 : 0;
    }
    // floating if 2/3rds of the vehicle is in deep water
    is_floating = 3 * deep_water_tiles >= 2 * pts.size();
    // in_water if 1/2 of the vehicle is in water at all
    in_water =  2 * water_tiles >= pts.size();
}

float map::vehicle_wheel_traction( const vehicle &veh ) const
{
    if( veh.is_in_water( true ) ) {
        return veh.can_float() ? 1.0f : -1.0f;
    }
    if( veh.is_in_water() && veh.is_watercraft() && veh.can_float() ) {
        return 1.0f;
    }

    const auto &wheel_indices = veh.wheelcache;
    int num_wheels = wheel_indices.size();
    if( num_wheels == 0 ) {
        // TODO: Assume it is digging in dirt
        // TODO: Return something that could be reused for dragging
        return 0.0f;
    }

    float traction_wheel_area = 0.0f;
    for( int p : wheel_indices ) {
        const tripoint &pp = veh.global_part_pos3( p );
        const int wheel_area = veh.parts[ p ].wheel_area();

        const auto &tr = ter( pp ).obj();
        // Deep water and air
        if( tr.has_flag( TFLAG_DEEP_WATER ) || tr.has_flag( TFLAG_NO_FLOOR ) ) {
            // No traction from wheel in water or air
            continue;
        }

        int move_mod = move_cost_ter_furn( pp );
        if( move_mod == 0 ) {
            // Vehicle locked in wall
            // Shouldn't happen, but does
            return 0.0f;
        }

        for( const auto &terrain_mod : veh.part_info( p ).wheel_terrain_mod() ) {
            if( terrain_mod.second.movecost > 0 &&
                tr.has_flag( terrain_mod.first ) ) {
                move_mod = terrain_mod.second.movecost;
                break;
            } else if( terrain_mod.second.penalty && !tr.has_flag( terrain_mod.first ) ) {
                move_mod += terrain_mod.second.penalty;
                break;
            }
        }
        traction_wheel_area += 2.0 * wheel_area / move_mod;
    }

    return traction_wheel_area;
}

int map::shake_vehicle( vehicle &veh, const int velocity_before, const int direction )
{
    const int d_vel = abs( veh.velocity - velocity_before ) / 100;

    std::vector<rider_data> riders = veh.get_riders();

    int coll_turn = 0;
    for( const rider_data &r : riders ) {
        const int ps = r.prt;
        Creature *rider = r.psg;
        if( rider == nullptr ) {
            debugmsg( "throw passenger: empty passenger at part %d", ps );
            continue;
        }

        const tripoint part_pos = veh.global_part_pos3( ps );
        if( rider->pos() != part_pos ) {
            debugmsg( "throw passenger: passenger at %d,%d,%d, part at %d,%d,%d",
                      rider->posx(), rider->posy(), rider->posz(),
                      part_pos.x, part_pos.y, part_pos.z );
            veh.parts[ps].remove_flag( vehicle_part::passenger_flag );
            continue;
        }

        player *psg = dynamic_cast<player *>( rider );
        monster *pet = dynamic_cast<monster *>( rider );

        bool throw_from_seat = false;
        int move_resist = 1;
        if( psg ) {
            ///\EFFECT_STR reduces chance of being thrown from your seat when not wearing a seatbelt
            move_resist = psg->str_cur * 150 + 500;
        } else {
            int pet_resist = 0;
            if( pet != nullptr ) {
                pet_resist = static_cast<int>( to_kilogram( pet->get_weight() ) * 200 );
            }
            move_resist = std::max( 100, pet_resist );
        }
        if( veh.part_with_feature( ps, VPFLAG_SEATBELT, true ) == -1 ) {
            ///\EFFECT_STR reduces chance of being thrown from your seat when not wearing a seatbelt
            throw_from_seat = d_vel * rng( 80, 120 ) > move_resist;
        }

        // Damage passengers if d_vel is too high
        if( !throw_from_seat && ( 10 * d_vel ) > 6 * rng( 50, 100 ) ) {
            const int dmg = d_vel * rng( 70, 100 ) / 400;
            if( psg ) {
                psg->hurtall( dmg, nullptr );
                psg->add_msg_player_or_npc( m_bad,
                                            _( "You take %d damage by the power of the impact!" ),
                                            _( "<npcname> takes %d damage by the power of the "
                                               "impact!" ),  dmg );
            } else {
                pet->apply_damage( nullptr, bp_torso, dmg );
            }
        }

        if( psg && veh.player_in_control( *psg ) ) {
            const int lose_ctrl_roll = rng( 0, d_vel );
            ///\EFFECT_DEX reduces chance of losing control of vehicle when shaken

            ///\EFFECT_DRIVING reduces chance of losing control of vehicle when shaken
            if( lose_ctrl_roll > psg->dex_cur * 2 + psg->get_skill_level( skill_driving ) * 3 ) {
                psg->add_msg_player_or_npc( m_warning,
                                            _( "You lose control of the %s." ),
                                            _( "<npcname> loses control of the %s." ), veh.name );
                int turn_amount = rng( 1, 3 ) * sqrt( abs( veh.velocity ) ) / 30;
                if( turn_amount < 1 ) {
                    turn_amount = 1;
                }
                turn_amount *= 15;
                if( turn_amount > 120 ) {
                    turn_amount = 120;
                }
                coll_turn = one_in( 2 ) ? turn_amount : -turn_amount;
            }
        }

        if( throw_from_seat ) {
            if( psg ) {
                psg->add_msg_player_or_npc( m_bad,
                                            _( "You are hurled from the %s's seat by "
                                               "the power of the impact!" ),
                                            _( "<npcname> is hurled from the %s's seat by "
                                               "the power of the impact!" ), veh.name );
                unboard_vehicle( part_pos );
            } else if( g->u.sees( part_pos ) ) {
                add_msg( m_bad, _( "The %s is hurled from %s's by the power of the impact!" ),
                         pet->disp_name(), veh.name );
            }
            ///\EFFECT_STR reduces distance thrown from seat in a vehicle impact
            g->fling_creature( rider, direction + rng( 0, 60 ) - 30,
                               std::max( 10, d_vel - move_resist / 100 ) );
        }
    }

    return coll_turn;
}
