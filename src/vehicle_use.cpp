#include "vehicle.h" // IWYU pragma: associated

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>

#include "action.h"
#include "activity_actor_definitions.h"
#include "activity_handlers.h"
#include "activity_type.h"
#include "avatar.h"
#include "character.h"
#include "clzones.h"
#include "color.h"
#include "creature.h"
#include "creature_tracker.h"
#include "debug.h"
#include "enums.h"
#include "game.h"
#include "iexamine.h"
#include "input.h"
#include "inventory.h"
#include "item.h"
#include "item_pocket.h"
#include "itype.h"
#include "iuse.h"
#include "json.h"
#include "make_static.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pickup.h"
#include "player_activity.h"
#include "requirements.h"
#include "ret_val.h"
#include "rng.h"
#include "smart_controller_ui.h"
#include "sounds.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_interact.h"
#include "veh_type.h"
#include "veh_utils.h"
#include "vehicle_selector.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather.h"

static const activity_id ACT_REPAIR_ITEM( "ACT_REPAIR_ITEM" );
static const activity_id ACT_START_ENGINES( "ACT_START_ENGINES" );

static const ammotype ammo_battery( "battery" );

static const efftype_id effect_harnessed( "harnessed" );
static const efftype_id effect_tied( "tied" );

static const fault_id fault_engine_starter( "fault_engine_starter" );

static const flag_id json_flag_FILTHY( "FILTHY" );

static const itype_id fuel_type_battery( "battery" );
static const itype_id fuel_type_muscle( "muscle" );
static const itype_id fuel_type_none( "null" );
static const itype_id fuel_type_wind( "wind" );
static const itype_id itype_battery( "battery" );
static const itype_id itype_detergent( "detergent" );
static const itype_id itype_fungal_seeds( "fungal_seeds" );
static const itype_id itype_marloss_seed( "marloss_seed" );
static const itype_id itype_null( "null" );
static const itype_id itype_pseudo_water_purifier( "pseudo_water_purifier" );
static const itype_id itype_soldering_iron( "soldering_iron" );
static const itype_id itype_water( "water" );
static const itype_id itype_water_clean( "water_clean" );
static const itype_id itype_water_faucet( "water_faucet" );
static const itype_id itype_welder( "welder" );

static const quality_id qual_SCREW( "SCREW" );

static const skill_id skill_mechanics( "mechanics" );

static const vpart_id vpart_horn_bicycle( "horn_bicycle" );

static const zone_type_id zone_type_VEHICLE_PATROL( "VEHICLE_PATROL" );

void handbrake()
{
    Character &player_character = get_player_character();
    const optional_vpart_position vp = get_map().veh_at( player_character.pos() );
    if( !vp ) {
        return;
    }
    vehicle *const veh = &vp->vehicle();
    add_msg( _( "You pull a handbrake." ) );
    veh->cruise_velocity = 0;
    if( veh->last_turn != 0_degrees && rng( 15, 60 ) * 100 < std::abs( veh->velocity ) ) {
        veh->skidding = true;
        add_msg( m_warning, _( "You lose control of %s." ), veh->name );
        veh->turn( veh->last_turn > 0_degrees ? 60_degrees : -60_degrees );
    } else {
        int braking_power = std::abs( veh->velocity ) / 2 + 10 * 100;
        if( std::abs( veh->velocity ) < braking_power ) {
            veh->stop();
        } else {
            int sgn = veh->velocity > 0 ? 1 : -1;
            veh->velocity = sgn * ( std::abs( veh->velocity ) - braking_power );
        }
    }
    player_character.moves = 0;
}

void vehicle::control_doors()
{
    const auto open_or_close_all = [this]( bool new_open, const std::string & require_flag ) {
        for( const vpart_reference &vpr_motor : get_avail_parts( "DOOR_MOTOR" ) ) {
            const int motorized_idx = new_open
                                      ? next_part_to_open( vpr_motor.part_index() )
                                      : next_part_to_close( vpr_motor.part_index() );
            if( motorized_idx == -1 ) {
                continue;
            }
            if( !require_flag.empty() && !part_flag( motorized_idx, require_flag ) ) {
                continue;
            }
            if( new_open || can_close( motorized_idx, get_player_character() ) ) {
                open_or_close( motorized_idx, new_open );
            }
        }
    };

    const auto add_openable = [this]( veh_menu & menu, int vp_idx ) {
        if( vp_idx == -1 ) {
            return;
        }
        const vehicle_part &vp = part( vp_idx );
        const std::string actname = vp.open ? _( "Close" ) : _( "Open" );
        const bool open = !vp.open;
        menu.add( string_format( "%s %s", actname, vp.name() ) )
        .hotkey_auto()
        .location( global_part_pos3( vp ) )
        .keep_menu_open()
        .on_submit( [this, vp_idx, open] {
            if( can_close( vp_idx, get_player_character() ) )
            {
                open_or_close( vp_idx, open );
            }
        } );
    };

    veh_menu menu( this, _( "Select door to toggle" ) );

    do {
        menu.reset();

        menu.add( _( "Open all curtains" ) )
        .hotkey_auto()
        .location( get_player_character().pos() )
        .on_submit( [&open_or_close_all] { open_or_close_all( true, "CURTAIN" ); } );

        menu.add( _( "Open all curtains and doors" ) )
        .hotkey_auto()
        .location( get_player_character().pos() )
        .on_submit( [&open_or_close_all] { open_or_close_all( true, "" ); } );

        menu.add( _( "Close all doors" ) )
        .hotkey_auto()
        .location( get_player_character().pos() )
        .on_submit( [&open_or_close_all] { open_or_close_all( false, "DOOR" ); } );

        menu.add( _( "Close all curtains and doors" ) )
        .hotkey_auto()
        .location( get_player_character().pos() )
        .on_submit( [&open_or_close_all] { open_or_close_all( false, "" ); } );

        for( const vpart_reference &vp_motor : get_avail_parts( "DOOR_MOTOR" ) ) {
            add_openable( menu, next_part_to_open( vp_motor.part_index() ) );
            add_openable( menu, next_part_to_close( vp_motor.part_index() ) );
        }

        if( menu.get_items_size() == 4 ) {
            debugmsg( "vehicle::control_doors called but no door motors found" );
            return;
        }
    } while( menu.query() );
}

static void add_electronic_toggle( vehicle &veh, veh_menu &menu, const std::string &name,
                                   const std::string &action, const std::string &flag )
{
    // fetch matching parts and abort early if none found
    const auto found = veh.get_avail_parts( flag );
    if( empty( found ) ) {
        return;
    }

    // determine target state - currently parts of similar type are all switched concurrently
    bool state = std::none_of( found.begin(), found.end(), []( const vpart_reference & vp ) {
        return vp.part().enabled;
    } );

    // can this menu option be selected by the user?
    // if toggled part potentially usable check if could be enabled now (sufficient fuel etc.)
    bool allow = !state || std::any_of( found.begin(), found.end(), []( const vpart_reference & vp ) {
        return vp.vehicle().can_enable( vp.part() );
    } );

    const std::string msg = state ? _( "Turn on %s" ) : colorize( _( "Turn off %s" ), c_pink );

    menu.add( string_format( msg, name ) )
    .enable( allow )
    .hotkey( action )
    .keep_menu_open()
    .on_submit( [found, state] {
        for( const vpart_reference &vp : found )
        {
            vehicle_part &e = vp.part();
            if( e.enabled != state ) {
                add_msg( state ? _( "Turned on %s." ) : _( "Turned off %s." ), e.name() );
                e.enabled = state;
            }
        }
    } );
}

void vehicle::build_electronics_menu( veh_menu &menu )
{
    if( has_part( "DOOR_MOTOR" ) ) {
        menu.add( _( "Control doors and curtains" ) )
        .hotkey( "TOGGLE_DOORS" )
        .on_submit( [this] { control_doors(); } );
    }

    if( camera_on || ( has_part( "CAMERA" ) && has_part( "CAMERA_CONTROL" ) ) ) {
        menu.add( camera_on
                  ? colorize( _( "Turn off camera system" ), c_pink )
                  : _( "Turn on camera system" ) )
        .enable( fuel_left( fuel_type_battery, true ) )
        .hotkey( "TOGGLE_CAMERA" )
        .keep_menu_open()
        .on_submit( [&] {
            if( camera_on )
            {
                add_msg( _( "Camera system disabled" ) );
            } else
            {
                add_msg( _( "Camera system enabled" ) );
            }
            camera_on = !camera_on;
        } );
    }

    auto add_toggle = [this, &menu]( const std::string & name, const std::string & action,
    const std::string & flag ) {
        add_electronic_toggle( *this, menu, name, action, flag );
    };
    add_toggle( pgettext( "electronics menu option", "reactor" ),
                "TOGGLE_REACTOR", "REACTOR" );
    add_toggle( pgettext( "electronics menu option", "headlights" ),
                "TOGGLE_HEADLIGHT", "CONE_LIGHT" );
    add_toggle( pgettext( "electronics menu option", "wide angle headlights" ),
                "TOGGLE_WIDE_HEADLIGHT", "WIDE_CONE_LIGHT" );
    add_toggle( pgettext( "electronics menu option", "directed overhead lights" ),
                "TOGGLE_HALF_OVERHEAD_LIGHT", "HALF_CIRCLE_LIGHT" );
    add_toggle( pgettext( "electronics menu option", "overhead lights" ),
                "TOGGLE_OVERHEAD_LIGHT", "CIRCLE_LIGHT" );
    add_toggle( pgettext( "electronics menu option", "aisle lights" ),
                "TOGGLE_AISLE_LIGHT", "AISLE_LIGHT" );
    add_toggle( pgettext( "electronics menu option", "dome lights" ),
                "TOGGLE_DOME_LIGHT", "DOME_LIGHT" );
    add_toggle( pgettext( "electronics menu option", "atomic lights" ),
                "TOGGLE_ATOMIC_LIGHT", "ATOMIC_LIGHT" );
    add_toggle( pgettext( "electronics menu option", "stereo" ),
                "TOGGLE_STEREO", "STEREO" );
    add_toggle( pgettext( "electronics menu option", "chimes" ),
                "TOGGLE_CHIMES", "CHIMES" );
    add_toggle( pgettext( "electronics menu option", "fridge" ),
                "TOGGLE_FRIDGE", "FRIDGE" );
    add_toggle( pgettext( "electronics menu option", "freezer" ),
                "TOGGLE_FREEZER", "FREEZER" );
    add_toggle( pgettext( "electronics menu option", "arcade" ),
                "TOGGLE_ARCADE", "ARCADE" );
    add_toggle( pgettext( "electronics menu option", "space heater" ),
                "TOGGLE_SPACE_HEATER", "SPACE_HEATER" );
    add_toggle( pgettext( "electronics menu option", "heated tank" ),
                "TOGGLE_HEATED_TANK", "HEATED_TANK" );
    add_toggle( pgettext( "electronics menu option", "cooler" ),
                "TOGGLE_COOLER", "COOLER" );
    add_toggle( pgettext( "electronics menu option", "recharger" ),
                "TOGGLE_RECHARGER", "RECHARGE" );
    add_toggle( pgettext( "electronics menu option", "plow" ),
                "TOGGLE_PLOW", "PLOW" );
    add_toggle( pgettext( "electronics menu option", "reaper" ),
                "TOGGLE_REAPER", "REAPER" );
    add_toggle( pgettext( "electronics menu option", "planter" ),
                "TOGGLE_PLANTER", "PLANTER" );
    add_toggle( pgettext( "electronics menu option", "rockwheel" ),
                "TOGGLE_PLOW", "ROCKWHEEL" );
    add_toggle( pgettext( "electronics menu option", "roadheader" ),
                "TOGGLE_PLOW", "ROADHEAD" );
    add_toggle( pgettext( "electronics menu option", "scoop" ),
                "TOGGLE_SCOOP", "SCOOP" );
    add_toggle( pgettext( "electronics menu option", "water purifier" ),
                "TOGGLE_WATER_PURIFIER", "WATER_PURIFIER" );
    add_toggle( pgettext( "electronics menu option", "smart controller" ),
                "TOGGLE_SMART_ENGINE_CONTROLLER", "SMART_ENGINE_CONTROLLER" );

    for( const vpart_reference &arc_vp : get_any_parts( "ARCADE" ) ) {
        if( arc_vp.part().enabled ) {
            item *arc_itm = &arc_vp.part().base;

            menu.add( _( "Play arcade machine" ) )
            .hotkey( "ARCADE" )
            .enable( !!arc_itm )
            .on_submit( [arc_itm] { iuse::portable_game( &get_avatar(), arc_itm, false, tripoint_zero ); } );
            break;
        }
    }
}

