#include "vehicle.h" // IWYU pragma: associated

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
#include <tuple>

#include "avatar.h"
#include "bodypart.h"
#include "cata_assert.h"
#include "cata_utility.h"
#include "character.h"
#include "creature.h"
#include "creature_tracker.h"
#include "debug.h"
#include "enums.h"
#include "explosion.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "material.h"
#include "messages.h"
#include "monster.h"
#include "options.h"
#include "rng.h"
#include "sounds.h"
#include "translations.h"
#include "trap.h"
#include "units.h"
#include "units_utility.h"
#include "veh_type.h"
#include "vpart_position.h"
#include "vpart_range.h"

#define dbg(x) DebugLog((x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

static const damage_type_id damage_bash( "bash" );
static const damage_type_id damage_cut( "cut" );

static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_harnessed( "harnessed" );
static const efftype_id effect_pet( "pet" );
static const efftype_id effect_stunned( "stunned" );

static const itype_id fuel_type_animal( "animal" );
static const itype_id fuel_type_battery( "battery" );
static const itype_id fuel_type_muscle( "muscle" );

static const proficiency_id proficiency_prof_boat_pilot( "prof_boat_pilot" );
static const proficiency_id proficiency_prof_driver( "prof_driver" );

static const skill_id skill_driving( "driving" );

static const ter_str_id ter_t_railroad_track( "t_railroad_track" );
static const ter_str_id ter_t_railroad_track_d( "t_railroad_track_d" );
static const ter_str_id ter_t_railroad_track_d1( "t_railroad_track_d1" );
static const ter_str_id ter_t_railroad_track_d2( "t_railroad_track_d2" );
static const ter_str_id ter_t_railroad_track_d_on_tie( "t_railroad_track_d_on_tie" );
static const ter_str_id ter_t_railroad_track_h( "t_railroad_track_h" );
static const ter_str_id ter_t_railroad_track_h_on_tie( "t_railroad_track_h_on_tie" );
static const ter_str_id ter_t_railroad_track_on_tie( "t_railroad_track_on_tie" );
static const ter_str_id ter_t_railroad_track_v( "t_railroad_track_v" );
static const ter_str_id ter_t_railroad_track_v_on_tie( "t_railroad_track_v_on_tie" );

static const trait_id trait_DEFT( "DEFT" );
static const trait_id trait_PROF_SKATER( "PROF_SKATER" );

static const std::string part_location_structure( "structure" );

// tile height in meters
static const float tile_height = 4.0f;
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
        const double skid_factor = 1 + 24 * std::abs( units::sin( face.dir() - move.dir() ) );
        f_total_drag += f_rolling_drag * skid_factor;
    }
    // check mass to make sure it's not 0 which happens for some reason
    double accel_slowdown = total_mass().value() > 0 ? f_total_drag / to_kilogram( total_mass() ) : 0;
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
    add_msg_debug( debugmode::DF_VEHICLE_MOVE,
                   "%s at %d vimph, f_drag %3.2f, drag accel %d vmiph - extra drag %d",
                   name, at_velocity, f_total_drag, slowdown, units::to_watt( static_drag() ) );
    // plows slow rolling vehicles, but not falling or floating vehicles
    if( !( is_falling || ( is_watercraft() && can_float() ) || is_flying ) ) {
        slowdown -= units::to_watt( static_drag() );
    }

    return std::max( 1, slowdown );
}

