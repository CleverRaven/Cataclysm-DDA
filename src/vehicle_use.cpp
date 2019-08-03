#include "vehicle.h" // IWYU pragma: associated

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <memory>
#include <tuple>
#include <list>

#include "action.h"
#include "activity_handlers.h"
#include "avatar.h"
#include "debug.h"
#include "game.h"
#include "iexamine.h"
#include "item.h"
#include "itype.h"
#include "json.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "messages.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pickup.h"
#include "sounds.h"
#include "string_formatter.h"
#include "translations.h"
#include "ui.h"
#include "veh_interact.h"
#include "veh_type.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "string_input_popup.h"
#include "color.h"
#include "input.h"
#include "int_id.h"
#include "inventory.h"
#include "iuse.h"
#include "player.h"
#include "player_activity.h"
#include "pldata.h"
#include "requirements.h"
#include "rng.h"
#include "string_id.h"
#include "field.h"
#include "bodypart.h"
#include "enums.h"
#include "monster.h"
#include "mtype.h"
#include "weather.h"

static const itype_id fuel_type_none( "null" );
static const itype_id fuel_type_battery( "battery" );
static const itype_id fuel_type_muscle( "muscle" );
static const itype_id fuel_type_wind( "wind" );

static const fault_id fault_diesel( "fault_engine_pump_diesel" );
static const fault_id fault_glowplug( "fault_engine_glow_plug" );
static const fault_id fault_immobiliser( "fault_engine_immobiliser" );
static const fault_id fault_pump( "fault_engine_pump_fuel" );
static const fault_id fault_starter( "fault_engine_starter" );
const efftype_id effect_harnessed( "harnessed" );
const efftype_id effect_tied( "tied" );
const skill_id skill_mechanics( "mechanics" );

enum change_types : int {
    OPENCURTAINS = 0,
    OPENBOTH,
    CLOSEDOORS,
    CLOSEBOTH,
    CANCEL
};

char keybind( const std::string &opt, const std::string &context )
{
    const auto keys = input_context( context ).keys_bound_to( opt );
    return keys.empty() ? ' ' : keys.front();
}

void vehicle::add_toggle_to_opts( std::vector<uilist_entry> &options,
                                  std::vector<std::function<void()>> &actions, const std::string &name, char key,
                                  const std::string &flag )
{
    // fetch matching parts and abort early if none found
    const auto found = get_avail_parts( flag );
    if( empty( found ) ) {
        return;
    }

    // can this menu option be selected by the user?
    bool allow = true;

    // determine target state - currently parts of similar type are all switched concurrently
    bool state = std::none_of( found.begin(), found.end(), []( const vpart_reference & vp ) {
        return vp.part().enabled;
    } );

    // if toggled part potentially usable check if could be enabled now (sufficient fuel etc.)
    if( state ) {
        allow = std::any_of( found.begin(), found.end(), []( const vpart_reference & vp ) {
            return vp.vehicle().can_enable( vp.part() );
        } );
    }

    auto msg = string_format( state ?
                              _( "Turn on %s" ) :
                              colorize( _( "Turn off %s" ), c_pink ),
                              name );
    options.emplace_back( -1, allow, key, msg );

    actions.push_back( [ = ] {
        for( const vpart_reference &vp : found )
        {
            vehicle_part &e = vp.part();
            if( e.enabled != state ) {
                add_msg( state ? _( "Turned on %s" ) : _( "Turned off %s." ), e.name() );
                e.enabled = state;
            }
        }
        refresh();
    } );
}

void vehicle::control_doors()
{
    const auto door_motors = get_avail_parts( "DOOR_MOTOR" );
    std::vector< int > doors_with_motors; // Indices of doors
    std::vector< tripoint > locations; // Locations used to display the doors
    // it is possible to have one door to open and one to close for single motor
    if( empty( door_motors ) ) {
        debugmsg( "vehicle::control_doors called but no door motors found" );
        return;
    }

    uilist pmenu;
    pmenu.title = _( "Select door to toggle" );
    for( const vpart_reference &vp : door_motors ) {
        const size_t p = vp.part_index();
        if( vp.part().is_unavailable() ) {
            continue;
        }
        const std::array<int, 2> doors = { { next_part_to_open( p ), next_part_to_close( p ) } };
        for( int door : doors ) {
            if( door == -1 ) {
                continue;
            }

            int val = doors_with_motors.size();
            doors_with_motors.push_back( door );
            locations.push_back( global_part_pos3( p ) );
            const char *actname = parts[door].open ? _( "Close" ) : _( "Open" );
            pmenu.addentry( val, true, MENU_AUTOASSIGN, "%s %s", actname, parts[ door ].name() );
        }
    }

    pmenu.addentry( doors_with_motors.size() + OPENCURTAINS, true, MENU_AUTOASSIGN,
                    _( "Open all curtains" ) );
    pmenu.addentry( doors_with_motors.size() + OPENBOTH, true, MENU_AUTOASSIGN,
                    _( "Open all curtains and doors" ) );
    pmenu.addentry( doors_with_motors.size() + CLOSEDOORS, true, MENU_AUTOASSIGN,
                    _( "Close all doors" ) );
    pmenu.addentry( doors_with_motors.size() + CLOSEBOTH, true, MENU_AUTOASSIGN,
                    _( "Close all curtains and doors" ) );

    pointmenu_cb callback( locations );
    pmenu.callback = &callback;
    pmenu.w_y = 0; // Move the menu so that we can see our vehicle
    pmenu.query();

    if( pmenu.ret >= 0 ) {
        if( pmenu.ret < static_cast<int>( doors_with_motors.size() ) ) {
            int part = doors_with_motors[pmenu.ret];
            open_or_close( part, !( parts[part].open ) );
        } else if( pmenu.ret < ( static_cast<int>( doors_with_motors.size() ) + CANCEL ) ) {
            int option = pmenu.ret - static_cast<int>( doors_with_motors.size() );
            bool open = option == OPENBOTH || option == OPENCURTAINS;
            for( const vpart_reference &vp : door_motors ) {
                const size_t motor = vp.part_index();
                int next_part = -1;
                if( open ) {
                    int part = next_part_to_open( motor );
                    if( part != -1 ) {
                        if( ! part_flag( part, "CURTAIN" ) &&  option == OPENCURTAINS ) {
                            continue;
                        }
                        open_or_close( part, open );
                        if( option == OPENBOTH ) {
                            next_part = next_part_to_open( motor );
                        }
                        if( next_part != -1 ) {
                            open_or_close( next_part, open );
                        }
                    }
                } else {
                    int part = next_part_to_close( motor );
                    if( part != -1 ) {
                        if( part_flag( part, "CURTAIN" ) &&  option == CLOSEDOORS ) {
                            continue;
                        }
                        open_or_close( part, open );
                        if( option == CLOSEBOTH ) {
                            next_part = next_part_to_close( motor );
                        }
                        if( next_part != -1 ) {
                            open_or_close( next_part, open );
                        }
                    }
                }
            }
        }
    }
}

