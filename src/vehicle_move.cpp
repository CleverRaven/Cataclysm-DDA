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
#include "map_iterator.h"
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
#include "vpart_range.h"

#define dbg(x) DebugLog((x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

static const itype_id fuel_type_muscle( "muscle" );
static const itype_id fuel_type_animal( "animal" );
static const itype_id fuel_type_battery( "battery" );

static const skill_id skill_driving( "driving" );

static const efftype_id effect_harnessed( "harnessed" );
static const efftype_id effect_pet( "pet" );
static const efftype_id effect_stunned( "stunned" );

static const std::string part_location_structure( "structure" );

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
    double mps = vmiph_to_mps( std::abs( at_velocity ) );

    // slowdown due to air resistance is proportional to square of speed
    double f_total_drag = coeff_air_drag() * mps * mps;
    if( is_watercraft() ) {
        // same with water resistance
        f_total_drag += coeff_water_drag() * mps * mps;
    } else if( !is_falling && !is_flying ) {
        double f_rolling_drag = coeff_rolling_drag() * ( vehicles::rolling_constant_to_variable + mps );
        // increase rolling resistance by up to 25x if the vehicle is skidding at right angle to facing
        const double skid_factor = 1 + 24 * std::abs( std::sin( DEGREES( face.dir() - move.dir() ) ) );
        f_total_drag += f_rolling_drag * skid_factor;
    }
    double accel_slowdown = f_total_drag / to_kilogram( total_mass() );
    // converting m/s^2 to vmiph/s
    int slowdown = mps_to_vmiph( accel_slowdown );
    if( is_towing() ) {
        vehicle *other_veh = tow_data.get_towed();
        if( other_veh ) {
            slowdown += other_veh->slowdown( at_velocity );
        }
    }
    if( slowdown < 0 ) {
        debugmsg( "vehicle %s has negative drag slowdown %d\n", name, slowdown );
    }
    add_msg( m_debug, "%s at %d vimph, f_drag %3.2f, drag accel %d vmiph - extra drag %d",
             name, at_velocity, f_total_drag, slowdown, static_drag() );
    // plows slow rolling vehicles, but not falling or floating vehicles
    if( !( is_falling || is_floating || is_flying ) ) {
        slowdown -= static_drag();
    }

    return std::max( 1, slowdown );
}