void vehicle::smart_controller_handle_turn( const std::optional<float> &k_traction_cache )
{
    // get settings or defaults
    smart_controller_config cfg = smart_controller_cfg.value_or( smart_controller_config() );

    if( !has_enabled_smart_controller ) {
        smart_controller_state = std::nullopt;
        return;
    }

    if( smart_controller_state && smart_controller_state->created == calendar::turn ) {
        return;
    }

    // controlled engines
    // note: contains indices of of elements in `engines` array, not the part ids
    std::vector<int> c_engines;
    bool has_electric_engine = false;
    for( int i = 0; i < static_cast<int>( engines.size() ); ++i ) {
        const vehicle_part &vp = parts[engines[i]];
        const bool is_electric = is_engine_type( vp, fuel_type_battery );
        if( ( is_electric || is_engine_type_combustion( vp ) ) &&
            ( ( vp.is_available() && engine_fuel_left( vp ) ) || vp.enabled ) ) {
            c_engines.push_back( i );
            if( is_electric ) {
                has_electric_engine = true;
            }
        }
    }

    bool rotorcraft = is_flying && is_rotorcraft();

    Character &player_character = get_player_character();

    // bail and shut down
    if( rotorcraft || c_engines.empty() || ( has_electric_engine && c_engines.size() == 1 ) ||
        c_engines.size() > 5 ) {
        for( const vpart_reference &vp : get_avail_parts( "SMART_ENGINE_CONTROLLER" ) ) {
            vp.part().enabled = false;
        }

        if( player_in_control( player_character ) ) {
            if( rotorcraft ) {
                add_msg( _( "Smart controller does not support flying vehicles." ) );
            } else if( c_engines.empty() ) {
                //TODO: make translation
                add_msg( _( "Smart controller can not detect any controllable engine." ) );
            } else if( c_engines.size() == 1 ) {
                //TODO: make translation
                add_msg( _( "Smart controller detects only a single electric engine." ) );
                add_msg( _( "An electric engine does not need optimization." ) );
            } else {
                add_msg( _( "Smart controller does not support more than five engines." ) );
            }
            add_msg( m_bad, _( "Smart controller is shutting down." ) );
        }
        has_enabled_smart_controller = false;
        smart_controller_state = std::nullopt;
        return;
    }

    int cur_battery_level;
    int max_battery_level;
    std::tie( cur_battery_level, max_battery_level ) = battery_power_level();
    int battery_level_percent = max_battery_level == 0 ? 0 : cur_battery_level * 100 /
                                max_battery_level;

    // ensure sane values
    cfg.battery_hi = clamp( cfg.battery_hi, 0, 100 );
    cfg.battery_lo = clamp( cfg.battery_lo, 0, cfg.battery_hi );

    // when battery > 90%, discharge is allowed
    // otherwise trying to charge battery to 90% within 30 minutes
    bool discharge_forbidden_soft = battery_level_percent <= cfg.battery_hi;
    bool discharge_forbidden_hard = battery_level_percent <= cfg.battery_lo;
    units::power target_charging_rate;
    if( max_battery_level == 0 || !discharge_forbidden_soft ) {
        target_charging_rate = 0_W;
    } else {
        target_charging_rate = units::from_watt( static_cast<std::int64_t>( ( max_battery_level *
                               cfg.battery_hi / 100 -
                               cur_battery_level ) * 10 / ( 6 * 3 ) ) );
    }
    //      ( max_battery_level * battery_hi / 100 - cur_battery_level )  * (1000 / (60 * 30))   // originally
    //                                ^ battery_hi%                  bat to W ^         ^ 30 minutes

    // using avg_velocity reduces unnecessary oscillations when traction is low
    int accel_demand = std::max( std::abs( cruise_velocity - velocity ),
                                 std::abs( cruise_velocity - avg_velocity ) );
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
    units::power opt_net_echarge_rate = net_battery_charge_rate( /* include_reactors = */ true );
    // total engine fuel energy usage (J)
    units::power opt_fuel_usage = 0_W;

    int opt_accel = is_stationary ? 1 : current_acceleration() * traction;
    int opt_safe_vel = is_stationary ? 1 : safe_ground_velocity( true );
    float cur_load_approx = static_cast<float>( std::min( accel_demand,
                            opt_accel ) )  / std::max( opt_accel, 1 );
    float cur_load_alternator = std::min( 0.01f, static_cast<float>( alternator_load ) / 1000 );

    for( size_t i = 0; i < c_engines.size(); ++i ) {
        const vehicle_part &vp = parts[engines[c_engines[i]]];
        if( is_engine_on( vp ) ) {
            const bool is_electric = is_engine_type( vp, fuel_type_battery );
            prev_mask |= 1 << i;
            units::power fu = engine_fuel_usage( vp ) * ( cur_load_approx + ( is_electric ? 0 :
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

    // turn on/off combustion engines when necessary
    if( !has_electric_engine ) {
        Character &player_character = get_player_character();
        if( !discharge_forbidden_soft && is_stationary && engine_on && !autopilot_on &&
            !player_in_control( player_character ) ) {
            stop_engines();
            sfx::do_vehicle_engine_sfx();
            // temporary solution
        } else if( discharge_forbidden_hard && !engine_on && cur_battery_level > 0 ) {
            engine_on = true;
            sfx::do_vehicle_engine_sfx();
        }
    }

    // trying all combinations of engine state (max 31 iterations for 5 engines)
    for( int mask = 1; mask < static_cast<int>( 1 << c_engines.size() ); ++mask ) {
        if( mask == prev_mask ) {
            continue;
        }

        bool gas_engine_to_shut_down = false;
        for( size_t i = 0; i < c_engines.size(); ++i ) {
            vehicle_part &vp = parts[engines[c_engines[i]]];
            bool old_state = ( prev_mask & ( 1 << i ) ) != 0;
            bool new_state = ( mask & ( 1 << i ) ) != 0;
            // switching enabled flag temporarily to perform calculations below
            vp.enabled = new_state;

            if( old_state && !new_state && !is_engine_type( vp, fuel_type_battery ) ) {
                gas_engine_to_shut_down = true;
            }
        }

        if( gas_engine_to_shut_down && gas_engine_shutdown_forbidden ) {
            continue; // skip checking this state
        }

        int safe_vel =  is_stationary ? 1 : safe_ground_velocity( true );
        int accel = is_stationary ? 1 : current_acceleration() * traction;
        units::power fuel_usage = 0_W;
        units::power net_echarge_rate = net_battery_charge_rate( /* include_reactors = */ true );
        float load_approx = static_cast<float>( std::min( accel_demand, accel ) ) / std::max( accel, 1 );
        update_alternator_load();
        float load_approx_alternator  = std::min( 0.01f, static_cast<float>( alternator_load ) / 1000 );

        for( int e : c_engines ) {
            const vehicle_part &vp = parts[engines[e]];
            const bool is_electric = is_engine_type( vp, fuel_type_battery );
            units::power fu = engine_fuel_usage( vp ) * ( load_approx + ( is_electric ? 0 :
                              load_approx_alternator ) );
            fuel_usage += fu;
            if( is_electric ) {
                net_echarge_rate -= fu;
            }
        }

        if( std::forward_as_tuple(
                !discharge_forbidden_hard || ( net_echarge_rate > 0_W ),
                accel >= accel_demand,
                opt_accel < accel_demand ? accel : 0, // opt_accel usage here is intentional
                safe_vel >= velocity_demand,
                opt_safe_vel < velocity_demand ? -safe_vel : 0, //opt_safe_vel usage here is intentional
                !discharge_forbidden_soft || ( net_echarge_rate > target_charging_rate ),
                -fuel_usage,
                net_echarge_rate
            ) >= std::forward_as_tuple(
                !discharge_forbidden_hard || ( opt_net_echarge_rate > 0_W ),
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
        vehicle_part &vp = parts[engines[c_engines[i]]];
        vp.enabled = static_cast<bool>( prev_mask & ( 1 << i ) );
    }

    if( opt_mask != prev_mask ) { // we found new configuration
        bool failed_to_start = false;
        bool turned_on_gas_engine = false;
        for( size_t i = 0; i < c_engines.size(); ++i ) {
            vehicle_part &vp = parts[engines[c_engines[i]]];
            // ..0.. < ..1..  was off, new state on
            if( ( prev_mask & ( 1 << i ) ) < ( opt_mask & ( 1 << i ) ) ) {
                if( !start_engine( vp, true ) ) {
                    failed_to_start = true;
                }
                turned_on_gas_engine |= !is_engine_type( vp, fuel_type_battery );
            }
        }
        if( failed_to_start ) {
            this->smart_controller_state = std::nullopt;

            for( size_t i = 0; i < c_engines.size(); ++i ) { // return to prev state
                vehicle_part &vp = parts[engines[c_engines[i]]];
                vp.enabled = static_cast<bool>( prev_mask & ( 1 << i ) );
            }
            for( const vpart_reference &vp : get_avail_parts( "SMART_ENGINE_CONTROLLER" ) ) {
                vp.part().enabled = false;
            }
            if( player_in_control( player_character ) ) {
                add_msg( m_bad, _( "Smart controller failed to start an engine." ) );
                add_msg( m_bad, _( "Smart controller is shutting down." ) );
            }
            has_enabled_smart_controller = false;

        } else {  //successfully changed engines state
            for( size_t i = 0; i < c_engines.size(); ++i ) {
                vehicle_part &vp = parts[engines[c_engines[i]]];
                // was on, needs to be off
                if( ( prev_mask & ( 1 << i ) ) > ( opt_mask & ( 1 << i ) ) ) {
                    start_engine( vp, false );
                }
            }
            if( turned_on_gas_engine ) {
                cur_state.gas_engine_last_turned_on = calendar::turn;
            }
            smart_controller_state = cur_state;

            if( player_in_control( player_character ) ) {
                add_msg_debug( debugmode::DF_VEHICLE_MOVE, "Smart controller optimizes engine state." );
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
    if( ( is_watercraft() && can_float() ) || ( is_rotorcraft() && ( z != 0 || is_flying ) ) ) {
        // we're good
    } else if( in_deep_water && !can_float() ) {
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
        smart_controller_handle_turn( traction );
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
            if( has_engine_type( fuel_type_muscle, true ) ) {
                add_msg( _( "The %s is too heavy to move!" ), name );
            } else {
                add_msg( n_gettext( "The %s is too heavy for its engine!",
                                    "The %s is too heavy for its engines!",
                                    engines.size() ), name );
            }
        }
        return;
    }
    const int max_vel = traction * max_velocity();
    // maximum braking is 20 mph/s, assumes high friction tires
    const int max_brake = 20 * 100;
    //pos or neg if accelerator or brake
    int vel_inc = ( accel + ( thrusting ? 0 : max_brake ) ) * thd;
    // Reverse is only 60% acceleration, unless an electric motor is in use
    if( thd == -1 && thrusting && !has_engine_type( fuel_type_battery, true ) ) {
        vel_inc = .6 * vel_inc;
    }

    //find ratio of used acceleration to maximum available, returned in tenths of a percent
    //so 1000 = 100% and 453 = 45.3%
    int load;
    if( accel != 0 ) {
        int effective_cruise = std::min( cruise_velocity, max_vel );
        if( thd > 0 ) {
            vel_inc = std::min( vel_inc, effective_cruise - velocity );
        } else {
            vel_inc = std::max( vel_inc, effective_cruise - velocity );
        }
        if( thrusting ) {
            load = 1000 * std::abs( vel_inc ) / accel;
        } else {
            // brakes provide 20 mph/s of slowdown and the rest is engine braking
            // TODO: braking depends on wheels, traction, driver skill
            load = 1000 * std::max( 0, std::abs( vel_inc ) - max_brake ) / accel;
        }
    } else {
        load = ( thrusting ? 1000 : 0 );
    }
    // rotorcraft need to spend 15% of load to hover, 30% to change z
    if( is_rotorcraft() && ( z > 0 || is_flying_in_air() ) ) {
        load = std::max( load, z > 0 ? 300 : 150 );
        thrusting = true;
    }

    // only consume resources if engine accelerating
    if( load >= 1 && thrusting ) {
        //abort if engines not operational
        if( total_power() <= 0_W || !engine_on || ( z == 0 && accel == 0 ) ) {
            if( pl_ctrl ) {
                if( total_power( false ) <= 0_W ) {
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
        noise_and_smoke( load + alternator_load );
        consume_fuel( load + alternator_load, false );
        if( z != 0 && is_rotorcraft() ) {
            requested_z_change = z;
        }
        //break the engines a bit, if going too fast.
        const int strn = static_cast<int>( strain() * strain() * 100 );
        for( const int p : engines ) {
            do_engine_damage( parts[p], strn );
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
        vehicle_part &vp = parts[e];
        if( vp.info().fuel_type == fuel_type_animal && engines.size() != 1 ) {
            monster *mon = get_monster( e );
            if( mon != nullptr && mon->has_effect( effect_harnessed ) ) {
                if( velocity > mon->get_speed() * 12 ) {
                    add_msg( m_bad, _( "Your %s is not fast enough to keep up with the %s" ), mon->get_name(), name );
                    int dmg = rng( 0, 10 );
                    damage_direct( get_map(), vp, dmg );
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
    int max_vel = autopilot_on ? safe_velocity() : max_velocity();
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

void vehicle::turn( units::angle deg )
{
    if( deg == 0_degrees ) {
        return;
    }
    if( velocity < 0 && !::get_option<bool>( "REVERSE_STEERING" ) ) {
        deg = -deg;
    }
    last_turn = deg;
    turn_dir = normalize( turn_dir + deg );
    // quick rounding the turn dir to a multiple of 15
    turn_dir = round_to_multiple_of( turn_dir, vehicles::steer_increment );
}

void vehicle::stop()
{
    velocity = 0;
    skidding = false;
    move = face;
    last_turn = 0_degrees;
    of_turn_carry = 0;
    map &here = get_map();
    for( const tripoint &p : get_points() ) {
        if( here.inbounds( p ) ) {
            here.memory_cache_dec_set_dirty( p, true );
        }
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
    for( int p = 0; p < part_count(); p++ ) {
        const vehicle_part &vp = parts.at( p );
        if( vp.removed || !vp.is_real_or_active_fake() ) {
            continue;
        }

        const vpart_info &info = vp.info();
        if( !vp.is_fake && info.location != part_location_structure && !info.has_flag( VPFLAG_ROTOR ) ) {
            continue;
        }
        empty = false;
        // Coordinates of where part will go due to movement (dx/dy/dz)
        //  and turning (precalc[1])
        const tripoint dsp = global_pos3() + dp + vp.precalc[1];
        veh_collision coll = part_collision( p, dsp, just_detect, bash_floor );
        if( coll.type == veh_coll_nothing && info.has_flag( VPFLAG_ROTOR ) ) {
            size_t radius = static_cast<size_t>( std::round( info.rotor_info->rotor_diameter / 2.0f ) );
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
        add_msg_debug( debugmode::DF_VEHICLE_MOVE, "Collision check on a dirty vehicle %s", name );
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
    Creature *critter = get_creature_tracker().creature_at( p, true );
    Character *ph = dynamic_cast<Character *>( critter );

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
    int &coll_velocity = ( parts[part].info().has_flag( VPFLAG_ROTOR ) == 0 ) ?
                         ( vert_coll ? vertical_velocity : velocity ) :
                         rotor_velocity;
    if( !just_detect && coll_velocity == 0 ) {
        return ret;
    }

    if( is_body_collision ) {
        // critters on a BOARDABLE part in this vehicle aren't colliding
        if( ovp && ( &ovp->vehicle() == this ) && get_monster( ovp->part_index() ) ) {
            return ret;
        }
        // we just ran into a fish, so move it out of the way
        if( here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, critter->pos() ) ) {
            tripoint end_pos = critter->pos();
            tripoint start_pos;
            const units::angle angle =
                move.dir() + 45_degrees * ( parts[part].mount.x > pivot_point().x ? -1 : 1 );
            const std::set<tripoint> &cur_points = get_points( true );
            // push the animal out of way until it's no longer in our vehicle and not in
            // anyone else's position
            while( get_creature_tracker().creature_at( end_pos, true ) ||
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
    vehicle_part &vp = this->part( ret.part );
    const vpart_info &vpi = vp.info();
    // Let's calculate type of collision & mass of object we hit
    float mass2 = 0.0f;
    // e = 0 -> plastic collision
    float e = 0.3f;
    // e = 1 -> inelastic collision
    //part density
    float part_dens = 0.0f;

    if( is_body_collision ) {
        // Check any monster/NPC/player on the way
        // body
        ret.type = veh_coll_body;
        ret.target = critter;
        e = 0.30f;
        part_dens = 15;
        mass2 = units::to_kilogram( critter->get_weight() );
        ret.target_name = critter->disp_name();
    } else if( ( bash_floor && here.is_bashable_ter_furn( p, true ) ) ||
               ( here.is_bashable_ter_furn( p, false ) && here.move_cost_ter_furn( p ) != 2 &&
                 // Don't collide with tiny things, like flowers, unless we have a wheel in our space.
                 ( part_with_feature( ret.part, VPFLAG_WHEEL, true ) >= 0 ||
                   !here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_TINY, p ) ) &&
                 // Protrusions don't collide with short terrain.
                 // Tiny also doesn't, but it's already excluded unless there's a wheel present.
                 !( part_with_feature( vp.mount, "PROTRUSION", true ) >= 0 &&
                    here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_SHORT, p ) ) &&
                 // These are bashable, but don't interact with vehicles.
                 !here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_NOCOLLIDE, p ) &&
                 // Do not collide with track tiles if we can use rails
                 !( here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_RAIL, p ) && this->can_use_rails() ) ) ) {
        // Movecost 2 indicates flat terrain like a floor, no collision there.
        ret.type = veh_coll_bashable;
        terrain_collision_data( p, bash_floor, mass2, part_dens, e );
        ret.target_name = here.disp_name( p );
    } else if( here.impassable_ter_furn( p ) ||
               ( bash_floor && !here.has_flag( ter_furn_flag::TFLAG_NO_FLOOR, p ) ) ) {
        // not destructible
        ret.type = veh_coll_other;
        mass2 = 1000;
        e = 0.10f;
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
    const float mass = vpi.has_flag( VPFLAG_ROTOR )
                       ? to_kilogram( vp.base.weight() )
                       : to_kilogram( total_mass() );

    //Calculate damage resulting from d_E
    const itype *type = item::find_type( vpi.base_item );
    const auto &mats = type->materials;
    float mat_total = type->mat_portion_total == 0 ? 1 : type->mat_portion_total;
    float vpart_dens = 0.0f;
    if( !mats.empty() ) {
        for( const std::pair<const material_id, int> &mat_id : mats ) {
            vpart_dens += mat_id.first->density() * ( static_cast<float>( mat_id.second ) / mat_total );
        }
        // average
        vpart_dens /= mats.size();
    }

    //k=100 -> 100% damage on part
    //k=0 -> 100% damage on obj
    float material_factor = ( part_dens - vpart_dens ) * 0.5f;
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

        add_msg_debug( debugmode::DF_VEHICLE_MOVE, "Deformation energy: %.2f", d_E );
        // Damage calculation
        // Damage dealt overall
        dmg += d_E / 400;
        // Damage for vehicle-part
        // Always if no critters, otherwise if critter is real
        if( critter == nullptr || !critter->is_hallucination() ) {
            part_dmg = dmg * k / 100.0f;
            add_msg_debug( debugmode::DF_VEHICLE_MOVE, "Part collision damage: %.2f", part_dmg );
        }
        // Damage for object
        const float obj_dmg = dmg * ( 100.0f - k ) / 100.0f;

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
                    e = 0.10f;
                    part_dens = 80;
                    ret.target_name = here.disp_name( p );
                }
            }
        } else if( ret.type == veh_coll_body ) {
            int dam = obj_dmg * vpi.dmg_mod / 100;

            // We know critter is set for this type.  Assert to inform static
            // analysis.
            cata_assert( critter );

            // No blood from hallucinations
            if( !critter->is_hallucination() ) {
                if( vpi.has_flag( "SHARP" ) ) {
                    vp.blood += 100 + 5 * dam;
                } else if( dam > rng( 10, 30 ) ) {
                    vp.blood += 50 + dam / 2 * 5;
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
                const int armor = vpi.has_flag( "SHARP" ) ?
                                  critter->get_armor_type( damage_cut, bodypart_id( "torso" ) ) :
                                  critter->get_armor_type( damage_bash, bodypart_id( "torso" ) );
                dam = std::max( 0, dam - armor );
                critter->apply_damage( driver, bodypart_id( "torso" ), dam );
                if( vpi.has_flag( "SHARP" ) ) {
                    critter->add_effect( effect_source( driver ), effect_bleed, 1_minutes * rng( 1, dam ),
                                         critter->get_random_body_part_of_type( body_part_type::type::torso ) );
                } else if( dam > 18 && rng( 1, 20 ) > 15 ) {
                    //low chance of lighter bleed even with non sharp objects.
                    critter->add_effect( effect_source( driver ), effect_bleed, 1_minutes,
                                         critter->get_random_body_part_of_type( body_part_type::type::torso ) );
                }
                add_msg_debug( debugmode::DF_VEHICLE_MOVE, "Critter collision damage: %d", dam );
            }

            // Don't fling if vertical - critter got smashed into the ground
            if( !vert_coll ) {
                if( std::fabs( vel2_a ) > 10.0f ||
                    std::fabs( e * mass * vel1_a ) > std::fabs( mass2 * ( 10.0f - vel2_a ) ) ) {
                    const units::angle angle = rng_float( -60_degrees, 60_degrees );
                    // Also handle the weird case when we don't have enough force
                    // but still have to push (in such case compare momentum)
                    const float push_force = std::max<float>( std::fabs( vel2_a ), 10.1f );
                    // move.dir is where the vehicle is facing. If velocity is negative,
                    // we're moving backwards and have to adjust the angle accordingly.
                    const units::angle angle_sum =
                        angle + move.dir() + ( vel2_a > 0 ? 0_degrees : 180_degrees );
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
            coll_velocity = vel1_a * 100;
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
                             name, vp.name(), ret.target_name );
                } else {
                    //~ 1$s - vehicle name, 2$s - part name, 3$s - NPC or monster
                    add_msg( m_warning, _( "Your %1$s's %2$s rams into %3$s!" ),
                             name, vp.name(), ret.target_name );
                }
            }

            if( vpi.has_flag( "SHARP" ) ) {
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
                         name, vp.name(), ret.target_name, snd );
            } else {
                //~ 1$s - vehicle name, 2$s - part name, 3$s - collision object name
                add_msg( m_warning, _( "Your %1$s's %2$s rams into %3$s." ),
                         name, vp.name(), ret.target_name );
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
            turn( units::from_degrees( one_in( 2 ) ? turn_amount : -turn_amount ) );
        }
    }

    ret.imp = part_dmg;
    return ret;
}

void vehicle::handle_trap( const tripoint &p, vehicle_part &vp_wheel )
{
    if( !vp_wheel.info().has_flag( VPFLAG_WHEEL ) ) {
        debugmsg( "vehicle::handle_trap called on non-WHEEL part" );
        return;
    }
    map &here = get_map();
    const trap &tr = here.tr_at( p );

    if( tr.is_null() ) {
        // If the trap doesn't exist, we can't interact with it, so just return
        return;
    }
    const vehicle_handle_trap_data &veh_data = tr.vehicle_data;

    if( veh_data.is_falling ) {
        return;
    }

    Character &player_character = get_player_character();
    const bool seen = player_character.sees( p );
    const bool known = tr.can_see( p, player_character );
    const bool damage_done = vp_wheel.info().durability <= veh_data.damage;
    if( seen && damage_done ) {
        if( known ) {
            //~ %1$s: name of the vehicle; %2$s: name of the related vehicle part; %3$s: trap name
            add_msg( m_bad, _( "The %1$s's %2$s runs over %3$s." ), name, vp_wheel.name(), tr.name() );
        } else {
            add_msg( m_bad, _( "The %1$s's %2$s runs over something." ), name, vp_wheel.name() );
        }
    }

    if( veh_data.chance >= rng( 1, 100 ) ) {
        if( veh_data.sound_volume > 0 ) {
            sounds::sound( p, veh_data.sound_volume, sounds::sound_t::combat, veh_data.sound, false,
                           veh_data.sound_type, veh_data.sound_variant );
        }
        if( veh_data.do_explosion ) {
            const Creature *source = player_in_control( player_character ) ? &player_character : nullptr;
            explosion_handler::explosion( source, p, veh_data.damage, 0.5f, false, veh_data.shrapnel );
            // Don't damage wheels with very high durability, such as roller drums or rail wheels
        } else if( damage_done ) {
            // Hit the wheel directly since it ran right over the trap.
            damage_direct( here, vp_wheel, veh_data.damage );
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

monster *vehicle::get_harnessed_animal() const
{
    for( size_t e = 0; e < parts.size(); e++ ) {
        const vehicle_part &vp = parts[ e ];
        if( vp.info().fuel_type == fuel_type_animal ) {
            monster *mon = get_monster( e );
            if( mon && mon->has_effect( effect_harnessed ) && mon->has_effect( effect_pet ) ) {
                return mon;
            }
        }
    }
    return nullptr;
}

void vehicle::selfdrive( const point &p )
{
    if( !is_towed() && !magic && !get_harnessed_animal() && !has_part( "AUTOPILOT" ) ) {
        is_following = false;
        return;
    }
    if( p.x != 0 ) {
        if( steering_effectiveness() <= 0 ) {
            return; // no steering
        }
        turn( p.x * vehicles::steer_increment );
    }
    if( p.y != 0 ) {
        if( !is_towed() ) {
            const int thr_amount = std::abs( velocity ) < 2000 ? 400 : 500;
            cruise_thrust( -p.y * thr_amount );
        } else {
            thrust( -p.y );
        }
    }
    // TODO: Actually check if we're on land on water (or disable water-skidding)
    if( skidding && valid_wheel_config() ) {
        const float handling_diff = handling_difficulty();
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
    if( global_pos3().z == 0 ||
        !get_map().has_flag_ter_or_furn( ter_furn_flag::TFLAG_NO_FLOOR, global_pos3() ) ) {
        is_flying = false;
        return true;
    }
    return false;
}

bool vehicle::check_heli_descend( Character &p ) const
{
    if( !is_rotorcraft() ) {
        debugmsg( "A vehicle is somehow flying without being an aircraft" );
        return true;
    }
    int count = 0;
    int air_count = 0;
    map &here = get_map();
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint &pt : get_points( true ) ) {
        tripoint below( pt.xy(), pt.z - 1 );
        if( pt.z < -OVERMAP_DEPTH || !here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_NO_FLOOR, pt ) ) {
            p.add_msg_if_player( _( "You are already landed!" ) );
            return false;
        }
        const optional_vpart_position ovp = here.veh_at( below );
        if( here.impassable_ter_furn( below ) || ovp || creatures.creature_at( below ) ) {
            p.add_msg_if_player( m_bad,
                                 _( "It would be unsafe to try and land when there are obstacles below you." ) );
            return false;
        }
        if( here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_NO_FLOOR, below ) ) {
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

bool vehicle::check_heli_ascend( Character &p ) const
{
    if( !is_rotorcraft() ) {
        debugmsg( "A vehicle is somehow flying without being an aircraft" );
        return true;
    }
    if( velocity > 0 && !is_flying_in_air() ) {
        p.add_msg_if_player( m_bad, _( "It would be unsafe to try and take off while you are moving." ) );
        return false;
    }
    if( sm_pos.z + 1 >= OVERMAP_HEIGHT ) {
        return false; // don't allow trying to ascend to max zlevel
    }
    map &here = get_map();
    creature_tracker &creatures = get_creature_tracker();
    for( const tripoint &pt : get_points( true ) ) {
        tripoint above( pt.xy(), pt.z + 1 );
        const optional_vpart_position ovp = here.veh_at( above );
        if( here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_INDOORS, pt ) ||
            here.impassable_ter_furn( above ) ||
            ovp ||
            creatures.creature_at( above ) ) {
            p.add_msg_if_player( m_bad,
                                 _( "It would be unsafe to try and ascend when there are obstacles above you." ) );
            return false;
        }
    }
    return true;
}

void vehicle::pldrive( Character &driver, const point &p, int z )
{
    bool is_non_proficient = false;
    float effective_driver_skill = driver.get_skill_level( skill_driving );
    float vehicle_proficiency;
    // Check if you're piloting on land or water, and reduce effective driving skill proportional to relevant proficiencies (10% Boat Proficiency = 10% driving skill on water)
    if( !driver.has_proficiency( proficiency_prof_driver ) && !in_deep_water ) {
        is_non_proficient = true;
        vehicle_proficiency = driver.get_proficiency_practice( proficiency_prof_driver );
    } else if( !driver.has_proficiency( proficiency_prof_boat_pilot ) && in_deep_water ) {
        is_non_proficient = true;
        vehicle_proficiency = driver.get_proficiency_practice( proficiency_prof_boat_pilot );
    }

    bool non_prof_fumble = false;
    float non_prof_penalty = 0;
    //If you lack the appropriate piloting proficiency, increase handling penalty, and roll chance to fumble while steering
    if( is_non_proficient ) {
        effective_driver_skill *= vehicle_proficiency;
        non_prof_penalty = std::max( 0.0f,
                                     ( 1.0f - vehicle_proficiency ) * 10.0f -
                                     ( driver.get_dex() + driver.get_per() ) * 0.25f );
        non_prof_fumble = one_in( vehicle_proficiency * 12.0f +
                                  ( driver.get_dex() + driver.get_per() ) * 0.5f + 6.0f );
        // Penalties mitigated by proficiency progress, and dex/per stats.
        // - Unskilled pilot at Per/Dex 4: 1-in-8 chance to fumble while turning
        // - Unskilled pilot at Per/Dex 8: 1-in-10
        // - Unskilled pilot at Per/Dex 12: 1-in-12
        // - 50% Skill at Per/Dex 4: 1-in-14 chance
        // - 50% Skill at Per/Dex 8: 1-in-16 chance
        // - 50% Skill at Per/Dex 12: 1-in-18 chance
    }
    if( z != 0 && is_rotorcraft() ) {
        driver.set_moves( std::min( driver.get_moves(), 0 ) );
        thrust( 0, z );
    }
    units::angle turn_delta = vehicles::steer_increment * p.x;
    const float handling_diff = handling_difficulty() + non_prof_penalty;
    if( turn_delta != 0_degrees ) {
        float eff = steering_effectiveness();
        if( eff == -2 ) {
            driver.add_msg_if_player( m_info,
                                      _( "You cannot steer an animal-drawn vehicle with no animal harnessed." ) );
            return;
        }

        if( eff < 0 ) {
            driver.add_msg_if_player( m_info,
                                      _( "This vehicle has no steering system installed, you can't turn it." ) );
            return;
        }

        if( eff == 0 ) {
            driver.add_msg_if_player( m_bad, _( "The steering is completely broken!" ) );
            return;
        }

        // If you've got more moves than speed, it's most likely time stop
        // Let's get rid of that
        driver.set_moves( std::min( driver.get_moves(), driver.get_speed() ) );

        ///\EFFECT_DEX reduces chance of losing control of vehicle when turning

        ///\EFFECT_PER reduces chance of losing control of vehicle when turning

        ///\EFFECT_DRIVING reduces chance of losing control of vehicle when turning
        float skill = std::min( 10.0f, effective_driver_skill +
                                ( driver.get_dex() + driver.get_per() ) / 10.0f );
        float penalty = rng_float( 0.0f, handling_diff ) - skill;

        int cost;
        if( penalty > 0.0f ) {
            // At 10 penalty (rather hard to get), we're taking 4 turns per turn
            cost = 100 * ( 1.0f + penalty / 2.5f );
        } else {
            // At 10 skill, with a perfect vehicle, we could turn up to 3 times per turn
            cost = std::max( driver.get_speed(), 100 ) * ( 1.0f - ( -penalty / 10.0f ) * 2 / 3 );
        }

        // Chance to fumble steering when in a non-proficient vehicle, or in difficult conditions
        if( non_prof_fumble || penalty > skill || ( penalty > 0 && cost > 400 ) ) {
            int fumble_roll = rng( 0, 6 );
            int fumble_factor = 0;
            int fumble_time = 1;
            if( fumble_roll <= 1 ) {
                // On a 0 or 1, fumble briefly instead of steering for 1 turn
                driver.add_msg_if_player( m_warning, _( "You fumble with the %s's controls." ), name );
            } else if( fumble_roll <= 4 ) {
                // On a 2, 3, or 4, steer as intended but take 2 turns to do it
                driver.add_msg_if_player( m_warning, _( "You turn slower than you meant to." ) );
                fumble_factor = 1;
                fumble_time = 2;
            } else {
                // On a 5 or 6, steer twice as far as you meant to, and take 2 turns to do it.
                driver.add_msg_if_player( m_warning, _( "You oversteer the %s!" ), name );
                fumble_factor = 2;
                fumble_time = 2;
            }
            turn_delta *= fumble_factor;
            cost = std::max( cost, driver.get_moves() + fumble_time * 100 );
        } else if( one_in( 10 ) ) {
            // Don't warn all the time or it gets spammy
            if( cost >= driver.get_speed() * 2 ) {
                driver.add_msg_if_player( m_warning, _( "It takes you a very long time to steer the vehicle!" ) );
            } else if( cost >= driver.get_speed() * 1.5f ) {
                driver.add_msg_if_player( m_warning, _( "It takes you a long time to steer the vehicle!" ) );
            }
        }

        turn( turn_delta );

        // At most 3 turns per turn, because otherwise it looks really weird and jumpy
        driver.mod_moves( -std::max( cost, driver.get_speed() / 3 + 1 ) );
    }

    if( p.y != 0 ) {
        cruise_thrust( -p.y * 400 );
    }

    // TODO: Actually check if we're on land on water (or disable water-skidding)
    if( skidding && p.x != 0 && valid_wheel_config() ) {
        ///\EFFECT_DEX increases chance of regaining control of a vehicle

        ///\EFFECT_DRIVING increases chance of regaining control of a vehicle
        if( handling_diff * rng( 1, 10 ) <
            driver.dex_cur + effective_driver_skill * 2 ) {
            driver.add_msg_if_player( _( "You regain control of the %s." ), name );
            driver.practice( skill_driving, velocity / 5 );
            velocity = static_cast<int>( forward_velocity() );
            skidding = false;
            move.init( turn_dir );
        }
    }
}

// A chance to stop skidding if moving in roughly the faced direction
void vehicle::possibly_recover_from_skid()
{
    if( last_turn > 13_degrees ) {
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

static rl_vec2d angle_to_vec( const units::angle &angle )
{
    return rl_vec2d( units::cos( angle ), units::sin( angle ) );
}

// normalized.
rl_vec2d vehicle::move_vec() const
{
    return angle_to_vec( move.dir() );
}

// normalized.
rl_vec2d vehicle::face_vec() const
{
    return angle_to_vec( face.dir() );
}

rl_vec2d vehicle::dir_vec() const
{
    return angle_to_vec( turn_dir );
}

float get_collision_factor( const float delta_v )
{
    if( std::abs( delta_v ) <= 31 ) {
        return 1 - ( 0.9 * std::abs( delta_v ) ) / 31;
    } else {
        return 0.1;
    }
}

void vehicle::precalculate_vehicle_turning( units::angle new_turn_dir, bool check_rail_direction,
        const ter_furn_flag ter_flag_to_check, int &wheels_on_rail,
        int &turning_wheels_that_are_one_axis ) const
{
    // The direction we're moving
    tileray mdir;
    // calculate direction after turn
    mdir.init( new_turn_dir );
    tripoint dp;
    bool is_diagonal_movement = std::lround( to_degrees( new_turn_dir ) ) % 90 == 45;

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
                const ter_str_id &terrain_at_wheel = here.ter( wheel_tripoint ).id();
                // check is it correct tile to turn into
                if( !is_diagonal_movement ) {
                    const std::unordered_set<ter_str_id> diagonal_track_ters = { ter_t_railroad_track_d, ter_t_railroad_track_d1, ter_t_railroad_track_d2, ter_t_railroad_track_d_on_tie };
                    if( diagonal_track_ters.find( terrain_at_wheel ) != diagonal_track_ters.end() ) {
                        incorrect_tiles_not_diagonal++;
                    }
                } else if( const std::unordered_set<ter_str_id> straight_track_ters = { ter_t_railroad_track, ter_t_railroad_track_on_tie, ter_t_railroad_track_h, ter_t_railroad_track_v, ter_t_railroad_track_h_on_tie, ter_t_railroad_track_v_on_tie };
                           straight_track_ters.find( terrain_at_wheel ) != straight_track_ters.end() ) {
                    incorrect_tiles_not_diagonal++;
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
static units::angle get_corrected_turn_dir( const units::angle &turn_dir,
        const units::angle &face_dir )
{
    units::angle corrected_turn_dir = 0_degrees;

    // Driver turned vehicle, round angle to 45 deg
    if( turn_dir > face_dir && turn_dir < face_dir + 180_degrees ) {
        corrected_turn_dir = face_dir + 45_degrees;
    } else if( turn_dir < face_dir || turn_dir > 270_degrees ) {
        corrected_turn_dir = face_dir - 45_degrees;
    }
    return normalize( corrected_turn_dir );
}

bool vehicle::allow_manual_turn_on_rails( units::angle &corrected_turn_dir ) const
{
    bool allow_turn_on_rail = false;
    // driver tried to turn rails vehicle
    if( turn_dir != face.dir() ) {
        corrected_turn_dir = get_corrected_turn_dir( turn_dir, face.dir() );

        int wheels_on_rail;
        int turning_wheels_that_are_one_axis;
        precalculate_vehicle_turning( corrected_turn_dir, true, ter_furn_flag::TFLAG_RAIL, wheels_on_rail,
                                      turning_wheels_that_are_one_axis );
        if( is_wheel_state_correct_to_turn_on_rails( wheels_on_rail, rail_wheelcache.size(),
                turning_wheels_that_are_one_axis ) ) {
            allow_turn_on_rail = true;
        }
    }
    return allow_turn_on_rail;
}

bool vehicle::allow_auto_turn_on_rails( units::angle &corrected_turn_dir ) const
{
    bool allow_turn_on_rail = false;
    // check if autoturn is possible
    if( turn_dir == face.dir() ) {
        // precalculate wheels for every direction
        int straight_wheels_on_rail;
        int straight_turning_wheels_that_are_one_axis;
        precalculate_vehicle_turning( face.dir(), true, ter_furn_flag::TFLAG_RAIL, straight_wheels_on_rail,
                                      straight_turning_wheels_that_are_one_axis );

        units::angle left_turn_dir =
            get_corrected_turn_dir( face.dir() - 45_degrees, face.dir() );
        int leftturn_wheels_on_rail;
        int leftturn_turning_wheels_that_are_one_axis;
        precalculate_vehicle_turning( left_turn_dir, true, ter_furn_flag::TFLAG_RAIL,
                                      leftturn_wheels_on_rail,
                                      leftturn_turning_wheels_that_are_one_axis );

        units::angle right_turn_dir =
            get_corrected_turn_dir( face.dir() + 45_degrees, face.dir() );
        int rightturn_wheels_on_rail;
        int rightturn_turning_wheels_that_are_one_axis;
        precalculate_vehicle_turning( right_turn_dir, true, ter_furn_flag::TFLAG_RAIL,
                                      rightturn_wheels_on_rail,
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
        stop();
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
    if( in_deep_water && !can_float() ) {
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
                Character *passenger = get_passenger( boarded );
                if( passenger != nullptr ) {
                    passenger->practice( skill_driving, 1 );
                }
            }
        }

        // Eventually send it skidding if no control
        // But not if it's remotely controlled, is in water or can use rails
        if( !controlled && !pl_ctrl && !( is_watercraft() && can_float() ) && !can_use_rails &&
            !is_flying && requested_z_change == 0 ) {
            skidding = true;
        }
    }

    if( skidding && one_in( 4 ) ) {
        // Might turn uncontrollably while skidding
        turn( vehicles::steer_increment * ( one_in( 2 ) ? -1 : 1 ) );
    }

    if( should_fall ) {
        // TODO: Insert a (hard) driving test to stop this from happening
        skidding = true;
    }

    bool allow_turn_on_rail = false;
    if( can_use_rails && !falling_only ) {
        units::angle corrected_turn_dir;
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
    if( is_flying && is_rotorcraft() ) {
        return true;
    }
    is_on_ramp = false;
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
            no_support[part_pos.z] = here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_NO_FLOOR, part_pos ) &&
                                     !here.supports_above( part_pos + tripoint_below );
        }
        if( !is_on_ramp &&
            ( here.has_flag( ter_furn_flag::TFLAG_RAMP_UP, tripoint( part_pos.xy(), part_pos.z - 1 ) ) ||
              here.has_flag( ter_furn_flag::TFLAG_RAMP_DOWN, tripoint( part_pos.xy(), part_pos.z + 1 ) ) ) ) {
            is_on_ramp = true;
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
    // If we're flying none of the rest of this matters.
    if( is_flying && is_rotorcraft() ) {
        is_falling = false;
        in_deep_water = false;
        in_water = false;
        return;
    }

    is_falling = true;
    is_flying = false;
    map &here = get_map();

    auto has_support = [&here]( const tripoint & position, const bool water_supports ) {
        // if we're at the bottom of the z-levels, we're supported
        if( position.z == -OVERMAP_DEPTH ) {
            return true;
        }
        // water counts as support if we're swimming and checking to see if we're falling, but
        // not to see if the wheels are supported at all
        if( here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_SWIMMABLE, position ) ) {
            return water_supports;
        }
        if( !here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_NO_FLOOR, position ) ) {
            return true;
        }
        tripoint below( position.xy(), position.z - 1 );
        return here.supports_above( below );
    };
    // Check under the wheels, if they're supported nothing else matters.
    int supported_wheels = 0;
    for( int wheel_index : wheelcache ) {
        const tripoint position = global_part_pos3( wheel_index );
        if( has_support( position, false ) ) {
            ++supported_wheels;
        }
    }
    // If half of the wheels are supported, we're not falling and we're not in water.
    if( supported_wheels > 0 &&
        static_cast<size_t>( supported_wheels ) * 2 >= wheelcache.size() ) {
        is_falling = false;
        in_water = false;
        in_deep_water = false;
        return;
    }
    // TODO: Make the vehicle "slide" towards its center of weight
    //  when it's not properly supported
    const std::set<tripoint> &pts = get_points();
    if( pts.empty() ) {
        // Dirty vehicle with no parts
        is_falling = false;
        in_deep_water = false;
        in_water = false;
        is_flying = false;
        return;
    }

    size_t deep_water_tiles = 0;
    size_t water_tiles = 0;
    for( const tripoint &position : pts ) {
        deep_water_tiles += here.has_flag( ter_furn_flag::TFLAG_DEEP_WATER, position ) ? 1 : 0;
        water_tiles += here.has_flag( ter_furn_flag::TFLAG_SWIMMABLE, position ) ? 1 : 0;
        if( !is_falling ) {
            continue;
        }
        is_falling = !has_support( position, true );
    }
    // in_deep_water if 2/3 of the vehicle is in deep water
    in_deep_water = 3 * deep_water_tiles >= 2 * pts.size();
    // in_water if 1/2 of the vehicle is in water at all
    in_water =  2 * water_tiles >= pts.size();
}

float map::vehicle_wheel_traction( const vehicle &veh, bool ignore_movement_modifiers ) const
{
    if( veh.is_in_water( /* deep_water = */ true ) ) {
        return veh.can_float() ? 1.0f : -1.0f;
    }
    if( veh.is_watercraft() && veh.can_float() ) {
        return 1.0f;
    }

    if( veh.wheelcache.empty() ) {
        // TODO: Assume it is digging in dirt
        // TODO: Return something that could be reused for dragging
        return 0.0f;
    }

    float traction_wheel_area = 0.0f;
    for( const int wheel_idx : veh.wheelcache ) {
        const vehicle_part &vp = veh.part( wheel_idx );
        const vpart_info &vpi = vp.info();
        const tripoint pp = veh.global_part_pos3( vp );
        const ter_t &tr = ter( pp ).obj();
        if( tr.has_flag( ter_furn_flag::TFLAG_DEEP_WATER ) ||
            tr.has_flag( ter_furn_flag::TFLAG_NO_FLOOR ) ) {
            continue; // No traction from wheel in deep water or air
        }

        int move_mod = move_cost_ter_furn( pp );
        if( move_mod == 0 ) {
            return 0.0f; // Vehicle locked in wall, shouldn't happen, but does
        }

        if( ignore_movement_modifiers ) {
            traction_wheel_area += vpi.wheel_info->contact_area;
            continue; // Ignore the movement modifier if caller specifies a bool
        }

        for( const veh_ter_mod &mod : vpi.wheel_info->terrain_modifiers ) {
            const bool ter_has_flag = tr.has_flag( mod.terrain_flag );
            if( ter_has_flag && mod.move_override ) {
                move_mod = mod.move_override;
                break;
            } else if( !ter_has_flag && mod.move_penalty ) {
                move_mod += mod.move_penalty;
                break;
            }
        }

        if( move_mod == 0 ) {
            debugmsg( "move_mod resulted in a 0, ignoring wheel" );
            continue;
        }

        traction_wheel_area += 2.0 * vpi.wheel_info->contact_area / move_mod;
    }

    return traction_wheel_area;
}

units::angle map::shake_vehicle( vehicle &veh, const int velocity_before,
                                 const units::angle &direction )
{
    const int d_vel = std::abs( veh.velocity - velocity_before ) / 100;

    std::vector<rider_data> riders = veh.get_riders();

    units::angle coll_turn = 0_degrees;
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
            veh.part( ps ).remove_flag( vp_flag::passenger_flag );
            continue;
        }

        Character *psg = dynamic_cast<Character *>( rider );
        monster *pet = dynamic_cast<monster *>( rider );

        bool throw_from_seat = false;
        int dmg = d_vel * rng_float( 1.0f, 2.0f );
        int move_resist = 1;
        if( psg ) {
            ///\EFFECT_STR reduces chance of being thrown from your seat when not wearing a seatbelt
            move_resist = psg->str_cur * 150 + 500;
            if( veh.part( ps ).info().has_flag( "SEAT_REQUIRES_BALANCE" ) ) {
                // Much harder to resist being thrown on a skateboard-like vehicle.
                // Penalty mitigated by Deft and Skater.
                int resist_penalty = 500;
                if( psg->has_trait( trait_PROF_SKATER ) ) {
                    resist_penalty -= 150;
                }
                if( psg->has_trait( trait_DEFT ) ) {
                    resist_penalty -= 150;
                }
                move_resist -= resist_penalty;
            }
        } else {
            int pet_resist = 0;
            if( pet != nullptr ) {
                pet_resist = static_cast<int>( to_kilogram( pet->get_weight() ) * 200 );
            }
            move_resist = std::max( 100, pet_resist );
        }
        const int belt_idx = veh.part_with_feature( ps, VPFLAG_SEATBELT, true );
        if( belt_idx == -1 ) {
            ///\EFFECT_STR reduces chance of being thrown from your seat when not wearing a seatbelt
            throw_from_seat = d_vel * rng( 80, 120 ) > move_resist;
        } else {
            // Reduce potential damage based on quality of seatbelt
            dmg -= veh.part( belt_idx ).info().bonus;
        }

        // Damage passengers if d_vel is too high
        if( !throw_from_seat && ( 10 * d_vel ) > 6 * rng( 25, 50 ) ) {
            if( psg ) {
                psg->deal_damage( nullptr, bodypart_id( "torso" ), damage_instance( damage_bash, dmg ) );
                psg->add_msg_player_or_npc( m_bad,
                                            _( "You take %d damage by the power of the impact!" ),
                                            _( "<npcname> takes %d damage by the power of the "
                                               "impact!" ), dmg );
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
                units::angle turn_angle = std::min( turn_amount * vehicles::steer_increment, 120_degrees );
                coll_turn = one_in( 2 ) ? turn_angle : -turn_angle;
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
            } else {
                add_msg_if_player_sees( part_pos, m_bad,
                                        _( "The %s is hurled from %s's by the power of the impact!" ),
                                        pet->disp_name(), veh.name );
            }
            ///\EFFECT_STR reduces distance thrown from seat in a vehicle impact
            g->fling_creature( rider, direction + rng_float( -30_degrees, 30_degrees ),
                               std::max( 10, d_vel - move_resist / 100 ) );
        }
    }

    return coll_turn;
}

bool vehicle::should_enable_fake( const tripoint &fake_precalc, const tripoint &parent_precalc,
                                  const tripoint &neighbor_precalc ) const
{
    // if parent's pos is diagonal to neighbor, but fake isn't, fake can fill a gap opened
    tripoint abs_parent_neighbor_diff = ( parent_precalc - neighbor_precalc ).abs();
    tripoint abs_fake_neighbor_diff = ( fake_precalc - neighbor_precalc ).abs();
    return ( abs_parent_neighbor_diff.x == 1 && abs_parent_neighbor_diff.y == 1 ) &&
           ( ( abs_fake_neighbor_diff.x == 1 && abs_fake_neighbor_diff.y == 0 ) ||
             ( abs_fake_neighbor_diff.x == 0 && abs_fake_neighbor_diff.y == 1 ) );
}

void vehicle::update_active_fakes()
{
    for( const int fake_index : fake_parts ) {
        vehicle_part &part_fake = parts.at( fake_index );
        if( part_fake.removed ) {
            continue;
        }
        const vehicle_part &part_real = parts.at( part_fake.fake_part_to );
        const tripoint &fake_precalc = part_fake.precalc[0];
        const tripoint &real_precalc = part_real.precalc[0];
        const vpart_edge_info &real_edge = edges[part_real.mount];
        const bool is_protrusion = part_real.info().has_flag( "PROTRUSION" );

        if( real_edge.forward != -1 ) {
            const tripoint &forward = parts.at( real_edge.forward ).precalc[0];
            part_fake.is_active_fake = should_enable_fake( fake_precalc, real_precalc, forward );
        }
        if( real_edge.back != -1 && ( !part_fake.is_active_fake || real_edge.forward == -1 ) ) {
            const tripoint &back = parts.at( real_edge.back ).precalc[0];
            part_fake.is_active_fake = should_enable_fake( fake_precalc, real_precalc, back );
        }
        if( is_protrusion && part_fake.fake_protrusion_on >= 0 ) {
            part_fake.is_active_fake = parts.at( part_fake.fake_protrusion_on ).is_active_fake;
        }
    }
}