void vehicle::set_electronics_menu_options( std::vector<uilist_entry> &options,
        std::vector<std::function<void()>> &actions )
{
    auto add_toggle = [&]( const std::string & name, char key, const std::string & flag ) {
        add_toggle_to_opts( options, actions, name, key, flag );
    };
    add_toggle( _( "reactor" ), keybind( "TOGGLE_REACTOR" ), "REACTOR" );
    add_toggle( _( "headlights" ), keybind( "TOGGLE_HEADLIGHT" ), "CONE_LIGHT" );
    add_toggle( _( "wide angle headlights" ), keybind( "TOGGLE_WIDE_HEADLIGHT" ), "WIDE_CONE_LIGHT" );
    add_toggle( _( "directed overhead lights" ), keybind( "TOGGLE_HALF_OVERHEAD_LIGHT" ),
                "HALF_CIRCLE_LIGHT" );
    add_toggle( _( "overhead lights" ), keybind( "TOGGLE_OVERHEAD_LIGHT" ), "CIRCLE_LIGHT" );
    add_toggle( _( "aisle lights" ), keybind( "TOGGLE_AISLE_LIGHT" ), "AISLE_LIGHT" );
    add_toggle( _( "dome lights" ), keybind( "TOGGLE_DOME_LIGHT" ), "DOME_LIGHT" );
    add_toggle( _( "atomic lights" ), keybind( "TOGGLE_ATOMIC_LIGHT" ), "ATOMIC_LIGHT" );
    add_toggle( _( "stereo" ), keybind( "TOGGLE_STEREO" ), "STEREO" );
    add_toggle( _( "chimes" ), keybind( "TOGGLE_CHIMES" ), "CHIMES" );
    add_toggle( _( "fridge" ), keybind( "TOGGLE_FRIDGE" ), "FRIDGE" );
    add_toggle( _( "freezer" ), keybind( "TOGGLE_FREEZER" ), "FREEZER" );
    add_toggle( _( "space heater" ), keybind( "TOGGLE_SPACE_HEATER" ), "SPACE_HEATER" );
    add_toggle( _( "cooler" ), keybind( "TOGGLE_COOLER" ), "COOLER" );
    add_toggle( _( "recharger" ), keybind( "TOGGLE_RECHARGER" ), "RECHARGE" );
    add_toggle( _( "plow" ), keybind( "TOGGLE_PLOW" ), "PLOW" );
    add_toggle( _( "reaper" ), keybind( "TOGGLE_REAPER" ), "REAPER" );
    add_toggle( _( "planter" ), keybind( "TOGGLE_PLANTER" ), "PLANTER" );
    add_toggle( _( "rockwheel" ), keybind( "TOGGLE_PLOW" ), "ROCKWHEEL" );
    add_toggle( _( "scoop" ), keybind( "TOGGLE_SCOOP" ), "SCOOP" );
    add_toggle( _( "water purifier" ), keybind( "TOGGLE_WATER_PURIFIER" ), "WATER_PURIFIER" );

    if( has_part( "DOOR_MOTOR" ) ) {
        options.emplace_back( _( "Toggle doors" ), keybind( "TOGGLE_DOORS" ) );
        actions.push_back( [&] { control_doors(); refresh(); } );
    }
    if( camera_on || ( has_part( "CAMERA" ) && has_part( "CAMERA_CONTROL" ) ) ) {
        options.emplace_back( camera_on ?
                              colorize( _( "Turn off camera system" ), c_pink ) :
                              _( "Turn on camera system" ),
                              keybind( "TOGGLE_CAMERA" ) );
        actions.push_back( [&] {
            if( camera_on )
            {
                camera_on = false;
                add_msg( _( "Camera system disabled" ) );
            } else if( fuel_left( fuel_type_battery, true ) )
            {
                camera_on = true;
                add_msg( _( "Camera system enabled" ) );
            } else
            {
                add_msg( _( "Camera system won't turn on" ) );
            }
            refresh();
        } );
    }
}

void vehicle::control_electronics()
{
    // exit early if you can't control the vehicle
    if( !interact_vehicle_locked() ) {
        return;
    }

    bool valid_option = false;
    do {
        std::vector<uilist_entry> options;
        std::vector<std::function<void()>> actions;

        set_electronics_menu_options( options, actions );

        uilist menu;
        menu.text = _( "Electronics controls" );
        menu.entries = options;
        menu.query();
        valid_option = menu.ret >= 0 && static_cast<size_t>( menu.ret ) < actions.size();
        if( valid_option ) {
            actions[menu.ret]();
        }
    } while( valid_option );
}

void vehicle::control_engines()
{
    int e_toggle = 0;
    bool dirty = false;
    //count active engines
    int active_mask = 0;
    int fuel_count = 0;
    int i = 0;
    for( int e : engines ) {
        if( is_part_on( e ) ) {
            active_mask |= 1 << i++;
        }
        fuel_count += part_info( e ).engine_fuel_opts().size();
    }

    const auto adjust_engine = [this]( int e_toggle ) {
        int i = 0;
        for( int e : engines ) {
            for( const itype_id &fuel : part_info( e ).engine_fuel_opts() ) {
                if( i == e_toggle ) {
                    if( parts[ e ].fuel_current() == fuel ) {
                        toggle_specific_part( e, !is_part_on( e ) );
                    } else {
                        parts[ e ].fuel_set( fuel );
                    }
                    return;
                }
                i += 1;
            }
        }
    };

    //show menu until user finishes
    do {
        e_toggle = select_engine();
        if( e_toggle < 0 || e_toggle >= fuel_count ) {
            break;
        }
        dirty = true;
        adjust_engine( e_toggle );
    } while( e_toggle >= 0 && e_toggle < fuel_count );

    if( !dirty ) {
        return;
    }

    bool engines_were_on = engine_on;
    int new_active_mask = 0;
    i = 0;
    for( int e : engines ) {
        engine_on |= is_part_on( e );
        new_active_mask |= 1 << i++;
    }

    // if current velocity greater than new configuration safe speed
    // drop down cruise velocity.
    int safe_vel = safe_velocity();
    if( velocity > safe_vel ) {
        cruise_velocity = safe_vel;
    }

    if( engines_were_on && !engine_on ) {
        add_msg( _( "You turn off the %s's engines to change their configurations." ), name );
    } else if( !g->u.controlling_vehicle ) {
        add_msg( _( "You change the %s's engine configuration." ), name );
        return;
    }

    if( engine_on ) {
        start_engines();
    }
}

int vehicle::select_engine()
{
    uilist tmenu;
    tmenu.text = _( "Toggle which?" );
    int i = 0;
    for( size_t x = 0; x < engines.size(); x++ ) {
        int e = engines[ x ];
        for( const itype_id &fuel_id : part_info( e ).engine_fuel_opts() ) {
            bool is_active = parts[ e ].enabled && parts[ e ].fuel_current() == fuel_id;
            bool is_available = parts[ e ].is_available() &&
                                ( is_perpetual_type( x ) || fuel_id == fuel_type_muscle ||
                                  fuel_left( fuel_id ) );
            tmenu.addentry( i++, is_available, -1, "[%s] %s %s",
                            is_active ? "x" : " ", parts[ e ].name(),
                            item::nname( fuel_id ) );
        }
    }
    tmenu.query();
    return tmenu.ret;
}