void vehicle:: smart_controller_handle_turn( bool thrusting,
        cata::optional<float> k_traction_cache )
{

    if( !engine_on || !has_enabled_smart_controller ) {
        smart_controller_state = cata::nullopt;
        return;
    }

    if( smart_controller_state && smart_controller_state->created == calendar::turn ) {
        return;
    }

    // controlled engines
    // note: contains indices of of elements in `engines` array, not the part ids
    std::vector<int> c_engines;
    for( int i = 0; i < static_cast<int>( engines.size() ); ++i ) {
        if( ( is_engine_type( i, fuel_type_battery ) || is_combustion_engine_type( i ) ) &&
            ( ( parts[ engines[ i ] ].is_available() && engine_fuel_left( i ) > 0 ) ||
              is_part_on( engines[ i ] ) ) ) {
            c_engines.push_back( i );
        }
    }

    bool rotorcraft = is_flying && is_rotorcraft();

    if( rotorcraft || c_engines.size() <= 1 || c_engines.size() > 5 ) { // bail and shut down
        for( const vpart_reference &vp : get_avail_parts( "SMART_ENGINE_CONTROLLER" ) ) {
            vp.part().enabled = false;
        }

        if( player_in_control( g->u ) ) {
            if( rotorcraft ) {
                add_msg( _( "Smart controller does not support flying vehicles." ) );
            } else if( c_engines.size() <= 1 ) {
                add_msg( _( "Smart controller detects only a single controllable engine." ) );
                add_msg( _( "Smart controller is designed to control more than one engine." ) );
            } else {
                add_msg( _( "Smart controller does not support more than five engines." ) );
            }
            add_msg( m_bad, _( "Smart controller is shutting down." ) );
        }
        has_enabled_smart_controller = false;
        smart_controller_state = cata::nullopt;
        return;
    }

    int cur_battery_level, max_battery_level;
    std::tie( cur_battery_level, max_battery_level ) = battery_power_level();
    int battery_level_percent = max_battery_level == 0 ? 0 : cur_battery_level * 100 /
                                max_battery_level;

    // when battery > 90%, discharge is allowed
    // otherwise trying to charge battery to 90% within 30 minutes
    bool discharge_forbidden_soft = battery_level_percent <= 90;
    bool discharge_forbidden_hard = battery_level_percent <= 25;
    int target_charging_rate = ( max_battery_level == 0 || !discharge_forbidden_soft ) ? 0 :
                               ( 9 * max_battery_level - 10 * cur_battery_level ) / ( 6 * 3 );
    //                         ( max_battery_level * 0.9 - cur_battery_level )  * (1000 / (60 * 30))   // originally
    //                                              ^ 90%                   bat to W ^         ^ 30 minutes

    int accel_demand = cruise_on
                       ? // using avg_velocity reduces unnecessary oscillations when traction is low
                       std::max( std::abs( cruise_velocity - velocity ), std::abs( cruise_velocity - avg_velocity ) ) :
                       ( thrusting ? 1000 : 0 );
    if( velocity != 0 && accel_demand == 0 ) {
        accel_demand = 1;    // to prevent zero fuel usage
    }

    int velocity_demand = std::max( std::abs( this->velocity ), std::abs( cruise_velocity ) );

    // for stationary vehicles all velocity and acceleration calculations are skipped
    bool is_stationary = avg_velocity == 0 && velocity_demand == 0 && accel_demand == 0;

    bool gas_engine_shutdown_forbidden = smart_controller_state &&
                                         ( calendar::turn - smart_controller_state->gas_engine_last_turned_on ) <
                                         15_seconds;


    smart_controller_cache cur_state;

    float traction = is_stationary ? 1.0f :
                     ( k_traction_cache ? *k_traction_cache : k_traction( get_map().vehicle_wheel_traction( *this ) ) );

    int prev_mask = 0;
    // opt_ prefix denotes values for currently found "optimal" engine configuration
    int opt_net_echarge_rate = net_battery_charge_rate_w();
    // total engine fuel energy usage (J)
    int opt_fuel_usage = 0;

    int opt_accel = is_stationary ? 1 : current_acceleration() * traction;
    int opt_safe_vel = is_stationary ? 1 : safe_ground_velocity( true );
    float cur_load_approx = static_cast<float>( std::min( accel_demand,
                            opt_accel ) )  / std::max( opt_accel, 1 );
    float cur_load_alternator = std::min( 0.01f, static_cast<float>( alternator_load ) / 1000 );

    for( size_t i = 0; i < c_engines.size(); ++i ) {
        if( is_engine_on( c_engines[i] ) ) {
            prev_mask |= 1 << i;
            bool is_electric = is_engine_type( c_engines[i], fuel_type_battery );
            int fu = engine_fuel_usage( c_engines[i] ) * ( cur_load_approx + ( is_electric ? 0 :
                     cur_load_alternator ) );
            opt_fuel_usage += fu;
            if( is_electric ) {
                opt_net_echarge_rate -= fu;
            }
        }
    }
    cur_state.created = calendar::turn;
    cur_state.battery_percent = battery_level_percent;
    cur_state.battery_net_charge_rate = opt_net_echarge_rate;
    cur_state.velocity = avg_velocity;
    cur_state.load = cur_load_approx + cur_load_alternator;
    if( smart_controller_state ) {
        cur_state.gas_engine_last_turned_on = smart_controller_state->gas_engine_last_turned_on;
    }
    cur_state.gas_engine_shutdown_forbidden = gas_engine_shutdown_forbidden;

    int opt_mask = prev_mask; // save current engine state, because it will be temporarily modified

    // if vehicle state has not change, skip actual optimization
    if( smart_controller_state &&
        std::abs( smart_controller_state->velocity - cur_state.velocity ) < 100 &&
        std::abs( smart_controller_state->battery_percent - cur_state.battery_percent ) <= 2 &&
        std::abs( smart_controller_state->load - cur_state.load ) < 0.1 && // load diff < 10%
        smart_controller_state->battery_net_charge_rate == cur_state.battery_net_charge_rate &&
        // reevaluate cache if when cache was created, gas engine shutdown was forbidden, but now it's not
        !( smart_controller_state->gas_engine_shutdown_forbidden && !gas_engine_shutdown_forbidden )
      ) {
        smart_controller_state->created = calendar::turn;
        return;
    }

    // trying all combinations of engine state (max 31 iterations for 5 engines)
    for( int mask = 1; mask < static_cast<int>( 1 << c_engines.size() ); ++mask ) {
        if( mask == prev_mask ) {
            continue;
        }

        bool gas_engine_to_shut_down = false;
        for( size_t i = 0; i < c_engines.size(); ++i ) {
            bool old_state = ( prev_mask & ( 1 << i ) ) != 0;
            bool new_state = ( mask & ( 1 << i ) ) != 0;
            // switching enabled flag temporarily to perform calculations below
            toggle_specific_engine( c_engines[i], new_state );

            if( old_state && !new_state && !is_engine_type( c_engines[i], fuel_type_battery ) ) {
                gas_engine_to_shut_down = true;
            }
        }

        if( gas_engine_to_shut_down && gas_engine_shutdown_forbidden ) {
            continue; // skip checking this state
        }

        int safe_vel =  is_stationary ? 1 : safe_ground_velocity( true );
        int accel = is_stationary ? 1 : current_acceleration() * traction;
        int fuel_usage = 0;
        int net_echarge_rate = net_battery_charge_rate_w();
        float load_approx = static_cast<float>( std::min( accel_demand, accel ) ) / std::max( accel, 1 );
        update_alternator_load();
        float load_approx_alternator  = std::min( 0.01f, static_cast<float>( alternator_load ) / 1000 );

        for( int e : c_engines ) {
            bool is_electric = is_engine_type( e, fuel_type_battery );
            int fu = engine_fuel_usage( e ) * ( load_approx + ( is_electric ? 0 : load_approx_alternator ) );
            fuel_usage += fu;
            if( is_electric ) {
                net_echarge_rate -= fu;
            }
        }

        if( std::forward_as_tuple(
                !discharge_forbidden_hard || ( net_echarge_rate > 0 ),
                accel >= accel_demand,
                opt_accel < accel_demand ? accel : 0, // opt_accel usage here is intentional
                safe_vel >= velocity_demand,
                opt_safe_vel < velocity_demand ? -safe_vel : 0, //opt_safe_vel usage here is intentional
                !discharge_forbidden_soft || ( net_echarge_rate > target_charging_rate ),
                -fuel_usage,
                net_echarge_rate
            ) >= std::forward_as_tuple(
                !discharge_forbidden_hard || ( opt_net_echarge_rate > 0 ),
                opt_accel >= accel_demand,
                opt_accel < accel_demand ? opt_accel : 0,
                opt_safe_vel >= velocity_demand,
                opt_safe_vel < velocity_demand ? -opt_safe_vel : 0,
                !discharge_forbidden_soft || ( opt_net_echarge_rate > target_charging_rate ),
                -opt_fuel_usage,
                opt_net_echarge_rate
            ) ) {
            opt_mask = mask;
            opt_fuel_usage = fuel_usage;
            opt_net_echarge_rate = net_echarge_rate;
            opt_accel = accel;
            opt_safe_vel = safe_vel;

            cur_state.battery_net_charge_rate = net_echarge_rate;
            cur_state.load = load_approx + load_approx_alternator;
            // other `cur_state` fields do not change for different engine state combinations
        }
    }

    for( size_t i = 0; i < c_engines.size(); ++i ) { // return to prev state
        toggle_specific_engine( c_engines[i], static_cast<bool>( prev_mask & ( 1 << i ) ) );
    }

    if( opt_mask != prev_mask ) { // we found new configuration
        bool failed_to_start = false;
        bool turned_on_gas_engine = false;
        for( size_t i = 0; i < c_engines.size(); ++i ) {
            // ..0.. < ..1..  was off, new state on
            if( ( prev_mask & ( 1 << i ) ) < ( opt_mask & ( 1 << i ) ) ) {
                if( !start_engine( c_engines[i], true ) ) {
                    failed_to_start = true;
                }
                turned_on_gas_engine |= !is_engine_type( c_engines[i], fuel_type_battery );
            }
        }
        if( failed_to_start ) {
            this->smart_controller_state = cata::nullopt;

            for( size_t i = 0; i < c_engines.size(); ++i ) { // return to prev state
                toggle_specific_engine( c_engines[i], static_cast<bool>( prev_mask & ( 1 << i ) ) );
            }
            for( const vpart_reference &vp : get_avail_parts( "SMART_ENGINE_CONTROLLER" ) ) {
                vp.part().enabled = false;
            }
            if( player_in_control( g->u ) ) {
                add_msg( m_bad, _( "Smart controller failed to start an engine." ) );
                add_msg( m_bad, _( "Smart controller is shutting down." ) );
            }
            has_enabled_smart_controller = false;

        } else {  //successfully changed engines state
            for( size_t i = 0; i < c_engines.size(); ++i ) {
                // was on, needs to be off
                if( ( prev_mask & ( 1 << i ) ) > ( opt_mask & ( 1 << i ) ) ) {
                    start_engine( c_engines[i], false );
                }
            }
            if( turned_on_gas_engine ) {
                cur_state.gas_engine_last_turned_on = calendar::turn;
            }
            smart_controller_state = cur_state;

            if( player_in_control( g->u ) ) {
                add_msg( m_debug, _( "Smart controller optimizes engine state." ) );
            }
        }
    } else {
        // as the optimization was performed (even without state change), cache needs to be updated as well
        smart_controller_state = cur_state;
    }
    update_alternator_load();
}