void vehicle::control_engines()
{
    bool dirty = false;

    veh_menu menu( this, _( "Toggle which?" ) );
    do {
        menu.reset();
        for( size_t e = 0; e < engines.size(); e++ ) {
            const int engine_idx = engines[e];
            const vehicle_part &vp = parts[engine_idx];
            for( const itype_id &fuel_type : vp.info().engine_fuel_opts() ) {
                bool is_active = vp.enabled && vp.fuel_current() == fuel_type;
                bool has_fuel = is_perpetual_type( e ) || fuel_left( fuel_type );
                menu.add( string_format( "[%s] %s %s", is_active ? "x" : " ", vp.name(), fuel_type->nname( 1 ) ) )
                .enable( vp.is_available() && has_fuel )
                .keep_menu_open()
                .on_submit( [this, engine_idx, fuel_type, &dirty] {
                    vehicle_part &vp = parts[engine_idx];
                    if( vp.fuel_current() == fuel_type )
                    {
                        toggle_specific_part( engine_idx, !is_part_on( engine_idx ) );
                    } else
                    {
                        vp.fuel_set( fuel_type );
                    }
                    dirty = true;
                } );
            }
        }
    } while( menu.query() );

    if( !dirty ) {
        return;
    }

    bool engines_were_on = engine_on;
    for( int e : engines ) {
        engine_on |= is_part_on( e );
    }

    // if current velocity greater than new configuration safe speed
    // drop down cruise velocity.
    int safe_vel = safe_velocity();
    if( velocity > safe_vel ) {
        cruise_velocity = safe_vel;
    }

    if( engines_were_on && !engine_on ) {
        add_msg( _( "You turn off the %s's engines to change their configurations." ), name );
    } else if( !get_player_character().controlling_vehicle ) {
        add_msg( _( "You change the %s's engine configuration." ), name );
    }

    if( engine_on ) {
        start_engines();
    }
}

void vehicle::smash_security_system()
{

    //get security and controls location
    int s = -1;
    int c = -1;
    for( int p : speciality ) {
        if( part_flag( p, "SECURITY" ) && !parts[ p ].is_broken() ) {
            s = p;
            c = part_with_feature( s, "CONTROLS", true );
            break;
        }
    }
    Character &player_character = get_player_character();
    map &here = get_map();
    //controls and security must both be valid
    if( c >= 0 && s >= 0 ) {
        ///\EFFECT_MECHANICS reduces chance of damaging controls when smashing security system
        int skill = player_character.get_skill_level( skill_mechanics );
        int percent_controls = 70 / ( 1 + skill );
        int percent_alarm = ( skill + 3 ) * 10;
        int rand = rng( 1, 100 );

        if( percent_controls > rand ) {
            damage_direct( here, c, part_info( c ).durability / 4 );

            if( parts[ c ].removed || parts[ c ].is_broken() ) {
                player_character.controlling_vehicle = false;
                is_alarm_on = false;
                add_msg( _( "You destroy the controls…" ) );
            } else {
                add_msg( _( "You damage the controls." ) );
            }
        }
        if( percent_alarm > rand ) {
            damage_direct( here, s, part_info( s ).durability / 5 );
            // chance to disable alarm immediately, or disable on destruction
            if( percent_alarm / 4 > rand || parts[ s ].is_broken() ) {
                is_alarm_on = false;
            }
        }
        add_msg( ( is_alarm_on ) ? _( "The alarm keeps going." ) : _( "The alarm stops." ) );
    } else {
        debugmsg( "No security system found on vehicle." );
    }
}

void vehicle::autopilot_patrol_check()
{
    zone_manager &mgr = zone_manager::get_manager();
    if( mgr.has_near( zone_type_VEHICLE_PATROL, global_square_location(), 60 ) ) {
        enable_patrol();
    } else {
        g->zones_manager();
    }
}

void vehicle::toggle_autopilot()
{
    veh_menu menu( this, _( "Choose action for the autopilot" ) );

    menu.add( _( "Patrol…" ) )
    .hotkey( "CONTROL_AUTOPILOT_PATROL" )
    .desc( _( "Program the autopilot to patrol a nearby vehicle patrol zone.  If no zones are nearby, you will be prompted to create one." ) )
    .on_submit( [this] {
        autopilot_patrol_check();
    } );

    menu.add( _( "Follow…" ) )
    .hotkey( "CONTROL_AUTOPILOT_FOLLOW" )
    .desc( _( "Program the autopilot to follow you.  It might be a good idea to have a remote control available to tell it to stop, too." ) )
    .on_submit( [this] {
        autopilot_on = true;
        is_following = true;
        is_patrolling = false;
        start_engines();
    } );

    menu.add( _( "Stop…" ) )
    .hotkey( "CONTROL_AUTOPILOT_STOP" )
    .desc( _( "Stop all autopilot related activities." ) )
    .on_submit( [this] {
        autopilot_on = false;
        is_patrolling = false;
        is_following = false;
        autodrive_local_target = tripoint_zero;
        add_msg( _( "You turn the engine off." ) );
        stop_engines();
    } );

    menu.query();
}

void vehicle::toggle_tracking()
{
    if( tracking_on ) {
        overmap_buffer.remove_vehicle( this );
        tracking_on = false;
        add_msg( _( "You stop keeping track of the vehicle position." ) );
    } else {
        overmap_buffer.add_vehicle( this );
        tracking_on = true;
        add_msg( _( "You start keeping track of this vehicle's position." ) );
    }
}

item vehicle::init_cord( const tripoint &pos )
{
    item powercord( "power_cord" );
    powercord.set_var( "source_x", pos.x );
    powercord.set_var( "source_y", pos.y );
    powercord.set_var( "source_z", pos.z );
    powercord.set_var( "state", "pay_out_cable" );
    powercord.active = true;

    return powercord;
}

void vehicle::plug_in( const tripoint &pos )
{
    item powercord = init_cord( pos );

    if( powercord.get_use( "CABLE_ATTACH" ) ) {
        powercord.get_use( "CABLE_ATTACH" )->call( get_player_character(), powercord, powercord.active,
                pos );
    }

}

void vehicle::connect( const tripoint &source_pos, const tripoint &target_pos )
{

    item cord = init_cord( source_pos );
    map &here = get_map();

    const optional_vpart_position target_vp = here.veh_at( target_pos );
    const optional_vpart_position source_vp = here.veh_at( source_pos );

    if( !target_vp ) {
        return;
    }
    vehicle *const target_veh = &target_vp->vehicle();
    vehicle *const source_veh = &source_vp->vehicle();
    if( source_veh == target_veh ) {
        return ;
    }

    tripoint target_global = here.getabs( target_pos );
    // TODO: make sure there is always a matching vpart id here. Maybe transform this into
    // a iuse_actor class, or add a check in item_factory.
    const vpart_id vpid( cord.typeId().str() );

    point vcoords = source_vp->mount();
    vehicle_part source_part( vpid, "", vcoords, item( cord ) );
    source_part.target.first = target_global;
    source_part.target.second = target_veh->global_square_location().raw();
    source_veh->install_part( vcoords, source_part );

    vcoords = target_vp->mount();
    vehicle_part target_part( vpid, "", vcoords, item( cord ) );
    tripoint source_global( cord.get_var( "source_x", 0 ),
                            cord.get_var( "source_y", 0 ),
                            cord.get_var( "source_z", 0 ) );
    target_part.target.first = here.getabs( source_global );
    target_part.target.second = source_veh->global_square_location().raw();
    target_veh->install_part( vcoords, target_part );
}

double vehicle::engine_cold_factor( const int e ) const
{
    if( !part_info( engines[e] ).has_flag( "E_COLD_START" ) ) {
        return 0.0;
    }

    double eff_temp = units::to_fahrenheit( get_weather().get_temperature(
            get_player_character().pos() ) );
    if( !parts[ engines[ e ] ].has_fault_flag( "BAD_COLD_START" ) ) {
        eff_temp = std::min( eff_temp, 20.0 );
    }

    return 1.0 - ( std::max( 0.0, std::min( 30.0, eff_temp ) ) / 30.0 );
}