bool vehicle::interact_vehicle_locked()
{
    if( is_locked ) {
        const inventory &crafting_inv = g->u.crafting_inventory();
        add_msg( _( "You don't find any keys in the %s." ), name );
        if( crafting_inv.has_quality( quality_id( "SCREW" ) ) ) {
            if( query_yn( _( "You don't find any keys in the %s. Attempt to hotwire vehicle?" ),
                          name ) ) {
                ///\EFFECT_MECHANICS speeds up vehicle hotwiring
                int mechanics_skill = g->u.get_skill_level( skill_mechanics );
                int hotwire_time = 6000 / ( ( mechanics_skill > 0 ) ? mechanics_skill : 1 );
                //assign long activity
                g->u.assign_activity( activity_id( "ACT_HOTWIRE_CAR" ), hotwire_time, -1, INT_MIN, _( "Hotwire" ) );
                // use part 0 as the reference point
                point q = coord_translate( parts[0].mount );
                g->u.activity.values.push_back( global_pos3().x + q.x ); //[0]
                g->u.activity.values.push_back( global_pos3().y + q.y ); //[1]
                g->u.activity.values.push_back( g->u.get_skill_level( skill_mechanics ) ); //[2]
            } else {
                if( has_security_working() && query_yn( _( "Trigger the %s's Alarm?" ), name ) ) {
                    is_alarm_on = true;
                } else {
                    add_msg( _( "You leave the controls alone." ) );
                }
            }
        } else {
            add_msg( _( "You could use a screwdriver to hotwire it." ) );
        }
    }

    return !( is_locked );
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
    //controls and security must both be valid
    if( c >= 0 && s >= 0 ) {
        ///\EFFECT_MECHANICS reduces chance of damaging controls when smashing security system
        int skill = g->u.get_skill_level( skill_mechanics );
        int percent_controls = 70 / ( 1 + skill );
        int percent_alarm = ( skill + 3 ) * 10;
        int rand = rng( 1, 100 );

        if( percent_controls > rand ) {
            damage_direct( c, part_info( c ).durability / 4 );

            if( parts[ c ].removed || parts[ c ].is_broken() ) {
                g->u.controlling_vehicle = false;
                is_alarm_on = false;
                add_msg( _( "You destroy the controls..." ) );
            } else {
                add_msg( _( "You damage the controls." ) );
            }
        }
        if( percent_alarm > rand ) {
            damage_direct( s, part_info( s ).durability / 5 );
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

std::string vehicle::tracking_toggle_string()
{
    return tracking_on ? _( "Forget vehicle position" ) : _( "Remember vehicle position" );
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

void vehicle::use_controls( const tripoint &pos )
{
    std::vector<uilist_entry> options;
    std::vector<std::function<void()>> actions;

    bool remote = g->remoteveh() == this;
    bool has_electronic_controls = false;

    if( remote ) {
        options.emplace_back( _( "Stop controlling" ), keybind( "RELEASE_CONTROLS" ) );
        actions.push_back( [&] {
            g->u.controlling_vehicle = false;
            g->setremoteveh( nullptr );
            add_msg( _( "You stop controlling the vehicle." ) );
            refresh();
        } );

        has_electronic_controls = has_part( "CTRL_ELECTRONIC" ) || has_part( "REMOTE_CONTROLS" );

    } else if( veh_pointer_or_null( g->m.veh_at( pos ) ) == this ) {
        if( g->u.controlling_vehicle ) {
            options.emplace_back( _( "Let go of controls" ), keybind( "RELEASE_CONTROLS" ) );
            actions.push_back( [&] {
                g->u.controlling_vehicle = false;
                add_msg( _( "You let go of the controls." ) );
                refresh();
            } );
        }
        has_electronic_controls = !get_parts_at( pos, "CTRL_ELECTRONIC",
                                  part_status_flag::any ).empty();
    }

    if( get_parts_at( pos, "CONTROLS", part_status_flag::any ).empty() && !has_electronic_controls ) {
        add_msg( m_info, _( "No controls there" ) );
        return;
    }

    // exit early if you can't control the vehicle
    if( !interact_vehicle_locked() ) {
        return;
    }

    if( has_part( "ENGINE" ) ) {
        if( g->u.controlling_vehicle || ( remote && engine_on ) ) {
            options.emplace_back( _( "Stop driving" ), keybind( "TOGGLE_ENGINE" ) );
            actions.push_back( [&] {
                if( engine_on && has_engine_type_not( fuel_type_muscle, true ) )
                {
                    add_msg( _( "You turn the engine off and let go of the controls." ) );
                    sounds::sound( pos, 2, sounds::sound_t::movement,
                                   _( "the engine go silent" ) );
                } else
                {
                    add_msg( _( "You let go of the controls." ) );
                }

                for( size_t e = 0; e < engines.size(); ++e )
                {
                    if( is_engine_on( e ) ) {
                        if( sfx::has_variant_sound( "engine_stop", parts[ engines[ e ] ].info().get_id().str() ) ) {
                            sfx::play_variant_sound( "engine_stop", parts[ engines[ e ] ].info().get_id().str(),
                                                     parts[ engines[ e ] ].info().engine_noise_factor() );
                        } else if( is_engine_type( e, fuel_type_muscle ) ) {
                            sfx::play_variant_sound( "engine_stop", "muscle",
                                                     parts[ engines[ e ] ].info().engine_noise_factor() );
                        } else if( is_engine_type( e, fuel_type_wind ) ) {
                            sfx::play_variant_sound( "engine_stop", "wind",
                                                     parts[ engines[ e ] ].info().engine_noise_factor() );
                        } else if( is_engine_type( e, fuel_type_battery ) ) {
                            sfx::play_variant_sound( "engine_stop", "electric",
                                                     parts[ engines[ e ] ].info().engine_noise_factor() );
                        } else {
                            sfx::play_variant_sound( "engine_stop", "combustion",
                                                     parts[ engines[ e ] ].info().engine_noise_factor() );
                        }
                    }
                }
                vehicle_noise = 0;
                engine_on = false;
                g->u.controlling_vehicle = false;
                g->setremoteveh( nullptr );
                sfx::do_vehicle_engine_sfx();
                refresh();
            } );

        } else if( has_engine_type_not( fuel_type_muscle, true ) ) {
            options.emplace_back( engine_on ? _( "Turn off the engine" ) : _( "Turn on the engine" ),
                                  keybind( "TOGGLE_ENGINE" ) );
            actions.push_back( [&] {
                if( engine_on )
                {
                    vehicle_noise = 0;
                    engine_on = false;
                    add_msg( _( "You turn the engine off." ) );
                    sounds::sound( pos, 2, sounds::sound_t::movement,
                                   _( "the engine go silent" ) );
                    for( size_t e = 0; e < engines.size(); ++e ) {
                        if( is_engine_on( e ) ) {
                            if( sfx::has_variant_sound( "engine_stop", parts[ engines[ e ] ].info().get_id().str() ) ) {
                                sfx::play_variant_sound( "engine_stop", parts[ engines[ e ] ].info().get_id().str(),
                                                         parts[ engines[ e ] ].info().engine_noise_factor() );
                            } else if( is_engine_type( e, fuel_type_battery ) ) {
                                sfx::play_variant_sound( "engine_stop", "electric",
                                                         parts[ engines[ e ] ].info().engine_noise_factor() );
                            } else {
                                sfx::play_variant_sound( "engine_stop", "combustion",
                                                         parts[ engines[ e ] ].info().engine_noise_factor() );
                            }
                        }
                    }
                    sfx::do_vehicle_engine_sfx();
                } else
                {
                    start_engines();
                }
                refresh();
            } );
        }
    }

    if( has_part( "HORN" ) ) {
        options.emplace_back( _( "Honk horn" ), keybind( "SOUND_HORN" ) );
        actions.push_back( [&] { honk_horn(); refresh(); } );
    }

    options.emplace_back( cruise_on ? _( "Disable cruise control" ) : _( "Enable cruise control" ),
                          keybind( "TOGGLE_CRUISE_CONTROL" ) );
    actions.emplace_back( [&] {
        cruise_on = !cruise_on;
        add_msg( cruise_on ? _( "Cruise control turned on" ) : _( "Cruise control turned off" ) );
        refresh();
    } );

    if( has_electronic_controls ) {
        set_electronics_menu_options( options, actions );
        options.emplace_back( _( "Control multiple electronics" ), keybind( "CONTROL_MANY_ELECTRONICS" ) );
        actions.push_back( [&] { control_electronics(); refresh(); } );
    }

    options.emplace_back( tracking_on ? _( "Forget vehicle position" ) :
                          _( "Remember vehicle position" ),
                          keybind( "TOGGLE_TRACKING" ) );
    actions.push_back( [&] { toggle_tracking(); } );

    if( ( is_foldable() || tags.count( "convertible" ) ) && !remote ) {
        options.emplace_back( string_format( _( "Fold %s" ), name ), keybind( "FOLD_VEHICLE" ) );
        actions.push_back( [&] { fold_up(); } );
    }

    if( has_part( "ENGINE" ) ) {
        options.emplace_back( _( "Control individual engines" ), keybind( "CONTROL_ENGINES" ) );
        actions.push_back( [&] { control_engines(); refresh(); } );
    }

    if( is_alarm_on ) {
        if( velocity == 0 && !remote ) {
            options.emplace_back( _( "Try to disarm alarm." ), keybind( "TOGGLE_ALARM" ) );
            actions.push_back( [&] { smash_security_system(); refresh(); } );

        } else if( has_electronic_controls && has_part( "SECURITY" ) ) {
            options.emplace_back( _( "Trigger alarm" ), keybind( "TOGGLE_ALARM" ) );
            actions.push_back( [&] {
                is_alarm_on = true;
                add_msg( _( "You trigger the alarm" ) );
                refresh();
            } );
        }
    }

    if( has_part( "TURRET" ) ) {
        options.emplace_back( _( "Set turret targeting modes" ), keybind( "TURRET_TARGET_MODE" ) );
        actions.push_back( [&] { turrets_set_targeting(); refresh(); } );

        options.emplace_back( _( "Set turret firing modes" ), keybind( "TURRET_FIRE_MODE" ) );
        actions.push_back( [&] { turrets_set_mode(); refresh(); } );

        // We can also fire manual turrets with ACTION_FIRE while standing at the controls.
        options.emplace_back( _( "Aim turrets manually" ), keybind( "TURRET_MANUAL_AIM" ) );
        actions.push_back( [&] { turrets_aim_and_fire( true, false ); refresh(); } );

        // This lets us manually override and set the target for the automatic turrets instead.
        options.emplace_back( _( "Aim automatic turrets" ), keybind( "TURRET_MANUAL_OVERRIDE" ) );
        actions.push_back( [&] { turrets_aim( false, true ); refresh(); } );

        options.emplace_back( _( "Aim individual turret" ), keybind( "TURRET_SINGLE_FIRE" ) );
        actions.push_back( [&] { turrets_aim_single(); refresh(); } );
    }

    uilist menu;
    menu.text = _( "Vehicle controls" );
    menu.entries = options;
    menu.query();
    if( menu.ret >= 0 ) {
        // allow player to turn off engine without triggering another warning
        if( menu.ret != 0 && menu.ret != 1 && menu.ret != 2 && menu.ret != 3 ) {
            if( !handle_potential_theft( dynamic_cast<player &>( g->u ) ) ) {
                return;
            }
        }
        actions[menu.ret]();
        // Don't access `this` from here on, one of the actions above is to call
        // fold_up(), which may have deleted `this` object.
    }
}

bool vehicle::fold_up()
{
    const bool can_be_folded = is_foldable();
    const bool is_convertible = ( tags.count( "convertible" ) > 0 );
    if( !( can_be_folded || is_convertible ) ) {
        debugmsg( _( "Tried to fold non-folding vehicle %s" ), name );
        return false;
    }

    if( g->u.controlling_vehicle ) {
        add_msg( m_warning,
                 _( "As the pitiless metal bars close on your nether regions, you reconsider trying to fold the %s while riding it." ),
                 name );
        return false;
    }

    if( velocity > 0 ) {
        add_msg( m_warning, _( "You can't fold the %s while it's in motion." ), name );
        return false;
    }

    add_msg( _( "You painstakingly pack the %s into a portable configuration." ), name );

    if( g->u.get_grab_type() != OBJECT_NONE ) {
        g->u.grab( OBJECT_NONE );
        add_msg( _( "You let go of %s as you fold it." ), name );
    }

    std::string itype_id = "folding_bicycle";
    for( const auto &elem : tags ) {
        if( elem.compare( 0, 12, "convertible:" ) == 0 ) {
            itype_id = elem.substr( 12 );
            break;
        }
    }

    // create a folding [non]bicycle item
    item bicycle( can_be_folded ? "generic_folded_vehicle" : "folding_bicycle", calendar::turn );

    // Drop stuff in containers on ground
    for( const vpart_reference &vp : get_any_parts( "CARGO" ) ) {
        const size_t p = vp.part_index();
        for( auto &elem : get_items( p ) ) {
            g->m.add_item_or_charges( g->u.pos(), elem );
        }
        while( !get_items( p ).empty() ) {
            get_items( p ).erase( get_items( p ).begin() );
        }
    }

    unboard_all();

    // Store data of all parts, iuse::unfold_bicyle only loads
    // some of them, some are expect to be
    // vehicle specific and therefore constant (like id, mount).
    // Writing everything here is easier to manage, as only
    // iuse::unfold_bicyle has to adopt to changes.
    try {
        std::ostringstream veh_data;
        JsonOut json( veh_data );
        json.write( parts );
        bicycle.set_var( "folding_bicycle_parts", veh_data.str() );
    } catch( const JsonError &e ) {
        debugmsg( "Error storing vehicle: %s", e.c_str() );
    }

    if( can_be_folded ) {
        bicycle.set_var( "weight", to_gram( total_mass() ) );
        bicycle.set_var( "volume", total_folded_volume() / units::legacy_volume_factor );
        bicycle.set_var( "name", string_format( _( "folded %s" ), name ) );
        bicycle.set_var( "vehicle_name", name );
        // TODO: a better description?
        bicycle.set_var( "description", string_format( _( "A folded %s." ), name ) );
    }

    g->m.add_item_or_charges( g->u.pos(), bicycle );
    g->m.destroy_vehicle( this );

    // TODO: take longer to fold bigger vehicles
    // TODO: make this interruptible
    g->u.moves -= 500;
    return true;
}

double vehicle::engine_cold_factor( const int e ) const
{
    if( !part_info( engines[e] ).has_flag( "E_COLD_START" ) ) {
        return 0.0;
    }

    int eff_temp = g->weather.get_temperature( g->u.pos() );
    if( !parts[ engines[ e ] ].faults().count( fault_glowplug ) ) {
        eff_temp = std::min( eff_temp, 20 );
    }

    return 1.0 - ( std::max( 0, std::min( 30, eff_temp ) ) / 30.0 );
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
    const int cold = ( 1 / tanh( 1 - std::min( engine_cold_factor( e ), 0.9 ) ) ) * 100;

    // watts to old vhp = watts / 373
    // divided by magic 16 = watts / 6000
    const int watts_per_time = 6000;
    return part_vpower_w( engines[ e ], true ) / watts_per_time  + ( 100 * dmg ) + cold;
}

bool vehicle::start_engine( const int e )
{
    if( !is_engine_on( e ) ) {
        return false;
    }

    const vpart_info &einfo = part_info( engines[e] );
    vehicle_part &eng = parts[ engines[ e ] ];

    bool out_of_fuel = false;
    if( einfo.fuel_type != fuel_type_none && engine_fuel_left( e ) <= 0 ) {
        for( const itype_id &fuel_id : einfo.engine_fuel_opts() ) {
            if( fuel_left( fuel_id ) > 0 ) {
                eng.fuel_set( fuel_id );
                break;
            }
        }
        out_of_fuel = true;
    }

    if( out_of_fuel ) {
        if( einfo.fuel_type == fuel_type_muscle ) {
            // Muscle engines cannot start with broken limbs
            if( einfo.has_flag( "MUSCLE_ARMS" ) && ( g->u.hp_cur[hp_arm_l] == 0 ||
                    g->u.hp_cur[hp_arm_r] == 0 ) ) {
                add_msg( _( "You cannot use %s with a broken arm." ), eng.name() );
                return false;
            } else if( einfo.has_flag( "MUSCLE_LEGS" ) && ( g->u.hp_cur[hp_leg_l] == 0 ||
                       g->u.hp_cur[hp_leg_r] == 0 ) ) {
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
    const int engine_power = abs( part_epower_w( engines[e] ) );
    const double cold_factor = engine_cold_factor( e );
    const int start_moves = engine_start_time( e );

    const tripoint pos = global_part_pos3( engines[e] );
    if( einfo.engine_backfire_threshold() ) {
        if( ( 1 - dmg ) < einfo.engine_backfire_threshold() &&
            one_in( einfo.engine_backfire_freq() ) ) {
            backfire( e );
        } else {
            sounds::sound( pos, start_moves / 10, sounds::sound_t::movement,
                           string_format( _( "the %s bang as it starts" ), eng.name() ), true, "vehicle",
                           "engine_bangs_start" );
        }
    }

    // Immobilizers need removing before the vehicle can be started
    if( eng.faults().count( fault_immobiliser ) ) {
        sounds::sound( pos, 5, sounds::sound_t::alarm,
                       string_format( _( "the %s making a long beep" ), eng.name() ), true, "vehicle",
                       "fault_immobiliser_beep" );
        return false;
    }

    // Engine with starter motors can fail on both battery and starter motor
    if( eng.faults_potential().count( fault_starter ) ) {
        if( eng.faults().count( fault_starter ) ) {
            sounds::sound( pos, eng.info().engine_noise_factor(), sounds::sound_t::alarm,
                           string_format( _( "the %s clicking once" ), eng.name() ), true, "vehicle",
                           "engine_single_click_fail" );
            return false;
        }
        const int start_draw_bat = power_to_energy_bat( engine_power *
                                   ( 1.0 + dmg / 2 + cold_factor / 5 ) * 10,
                                   TICKS_TO_SECONDS( start_moves ) );
        if( discharge_battery( start_draw_bat, true ) != 0 ) {
            sounds::sound( pos, eng.info().engine_noise_factor(), sounds::sound_t::alarm,
                           string_format( _( "the %s rapidly clicking" ), eng.name() ), true, "vehicle",
                           "engine_multi_click_fail" );
            return false;
        }
    }

    // Engines always fail to start with faulty fuel pumps
    if( eng.faults().count( fault_pump ) || eng.faults().count( fault_diesel ) ) {
        sounds::sound( pos, eng.info().engine_noise_factor(), sounds::sound_t::movement,
                       string_format( _( "the %s quickly stuttering out." ), eng.name() ), true, "vehicle",
                       "engine_stutter_fail" );
        return false;
    }

    // Damaged non-electric engines have a chance of failing to start
    if( !is_engine_type( e, fuel_type_battery ) && x_in_y( dmg * 100, 120 ) ) {
        sounds::sound( pos, eng.info().engine_noise_factor(), sounds::sound_t::movement,
                       string_format( _( "the %s clanking and grinding" ), eng.name() ), true, "vehicle",
                       "engine_clanking_fail" );
        return false;
    }
    sounds::sound( pos, eng.info().engine_noise_factor(), sounds::sound_t::movement,
                   string_format( _( "the %s starting" ), eng.name() ) );

    if( sfx::has_variant_sound( "engine_start", eng.info().get_id().str() ) ) {
        sfx::play_variant_sound( "engine_start", eng.info().get_id().str(),
                                 eng.info().engine_noise_factor() );
    } else if( is_engine_type( e, fuel_type_muscle ) ) {
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

void vehicle::start_engines( const bool take_control )
{
    bool has_engine = std::any_of( engines.begin(), engines.end(), [&]( int idx ) {
        return parts[ idx ].enabled && !parts[ idx ].is_broken();
    } );

    // if no engines enabled then enable all before trying to start the vehicle
    if( !has_engine ) {
        for( auto idx : engines ) {
            if( !parts[ idx ].is_broken() ) {
                parts[ idx ].enabled = true;
            }
        }
    }

    int start_time = 0;
    // record the first usable engine as the referenced position checked at the end of the engine starting activity
    bool has_starting_engine_position = false;
    tripoint starting_engine_position;
    for( size_t e = 0; e < engines.size(); ++e ) {
        if( !has_starting_engine_position && !parts[ engines[ e ] ].is_broken() &&
            parts[ engines[ e ] ].enabled ) {
            starting_engine_position = global_part_pos3( engines[ e ] );
            has_starting_engine_position = true;
        }
        has_engine = has_engine || is_engine_on( e );
        start_time = std::max( start_time, engine_start_time( e ) );
    }

    if( !has_starting_engine_position ) {
        starting_engine_position = global_pos3();
    }

    if( !has_engine ) {
        add_msg( m_info, _( "The %s doesn't have an engine!" ), name );
        return;
    }

    if( take_control && !g->u.controlling_vehicle ) {
        g->u.controlling_vehicle = true;
        add_msg( _( "You take control of the %s." ), name );
    }

    g->u.assign_activity( activity_id( "ACT_START_ENGINES" ), start_time );
    g->u.activity.placement = starting_engine_position - g->u.pos();
    g->u.activity.values.push_back( take_control );
}

void vehicle::honk_horn()
{
    const bool no_power = ! fuel_left( fuel_type_battery, true );
    bool honked = false;

    for( const vpart_reference &vp : get_avail_parts( "HORN" ) ) {
        //Only bicycle horn doesn't need electricity to work
        const vpart_info &horn_type = vp.info();
        if( ( horn_type.get_id() != vpart_id( "horn_bicycle" ) ) && no_power ) {
            continue;
        }
        if( ! honked ) {
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

    if( ! honked ) {
        add_msg( _( "You honk the horn, but nothing happens." ) );
    }
}

void vehicle::reload_seeds( const tripoint &pos )
{
    player &p = g->u;

    std::vector<item *> seed_inv = p.items_with( []( const item & itm ) {
        return itm.is_seed();
    } );

    auto seed_entries = iexamine::get_seed_entries( seed_inv );
    seed_entries.emplace( seed_entries.begin(), seed_tuple( itype_id( "null" ), _( "No seed" ), 0 ) );

    int seed_index = iexamine::query_seed( seed_entries );

    if( seed_index > 0 && seed_index < static_cast<int>( seed_entries.size() ) ) {
        const int count = std::get<2>( seed_entries[seed_index] );
        int amount = 0;
        const std::string popupmsg = string_format( _( "Move how many? [Have %d] (0 to cancel)" ), count );

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
                used_seed = p.use_charges( seed_id, actual_amount );
            } else {
                used_seed = p.use_amount( seed_id, actual_amount );
            }
            used_seed.front().set_age( 0_turns );
            //place seeds into the planter
            put_into_vehicle_or_drop( p, item_drop_reason::deliberate, used_seed, pos );
        }
    }
}

void vehicle::beeper_sound()
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

void vehicle::play_music()
{
    for( const vpart_reference &vp : get_enabled_parts( "STEREO" ) ) {
        iuse::play_music( g->u, vp.pos(), 15, 30 );
    }
}

void vehicle::play_chimes()
{
    if( !one_in( 3 ) ) {
        return;
    }

    for( const vpart_reference &vp : get_enabled_parts( "CHIMES" ) ) {
        sounds::sound( vp.pos(), 40, sounds::sound_t::music,
                       _( "a simple melody blaring from the loudspeakers." ), false, "vehicle", "chimes" );
    }
}

void vehicle::transform_terrain()
{
    for( const vpart_reference &vp : get_enabled_parts( "TRANSFORM_TERRAIN" ) ) {
        const tripoint start_pos = vp.pos();
        const transform_terrain_data &ttd = vp.info().transform_terrain;
        bool prereq_fulfilled = false;
        for( const std::string &flag : ttd.pre_flags ) {
            if( g->m.has_flag( flag, start_pos ) ) {
                prereq_fulfilled = true;
                break;
            }
        }
        if( prereq_fulfilled ) {
            const ter_id new_ter = ter_id( ttd.post_terrain );
            if( new_ter != t_null ) {
                g->m.ter_set( start_pos, new_ter );
            }
            const furn_id new_furn = furn_id( ttd.post_furniture );
            if( new_furn != f_null ) {
                g->m.furn_set( start_pos, new_furn );
            }
            const field_type_id new_field = field_type_id( ttd.post_field );
            if( new_field.id() ) {
                g->m.add_field( start_pos, new_field, ttd.post_field_intensity, ttd.post_field_age );
            }
        } else {
            const int speed = abs( velocity );
            int v_damage = rng( 3, speed );
            damage( vp.part_index(), v_damage, DT_BASH, false );
            sounds::sound( start_pos, v_damage, sounds::sound_t::combat, _( "Clanggggg!" ), false,
                           "smash_success", "hit_vehicle" );
        }
    }
}

void vehicle::operate_reaper()
{
    for( const vpart_reference &vp : get_enabled_parts( "REAPER" ) ) {
        const size_t reaper_id = vp.part_index();
        const tripoint reaper_pos = vp.pos();
        const int plant_produced = rng( 1, vp.info().bonus );
        const int seed_produced = rng( 1, 3 );
        const units::volume max_pickup_volume = vp.info().size / 20;
        if( g->m.furn( reaper_pos ) != f_plant_harvest ) {
            continue;
        }
        // Can't use item_stack::only_item() since there might be fertilizer
        map_stack items = g->m.i_at( reaper_pos );
        map_stack::iterator seed = std::find_if( items.begin(), items.end(), []( const item & it ) {
            return it.is_seed();
        } );
        if( seed == items.end() || seed->typeId() == "fungal_seeds" ||
            seed->typeId() == "marloss_seed" ) {
            // Otherworldly plants, the earth-made reaper can not handle those.
            continue;
        }
        g->m.furn_set( reaper_pos, f_null );
        g->m.i_clear( reaper_pos );
        for( auto &i : iexamine::get_harvest_items(
                 *seed->type, plant_produced, seed_produced, false ) ) {
            g->m.add_item_or_charges( reaper_pos, i );
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
    for( const vpart_reference &vp : get_enabled_parts( "PLANTER" ) ) {
        const size_t planter_id = vp.part_index();
        const tripoint loc = vp.pos();
        vehicle_stack v = get_items( planter_id );
        for( auto i = v.begin(); i != v.end(); i++ ) {
            if( i->is_seed() ) {
                // If it is an "advanced model" then it will avoid damaging itself or becoming damaged. It's a real feature.
                if( g->m.ter( loc ) != t_dirtmound && vp.has_feature( "ADVANCED_PLANTER" ) ) {
                    //then don't put the item there.
                    break;
                } else if( g->m.ter( loc ) == t_dirtmound ) {
                    g->m.set( loc, t_dirt, f_plant_seed );
                } else if( !g->m.has_flag( "PLOWABLE", loc ) ) {
                    //If it isn't plowable terrain, then it will most likely be damaged.
                    damage( planter_id, rng( 1, 10 ), DT_BASH, false );
                    sounds::sound( loc, rng( 10, 20 ), sounds::sound_t::combat, _( "Clink" ), false, "smash_success",
                                   "hit_vehicle" );
                }
                if( !i->count_by_charges() || i->charges == 1 ) {
                    i->set_age( 0_turns );
                    g->m.add_item( loc, *i );
                    v.erase( i );
                } else {
                    item tmp = *i;
                    tmp.charges = 1;
                    tmp.set_age( 0_turns );
                    g->m.add_item( loc, tmp );
                    i->charges--;
                }
                break;
            }
        }
    }
}

void vehicle::operate_scoop()
{
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
             g->m.points_in_radius( global_part_pos3( scoop ), 1 ) ) {
            parts_points.push_back( current );
        }
        for( const tripoint &position : parts_points ) {
            g->m.mop_spills( position );
            if( !g->m.has_items( position ) ) {
                continue;
            }
            item *that_item_there = nullptr;
            map_stack items = g->m.i_at( position );
            if( g->m.has_flag( "SEALED", position ) ) {
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
                that_item_there->inc_damage( DT_BASH );
                //The scoop gets a lot louder when breaking an item.
                sounds::sound( position, rng( 10,
                                              that_item_there->volume() / units::legacy_volume_factor * 2 + 10 ),
                               sounds::sound_t::combat, _( "BEEEThump" ), false, "vehicle", "scoop_thump" );
            }
            //This attempts to add the item to the scoop inventory and if successful, removes it from the map.
            if( add_item( scoop, *that_item_there ) ) {
                g->m.i_rem( position, that_item_there );
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

bool vehicle::is_open( int part_index ) const
{
    return parts[part_index].open;
}

void vehicle::open_all_at( int p )
{
    std::vector<int> parts_here = parts_at_relative( parts[p].mount, true );
    for( auto &elem : parts_here ) {
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
    //find_lines_of_parts() doesn't return the part_index we passed, so we set it on it's own
    parts[part_index].open = opening;
    insides_dirty = true;
    g->m.set_transparency_cache_dirty( sm_pos.z );
    const int dist = rl_dist( g->u.pos(), mount_to_tripoint( parts[part_index].mount ) );
    if( dist < 20 ) {
        sfx::play_variant_sound( opening ? "vehicle_open" : "vehicle_close",
                                 parts[ part_index ].info().get_id().str(), 100 - dist * 3 );
    }
    for( auto const &vec : find_lines_of_parts( part_index, "OPENABLE" ) ) {
        for( auto const &partID : vec ) {
            parts[partID].open = opening;
        }
    }

    coeff_air_changed = true;
}

void vehicle::use_autoclave( int p )
{
    auto items = get_items( p );
    static const std::string filthy( "FILTHY" );
    bool filthy_items = std::any_of( items.begin(), items.end(), []( const item & i ) {
        return i.has_flag( filthy );
    } );

    bool cbms = std::all_of( items.begin(), items.end(), []( const item & i ) {
        return i.is_bionic();
    } );

    if( parts[p].enabled ) {
        parts[p].enabled = false;
        add_msg( m_bad,
                 _( "You turn the autoclave off before it's finished the program, and open its door." ) );
    } else if( fuel_left( "water" ) < 8 && fuel_left( "water_clean" ) < 8 ) {
        add_msg( m_bad, _( "You need 8 charges of water in tanks of the %s for the autoclave to run." ),
                 name );
    } else if( filthy_items ) {
        add_msg( m_bad,
                 _( "You need to remove all filthy items from the autoclave to start the sterilizing cycle." ) );
    } else if( !cbms ) {
        add_msg( m_bad,
                 _( "Only CBMs can be sterilized in an autoclave." ) );
    } else {
        parts[p].enabled = true;
        for( auto &n : items ) {
            n.set_age( 0_turns );
        }

        if( fuel_left( "water" ) >= 8 ) {
            drain( "water", 8 );
        } else {
            drain( "water_clean", 8 );
        }

        add_msg( m_good,
                 _( "You turn the autoclave on and it starts its cycle." ) );
    }
}

void vehicle::use_washing_machine( int p )
{
    bool detergent_is_enough = g->u.crafting_inventory().has_charges( "detergent", 5 );
    auto items = get_items( p );
    static const std::string filthy( "FILTHY" );
    bool filthy_items = std::all_of( items.begin(), items.end(), []( const item & i ) {
        return i.has_flag( filthy );
    } );

    bool cbms = std::any_of( items.begin(), items.end(), []( const item & i ) {
        return i.is_bionic();
    } );

    if( parts[p].enabled ) {
        parts[p].enabled = false;
        add_msg( m_bad,
                 _( "You turn the washing machine off before it's finished the program, and open its lid." ) );
    } else if( fuel_left( "water" ) < 24 && fuel_left( "water_clean" ) < 24 ) {
        add_msg( m_bad, _( "You need 24 charges of water in tanks of the %s to fill the washing machine." ),
                 name );
    } else if( !detergent_is_enough ) {
        add_msg( m_bad, _( "You need 5 charges of detergent for the washing machine." ) );
    } else if( !filthy_items ) {
        add_msg( m_bad,
                 _( "You need to remove all non-filthy items from the washing machine to start the washing program." ) );
    } else if( cbms ) {
        add_msg( m_bad,
                 _( "CBMs can't be cleaned in a washing machine.  You need to remove them." ) );
    } else {
        parts[p].enabled = true;
        for( auto &n : items ) {
            n.set_age( 0_turns );
        }

        if( fuel_left( "water" ) >= 24 ) {
            drain( "water", 24 );
        } else {
            drain( "water_clean", 24 );
        }

        std::vector<item_comp> detergent;
        detergent.push_back( item_comp( "detergent", 5 ) );
        g->u.consume_items( detergent, 1, is_crafting_component );

        add_msg( m_good,
                 _( "You pour some detergent into the washing machine, close its lid, and turn it on.  The washing machine is being filled with water from vehicle tanks." ) );
    }
}

void vehicle::use_dishwasher( int p )
{
    bool detergent_is_enough = g->u.crafting_inventory().has_charges( "detergent", 5 );
    auto items = get_items( p );
    static const std::string filthy( "FILTHY" );
    bool filthy_items = std::all_of( items.begin(), items.end(), []( const item & i ) {
        return i.has_flag( filthy );
    } );

    std::ostringstream buffer;
    buffer << _( "Soft items can't be cleaned in a dishwasher, you should use a washing machine for that.  You need to remove them:" );
    bool soft_items = false;
    for( const item &it : items ) {
        if( it.is_soft() ) {
            soft_items = true;
            buffer << " " << it.tname();
        }
    }

    if( parts[p].enabled ) {
        parts[p].enabled = false;
        add_msg( m_bad,
                 _( "You turn the dishwasher off before it's finished the program, and open its lid." ) );
    } else if( fuel_left( "water" ) < 24 && fuel_left( "water_clean" ) < 24 ) {
        add_msg( m_bad, _( "You need 24 charges of water in tanks of the %s to fill the dishwasher." ),
                 name );
    } else if( !detergent_is_enough ) {
        add_msg( m_bad, _( "You need 5 charges of detergent for the dishwasher." ) );
    } else if( !filthy_items ) {
        add_msg( m_bad,
                 _( "You need to remove all non-filthy items from the dishwasher to start the washing program." ) );
    } else if( soft_items ) {
        add_msg( m_bad, buffer.str() );
    } else {
        parts[p].enabled = true;
        for( auto &n : items ) {
            n.set_age( 0_turns );
        }

        if( fuel_left( "water" ) >= 24 ) {
            drain( "water", 24 );
        } else {
            drain( "water_clean", 24 );
        }

        std::vector<item_comp> detergent;
        detergent.push_back( item_comp( "detergent", 5 ) );
        g->u.consume_items( detergent, 1, is_crafting_component );

        add_msg( m_good,
                 _( "You pour some detergent into the dishwasher, close its lid, and turn it on.  The dishwasher is being filled with water from vehicle tanks." ) );
    }
}

void vehicle::use_monster_capture( int part, const tripoint &pos )
{
    if( parts[part].is_broken() || parts[part].removed ) {
        return;
    }
    item base = item( parts[part].get_base() );
    base.type->invoke( g->u, base, pos );
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
        add_msg( "is unavailable" );
        return;
    }
    const cata::optional<tripoint> target_ = choose_adjacent(
                _( "Where is the creature to harness?" ) );
    const tripoint target = *target_;
    monster *mon_ptr = g->critter_at<monster>( target );
    if( mon_ptr != nullptr ) {
        monster &f = *mon_ptr;
        if( f.friendly != 0 && f.has_flag( MF_PET_MOUNTABLE ) && g->is_empty( pos ) ) {
            f.add_effect( effect_harnessed, 1_turns, num_bp, true );
            f.setpos( pos );
            add_msg( m_info, _( "You harness your %s to the %s." ), f.get_name(), name );
            if( f.has_effect( effect_tied ) ) {
                add_msg( m_info, _( "You untie your %s." ), f.get_name() );
                f.remove_effect( effect_tied );
                if( f.tied_item ) {
                    g->u.i_add( *f.tied_item );
                    f.tied_item = cata::nullopt;
                }
            }
        } else if( f.friendly == 0 ) {
            add_msg( m_info, _( "This creature is not friendly!" ) );
        } else if( !f.has_flag( MF_PET_MOUNTABLE ) ) {
            add_msg( m_info, _( "This creature cannot be harnessed." ) );
        } else if( !g->is_empty( pos ) ) {
            add_msg( m_info, _( "The harness is blocked." ) );
        }
    } else {
        add_msg( m_info, _( "No creature there." ) );
    }
}

void vehicle::use_bike_rack( int part )
{
    if( parts[part].is_unavailable() || parts[part].removed ) {
        return;
    }
    std::vector<std::vector <int>> racks_parts = find_lines_of_parts( part, "BIKE_RACK_VEH" );
    if( racks_parts.empty() ) {
        return;
    }

    // check if we're storing a vehicle on this rack
    std::vector<int> carried_parts;
    std::vector<int> carry_rack;
    bool found_vehicle = false;
    for( const auto &rack_parts : racks_parts ) {
        for( auto rack_part : rack_parts ) {
            // skip parts that aren't carrying anything
            if( !parts[ rack_part ].has_flag( vehicle_part::carrying_flag ) ) {
                continue;
            }
            for( int i = 0; i < 4; i++ ) {
                point near_loc = parts[ rack_part ].mount + vehicles::cardinal_d[ i ];
                std::vector<int> near_parts = parts_at_relative( near_loc, true );
                if( near_parts.empty() ) {
                    continue;
                }
                if( parts[ near_parts[ 0 ] ].has_flag( vehicle_part::carried_flag ) ) {
                    found_vehicle = true;
                    // found a carried vehicle part
                    for( auto carried_part : near_parts ) {
                        carried_parts.push_back( carried_part );
                    }
                    carry_rack.push_back( rack_part );
                    // we're not adjacent to another carried vehicle on this rack
                    break;
                }
            }
        }
        if( found_vehicle ) {
            break;
        }
    }
    bool success = false;
    if( found_vehicle ) {
        success = remove_carried_vehicle( carried_parts );
        if( success ) {
            for( auto rack_part : carry_rack ) {
                parts[ rack_part ].remove_flag( vehicle_part::carrying_flag );
            }
        }
    } else {
        success = try_to_rack_nearby_vehicle( racks_parts );
    }
    if( success ) {
        g->m.invalidate_map_cache( g->get_levz() );
        g->refresh_all();
    }
}

// Handles interactions with a vehicle in the examine menu.
void vehicle::interact_with( const tripoint &pos, int interact_part )
{
    std::vector<std::string> menu_items;
    std::vector<uilist_entry> options_message;
    const bool has_items_on_ground = g->m.sees_some_items( pos, g->u );
    const bool items_are_sealed = g->m.has_flag( "SEALED", pos );

    auto turret = turret_query( pos );

    const bool has_kitchen = avail_part_with_feature( interact_part, "KITCHEN", true ) >= 0;
    const bool has_faucet = avail_part_with_feature( interact_part, "FAUCET", true ) >= 0;
    const bool has_towel = avail_part_with_feature( interact_part, "TOWEL", true ) >= 0;
    const bool has_weldrig = avail_part_with_feature( interact_part, "WELDRIG", true ) >= 0;
    const bool has_chemlab = avail_part_with_feature( interact_part, "CHEMLAB", true ) >= 0;
    const bool has_purify = avail_part_with_feature( interact_part, "WATER_PURIFIER", true ) >= 0;
    const bool has_controls = avail_part_with_feature( interact_part, "CONTROLS", true ) >= 0;
    const bool has_electronics = avail_part_with_feature( interact_part, "CTRL_ELECTRONIC", true ) >= 0;
    const int cargo_part = part_with_feature( interact_part, "CARGO", false );
    const bool from_vehicle = cargo_part >= 0 && !get_items( cargo_part ).empty();
    const bool can_be_folded = is_foldable();
    const bool is_convertible = tags.count( "convertible" ) > 0;
    const bool remotely_controlled = g->remoteveh() == this;
    const int autoclave_part = avail_part_with_feature( interact_part, "AUTOCLAVE", true );
    const bool has_autoclave = autoclave_part >= 0;
    bool autoclave_on = ( autoclave_part == -1 ) ? false :
                        parts[autoclave_part].enabled;
    const int washing_machine_part = avail_part_with_feature( interact_part, "WASHING_MACHINE", true );
    const bool has_washmachine = washing_machine_part >= 0;
    bool washing_machine_on = ( washing_machine_part == -1 ) ? false :
                              parts[washing_machine_part].enabled;
    const int dishwasher_part = avail_part_with_feature( interact_part, "DISHWASHER", true );
    const bool has_dishwasher = dishwasher_part >= 0;
    bool dishwasher_on = ( dishwasher_part == -1 ) ? false :
                         parts[dishwasher_part].enabled;
    const int monster_capture_part = avail_part_with_feature( interact_part, "CAPTURE_MONSTER_VEH",
                                     true );
    const bool has_monster_capture = monster_capture_part >= 0;
    const int bike_rack_part = avail_part_with_feature( interact_part, "BIKE_RACK_VEH", true );
    const int harness_part = avail_part_with_feature( interact_part, "ANIMAL_CTRL", true );
    const bool has_harness = harness_part >= 0;
    const bool has_bike_rack = bike_rack_part >= 0;
    const bool has_planter = avail_part_with_feature( interact_part, "PLANTER", true ) >= 0 ||
                             avail_part_with_feature( interact_part, "ADVANCED_PLANTER", true ) >= 0;
    const int workbench_part = avail_part_with_feature( interact_part, "WORKBENCH", true );
    const bool has_workbench = workbench_part >= 0;

    enum {
        EXAMINE, TRACK, CONTROL, CONTROL_ELECTRONICS, GET_ITEMS, GET_ITEMS_ON_GROUND, FOLD_VEHICLE, UNLOAD_TURRET, RELOAD_TURRET,
        USE_HOTPLATE, FILL_CONTAINER, DRINK, USE_WELDER, USE_PURIFIER, PURIFY_TANK, USE_AUTOCLAVE, USE_WASHMACHINE, USE_DISHWASHER,
        USE_MONSTER_CAPTURE, USE_BIKE_RACK, USE_HARNESS, RELOAD_PLANTER, WORKBENCH, USE_TOWEL,
    };
    uilist selectmenu;

    selectmenu.addentry( EXAMINE, true, 'e', _( "Examine vehicle" ) );
    selectmenu.addentry( TRACK, true, keybind( "TOGGLE_TRACKING" ), tracking_toggle_string() );
    if( has_controls ) {
        selectmenu.addentry( CONTROL, true, 'v', _( "Control vehicle" ) );
    }
    if( has_electronics ) {
        selectmenu.addentry( CONTROL_ELECTRONICS, true, keybind( "CONTROL_MANY_ELECTRONICS" ),
                             _( "Control multiple electronics" ) );
    }
    if( has_autoclave ) {
        selectmenu.addentry( USE_AUTOCLAVE, true, 'a',
                             autoclave_on ? _( "Deactivate the autoclave" ) :
                             _( "Activate the autoclave (1.5 hours)" ) );
    }
    if( has_washmachine ) {
        selectmenu.addentry( USE_WASHMACHINE, true, 'W',
                             washing_machine_on ? _( "Deactivate the washing machine" ) :
                             _( "Activate the washing machine (1.5 hours)" ) );
    }
    if( has_dishwasher ) {
        selectmenu.addentry( USE_DISHWASHER, true, 'D',
                             dishwasher_on ? _( "Deactivate the dishwasher" ) :
                             _( "Activate the dishwasher (1.5 hours)" ) );
    }
    if( from_vehicle && !washing_machine_on && !dishwasher_on ) {
        selectmenu.addentry( GET_ITEMS, true, 'g', _( "Get items" ) );
    }
    if( has_items_on_ground && !items_are_sealed ) {
        selectmenu.addentry( GET_ITEMS_ON_GROUND, true, 'i', _( "Get items on the ground" ) );
    }
    if( ( can_be_folded || is_convertible ) && !remotely_controlled ) {
        selectmenu.addentry( FOLD_VEHICLE, true, 'f', _( "Fold vehicle" ) );
    }
    if( turret.can_unload() ) {
        selectmenu.addentry( UNLOAD_TURRET, true, 'u', _( "Unload %s" ), turret.name() );
    }
    if( turret.can_reload() ) {
        selectmenu.addentry( RELOAD_TURRET, true, 'r', _( "Reload %s" ), turret.name() );
    }
    if( ( has_kitchen || has_chemlab ) && fuel_left( "battery" ) > 0 ) {
        selectmenu.addentry( USE_HOTPLATE, true, 'h', _( "Use the hotplate" ) );
    }
    if( has_faucet && fuel_left( "water_clean" ) > 0 ) {
        selectmenu.addentry( FILL_CONTAINER, true, 'c', _( "Fill a container with water" ) );
        selectmenu.addentry( DRINK, true, 'd', _( "Have a drink" ) );
    }
    if( has_towel ) {
        selectmenu.addentry( USE_TOWEL, true, 't', _( "Use a towel" ) );
    }
    if( has_weldrig && fuel_left( "battery" ) > 0 ) {
        selectmenu.addentry( USE_WELDER, true, 'w', _( "Use the welding rig?" ) );
    }
    if( has_purify ) {
        bool can_purify = fuel_left( "battery" ) >= item::find_type( "water_purifier" )->charges_to_use();
        selectmenu.addentry( USE_PURIFIER, can_purify,
                             'p', _( "Purify water in carried container" ) );
        selectmenu.addentry( PURIFY_TANK, can_purify && fuel_left( "water" ),
                             'P', _( "Purify water in vehicle tank" ) );
    }
    if( has_monster_capture ) {
        selectmenu.addentry( USE_MONSTER_CAPTURE, true, 'G', _( "Capture or release a creature" ) );
    }
    if( has_bike_rack ) {
        selectmenu.addentry( USE_BIKE_RACK, true, 'R', _( "Load or unload a vehicle" ) );
    }
    if( has_harness ) {
        selectmenu.addentry( USE_HARNESS, true, 'H', _( "Harness an animal" ) );
    }
    if( has_planter ) {
        selectmenu.addentry( RELOAD_PLANTER, true, 's', _( "Reload seed drill with seeds" ) );
    }
    if( has_workbench ) {
        selectmenu.addentry( WORKBENCH, true, '&', string_format( _( "Craft at the %s" ),
                             parts[workbench_part].name() ) );
    }

    int choice;
    if( selectmenu.entries.size() == 1 ) {
        choice = selectmenu.entries.front().retval;
    } else {
        selectmenu.text = _( "Select an action" );
        selectmenu.query();
        choice = selectmenu.ret;
    }
    if( choice != EXAMINE && choice != TRACK && choice != GET_ITEMS_ON_GROUND ) {
        if( !handle_potential_theft( dynamic_cast<player &>( g->u ) ) ) {
            return;
        }
    }
    auto veh_tool = [&]( const itype_id & obj ) {
        item pseudo( obj );
        if( fuel_left( "battery" ) < pseudo.ammo_required() ) {
            return false;
        }
        auto capacity = pseudo.ammo_capacity( true );
        auto qty = capacity - discharge_battery( capacity );
        pseudo.ammo_set( "battery", qty );
        g->u.invoke_item( &pseudo );
        charge_battery( pseudo.ammo_remaining() );
        return true;
    };

    switch( choice ) {
        case USE_BIKE_RACK: {
            use_bike_rack( bike_rack_part );
            return;
        }
        case USE_HARNESS: {
            use_harness( harness_part, pos );
            return;
        }
        case USE_MONSTER_CAPTURE: {
            use_monster_capture( monster_capture_part, pos );
            return;
        }
        case USE_HOTPLATE: {
            veh_tool( "hotplate" );
            return;
        }
        case USE_TOWEL: {
            iuse::towel_common( &g->u, nullptr, false );
            return;
        }
        case USE_AUTOCLAVE: {
            use_autoclave( autoclave_part );
            return;
        }
        case USE_WASHMACHINE: {
            use_washing_machine( washing_machine_part );
            return;
        }
        case USE_DISHWASHER: {
            use_dishwasher( dishwasher_part );
            return;
        }
        case FILL_CONTAINER: {
            g->u.siphon( *this, "water_clean" );
            return;
        }
        case DRINK: {
            item water( "water_clean", 0 );
            if( g->u.eat( water ) ) {
                drain( "water_clean", 1 );
                g->u.moves -= 250;
            }
            return;
        }
        case USE_WELDER: {
            if( veh_tool( "welder" ) ) {
                // Evil hack incoming
                auto &act = g->u.activity;
                if( act.id() == activity_id( "ACT_REPAIR_ITEM" ) ) {
                    // Magic: first tell activity the item doesn't really exist
                    act.index = INT_MIN;
                    // Then tell it to search it on `pos`
                    act.coords.push_back( pos );
                    // Finally tell if it is the vehicle part with welding rig
                    act.values.resize( 2 );
                    act.values[1] = part_with_feature( interact_part, "WELDRIG", true );
                }
            }
            return;
        }
        case USE_PURIFIER: {
            veh_tool( "water_purifier" );
            return;
        }
        case PURIFY_TANK: {
            auto sel = []( const vehicle_part & pt ) {
                return pt.is_tank() && pt.ammo_current() == "water";
            };
            auto title = string_format( _( "Purify <color_%s>water</color> in tank" ),
                                        get_all_colors().get_name( item::find_type( "water" )->color ) );
            auto &tank = veh_interact::select_part( *this, sel, title );
            if( tank ) {
                double cost = item::find_type( "water_purifier" )->charges_to_use();
                if( fuel_left( "battery" ) < tank.ammo_remaining() * cost ) {
                    //~ $1 - vehicle name, $2 - part name
                    add_msg( m_bad, _( "Insufficient power to purify the contents of the %1$s's %2$s" ),
                             name, tank.name() );
                } else {
                    //~ $1 - vehicle name, $2 - part name
                    add_msg( m_good, _( "You purify the contents of the %1$s's %2$s" ), name, tank.name() );
                    discharge_battery( tank.ammo_remaining() * cost );
                    tank.ammo_set( "water_clean", tank.ammo_remaining() );
                }
            }
            return;
        }
        case UNLOAD_TURRET: {
            g->unload( *turret.base() );
            return;
        }
        case RELOAD_TURRET: {
            item::reload_option opt = g->u.select_ammo( *turret.base(), true );
            if( opt ) {
                g->u.assign_activity( activity_id( "ACT_RELOAD" ), opt.moves(), opt.qty() );
                g->u.activity.targets.emplace_back( turret.base() );
                g->u.activity.targets.push_back( std::move( opt.ammo ) );
            }
            return;
        }
        case FOLD_VEHICLE: {
            fold_up();
            return;
        }
        case CONTROL: {
            use_controls( pos );
            return;
        }
        case CONTROL_ELECTRONICS: {
            control_electronics();
            return;
        }
        case EXAMINE: {
            g->exam_vehicle( *this );
            return;
        }
        case TRACK: {
            toggle_tracking( );
            return;
        }
        case GET_ITEMS_ON_GROUND: {
            Pickup::pick_up( pos, 0, Pickup::from_ground );
            return;
        }
        case GET_ITEMS: {
            if( from_vehicle ) {
                Pickup::pick_up( pos, 0, Pickup::from_cargo );
            } else {
                Pickup::pick_up( pos, 0, Pickup::from_ground );
            }
            return;
        }
        case RELOAD_PLANTER: {
            reload_seeds( pos );
            return;
        }
        case WORKBENCH: {
            iexamine::workbench_internal( g->u, pos, vpart_reference( *this, workbench_part ) );
            return;
        }
    }
    return;
}