void vehicle::thrust( int thd, int z )
{
    //if vehicle is stopped, set target direction to forward.
    //ensure it is not skidding. Set turns used to 0.
    if( !is_moving() && z == 0 ) {
        turn_dir = face.dir();
        stop();
    }
    bool pl_ctrl = player_in_control( get_player_character() );

    // No need to change velocity if there are no wheels
    if( ( in_water && can_float() ) || ( is_rotorcraft() && ( z != 0 || is_flying ) ) ) {
        // we're good
    } else if( is_floating && !can_float() ) {
        stop();
        if( pl_ctrl ) {
            add_msg( _( "The %s is too leaky!" ), name );
        }
        return;
    } else if( !valid_wheel_config()  && z == 0 ) {
        stop();
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
    float traction = k_traction( get_map().vehicle_wheel_traction( *this ) );

    if( thrusting ) {
        smart_controller_handle_turn( true, traction );
    }

    int accel = current_acceleration() * traction;
    if( accel < 200 && velocity > 0 && is_towing() ) {
        if( pl_ctrl ) {
            add_msg( _( "The %s struggles to pull the %s on this surface!" ), name,
                     tow_data.get_towed()->name );
        }
        return;
    }
    if( thrusting && accel == 0 ) {
        if( pl_ctrl ) {
            add_msg( _( "The %s is too heavy for its engine(s)!" ), name );
        }
        return;
    }
    int max_vel = traction * max_velocity();

    // Get braking power
    int brk = std::max( 1000, std::abs( max_vel ) * 3 / 10 );

    //pos or neg if accelerator or brake
    int vel_inc = ( ( thrusting ) ? accel : brk ) * thd;
    // Reverse is only 60% acceleration, unless an electric motor is in use
    if( thd == -1 && thrusting && !has_engine_type( fuel_type_battery, true ) ) {
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
        if( z != 0 ) {
            // @TODO : actual engine strain / load for going up a z-level.
            load = 1;
            thrusting = true;
        }
    } else {
        if( z != 0 ) {
            load = 1;
            thrusting = true;
        } else {
            load = ( thrusting ? 1000 : 0 );
        }
    }

    // only consume resources if engine accelerating
    if( load >= 1 && thrusting ) {
        //abort if engines not operational
        if( total_power_w() <= 0 || !engine_on || ( z == 0 && accel == 0 ) ) {
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
        // helicopters improve efficiency the closer they get to 50-70 knots
        // then it drops off as they go over that.
        // see https://i.stack.imgur.com/0zIO7.jpg
        if( is_rotorcraft() && is_flying_in_air() ) {
            const int velocity_kt = velocity * 0.01;
            int value;
            if( velocity_kt < 70 ) {
                value = 49 * std::pow( velocity_kt, 3 ) -
                        4118 * std::pow( velocity_kt, 2 ) - 76512 * velocity_kt + 18458000;
            } else {
                value = 1864 * std::pow( velocity_kt, 2 ) - 272190 * velocity_kt + 19473000;
            }
            value *= 0.0001;
            load = std::max( 200, std::min( 1000, ( ( value / 2 ) + 100 ) ) );
        }
        //make noise and consume fuel
        noise_and_smoke( load );
        consume_fuel( load, 1 );
        if( z != 0 && is_rotorcraft() ) {
            requested_z_change = z;
        }
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
        const int min_vel = max_reverse_velocity();
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
    int max_rev_vel = max_reverse_velocity();

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
        cruise_velocity = ( cruise_velocity / std::abs( amount ) ) * std::abs( amount );
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
    map &here = get_map();
    for( const tripoint &p : get_points() ) {
        here.set_memory_seen_cache_dirty( p );
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
    // Skip collisions when there is no apparent movement, except verticially moving rotorcraft.
    if( coll_velocity == 0 && !is_rotorcraft() ) {
        just_detect = true;
    }

    const int velocity_before = coll_velocity;
    int lowest_velocity = coll_velocity;
    const int sign_before = sgn( velocity_before );
    bool empty = true;
    map &here = get_map();
    for( int p = 0; static_cast<size_t>( p ) < parts.size(); p++ ) {
        const vpart_info &info = part_info( p );
        if( ( info.location != part_location_structure && info.rotor_diameter() == 0 ) ||
            parts[ p ].removed ) {
            continue;
        }
        empty = false;
        // Coordinates of where part will go due to movement (dx/dy/dz)
        //  and turning (precalc[1])
        const tripoint dsp = global_pos3() + dp + parts[p].precalc[1];
        veh_collision coll = part_collision( p, dsp, just_detect, bash_floor );
        if( coll.type == veh_coll_nothing && info.rotor_diameter() > 0 ) {
            size_t radius = static_cast<size_t>( std::round( info.rotor_diameter() / 2.0f ) );
            for( const tripoint &rotor_point : here.points_in_radius( dsp, radius ) ) {
                veh_collision rotor_coll = part_collision( p, rotor_point, just_detect, false );
                if( rotor_coll.type != veh_coll_nothing ) {
                    coll = rotor_coll;
                    if( just_detect ) {
                        break;
                    } else {
                        colls.push_back( rotor_coll );
                    }
                }
            }
        }
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
            if( velocity_before < 0 ) {
                lowest_velocity = std::max( lowest_velocity, coll_velocity );
            } else {
                lowest_velocity = std::min( lowest_velocity, coll_velocity );
            }
            vertical_velocity = velocity_before;
        } else if( sgn( velocity_after ) != sign_before ) {
            // Sign of velocity inverted, collisions would be in wrong direction
            break;
        }
    }

    if( vertical ) {
        vertical_velocity = lowest_velocity;
        if( vertical_velocity == 0 ) {
            is_falling = false;
        }
    }

    if( empty ) {
        // HACK: Hack for dirty vehicles that didn't yet get properly removed
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
    map &here = get_map();
    // Just a rough rescale for now to obtain approximately equal numbers
    const int bash_min = here.bash_resistance( p, bash_floor );
    const int bash_max = here.bash_strength( p, bash_floor );
    mass = ( bash_min + bash_max ) / 2.0;
    density = bash_min;
}

veh_collision vehicle::part_collision( int part, const tripoint &p,
                                       bool just_detect, bool bash_floor )
{
    // Vertical collisions need to be handled differently
    // All collisions have to be either fully vertical or fully horizontal for now
    const bool vert_coll = bash_floor || p.z != sm_pos.z;
    Character &player_character = get_player_character();
    const bool pl_ctrl = player_in_control( player_character );
    Creature *critter = g->critter_at( p, true );
    player *ph = dynamic_cast<player *>( critter );

    Creature *driver = pl_ctrl ? &player_character : nullptr;

    // If in a vehicle assume it's this one
    if( ph != nullptr && ph->in_vehicle ) {
        critter = nullptr;
        ph = nullptr;
    }

    map &here = get_map();
    const optional_vpart_position ovp = here.veh_at( p );
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

    // Typical rotor tip speed in MPH * 100.
    int rotor_velocity = 45600;
    // Non-vehicle collisions can't happen when the vehicle is not moving
    int &coll_velocity = ( part_info( part ).rotor_diameter() == 0 ) ?
                         ( vert_coll ? vertical_velocity : velocity ) :
                         rotor_velocity;
    if( !just_detect && coll_velocity == 0 ) {
        return ret;
    }

    if( is_body_collision ) {
        // critters on a BOARDABLE part in this vehicle aren't colliding
        if( ovp && ( &ovp->vehicle() == this ) && get_pet( ovp->part_index() ) ) {
            return ret;
        }
        // we just ran into a fish, so move it out of the way
        if( here.has_flag( "SWIMMABLE", critter->pos() ) ) {
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
    // e = 0 -> plastic collision
    float e = 0.3;
    // e = 1 -> inelastic collision
    //part density
    float part_dens = 0;

    if( is_body_collision ) {
        // Check any monster/NPC/player on the way
        // body
        ret.type = veh_coll_body;
        ret.target = critter;
        e = 0.30;
        part_dens = 15;
        mass2 = units::to_kilogram( critter->get_weight() );
        ret.target_name = critter->disp_name();
    } else if( ( bash_floor && here.is_bashable_ter_furn( p, true ) ) ||
               ( here.is_bashable_ter_furn( p, false ) && here.move_cost_ter_furn( p ) != 2 &&
                 // Don't collide with tiny things, like flowers, unless we have a wheel in our space.
                 ( part_with_feature( ret.part, VPFLAG_WHEEL, true ) >= 0 ||
                   !here.has_flag_ter_or_furn( "TINY", p ) ) &&
                 // Protrusions don't collide with short terrain.
                 // Tiny also doesn't, but it's already excluded unless there's a wheel present.
                 !( part_with_feature( ret.part, "PROTRUSION", true ) >= 0 &&
                    here.has_flag_ter_or_furn( "SHORT", p ) ) &&
                 // These are bashable, but don't interact with vehicles.
                 !here.has_flag_ter_or_furn( "NOCOLLIDE", p ) &&
                 // Do not collide with track tiles if we can use rails
                 !( here.has_flag_ter_or_furn( TFLAG_RAIL, p ) && this->can_use_rails() ) ) ) {
        // Movecost 2 indicates flat terrain like a floor, no collision there.
        ret.type = veh_coll_bashable;
        terrain_collision_data( p, bash_floor, mass2, part_dens, e );
        ret.target_name = here.disp_name( p );
    } else if( here.impassable_ter_furn( p ) ||
               ( bash_floor && !here.has_flag( TFLAG_NO_FLOOR, p ) ) ) {
        // not destructible
        ret.type = veh_coll_other;
        mass2 = 1000;
        e = 0.10;
        part_dens = 80;
        ret.target_name = here.disp_name( p );
    }

    if( ret.type == veh_coll_nothing || just_detect ) {
        // Hit nothing or we aren't actually hitting
        return ret;
    }
    stop_autodriving();
    // Calculate mass AFTER checking for collision
    //  because it involves iterating over all cargo
    // Rotors only use rotor mass in calculation.
    const float mass = ( part_info( part ).rotor_diameter() > 0 ) ?
                       to_kilogram( parts[ part ].base.weight() ) : to_kilogram( total_mass() );

    //Calculate damage resulting from d_E
    const itype *type = item::find_type( part_info( ret.part ).item );
    const auto &mats = type->materials;
    float vpart_dens = 0;
    if( !mats.empty() ) {
        for( auto &mat_id : mats ) {
            vpart_dens += mat_id.obj().density();
        }
        // average
        vpart_dens /= mats.size();
    }

    //k=100 -> 100% damage on part
    //k=0 -> 100% damage on obj
    float material_factor = ( part_dens - vpart_dens ) * 0.5;
    material_factor = std::max( -25.0f, std::min( 25.0f, material_factor ) );
    // factor = -25 if mass is much greater than mass2
    // factor = +25 if mass2 is much greater than mass
    const float weight_factor = mass >= mass2 ?
                                -25 * ( std::log( mass ) - std::log( mass2 ) ) / std::log( mass ) :
                                25 * ( std::log( mass2 ) - std::log( mass ) ) / std::log( mass2 );

    float k = 50 + material_factor + weight_factor;
    k = std::max( 10.0f, std::min( 90.0f, k ) );

    bool smashed = true;
    const std::string snd = _( "smash!" );
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
            if( std::fabs( vel1_a ) < std::fabs( vel1 ) ) {
                // Lower vehicle's speed to prevent infinite loops
                coll_velocity = vel1_a * 90;
            }
            if( std::fabs( vel2_a ) > std::fabs( vel2 ) ) {
                vel2 = vel2_a;
            }
            // this causes infinite loop
            if( mass2 == 0 ) {
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

        if( ret.type == veh_coll_bashable ) {
            // Something bashable -- use map::bash to determine outcome
            // NOTE: Floor bashing disabled for balance reasons
            //       Floor values are still used to set damage dealt to vehicle
            smashed = here.is_bashable_ter_furn( p, false ) &&
                      here.bash_resistance( p, bash_floor ) <= obj_dmg &&
                      here.bash( p, obj_dmg, false, false, false, this ).success;
            if( smashed ) {
                if( here.is_bashable_ter_furn( p, bash_floor ) ) {
                    // There's new terrain there to smash
                    smashed = false;
                    terrain_collision_data( p, bash_floor, mass2, part_dens, e );
                    ret.target_name = here.disp_name( p );
                } else if( here.impassable_ter_furn( p ) ) {
                    // There's new terrain there, but we can't smash it!
                    smashed = false;
                    ret.type = veh_coll_other;
                    mass2 = 1000;
                    e = 0.10;
                    part_dens = 80;
                    ret.target_name = here.disp_name( p );
                }
            }
        } else if( ret.type == veh_coll_body ) {
            int dam = obj_dmg * dmg_mod / 100;

            // We know critter is set for this type.  Assert to inform static
            // analysis.
            assert( critter );

            // No blood from hallucinations
            if( !critter->is_hallucination() ) {
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
                                  critter->get_armor_cut( bodypart_id( "torso" ) ) :
                                  critter->get_armor_bash( bodypart_id( "torso" ) );
                dam = std::max( 0, dam - armor );
                critter->apply_damage( driver, bodypart_id( "torso" ), dam );
                add_msg( m_debug, "Critter collision damage: %d", dam );
            }

            // Don't fling if vertical - critter got smashed into the ground
            if( !vert_coll ) {
                if( std::fabs( vel2_a ) > 10.0f ||
                    std::fabs( e * mass * vel1_a ) > std::fabs( mass2 * ( 10.0f - vel2_a ) ) ) {
                    const int angle = rng( -60, 60 );
                    // Also handle the weird case when we don't have enough force
                    // but still have to push (in such case compare momentum)
                    const float push_force = std::max<float>( std::fabs( vel2_a ), 10.1f );
                    // move.dir is where the vehicle is facing. If velocity is negative,
                    // we're moving backwards and have to adjust the angle accordingly.
                    const int angle_sum = angle + move.dir() + ( vel2_a > 0 ? 0 : 180 );
                    g->fling_creature( critter, angle_sum, push_force );
                } else if( std::fabs( vel2_a ) > std::fabs( vel2 ) ) {
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
            if( !snd.empty() ) {
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
        int turn_amount = rng( 1, 3 ) * std::sqrt( static_cast<double>( part_dmg ) );
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
    map &here = get_map();
    const trap &tr = here.tr_at( p );

    if( tr.is_null() ) {
        // If the trap doesn't exist, we can't interact with it, so just return
        return;
    }
    vehicle_handle_trap_data veh_data = tr.vehicle_data;

    if( veh_data.is_falling ) {
        return;
    }

    Character &player_character = get_player_character();
    const bool seen = player_character.sees( p );
    const bool known = tr.can_see( p, player_character );
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
            here.remove_trap( p );
            still_has_trap = false;
        }
        for( const auto &it : veh_data.spawn_items ) {
            int cnt = roll_remainder( it.second );
            if( cnt > 0 ) {
                here.spawn_item( p, it.first, cnt );
            }
        }
        if( veh_data.set_trap ) {
            here.trap_set( p, veh_data.set_trap.id() );
            still_has_trap = true;
        }
        if( still_has_trap ) {
            const trap &tr = here.tr_at( p );
            if( seen || known ) {
                // known status has been reset by map::trap_set()
                player_character.add_known_trap( p, tr );
            }
            if( seen && !known ) {
                // hard to miss!
                const std::string direction = direction_name( direction_from( player_character.pos(), p ) );
                add_msg( _( "You've spotted a %1$s to the %2$s!" ), tr.name(), direction );
            }
        }
    }
}

bool vehicle::has_harnessed_animal() const
{
    for( size_t e = 0; e < parts.size(); e++ ) {
        const vehicle_part &vp = parts[ e ];
        if( vp.info().fuel_type == fuel_type_animal ) {
            monster *mon = get_pet( e );
            if( mon && mon->has_effect( effect_harnessed ) && mon->has_effect( effect_pet ) ) {
                return true;
            }
        }
    }
    return false;
}

void vehicle::autodrive( const point &p )
{
    if( !is_towed() && !magic ) {
        for( size_t e = 0; e < parts.size(); e++ ) {
            const vehicle_part &vp = parts[ e ];
            if( vp.info().fuel_type == fuel_type_animal ) {
                monster *mon = get_pet( e );
                if( !mon || !mon->has_effect( effect_harnessed ) || !mon->has_effect( effect_pet ) ) {
                    is_following = false;
                    return;
                }
            }
        }
    }
    int turn_delta = 15 * p.x;
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
    if( p.y != 0 ) {
        int thr_amount = 100 * ( std::abs( velocity ) < 2000 ? 4 : 5 );
        if( cruise_on ) {
            cruise_thrust( -p.y * thr_amount );
        } else {
            thrust( -p.y );
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

bool vehicle::check_is_heli_landed()
{
    // @TODO - when there are chasms that extend below z-level 0 - perhaps the heli
    // will be able to descend into them but for now, assume z-level-0 == the ground.
    if( global_pos3().z == 0 || !get_map().has_flag_ter_or_furn( TFLAG_NO_FLOOR, global_pos3() ) ) {
        is_flying = false;
        return true;
    }
    return false;
}

bool vehicle::check_heli_descend( player &p )
{
    if( !is_rotorcraft() ) {
        debugmsg( "A vehicle is somehow flying without being an aircraft" );
        return true;
    }
    int count = 0;
    int air_count = 0;
    map &here = get_map();
    for( const tripoint &pt : get_points( true ) ) {
        tripoint below( pt.xy(), pt.z - 1 );
        if( here.has_zlevels() && ( pt.z < -OVERMAP_DEPTH ||
                                    !here.has_flag_ter_or_furn( TFLAG_NO_FLOOR, pt ) ) ) {
            p.add_msg_if_player( _( "You are already landed!" ) );
            return false;
        }
        const optional_vpart_position ovp = here.veh_at( below );
        if( here.impassable_ter_furn( below ) || ovp || g->critter_at( below ) ) {
            p.add_msg_if_player( m_bad,
                                 _( "It would be unsafe to try and land when there are obstacles below you." ) );
            return false;
        }
        if( here.has_flag_ter_or_furn( TFLAG_NO_FLOOR, below ) ) {
            air_count++;
        }
        count++;
    }
    if( velocity > 0 && air_count != count ) {
        p.add_msg_if_player( m_bad, _( "It would be unsafe to try and land while you are moving." ) );
        return false;
    }
    return true;

}

bool vehicle::check_heli_ascend( player &p )
{
    if( !is_rotorcraft() ) {
        debugmsg( "A vehicle is somehow flying without being an aircraft" );
        return true;
    }
    if( velocity > 0 && !is_flying_in_air() ) {
        p.add_msg_if_player( m_bad, _( "It would be unsafe to try and take off while you are moving." ) );
        return false;
    }
    map &here = get_map();
    for( const tripoint &pt : get_points( true ) ) {
        tripoint above( pt.xy(), pt.z + 1 );
        const optional_vpart_position ovp = here.veh_at( above );
        if( here.has_flag_ter_or_furn( TFLAG_INDOORS, pt ) || here.impassable_ter_furn( above ) || ovp ||
            g->critter_at( above ) ) {
            p.add_msg_if_player( m_bad,
                                 _( "It would be unsafe to try and ascend when there are obstacles above you." ) );
            return false;
        }
    }
    return true;
}

void vehicle::pldrive( const point &p, int z )
{
    player &player_character = get_avatar();
    if( z != 0 && is_rotorcraft() ) {
        player_character.moves = std::min( player_character.moves, 0 );
        thrust( 0, z );
    }
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
        player_character.moves = std::min( player_character.moves, player_character.get_speed() );

        ///\EFFECT_DEX reduces chance of losing control of vehicle when turning

        ///\EFFECT_PER reduces chance of losing control of vehicle when turning

        ///\EFFECT_DRIVING reduces chance of losing control of vehicle when turning
        float skill = std::min( 10.0f, player_character.get_skill_level( skill_driving ) +
                                ( player_character.get_dex() + player_character.get_per() ) / 10.0f );
        float penalty = rng_float( 0.0f, handling_diff ) - skill;
        int cost;
        if( penalty > 0.0f ) {
            // At 10 penalty (rather hard to get), we're taking 4 turns per turn
            cost = 100 * ( 1.0f + penalty / 2.5f );
        } else {
            // At 10 skill, with a perfect vehicle, we could turn up to 3 times per turn
            cost = std::max( player_character.get_speed(), 100 ) * ( 1.0f - ( -penalty / 10.0f ) * 2 / 3 );
        }

        if( penalty > skill || cost > 400 ) {
            add_msg( m_warning, _( "You fumble with the %s's controls." ), name );
            // Anything from a wasted attempt to 2 turns in the intended direction
            turn_delta *= rng( 0, 2 );
            // Also wastes next turn
            cost = std::max( cost, player_character.moves + 100 );
        } else if( one_in( 10 ) ) {
            // Don't warn all the time or it gets spammy
            if( cost >= player_character.get_speed() * 2 ) {
                add_msg( m_warning, _( "It takes you a very long time to steer that vehicle!" ) );
            } else if( cost >= player_character.get_speed() * 1.5f ) {
                add_msg( m_warning, _( "It takes you a long time to steer that vehicle!" ) );
            }
        }

        turn( turn_delta );

        // At most 3 turns per turn, because otherwise it looks really weird and jumpy
        player_character.moves -= std::max( cost, player_character.get_speed() / 3 + 1 );
    }

    if( p.y != 0 ) {
        int thr_amount = 100 * ( std::abs( velocity ) < 2000 ? 4 : 5 );
        if( cruise_on ) {
            cruise_thrust( -p.y * thr_amount );
        } else {
            thrust( -p.y );
            player_character.moves = std::min( player_character.moves, 0 );
        }
    }

    // TODO: Actually check if we're on land on water (or disable water-skidding)
    // Only check for recovering from a skid if we did active steering (not cruise control).
    if( skidding && ( p.x != 0 || ( p.y != 0 && !cruise_on ) ) && valid_wheel_config() ) {
        ///\EFFECT_DEX increases chance of regaining control of a vehicle

        ///\EFFECT_DRIVING increases chance of regaining control of a vehicle
        if( handling_diff * rng( 1, 10 ) <
            player_character.dex_cur + player_character.get_skill_level( skill_driving ) * 2 ) {
            add_msg( _( "You regain control of the %s." ), name );
            player_character.practice( skill_driving, velocity / 5 );
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

    if( std::fabs( dot ) * 100 > dice( 9, 20 ) ) {
        add_msg( _( "The %s recovers from its skid." ), name );
        // face_vec takes over.
        skidding = false;
        // Wheels absorb horizontal velocity.
        velocity *= dot;
        if( dot < -.8 ) {
            // Pointed backwards, velo-wise.
            // Move backwards.
            velocity *= -1;
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
    return rl_vec2d( std::cos( degrees * M_PI / 180 ), std::sin( degrees * M_PI / 180 ) );
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
    // The direction we're moving
    tileray mdir;
    // calculate direction after turn
    mdir.init( new_turn_dir );
    tripoint dp;
    bool is_diagonal_movement = new_turn_dir % 90 == 45;

    if( std::abs( velocity ) >= 20 ) {
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

    map &here = get_map();
    for( int part_index : wheelcache ) {
        const auto &wheel = parts[ part_index ];
        bool rails_ahead = true;
        tripoint wheel_point;
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

            if( !here.has_flag_ter_or_furn( ter_flag_to_check, wheel_tripoint ) ) {
                // this tile is not allowed, disallow turn
                rails_ahead = false;
                break;
            }

            // special case for rails
            if( check_rail_direction ) {
                ter_id terrain_at_wheel = here.ter( wheel_tripoint );
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
        // found a wheel that turns correctly on rails
        if( rails_ahead ) {
            // if wheel that lands on rail still not found
            if( yVal == INT_MAX ) {
                // store mount point.y of wheel
                yVal = wheel.mount.y;
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
    // driver tried to turn rails vehicle
    if( turn_dir != face.dir() ) {
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
    // check if autoturn is possible
    if( turn_dir == face.dir() ) {
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
                ( std::abs( rail_wheel_bounding_box.p2.x - rail_wheel_bounding_box.p1.x ) < 4 && velocity < 0 ) );
    // allow turn for vehicles with wheel distance < 4 when moving backwards
}

vehicle *vehicle::act_on_map()
{
    const tripoint pt = global_pos3();
    map &here = get_map();
    if( !here.inbounds( pt ) ) {
        dbg( D_INFO ) << "stopping out-of-map vehicle.  (x,y,z)=(" << pt.x << "," << pt.y << "," << pt.z <<
                      ")";
        stop( false );
        of_turn = 0;
        is_falling = false;
        return this;
    }
    if( decrement_summon_timer() ) {
        return nullptr;
    }
    Character &player_character = get_player_character();
    const bool pl_ctrl = player_in_control( player_character );
    // TODO: Remove this hack, have vehicle sink a z-level
    if( is_floating && !can_float() ) {
        add_msg( m_bad, _( "Your %s sank." ), name );
        if( pl_ctrl ) {
            unboard_all();
        }
        if( g->remoteveh() == this ) {
            g->setremoteveh( nullptr );
        }

        here.on_vehicle_moved( sm_pos.z );
        // Destroy vehicle (sank to nowhere)
        here.destroy_vehicle( this );
        return nullptr;
    }

    // It needs to fall when it has no support OR was falling before
    //  so that vertical collisions happen.
    const bool should_fall = is_falling || vertical_velocity != 0;

    // TODO: Saner diagonal movement, so that you can jump off cliffs properly
    // The ratio of vertical to horizontal movement should be vertical_velocity/velocity
    //  for as long as of_turn doesn't run out.
    if( should_fall ) {
        // 9.8 m/s^2
        const float g = 9.8f;
        // Convert from 100*mph to m/s
        const float old_vel = vmiph_to_mps( vertical_velocity );
        // Formula is v_2 = sqrt( 2*d*g + v_1^2 )
        // Note: That drops the sign
        const float new_vel = -std::sqrt( 2 * tile_height * g + old_vel * old_vel );
        vertical_velocity = mps_to_vmiph( new_vel );
        is_falling = true;
    } else {
        // Not actually falling, was just marked for fall test
        is_falling = false;
    }

    // Low enough for bicycles to go in reverse.
    // If the movement is due to a change in z-level, i.e a helicopter then the lateral movement will often be zero.
    if( !should_fall && std::abs( velocity ) < 20 && requested_z_change == 0 ) {
        stop();
        of_turn -= .321f;
        return this;
    }

    const float wheel_traction_area = here.vehicle_wheel_traction( *this );
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
    const float turn_cost = vehicles::vmiph_per_tile / std::max<float>( 0.0001f, std::abs( velocity ) );

    // Can't afford it this turn?
    // Low speed shouldn't prevent vehicle from falling, though
    bool falling_only = false;
    if( turn_cost >= of_turn && ( ( !is_flying && requested_z_change == 0 ) || !is_rotorcraft() ) ) {
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
        if( !controlled && !pl_ctrl && !is_floating && !can_use_rails && !is_flying &&
            requested_z_change == 0 ) {
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

    // The direction we're moving
    tileray mdir;
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
    if( std::abs( velocity ) >= 20 && !falling_only ) {
        mdir.advance( velocity < 0 ? -1 : 1 );
        dp.x = mdir.dx();
        dp.y = mdir.dy();
    }

    if( should_fall ) {
        dp.z = -1;
        is_flying = false;
    } else {
        dp.z = requested_z_change;
        requested_z_change = 0;
        if( dp.z > 0 && is_rotorcraft() ) {
            is_flying = true;
        }
    }

    return here.move_vehicle( *this, dp, mdir );
}

bool vehicle::level_vehicle()
{
    map &here = get_map();
    if( !here.has_zlevels() || ( is_flying && is_rotorcraft() ) ) {
        return true;
    }
    // make sure that all parts are either supported across levels or on the same level
    std::map<int, bool> no_support;
    for( vehicle_part &prt : parts ) {
        if( prt.info().location != part_location_structure ) {
            continue;
        }
        const tripoint part_pos = global_part_pos3( prt );
        if( no_support.find( part_pos.z ) == no_support.end() ) {
            no_support[part_pos.z] = part_pos.z > -OVERMAP_DEPTH;
        }
        if( no_support[part_pos.z] ) {
            no_support[part_pos.z] = here.has_flag_ter_or_furn( TFLAG_NO_FLOOR, part_pos ) &&
                                     !here.supports_above( part_pos + tripoint_below );
        }
    }
    std::set<int> dropped_parts;
    // if it's unsupported but on the same level, just let it fall
    bool center_drop = false;
    bool adjust_level = false;
    if( no_support.size() > 1 ) {
        for( int zlevel = -OVERMAP_DEPTH; zlevel <= OVERMAP_DEPTH; zlevel++ ) {
            if( no_support.find( zlevel ) == no_support.end() || !no_support[zlevel] ) {
                continue;
            }
            center_drop |= global_pos3().z == zlevel;
            adjust_level = true;
            // drop unsupported parts 1 zlevel
            for( size_t prt = 0; prt < parts.size(); prt++ ) {
                if( global_part_pos3( prt ).z == zlevel ) {
                    dropped_parts.insert( static_cast<int>( prt ) );
                }
            }
        }
    }
    if( adjust_level ) {
        here.displace_vehicle( *this, tripoint_below, center_drop, dropped_parts );
        return false;
    } else {
        return true;
    }
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
        is_flying = false;
        return;
    }

    map &here = get_map();
    is_falling = here.has_zlevels();

    if( is_flying && is_rotorcraft() ) {
        is_falling = false;
    } else {
        is_flying = false;
    }

    size_t deep_water_tiles = 0;
    size_t water_tiles = 0;
    for( const tripoint &p : pts ) {
        if( is_falling ) {
            tripoint below( p.xy(), p.z - 1 );
            is_falling &= here.has_flag_ter_or_furn( TFLAG_NO_FLOOR, p ) &&
                          ( p.z > -OVERMAP_DEPTH ) && !here.supports_above( below );
        }
        deep_water_tiles += here.has_flag( TFLAG_DEEP_WATER, p ) ? 1 : 0;
        water_tiles += here.has_flag( TFLAG_SWIMMABLE, p ) ? 1 : 0;
    }
    // floating if 2/3rds of the vehicle is in deep water
    is_floating = 3 * deep_water_tiles >= 2 * pts.size();
    // in_water if 1/2 of the vehicle is in water at all
    in_water =  2 * water_tiles >= pts.size();
}

float map::vehicle_wheel_traction( const vehicle &veh,
                                   const bool ignore_movement_modifiers /*=false*/ ) const
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
        const int wheel_area = veh.cpart( p ).wheel_area();

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

        // Ignore the movement modifier if needed.
        if( ignore_movement_modifiers ) {
            move_mod = 2;
        }

        traction_wheel_area += 2.0 * wheel_area / move_mod;
    }

    return traction_wheel_area;
}

int map::shake_vehicle( vehicle &veh, const int velocity_before, const int direction )
{
    const int d_vel = std::abs( veh.velocity - velocity_before ) / 100;

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
            veh.part( ps ).remove_flag( vehicle_part::passenger_flag );
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
                pet->apply_damage( nullptr, bodypart_id( "torso" ), dmg );
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
                int turn_amount = rng( 1, 3 ) * std::sqrt( std::abs( veh.velocity ) ) / 30;
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
            } else if( get_player_character().sees( part_pos ) ) {
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