int vehicle::engine_start_time( const int e ) const
{
    if( !is_engine_on( e ) || part_info( engines[e] ).has_flag( "E_STARTS_INSTANTLY" ) ||
        !engine_fuel_left( e ) ) {
        return 0;
    }

    const double dmg = parts[engines[e]].damage_percent();

    // non-linear range [100-1000]; f(0.0) = 100, f(0.6) = 250, f(0.8) = 500, f(0.9) = 1000
    // diesel engines with working glow plugs always start with f = 0.6 (or better)
    const double cold = 100 / tanh( 1 - std::min( engine_cold_factor( e ), 0.9 ) );

    // watts to old vhp = watts / 373
    // divided by magic 16 = watts / 6000
    const double watts_per_time = 6000;
    return part_vpower_w( engines[ e ], true ) / watts_per_time + 100 * dmg + cold;
}

bool vehicle::auto_select_fuel( int e )
{
    vehicle_part &vp_engine = parts[ engines[ e ] ];
    const vpart_info &vp_engine_info = part_info( engines[e] );
    if( !vp_engine.is_available() ) {
        return false;
    }
    if( vp_engine_info.fuel_type == fuel_type_none ||
        vp_engine_info.has_flag( "PERPETUAL" ) ||
        engine_fuel_left( e ) > 0 ) {
        return true;
    }
    for( const itype_id &fuel_id : vp_engine_info.engine_fuel_opts() ) {
        if( fuel_left( fuel_id ) > 0 ) {
            vp_engine.fuel_set( fuel_id );
            return true;
        }
    }
    return false; // not a single fuel type left for this engine
}

bool vehicle::start_engine( const int e )
{
    if( !is_engine_on( e ) ) {
        return false;
    }

    const vpart_info &einfo = part_info( engines[e] );
    vehicle_part &eng = parts[ engines[ e ] ];

    bool out_of_fuel = !auto_select_fuel( e );

    Character &player_character = get_player_character();
    if( out_of_fuel ) {
        if( einfo.fuel_type == fuel_type_muscle ) {
            // Muscle engines cannot start with broken limbs
            if( einfo.has_flag( "MUSCLE_ARMS" ) && !player_character.has_two_arms_lifting() ) {
                add_msg( _( "You cannot use %s with a broken arm." ), eng.name() );
                return false;
            } else if( einfo.has_flag( "MUSCLE_LEGS" ) && ( player_character.get_working_leg_count() < 2 ) ) {
                add_msg( _( "You cannot use %s with a broken leg." ), eng.name() );
                return false;
            }
        } else {
            add_msg( _( "Looks like the %1$s is out of %2$s." ), eng.name(),
                     item::nname( einfo.fuel_type ) );
            return false;
        }
    }

    const double dmg = parts[engines[e]].damage_percent();
    const int engine_power = std::abs( part_epower_w( engines[e] ) );
    const double cold_factor = engine_cold_factor( e );
    const int start_moves = engine_start_time( e );

    const tripoint pos = global_part_pos3( engines[e] );
    if( einfo.engine_backfire_threshold() ) {
        if( ( 1 - dmg ) < einfo.engine_backfire_threshold() &&
            one_in( einfo.engine_backfire_freq() ) ) {
            backfire( e );
        } else {
            sounds::sound( pos, start_moves / 10, sounds::sound_t::movement,
                           string_format( _( "the %s bang as it starts!" ), eng.name() ), true, "vehicle",
                           "engine_bangs_start" );
        }
    }

    // Immobilizers need removing before the vehicle can be started
    if( eng.has_fault_flag( "IMMOBILIZER" ) ) {
        sounds::sound( pos, 5, sounds::sound_t::alarm,
                       string_format( _( "the %s making a long beep." ), eng.name() ), true, "vehicle",
                       "fault_immobiliser_beep" );
        return false;
    }

    // Engine with starter motors can fail on both battery and starter motor
    if( eng.faults_potential().count( fault_engine_starter ) ) {
        if( eng.has_fault_flag( "BAD_STARTER" ) ) {
            sounds::sound( pos, eng.info().engine_noise_factor(), sounds::sound_t::alarm,
                           string_format( _( "the %s clicking once." ), eng.name() ), true, "vehicle",
                           "engine_single_click_fail" );
            return false;
        }

        const int start_draw_bat = power_to_energy_bat( engine_power *
                                   ( 1.0 + dmg / 2 + cold_factor / 5 ) * 10, time_duration::from_moves( start_moves ) );
        if( discharge_battery( start_draw_bat, true ) != 0 ) {
            sounds::sound( pos, eng.info().engine_noise_factor(), sounds::sound_t::alarm,
                           string_format( _( "the %s rapidly clicking." ), eng.name() ), true, "vehicle",
                           "engine_multi_click_fail" );
            return false;
        }
    }

    // Engines always fail to start with faulty fuel pumps
    if( eng.has_fault_flag( "BAD_FUEL_PUMP" ) ) {
        sounds::sound( pos, eng.info().engine_noise_factor(), sounds::sound_t::movement,
                       string_format( _( "the %s quickly stuttering out." ), eng.name() ), true, "vehicle",
                       "engine_stutter_fail" );
        return false;
    }

    // Damaged non-electric engines have a chance of failing to start
    if( !is_engine_type( e, fuel_type_battery ) && einfo.fuel_type != fuel_type_muscle &&
        x_in_y( dmg * 100, 120 ) ) {
        sounds::sound( pos, eng.info().engine_noise_factor(), sounds::sound_t::movement,
                       string_format( _( "the %s clanking and grinding." ), eng.name() ), true, "vehicle",
                       "engine_clanking_fail" );
        return false;
    }
    sounds::sound( pos, eng.info().engine_noise_factor(), sounds::sound_t::movement,
                   string_format( _( "the %s starting." ), eng.name() ) );

    if( sfx::has_variant_sound( "engine_start", eng.info().get_id().str() ) ) {
        sfx::play_variant_sound( "engine_start", eng.info().get_id().str(),
                                 eng.info().engine_noise_factor() );
    } else if( einfo.fuel_type == fuel_type_muscle ) {
        sfx::play_variant_sound( "engine_start", "muscle", eng.info().engine_noise_factor() );
    } else if( is_engine_type( e, fuel_type_wind ) ) {
        sfx::play_variant_sound( "engine_start", "wind", eng.info().engine_noise_factor() );
    } else if( is_engine_type( e, fuel_type_battery ) ) {
        sfx::play_variant_sound( "engine_start", "electric", eng.info().engine_noise_factor() );
    } else {
        sfx::play_variant_sound( "engine_start", "combustion", eng.info().engine_noise_factor() );
    }
    return true;
}

void vehicle::stop_engines()
{
    vehicle_noise = 0;
    engine_on = false;
    for( size_t e = 0; e < engines.size(); ++e ) {
        if( !is_engine_on( e ) ) {
            continue;
        }

        const vehicle_part &epart = parts[ engines[ e ] ];
        const vpart_info &einfo = epart.info();
        const tripoint epos = global_part_pos3( epart );

        sounds::sound( epos, 2, sounds::sound_t::movement, _( "the engine go silent" ) );

        std::string variant = einfo.get_id().str();

        if( sfx::has_variant_sound( "engine_stop", variant ) ) {
            // has special sound variant for this vpart id
        } else if( is_engine_type( e, fuel_type_battery ) ) {
            variant = "electric";
        } else if( is_engine_type( e, fuel_type_muscle ) ) {
            variant = "muscle";
        } else if( is_engine_type( e, fuel_type_wind ) ) {
            variant = "wind";
        } else {
            variant = "combustion";
        }

        sfx::play_variant_sound( "engine_stop", variant, einfo.engine_noise_factor() );
    }
    sfx::do_vehicle_engine_sfx();
    refresh();
}

void vehicle::start_engines( const bool take_control, const bool autodrive )
{
    bool has_engine = std::any_of( engines.begin(), engines.end(), [&]( int idx ) {
        return parts[ idx ].enabled && !parts[ idx ].is_broken();
    } );

    // if no engines enabled then enable all before trying to start the vehicle
    if( !has_engine ) {
        for( int idx : engines ) {
            if( !parts[ idx ].is_broken() ) {
                parts[ idx ].enabled = true;
            }
        }
    }

    int start_time = 0;
    // record the first usable engine as the referenced position checked at the end of the engine starting activity
    bool has_starting_engine_position = false;
    tripoint_bub_ms starting_engine_position;
    for( size_t e = 0; e < engines.size(); ++e ) {
        if( !has_starting_engine_position && !parts[ engines[ e ] ].is_broken() &&
            parts[ engines[ e ] ].enabled ) {
            starting_engine_position = bub_part_pos( engines[ e ] );
            has_starting_engine_position = true;
        }
        has_engine = has_engine || is_engine_on( e );
        start_time = std::max( start_time, engine_start_time( e ) );
    }

    if( !has_starting_engine_position ) {
        starting_engine_position = {};
    }

    if( !has_engine ) {
        add_msg( m_info, _( "The %s doesn't have an engine!" ), name );
        refresh();
        return;
    }

    Character &player_character = get_player_character();
    if( take_control && !player_character.controlling_vehicle ) {
        player_character.controlling_vehicle = true;
        add_msg( _( "You take control of the %s." ), name );
    }
    if( !autodrive ) {
        player_character.assign_activity( ACT_START_ENGINES, start_time );
        player_character.activity.relative_placement =
            starting_engine_position - player_character.pos_bub();
        player_character.activity.values.push_back( take_control );
    }
    refresh();
}

void vehicle::enable_patrol()
{
    is_patrolling = true;
    autopilot_on = true;
    autodrive_local_target = tripoint_zero;
    start_engines();
}

void vehicle::honk_horn() const
{
    const bool no_power = !fuel_left( fuel_type_battery, true );
    bool honked = false;

    for( const vpart_reference &vp : get_avail_parts( "HORN" ) ) {
        //Only bicycle horn doesn't need electricity to work
        const vpart_info &horn_type = vp.info();
        if( ( horn_type.get_id() != vpart_horn_bicycle ) && no_power ) {
            continue;
        }
        if( !honked ) {
            add_msg( _( "You honk the horn!" ) );
            honked = true;
        }
        //Get global position of horn
        const tripoint horn_pos = vp.pos();
        //Determine sound
        if( horn_type.bonus >= 110 ) {
            //~ Loud horn sound
            sounds::sound( horn_pos, horn_type.bonus, sounds::sound_t::alarm, _( "HOOOOORNK!" ), false,
                           "vehicle", "horn_loud" );
        } else if( horn_type.bonus >= 80 ) {
            //~ Moderate horn sound
            sounds::sound( horn_pos, horn_type.bonus, sounds::sound_t::alarm, _( "BEEEP!" ), false, "vehicle",
                           "horn_medium" );
        } else {
            //~ Weak horn sound
            sounds::sound( horn_pos, horn_type.bonus, sounds::sound_t::alarm, _( "honk." ), false, "vehicle",
                           "horn_low" );
        }
    }

    if( !honked ) {
        add_msg( _( "You honk the horn, but nothing happens." ) );
    }
}

void vehicle::reload_seeds( const tripoint &pos )
{
    Character &player_character = get_player_character();
    std::vector<item *> seed_inv = player_character.items_with( []( const item & itm ) {
        return itm.is_seed();
    } );

    auto seed_entries = iexamine::get_seed_entries( seed_inv );
    seed_entries.emplace( seed_entries.begin(), seed_tuple( itype_null, _( "No seed" ), 0 ) );

    int seed_index = iexamine::query_seed( seed_entries );

    if( seed_index > 0 && seed_index < static_cast<int>( seed_entries.size() ) ) {
        const int count = std::get<2>( seed_entries[seed_index] );
        int amount = 0;
        const std::string popupmsg = string_format( _( "Move how many?  [Have %d] (0 to cancel)" ), count );

        amount = string_input_popup()
                 .title( popupmsg )
                 .width( 5 )
                 .only_digits( true )
                 .query_int();

        if( amount > 0 ) {
            int actual_amount = std::min( amount, count );
            itype_id seed_id = std::get<0>( seed_entries[seed_index] );
            std::list<item> used_seed;
            if( item::count_by_charges( seed_id ) ) {
                used_seed = player_character.use_charges( seed_id, actual_amount );
            } else {
                used_seed = player_character.use_amount( seed_id, actual_amount );
            }
            used_seed.front().set_age( 0_turns );
            //place seeds into the planter
            // TODO: fix point types
            put_into_vehicle_or_drop( player_character, item_drop_reason::deliberate, used_seed,
                                      tripoint_bub_ms( pos ) );
        }
    }
}

void vehicle::beeper_sound() const
{
    // No power = no sound
    if( fuel_left( fuel_type_battery, true ) == 0 ) {
        return;
    }

    const bool odd_turn = calendar::once_every( 2_turns );
    for( const vpart_reference &vp : get_avail_parts( "BEEPER" ) ) {
        if( ( odd_turn && vp.has_feature( VPFLAG_EVENTURN ) ) ||
            ( !odd_turn && vp.has_feature( VPFLAG_ODDTURN ) ) ) {
            continue;
        }

        //~ Beeper sound
        sounds::sound( vp.pos(), vp.info().bonus, sounds::sound_t::alarm, _( "beep!" ), false, "vehicle",
                       "rear_beeper" );
    }
}

void vehicle::play_music() const
{
    Character &player_character = get_player_character();
    for( const vpart_reference &vp : get_enabled_parts( "STEREO" ) ) {
        iuse::play_music( player_character, vp.pos(), 15, 30 );
    }
}

void vehicle::play_chimes() const
{
    if( !one_in( 3 ) ) {
        return;
    }

    for( const vpart_reference &vp : get_enabled_parts( "CHIMES" ) ) {
        sounds::sound( vp.pos(), 40, sounds::sound_t::music,
                       _( "a simple melody blaring from the loudspeakers." ), false, "vehicle", "chimes" );
    }
}

void vehicle::crash_terrain_around()
{
    if( total_power_w() <= 0 ) {
        return;
    }
    map &here = get_map();
    for( const vpart_reference &vp : get_enabled_parts( "CRASH_TERRAIN_AROUND" ) ) {
        tripoint crush_target( 0, 0, -OVERMAP_LAYERS );
        const tripoint start_pos = vp.pos();
        const transform_terrain_data &ttd = vp.info().transform_terrain;
        for( size_t i = 0; i < eight_horizontal_neighbors.size() &&
             !here.inbounds_z( crush_target.z ); i++ ) {
            tripoint cur_pos = start_pos + eight_horizontal_neighbors[i];
            bool busy_pos = false;
            for( const vpart_reference &vp_tmp : get_all_parts() ) {
                busy_pos |= vp_tmp.pos() == cur_pos;
            }
            for( const std::string &flag : ttd.pre_flags ) {
                if( here.has_flag( flag, cur_pos ) && !busy_pos ) {
                    crush_target = cur_pos;
                    break;
                }
            }
        }
        //target chosen
        if( here.inbounds_z( crush_target.z ) ) {
            velocity = 0;
            cruise_velocity = 0;
            here.destroy( crush_target );
            sounds::sound( crush_target, 500, sounds::sound_t::combat, _( "Clanggggg!" ), false,
                           "smash_success", "hit_vehicle" );
        }
    }
}

void vehicle::transform_terrain()
{
    map &here = get_map();
    for( const vpart_reference &vp : get_enabled_parts( "TRANSFORM_TERRAIN" ) ) {
        const tripoint start_pos = vp.pos();
        const transform_terrain_data &ttd = vp.info().transform_terrain;
        bool prereq_fulfilled = false;
        for( const std::string &flag : ttd.pre_flags ) {
            if( here.has_flag( flag, start_pos ) ) {
                prereq_fulfilled = true;
                break;
            }
        }
        if( prereq_fulfilled ) {
            const ter_id new_ter = ter_id( ttd.post_terrain );
            if( new_ter != t_null ) {
                here.ter_set( start_pos, new_ter );
            }
            const furn_id new_furn = furn_id( ttd.post_furniture );
            if( new_furn != f_null ) {
                here.furn_set( start_pos, new_furn );
            }
            const field_type_id new_field = field_type_id( ttd.post_field );
            if( new_field.id() ) {
                here.add_field( start_pos, new_field, ttd.post_field_intensity, ttd.post_field_age );
            }
        } else {
            const int speed = std::abs( velocity );
            int v_damage = rng( 3, speed );
            damage( here, vp.part_index(), v_damage, damage_type::BASH, false );
            sounds::sound( start_pos, v_damage, sounds::sound_t::combat, _( "Clanggggg!" ), false,
                           "smash_success", "hit_vehicle" );
        }
    }
}

void vehicle::operate_reaper()
{
    map &here = get_map();
    for( const vpart_reference &vp : get_enabled_parts( "REAPER" ) ) {
        const size_t reaper_id = vp.part_index();
        const tripoint reaper_pos = vp.pos();
        const int plant_produced = rng( 1, vp.info().bonus );
        const int seed_produced = rng( 1, 3 );
        const units::volume max_pickup_volume = vp.info().size / 20;
        if( here.furn( reaper_pos ) != f_plant_harvest ) {
            continue;
        }
        // Can't use item_stack::only_item() since there might be fertilizer
        map_stack items = here.i_at( reaper_pos );
        map_stack::iterator seed = std::find_if( items.begin(), items.end(), []( const item & it ) {
            return it.is_seed();
        } );
        if( seed == items.end() || seed->typeId() == itype_fungal_seeds ||
            seed->typeId() == itype_marloss_seed ) {
            // Otherworldly plants, the earth-made reaper can not handle those.
            continue;
        }
        here.furn_set( reaper_pos, f_null );
        // Secure the seed type before i_clear destroys the item.
        const itype &seed_type = *seed->type;
        here.i_clear( reaper_pos );
        for( item &i : iexamine::get_harvest_items(
                 seed_type, plant_produced, seed_produced, false ) ) {
            here.add_item_or_charges( reaper_pos, i );
        }
        sounds::sound( reaper_pos, rng( 10, 25 ), sounds::sound_t::combat, _( "Swish" ), false, "vehicle",
                       "reaper" );
        if( vp.has_feature( "CARGO" ) ) {
            for( map_stack::iterator iter = items.begin(); iter != items.end(); ) {
                if( ( iter->volume() <= max_pickup_volume ) &&
                    add_item( reaper_id, *iter ) ) {
                    iter = items.erase( iter );
                } else {
                    ++iter;
                }
            }
        }
    }
}

void vehicle::operate_planter()
{
    map &here = get_map();
    for( const vpart_reference &vp : get_enabled_parts( "PLANTER" ) ) {
        const size_t planter_id = vp.part_index();
        const tripoint loc = vp.pos();
        vehicle_stack v = get_items( planter_id );
        for( auto i = v.begin(); i != v.end(); i++ ) {
            if( i->is_seed() ) {
                // If it is an "advanced model" then it will avoid damaging itself or becoming damaged. It's a real feature.
                if( here.ter( loc ) != t_dirtmound && vp.has_feature( "ADVANCED_PLANTER" ) ) {
                    //then don't put the item there.
                    break;
                } else if( here.ter( loc ) == t_dirtmound ) {
                    here.set( loc, t_dirt, f_plant_seed );
                } else if( !here.has_flag( ter_furn_flag::TFLAG_PLOWABLE, loc ) ) {
                    //If it isn't plowable terrain, then it will most likely be damaged.
                    damage( here, planter_id, rng( 1, 10 ), damage_type::BASH, false );
                    sounds::sound( loc, rng( 10, 20 ), sounds::sound_t::combat, _( "Clink" ), false, "smash_success",
                                   "hit_vehicle" );
                }
                if( !i->count_by_charges() || i->charges == 1 ) {
                    i->set_age( 0_turns );
                    here.add_item( loc, *i );
                    v.erase( i );
                } else {
                    item tmp = *i;
                    tmp.charges = 1;
                    tmp.set_age( 0_turns );
                    here.add_item( loc, tmp );
                    i->charges--;
                }
                break;
            }
        }
    }
}

void vehicle::operate_scoop()
{
    map &here = get_map();
    for( const vpart_reference &vp : get_enabled_parts( "SCOOP" ) ) {
        const size_t scoop = vp.part_index();
        const int chance_to_damage_item = 9;
        const units::volume max_pickup_volume = vp.info().size / 10;
        const std::array<std::string, 4> sound_msgs = {{
                _( "Whirrrr" ), _( "Ker-chunk" ), _( "Swish" ), _( "Cugugugugug" )
            }
        };
        sounds::sound( global_part_pos3( scoop ), rng( 20, 35 ), sounds::sound_t::combat,
                       random_entry_ref( sound_msgs ), false, "vehicle", "scoop" );
        std::vector<tripoint> parts_points;
        for( const tripoint &current :
             here.points_in_radius( global_part_pos3( scoop ), 1 ) ) {
            parts_points.push_back( current );
        }
        for( const tripoint &position : parts_points ) {
            // TODO: fix point types
            here.mop_spills( tripoint_bub_ms( position ) );
            if( !here.has_items( position ) ) {
                continue;
            }
            item *that_item_there = nullptr;
            map_stack items = here.i_at( position );
            if( here.has_flag( ter_furn_flag::TFLAG_SEALED, position ) ) {
                // Ignore it. Street sweepers are not known for their ability to harvest crops.
                continue;
            }
            for( item &it : items ) {
                if( it.volume() < max_pickup_volume ) {
                    that_item_there = &it;
                    break;
                }
            }
            if( !that_item_there ) {
                continue;
            }
            if( one_in( chance_to_damage_item ) && that_item_there->damage() < that_item_there->max_damage() ) {
                //The scoop will not destroy the item, but it may damage it a bit.
                that_item_there->inc_damage( damage_type::BASH );
                //The scoop gets a lot louder when breaking an item.
                sounds::sound( position, rng( 10,
                                              that_item_there->volume() / units::legacy_volume_factor * 2 + 10 ),
                               sounds::sound_t::combat, _( "BEEEThump" ), false, "vehicle", "scoop_thump" );
            }
            //This attempts to add the item to the scoop inventory and if successful, removes it from the map.
            if( add_item( scoop, *that_item_there ) ) {
                here.i_rem( position, that_item_there );
            } else {
                break;
            }
        }
    }
}

void vehicle::alarm()
{
    if( one_in( 4 ) ) {
        //first check if the alarm is still installed
        bool found_alarm = has_security_working();

        //if alarm found, make noise, else set alarm disabled
        if( found_alarm ) {
            const std::array<std::string, 4> sound_msgs = {{
                    _( "WHOOP WHOOP" ), _( "NEEeu NEEeu NEEeu" ), _( "BLEEEEEEP" ), _( "WREEP" )
                }
            };
            sounds::sound( global_pos3(), static_cast<int>( rng( 45, 80 ) ),
                           sounds::sound_t::alarm,  random_entry_ref( sound_msgs ), false, "vehicle", "car_alarm" );
            if( one_in( 1000 ) ) {
                is_alarm_on = false;
            }
        } else {
            is_alarm_on = false;
        }
    }
}

/**
 * Opens an openable part at the specified index. If it's a multipart, opens
 * all attached parts as well.
 * @param part_index The index in the parts list of the part to open.
 */
void vehicle::open( int part_index )
{
    if( !part_info( part_index ).has_flag( "OPENABLE" ) ) {
        debugmsg( "Attempted to open non-openable part %d (%s) on a %s!", part_index,
                  parts[ part_index ].name(), name );
    } else {
        open_or_close( part_index, true );
    }
}

/**
 * Closes an openable part at the specified index. If it's a multipart, closes
 * all attached parts as well.
 * @param part_index The index in the parts list of the part to close.
 */
void vehicle::close( int part_index )
{
    if( !part_info( part_index ).has_flag( "OPENABLE" ) ) {
        debugmsg( "Attempted to close non-closeable part %d (%s) on a %s!", part_index,
                  parts[ part_index ].name(), name );
    } else {
        open_or_close( part_index, false );
    }
}

bool vehicle::can_close( int part_index, Character &who )
{
    creature_tracker &creatures = get_creature_tracker();
    part_index = get_non_fake_part( part_index );
    std::vector<std::vector<int>> openable_parts = find_lines_of_parts( part_index, "OPENABLE" );
    if( openable_parts.empty() ) {
        std::vector<int> base_element;
        base_element.push_back( part_index );
        openable_parts.emplace_back( base_element );
    }
    for( const std::vector<int> &vec : openable_parts ) {
        for( int partID : vec ) {
            // Check the part for collisions, then if there's a fake part present check that too.
            while( partID >= 0 ) {
                const Creature *const mon = creatures.creature_at( global_part_pos3( parts[partID] ) );
                if( mon ) {
                    if( mon->is_avatar() ) {
                        who.add_msg_if_player( m_info, _( "There's some buffoon in the way!" ) );
                    } else if( mon->is_monster() ) {
                        // TODO: Houseflies, mosquitoes, etc shouldn't count
                        who.add_msg_if_player( m_info, _( "The %s is in the way!" ), mon->get_name() );
                    } else {
                        who.add_msg_if_player( m_info, _( "%s is in the way!" ), mon->disp_name() );
                    }
                    return false;
                }
                if( parts[partID].has_fake && parts[parts[partID].fake_part_at].is_active_fake ) {
                    partID = parts[partID].fake_part_at;
                } else {
                    partID = -1;
                }
            }
        }
    }
    return true;
}

void vehicle::open_all_at( int p )
{
    std::vector<int> parts_here = parts_at_relative( parts[p].mount, true, true );
    for( int &elem : parts_here ) {
        if( part_flag( elem, VPFLAG_OPENABLE ) ) {
            // Note that this will open multi-square and non-multipart parts in the tile. This
            // means that adjacent open multi-square openables can still have closed stuff
            // on same tile after this function returns
            open( elem );
        }
    }
}

/**
 * Opens or closes an openable part at the specified index based on the @opening value passed.
 * If it's a multipart, opens or closes all attached parts as well.
 * @param part_index The index in the parts list of the part to open or close.
 */
void vehicle::open_or_close( const int part_index, const bool opening )
{
    const auto part_open_or_close = [&]( const int parti, const bool opening ) {
        vehicle_part &prt = parts.at( parti );
        prt.open = opening;
        if( prt.is_fake ) {
            parts.at( prt.fake_part_to ).open = opening;
        } else if( prt.has_fake ) {
            parts.at( prt.fake_part_at ).open = opening;
        }
    };
    //find_lines_of_parts() doesn't return the part_index we passed, so we set it on it's own
    part_open_or_close( part_index, opening );
    insides_dirty = true;
    map &here = get_map();
    here.set_transparency_cache_dirty( sm_pos.z );
    const tripoint part_location = mount_to_tripoint( parts[part_index].mount );
    here.set_seen_cache_dirty( part_location );
    const int dist = rl_dist( get_player_character().pos(), part_location );
    if( dist < 20 ) {
        sfx::play_variant_sound( opening ? "vehicle_open" : "vehicle_close",
                                 parts[ part_index ].info().get_id().str(), 100 - dist * 3 );
    }
    for( const std::vector<int> &vec : find_lines_of_parts( part_index, "OPENABLE" ) ) {
        for( const int &partID : vec ) {
            part_open_or_close( partID, opening );
        }
    }

    coeff_air_changed = true;
    coeff_air_dirty = true;
}

void vehicle::use_autoclave( int p )
{
    vehicle_stack items = get_items( p );
    bool filthy_items = std::any_of( items.begin(), items.end(), []( const item & i ) {
        return i.has_flag( json_flag_FILTHY );
    } );

    bool unpacked_items = std::any_of( items.begin(), items.end(), []( const item & i ) {
        return i.has_flag( STATIC( flag_id( "NO_PACKED" ) ) );
    } );

    bool cbms = std::all_of( items.begin(), items.end(), []( const item & i ) {
        return i.is_bionic();
    } );

    if( parts[p].enabled ) {
        parts[p].enabled = false;
        add_msg( m_bad,
                 _( "You turn the autoclave off before it's finished the program, and open its door." ) );
    } else if( items.empty() ) {
        add_msg( m_bad, _( "The autoclave is empty; there's no point in starting it." ) );
    } else if( fuel_left( itype_water ) < 8 && fuel_left( itype_water_clean ) < 8 ) {
        add_msg( m_bad, _( "You need 8 charges of water in the tanks of the %s for the autoclave to run." ),
                 name );
    } else if( filthy_items ) {
        add_msg( m_bad,
                 _( "You need to remove all filthy items from the autoclave to start the sterilizing cycle." ) );
    } else if( !cbms ) {
        add_msg( m_bad, _( "Only CBMs can be sterilized in an autoclave." ) );
    } else if( unpacked_items ) {
        add_msg( m_bad, _( "You should put your CBMs in autoclave pouches to keep them sterile." ) );
    } else {
        parts[p].enabled = true;
        for( item &n : items ) {
            n.set_age( 0_turns );
        }

        if( fuel_left( itype_water ) >= 8 ) {
            drain( itype_water, 8 );
        } else {
            drain( itype_water_clean, 8 );
        }

        add_msg( m_good,
                 _( "You turn the autoclave on and it starts its cycle." ) );
    }
}

void vehicle::use_washing_machine( int p )
{
    avatar &player_character = get_avatar();
    // Get all the items that can be used as detergent
    const inventory &inv = player_character.crafting_inventory();
    std::vector<const item *> detergents = inv.items_with( [inv]( const item & it ) {
        return it.has_flag( STATIC( flag_id( "DETERGENT" ) ) ) && inv.has_charges( it.typeId(), 5 );
    } );

    vehicle_stack items = get_items( p );
    bool filthy_items = std::all_of( items.begin(), items.end(), []( const item & i ) {
        return i.has_flag( json_flag_FILTHY );
    } );

    bool cbms = std::any_of( items.begin(), items.end(), []( const item & i ) {
        return i.is_bionic();
    } );

    if( parts[p].enabled ) {
        parts[p].enabled = false;
        add_msg( m_bad,
                 _( "You turn the washing machine off before it's finished its cycle, and open its lid." ) );
    } else if( items.empty() ) {
        add_msg( m_bad,
                 _( "The washing machine is empty; there's no point in starting it." ) );
    } else if( fuel_left( itype_water ) < 24 && fuel_left( itype_water_clean ) < 24 ) {
        add_msg( m_bad,
                 _( "You need 24 charges of water in the tanks of the %s to fill the washing machine." ),
                 name );
    } else if( detergents.empty() ) {
        add_msg( m_bad, _( "You need 5 charges of a detergent for the washing machine." ) );
    } else if( !filthy_items ) {
        add_msg( m_bad,
                 _( "You need to remove all non-filthy items from the washing machine to start the washing program." ) );
    } else if( cbms ) {
        add_msg( m_bad,
                 _( "CBMs can't be cleaned in a washing machine.  You need to remove them." ) );
    } else {
        uilist detergent_selector;
        detergent_selector.text = _( "Use what detergent?" );

        std::vector<itype_id> det_types;
        for( const item *it : detergents ) {
            itype_id det_type = it->typeId();
            // If the vector does not contain the detergent type, add it
            if( std::find( det_types.begin(), det_types.end(), det_type ) == det_types.end() ) {
                det_types.emplace_back( det_type );
            }

        }
        int chosen_detergent = 0;
        // If there's a choice to be made on what detergent to use, ask the player
        if( det_types.size() > 1 ) {
            for( size_t i = 0; i < det_types.size(); ++i ) {
                detergent_selector.addentry( i, true, 0, item::nname( det_types[i] ) );
            }
            detergent_selector.addentry( UILIST_CANCEL, true, 0, _( "Cancel" ) );
            detergent_selector.query();
            chosen_detergent = detergent_selector.ret;
        }

        // If the player exits the menu, don't do anything else
        if( chosen_detergent == UILIST_CANCEL ) {
            return;
        }

        parts[p].enabled = true;
        for( item &n : items ) {
            n.set_age( 0_turns );
        }

        if( fuel_left( itype_water ) >= 24 ) {
            drain( itype_water, 24 );
        } else {
            drain( itype_water_clean, 24 );
        }

        std::vector<item_comp> detergent;
        detergent.emplace_back( det_types[chosen_detergent], 5 );
        player_character.consume_items( detergent, 1, is_crafting_component );

        add_msg( m_good,
                 _( "You pour some detergent into the washing machine, close its lid, and turn it on.  The washing machine is being filled from the water tanks." ) );
    }
}

void vehicle::use_dishwasher( int p )
{
    avatar &player_character = get_avatar();
    bool detergent_is_enough = player_character.crafting_inventory().has_charges( itype_detergent, 5 );
    vehicle_stack items = get_items( p );
    bool filthy_items = std::all_of( items.begin(), items.end(), []( const item & i ) {
        return i.has_flag( json_flag_FILTHY );
    } );

    std::string buffer;
    buffer += _( "Soft items can't be cleaned in a dishwasher; you should use a washing machine for that.  You need to remove them:" );
    bool soft_items = false;
    for( const item &it : items ) {
        if( it.is_soft() ) {
            soft_items = true;
            buffer += " " + it.tname();
        }
    }

    if( parts[p].enabled ) {
        parts[p].enabled = false;
        add_msg( m_bad,
                 _( "You turn the dishwasher off before it's finished its cycle, and open its lid." ) );
    } else if( items.empty() ) {
        add_msg( m_bad,
                 _( "The dishwasher is empty, there's no point in starting it." ) );
    } else if( fuel_left( itype_water ) < 24 && fuel_left( itype_water_clean ) < 24 ) {
        add_msg( m_bad, _( "You need 24 charges of water in the tanks of the %s to fill the dishwasher." ),
                 name );
    } else if( !detergent_is_enough ) {
        add_msg( m_bad, _( "You need 5 charges of a detergent for the dishwasher." ) );
    } else if( !filthy_items ) {
        add_msg( m_bad,
                 _( "You need to remove all non-filthy items from the dishwasher to start the washing program." ) );
    } else if( soft_items ) {
        add_msg( m_bad, buffer );
    } else {
        parts[p].enabled = true;
        for( item &n : items ) {
            n.set_age( 0_turns );
        }

        if( fuel_left( itype_water ) >= 24 ) {
            drain( itype_water, 24 );
        } else {
            drain( itype_water_clean, 24 );
        }

        std::vector<item_comp> detergent;
        detergent.emplace_back( itype_detergent, 5 );
        player_character.consume_items( detergent, 1, is_crafting_component );

        add_msg( m_good,
                 _( "You pour some detergent into the dishwasher, close its lid, and turn it on.  The dishwasher is being filled from the water tanks" ) );
    }
}

void vehicle::use_monster_capture( int part, const tripoint &pos )
{
    if( parts[part].is_broken() || parts[part].removed ) {
        return;
    }
    item base = item( parts[part].get_base() );
    base.type->invoke( get_avatar(), base, pos );
    parts[part].set_base( base );
    if( base.has_var( "contained_name" ) ) {
        parts[part].set_flag( vehicle_part::animal_flag );
    } else {
        parts[part].remove_flag( vehicle_part::animal_flag );
    }
    invalidate_mass();
}

void vehicle::use_harness( int part, const tripoint &pos )
{
    if( parts[part].is_unavailable() || parts[part].removed ) {
        return;
    }
    if( !g->is_empty( pos ) ) {
        add_msg( m_info, _( "The harness is blocked." ) );
        return;
    }
    creature_tracker &creatures = get_creature_tracker();
    const std::function<bool( const tripoint & )> f = [&creatures]( const tripoint & pnt ) {
        monster *mon_ptr = creatures.creature_at<monster>( pnt );
        if( mon_ptr == nullptr ) {
            return false;
        }
        monster &f = *mon_ptr;
        return f.friendly != 0 && ( f.has_flag( MF_PET_MOUNTABLE ) ||
                                    f.has_flag( MF_PET_HARNESSABLE ) );
    };

    const cata::optional<tripoint> pnt_ = choose_adjacent_highlight(
            _( "Where is the creature to harness?" ), _( "There is no creature to harness nearby." ), f,
            false );
    if( !pnt_ ) {
        add_msg( m_info, _( "Never mind." ) );
        return;
    }
    const tripoint &target = *pnt_;
    monster *mon_ptr = creatures.creature_at<monster>( target );
    if( mon_ptr == nullptr ) {
        add_msg( m_info, _( "No creature there." ) );
        return;
    }
    monster &m = *mon_ptr;
    std::string Harness_Bodytype = "HARNESS_" + m.type->bodytype;
    if( m.friendly == 0 ) {
        add_msg( m_info, _( "This creature is not friendly!" ) );
        return;
    } else if( !m.has_flag( MF_PET_MOUNTABLE ) && !m.has_flag( MF_PET_HARNESSABLE ) ) {
        add_msg( m_info, _( "This creature cannot be harnessed." ) );
        return;
    } else if( !part_flag( part, Harness_Bodytype ) && !part_flag( part, "HARNESS_any" ) ) {
        add_msg( m_info, _( "The harness is not adapted for this creature morphology." ) );
        return;
    }

    m.add_effect( effect_harnessed, 1_turns, true );
    m.setpos( pos );
    //~ %1$s: monster name, %2$s: vehicle name
    add_msg( m_info, _( "You harness your %1$s to %2$s." ), m.get_name(), disp_name() );
    if( m.has_effect( effect_tied ) ) {
        add_msg( m_info, _( "You untie your %s." ), m.get_name() );
        m.remove_effect( effect_tied );
        if( m.tied_item ) {
            get_player_character().i_add( *m.tied_item );
            m.tied_item.reset();
        }
    }
}

void vehicle::build_bike_rack_menu( veh_menu &menu, int part )
{
    // prevent racking two vehicles with same name on single vehicle
    // @returns true if vehicle already has a vehicle with this name racked
    const auto has_veh_name_racked = [this]( const std::string & name ) {
        for( const vpart_reference &vpr : get_any_parts( "BIKE_RACK_VEH" ) ) {
            for( const unrackable_vehicle &unrackable : find_vehicles_to_unrack( vpr.part_index() ) ) {
                if( unrackable.name == name ) {
                    return true;
                }
            }
        }
        return false;
    };

    menu.desc_lines_hint = std::max( 1, menu.desc_lines_hint );
    bool has_rack_actions = false;

    for( const rackable_vehicle &rackable : find_vehicles_to_rack( part ) ) {
        const bool has_this_name_racked = has_veh_name_racked( rackable.name );

        menu.add( string_format( _( "Attach the %s to the rack" ), rackable.name ) )
        .desc( has_this_name_racked
               ? string_format( _( "This vehicle already has '%s' racked.  "
                                   "Please rename before racking." ), rackable.name )
               : "" )
        .enable( !has_this_name_racked )
        .hotkey_auto()
        .skip_locked_check()
        .on_submit( [this, rackable] {
            bikerack_racking_activity_actor rack( *this, *rackable.veh, rackable.racks );
            get_player_character().assign_activity( player_activity( rack ), false );
        } );

        has_rack_actions = true;
    }

    for( const unrackable_vehicle &unrackable : find_vehicles_to_unrack( part ) ) {
        menu.add( string_format( _( "Remove the %s from the rack" ), unrackable.name ) )
        .hotkey_auto()
        .skip_locked_check()
        .on_submit( [this, unrackable] {
            bikerack_unracking_activity_actor unrack( *this, unrackable.parts, unrackable.racks );
            get_player_character().assign_activity( player_activity( unrack ), false );
        } );

        has_rack_actions = true;
    }

    if( !has_rack_actions ) {
        menu.add( _( "Bike rack is empty" ) )
        .desc( _( "Nothing to take off or put on the rack is nearby." ) )
        .enable( false )
        .skip_locked_check();
    }
}

void vpart_position::form_inventory( inventory &inv ) const
{
    const int veh_battery = vehicle().fuel_left( itype_battery, true );
    const cata::optional<vpart_reference> vp_faucet = part_with_tool( itype_water_faucet );
    const cata::optional<vpart_reference> vp_cargo = part_with_feature( "CARGO", true );

    if( vp_cargo ) {
        const vehicle_stack items = vehicle().get_items( vp_cargo->part_index() );
        for( const item &it : items ) {
            if( it.empty_container() && it.is_watertight_container() ) {
                const int count = it.count_by_charges() ? it.charges : 1;
                inv.update_liq_container_count( it.typeId(), count );
            }
            inv.add_item( it );
        }
    }

    // HACK: water_faucet pseudo tool gives access to liquids in tanks
    if( vp_faucet && inv.provide_pseudo_item( itype_water_faucet, 0 ) ) {
        for( const item *it : vehicle().fuel_items_left() ) {
            if( it->made_of( phase_id::LIQUID ) ) {
                item fuel( *it );
                inv.add_item( fuel );
            }
        }
    }

    for( const std::pair<itype_id, int> &tool : get_tools() ) {
        inv.provide_pseudo_item( tool.first, veh_battery );
    }
}

static bool tool_wants_battery( const itype_id &type )
{
    item tool( type, calendar::turn_zero );
    item mag( tool.magazine_default() );
    mag.clear_items();

    return tool.can_contain( mag ).success() &&
           tool.put_in( mag, item_pocket::pocket_type::MAGAZINE_WELL ).success() &&
           tool.ammo_capacity( ammo_battery ) > 0;
}

static bool use_vehicle_tool( vehicle &veh, const tripoint &vp_pos, const itype_id &tool_type )
{
    item pseudo( tool_type, calendar::turn_zero );
    pseudo.set_flag( STATIC( flag_id( "PSEUDO" ) ) );
    if( !tool_wants_battery( tool_type ) ) {
        get_player_character().invoke_item( &pseudo );
        return true;
    }
    if( veh.fuel_left( itype_battery, true ) < pseudo.ammo_required() ) {
        return false;
    }
    // TODO: Figure out this comment: Pseudo items don't have a magazine in it, and they don't need it anymore.
    item pseudo_magazine( pseudo.magazine_default() );
    pseudo_magazine.clear_items(); // no initial ammo
    pseudo.put_in( pseudo_magazine, item_pocket::pocket_type::MAGAZINE_WELL );
    const int capacity = pseudo.ammo_capacity( ammo_battery );
    const int qty = capacity - veh.discharge_battery( capacity );
    pseudo.ammo_set( itype_battery, qty );
    get_player_character().invoke_item( &pseudo );
    player_activity &act = get_player_character().activity;

    // HACK: Evil hack incoming
    if( act.id() == ACT_REPAIR_ITEM &&
        ( tool_type == itype_welder || tool_type == itype_soldering_iron ) ) {
        act.index = INT_MIN; // tell activity the item doesn't really exist
        act.coords.push_back( vp_pos ); // tell it to search for the tool on `pos`
        act.str_values.push_back( tool_type.str() ); // specific tool on the rig
    }

    veh.charge_battery( pseudo.ammo_remaining() );
    return true;
}

void vehicle::build_interact_menu( veh_menu &menu, const tripoint &p, bool with_pickup )
{
    const optional_vpart_position ovp = get_map().veh_at( p );
    if( !ovp ) {
        debugmsg( "vehicle::build_interact_menu couldn't find vehicle at %s", p.to_string() );
        return;
    }
    const vpart_position vp = *ovp;
    const tripoint vppos = vp.pos();

    // @returns true if pos contains available part with a flag
    const auto has_part_here = [this, vppos]( const std::string & flag ) {
        return !get_parts_at( vppos, flag, part_status_flag::working ).empty();
    };

    const bool remote = g->remoteveh() == this;
    const bool has_electronic_controls = remote
                                         ? has_part( "CTRL_ELECTRONIC" ) || has_part( "REMOTE_CONTROLS" )
                                         : has_part_here( "CTRL_ELECTRONIC" );
    const bool controls_here = has_part_here( "CONTROLS" );
    const bool player_is_driving = get_player_character().controlling_vehicle;

    if( !is_appliance() ) {
        menu.add( _( "Examine vehicle" ) )
        .skip_theft_check()
        .skip_locked_check()
        .hotkey( "EXAMINE_VEHICLE" )
        .on_submit( [this] { g->exam_vehicle( *this ); } );

        menu.add( tracking_on ? _( "Forget vehicle position" ) : _( "Remember vehicle position" ) )
        .skip_theft_check()
        .skip_locked_check()
        .keep_menu_open()
        .hotkey( "TOGGLE_TRACKING" )
        .on_submit( [this] { toggle_tracking(); } );
    }

    if( is_locked && controls_here ) {
        menu.add( _( "Hotwire" ) )
        .enable( get_player_character().crafting_inventory().has_quality( qual_SCREW ) )
        .desc( _( "Attempt to hotwire the car using a screwdriver." ) )
        .skip_locked_check()
        .hotkey( "HOTWIRE" )
        .on_submit( [this] {
            ///\EFFECT_MECHANICS speeds up vehicle hotwiring
            const int skill = std::max( 1, get_player_character().get_skill_level( skill_mechanics ) );
            const int moves = to_moves<int>( 6000_seconds / skill );
            const tripoint target = global_square_location().raw() + coord_translate( parts[0].mount );
            const hotwire_car_activity_actor hotwire_act( moves, target );
            get_player_character().assign_activity( player_activity( hotwire_act ) );
        } );

        if( !is_alarm_on ) {
            menu.add( _( "Trigger the alarm" ) )
            .enable( has_security_working() )
            .desc( _( "Trigger the alarm to make noise." ) )
            .skip_locked_check()
            .hotkey( "TOGGLE_ALARM" )
            .on_submit( [this] {
                is_alarm_on = true;
                add_msg( _( "You trigger the alarm" ) );
            } );
        } else { // alarm is on
            if( velocity == 0 && !remote ) {
                menu.add( _( "Try to disarm alarm" ) )
                .skip_locked_check()
                .hotkey( "TOGGLE_ALARM" )
                .on_submit( [this] { smash_security_system(); } );
            }
        }
    }

    if( remote ) {
        menu.add( _( "Stop controlling" ) )
        .hotkey( "RELEASE_CONTROLS" )
        .skip_theft_check()
        .on_submit( [] {
            get_player_character().controlling_vehicle = false;
            g->setremoteveh( nullptr );
            add_msg( _( "You stop controlling the vehicle." ) );
        } );
    } else {
        if( has_part( "ENGINE" ) ) {
            if( ( controls_here && player_is_driving ) || ( remote && engine_on ) ) {
                menu.add( _( "Stop driving" ) )
                .hotkey( "TOGGLE_ENGINE" )
                .skip_theft_check()
                .on_submit( [this] {
                    if( engine_on && has_engine_type_not( fuel_type_muscle, true ) )
                    {
                        add_msg( _( "You turn the engine off and let go of the controls." ) );
                    } else
                    {
                        add_msg( _( "You let go of the controls." ) );
                    }
                    stop_engines();
                    get_player_character().controlling_vehicle = false;
                    g->setremoteveh( nullptr );
                } );
            } else if( controls_here && has_engine_type_not( fuel_type_muscle, true ) ) {
                menu.add( engine_on ? _( "Turn off the engine" ) : _( "Turn on the engine" ) )
                .hotkey( "TOGGLE_ENGINE" )
                .skip_theft_check()
                .on_submit( [this] {
                    if( engine_on )
                    {
                        add_msg( _( "You turn the engine off." ) );
                        stop_engines();
                    } else
                    {
                        start_engines();
                    }
                } );
            }
        }

        if( player_is_driving ) {
            menu.add( _( "Let go of controls" ) )
            .hotkey( "RELEASE_CONTROLS" )
            .skip_locked_check() // in case player somehow controls locked vehicle
            .skip_theft_check()
            .on_submit( [] {
                get_player_character().controlling_vehicle = false;
                add_msg( _( "You let go of the controls." ) );
            } );

            menu.add( _( "Pull handbrake" ) )
            .hotkey( "PULL_HANDBRAKE" )
            .on_submit( [] { handbrake(); } );
        }

        if( controls_here ) {
            menu.add( _( "Control individual engines" ) )
            .hotkey( "CONTROL_ENGINES" )
            .on_submit( [this] { control_engines(); } );
        }
    }

    if( controls_here && has_part( "AUTOPILOT" ) && has_electronic_controls ) {
        menu.add( _( "Control autopilot" ) )
        .hotkey( "CONTROL_AUTOPILOT" )
        .on_submit( [this] { toggle_autopilot(); } );
    }

    if( is_appliance() || vp.avail_part_with_feature( "CTRL_ELECTRONIC" ) ) {
        build_electronics_menu( menu );
    }

    if( controls_here ) {
        menu.add( cruise_on ? _( "Disable cruise control" ) : _( "Enable cruise control" ) )
        .hotkey( "TOGGLE_CRUISE_CONTROL" )
        .keep_menu_open()
        .on_submit( [this] {
            cruise_on = !cruise_on;
            add_msg( cruise_on ? _( "Cruise control turned on" ) : _( "Cruise control turned off" ) );
        } );
    }

    if( has_electronic_controls && has_part( "SMART_ENGINE_CONTROLLER" ) ) {
        menu.add( _( "Smart controller settings" ) )
        .hotkey( "CONTROL_SMART_ENGINE" )
        .on_submit( [this] {
            if( !smart_controller_cfg )
            {
                smart_controller_cfg = smart_controller_config();
            }

            smart_controller_settings cfg_view = smart_controller_settings(
                has_enabled_smart_controller,
                smart_controller_cfg -> battery_lo,
                smart_controller_cfg -> battery_hi );
            smart_controller_ui( cfg_view ).control();
            for( const vpart_reference &vp : get_avail_parts( "SMART_ENGINE_CONTROLLER" ) )
            {
                vp.part().enabled = cfg_view.enabled;
            }
        } );
    }

    const turret_data turret = turret_query( vp.pos() );

    if( turret.can_unload() ) {
        menu.add( string_format( _( "Unload %s" ), turret.name() ) )
        .hotkey( "UNLOAD_TURRET" )
        .skip_locked_check()
        .on_submit( [this, vppos] {
            item_location loc = turret_query( vppos ).base();
            get_player_character().unload( loc );
        } );
    }

    if( turret.can_reload() ) {
        menu.add( string_format( _( "Reload %s" ), turret.name() ) )
        .hotkey( "RELOAD_TURRET" )
        .skip_locked_check()
        .on_submit( [this, vppos] {
            item_location loc = turret_query( vppos ).base();
            item::reload_option opt = get_player_character().select_ammo( loc, true );
            if( opt )
            {
                reload_activity_actor reload_act( std::move( opt ) );
                get_player_character().assign_activity( player_activity( reload_act ) );
            }
        } );
    }

    if( controls_here && has_part( "TURRET" ) ) {
        menu.add( _( "Set turret targeting modes" ) )
        .hotkey( "TURRET_TARGET_MODE" )
        .on_submit( [this] { turrets_set_targeting(); } );

        menu.add( _( "Set turret firing modes" ) )
        .hotkey( "TURRET_FIRE_MODE" )
        .on_submit( [this] { turrets_set_mode(); } );

        // We can also fire manual turrets with ACTION_FIRE while standing at the controls.
        menu.add( _( "Aim turrets manually" ) )
        .hotkey( "TURRET_MANUAL_AIM" )
        .on_submit( [this] { turrets_aim_and_fire_all_manual( true ); } );

        // This lets us manually override and set the target for the automatic turrets instead.
        menu.add( _( "Aim automatic turrets" ) )
        .hotkey( "TURRET_MANUAL_OVERRIDE" )
        .on_submit( [this] { turrets_override_automatic_aim(); } );

        menu.add( _( "Aim individual turret" ) )
        .hotkey( "TURRET_SINGLE_FIRE" )
        .on_submit( [this] { turrets_aim_and_fire_single(); } );
    }

    if( controls_here ) {
        if( has_part( "HORN" ) ) {
            menu.add( _( "Honk horn" ) )
            .skip_locked_check()
            .hotkey( "SOUND_HORN" )
            .on_submit( [this] { honk_horn(); } );
        }
    }

    for( const std::pair<itype_id, int> &pair : vp.get_tools() ) {
        const itype_id tool = pair.first;
        const char hotkey = pair.second;
        if( hotkey == -1 || !tool->has_use() ) {
            continue; // passive tool
        }

        menu.add( _( "Use " ) + tool->nname( 1 ) )
        .enable( fuel_left( itype_battery, true ) >= tool->charges_to_use() )
        .hotkey( hotkey )
        .skip_locked_check( !tool_wants_battery( tool ) )
        .on_submit( [this, vppos, tool] { use_vehicle_tool( *this, vppos, tool ); } );
    }

    const cata::optional<vpart_reference> vp_autoclave = vp.avail_part_with_feature( "AUTOCLAVE" );
    if( vp_autoclave ) {
        const size_t cl_idx = vp_autoclave->part_index();
        menu.add( vp_autoclave->part().enabled
                  ? _( "Deactivate the autoclave" )
                  : _( "Activate the autoclave (1.5 hours)" ) )
        .hotkey( "TOGGLE_AUTOCLAVE" )
        .on_submit( [this, cl_idx] { use_autoclave( cl_idx ); } );
    }

    const cata::optional<vpart_reference> vp_washing_machine =
        vp.avail_part_with_feature( "WASHING_MACHINE" );
    if( vp_washing_machine ) {
        const size_t wm_idx = vp_washing_machine->part_index();
        menu.add( vp_washing_machine->part().enabled
                  ? _( "Deactivate the washing machine" )
                  : _( "Activate the washing machine (1.5 hours)" ) )
        .hotkey( "TOGGLE_WASHING_MACHINE" )
        .on_submit( [this, wm_idx] { use_washing_machine( wm_idx ); } );
    }

    const cata::optional<vpart_reference> vp_dishwasher = vp.avail_part_with_feature( "DISHWASHER" );
    if( vp_dishwasher ) {
        const size_t dw_idx = vp_dishwasher->part_index();
        menu.add( vp_dishwasher->part().enabled
                  ? _( "Deactivate the dishwasher" )
                  : _( "Activate the dishwasher (1.5 hours)" ) )
        .hotkey( "TOGGLE_DISHWASHER" )
        .on_submit( [this, dw_idx] { use_dishwasher( dw_idx ); } );
    }

    const cata::optional<vpart_reference> vp_cargo = vp.part_with_feature( "CARGO", false );
    // Whether vehicle part (cargo) contains items, and whether map tile (ground) has items
    if( with_pickup && (
            get_map().has_items( vp.pos() ) ||
            ( vp_cargo && !get_items( vp_cargo->part_index() ).empty() ) ) ) {
        menu.add( _( "Get items" ) )
        .hotkey( "GET_ITEMS" )
        .skip_locked_check()
        .skip_theft_check()
        .on_submit( [vppos] { g->pickup( vppos ); } );
    }

    const cata::optional<vpart_reference> vp_curtain = vp.avail_part_with_feature( "CURTAIN" );
    if( vp_curtain && !vp_curtain->part().open ) {
        menu.add( _( "Peek through the closed curtains" ) )
        .hotkey( "CURTAIN_PEEK" )
        .skip_locked_check()
        .on_submit( [vppos] {
            add_msg( _( "You carefully peek through the curtains." ) );
            g->peek( vppos );
        } );
    }

    if( vp.part_with_tool( itype_water_faucet ) ) {
        int vp_tank_idx = -1;
        item *water_item = nullptr;
        for( const int i : fuel_containers ) {
            vehicle_part &part = parts[i];
            if( part.ammo_current() == itype_water_clean &&
                part.base.only_item().made_of( phase_id::LIQUID ) ) {
                vp_tank_idx = i;
                water_item = &part.base.only_item();
                break;
            }
        }

        if( vp_tank_idx != -1 && water_item != nullptr ) {
            menu.add( _( "Fill a container with water" ) )
            .hotkey( "FAUCET_FILL" )
            .skip_locked_check()
            .on_submit( [this, vp_tank_idx] {
                item &vp_tank_item = parts[vp_tank_idx].base;
                item &water = vp_tank_item.only_item();
                liquid_handler::handle_liquid( water, &vp_tank_item, 1, nullptr, this, vp_tank_idx );
            } );

            menu.add( _( "Have a drink" ) )
            .enable( get_player_character().will_eat( *water_item ).success() )
            .hotkey( "FAUCET_DRINK" )
            .skip_locked_check()
            .on_submit( [this, vp_tank_idx] {
                vehicle_part &vp_tank = parts[vp_tank_idx];
                // this is not "proper" use of vehicle_cursor, but should be good enough for reducing
                // charges and deleting the liquid on last charge drained, for more details see #61164
                item_location base_loc( vehicle_cursor( *this, vp_tank_idx ), &vp_tank.base );
                item_location water_loc( base_loc, &vp_tank.base.only_item() );
                const consume_activity_actor consume_act( water_loc );
                get_player_character().assign_activity( player_activity( consume_act ) );
            } );
        }
    }

    if( vp.part_with_tool( itype_pseudo_water_purifier ) ) {
        menu.add( _( "Purify water in vehicle tank" ) )
        .enable( fuel_left( itype_water ) &&
                 fuel_left( itype_battery, true ) >= itype_pseudo_water_purifier->charges_to_use() )
        .hotkey( "PURIFY_WATER" )
        .on_submit( [this] {
            const auto sel = []( const vehicle_part & pt )
            {
                return pt.is_tank() && pt.ammo_current() == itype_water;
            };
            std::string title = string_format( _( "Purify <color_%s>water</color> in tank" ),
                                               get_all_colors().get_name( itype_water->color ) );
            vehicle_part &tank = veh_interact::select_part( *this, sel, title );
            if( !tank )
            {
                return;
            }
            int64_t cost = static_cast<int64_t>( itype_pseudo_water_purifier->charges_to_use() );
            if( fuel_left( itype_battery, true ) < tank.ammo_remaining() * cost )
            {
                //~ $1 - vehicle name, $2 - part name
                add_msg( m_bad, _( "Insufficient power to purify the contents of the %1$s's %2$s" ),
                         name, tank.name() );
            } else
            {
                //~ $1 - vehicle name, $2 - part name
                add_msg( m_good, _( "You purify the contents of the %1$s's %2$s" ), name, tank.name() );
                discharge_battery( tank.ammo_remaining() * cost );
                tank.ammo_set( itype_water_clean, tank.ammo_remaining() );
            }
        } );
    }

    const cata::optional<vpart_reference> vp_monster_capture =
        vp.avail_part_with_feature( "CAPTURE_MONSTER_VEH" );
    if( vp_monster_capture ) {
        const size_t mc_idx = vp_monster_capture->part_index();
        menu.add( _( "Capture or release a creature" ) )
        .hotkey( "USE_CAPTURE_MONSTER_VEH" )
        .on_submit( [this, mc_idx, vppos] { use_monster_capture( mc_idx, vppos ); } );
    }

    const cata::optional<vpart_reference> vp_bike_rack = vp.avail_part_with_feature( "BIKE_RACK_VEH" );
    if( vp_bike_rack ) {
        build_bike_rack_menu( menu, vp_bike_rack->part_index() );
    }

    const cata::optional<vpart_reference> vp_harness = vp.avail_part_with_feature( "ANIMAL_CTRL" );
    if( vp_harness ) {
        const size_t hn_idx = vp_harness->part_index();
        menu.add( _( "Harness an animal" ) )
        .hotkey( "USE_ANIMAL_CTRL" )
        .on_submit( [this, hn_idx, vppos] { use_harness( hn_idx, vppos ); } );
    }

    if( vp.avail_part_with_feature( "PLANTER" ) ) {
        menu.add( _( "Reload seed drill with seeds" ) )
        .hotkey( "USE_PLANTER" )
        .on_submit( [this, vppos] { reload_seeds( vppos ); } );
    }

    const cata::optional<vpart_reference> vp_workbench = vp.avail_part_with_feature( "WORKBENCH" );
    if( vp_workbench ) {
        const size_t wb_idx = vp_workbench->part_index();
        menu.add( string_format( _( "Craft at the %s" ), vp_workbench->part().name() ) )
        .hotkey( "USE_WORKBENCH" )
        .skip_locked_check()
        .on_submit( [this, wb_idx, vppos] {
            const vpart_reference vp_workbench( *this, wb_idx );
            iexamine::workbench_internal( get_player_character(), vppos, vp_workbench );
        } );
    }

    if( is_foldable() && !remote ) {
        menu.add( string_format( _( "Fold %s" ), name ) )
        .hotkey( "FOLD_VEHICLE" )
        .on_submit( [this] {
            vehicle_folding_activity_actor folding_act( *this );
            get_avatar().assign_activity( player_activity( folding_act ) );
        } );
    }
}

void vehicle::interact_with( const tripoint &p, bool with_pickup )
{
    const optional_vpart_position ovp = get_map().veh_at( p );
    if( !ovp ) {
        debugmsg( "interact_with called at %s and no vehicle is found", p.to_string() );
        return;
    }

    veh_menu menu( *this, _( "Select an action" ) );
    do {
        menu.reset();
        build_interact_menu( menu, p, with_pickup );
    } while( menu.query() );
}
