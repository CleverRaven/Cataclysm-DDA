#include "vehicle.h"

#include "coordinate_conversions.h"
#include "map.h"
#include "output.h"
#include "game.h"
#include "item.h"
#include "veh_interact.h"
#include "cursesdef.h"
#include "catacharset.h"
#include "overmapbuffer.h"
#include "messages.h"
#include "iexamine.h"
#include "vpart_range.h"
#include "vpart_position.h"
#include "vpart_reference.h"
#include "string_formatter.h"
#include "ui.h"
#include "debug.h"
#include "sounds.h"
#include "translations.h"
#include "ammo.h"
#include "options.h"
#include "monster.h"
#include "npc.h"
#include "veh_type.h"
#include "itype.h"
#include "submap.h"
#include "mapdata.h"
#include "mtype.h"
#include "json.h"
#include "map_iterator.h"
#include "vehicle_selector.h"
#include "cata_utility.h"

#include <sstream>
#include <stdlib.h>
#include <set>
#include <queue>
#include <math.h>
#include <array>
#include <numeric>
#include <algorithm>
#include <cassert>

static const itype_id fuel_type_none( "null" );
static const itype_id fuel_type_gasoline( "gasoline" );
static const itype_id fuel_type_diesel( "diesel" );
static const itype_id fuel_type_battery( "battery" );
static const itype_id fuel_type_water( "water_clean" );
static const itype_id fuel_type_muscle( "muscle" );

static const fault_id fault_belt( "fault_engine_belt_drive" );
static const fault_id fault_diesel( "fault_engine_pump_diesel" );
static const fault_id fault_glowplug( "fault_engine_glow_plug" );
static const fault_id fault_immobiliser( "fault_engine_immobiliser" );
static const fault_id fault_pump( "fault_engine_pump_fuel" );
static const fault_id fault_starter( "fault_engine_starter" );
static const fault_id fault_filter_air( "fault_engine_filter_air" );
static const fault_id fault_filter_fuel( "fault_engine_filter_fuel" );

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
    auto const keys = input_context( context ).keys_bound_to( opt );
    return keys.empty() ? ' ' : keys.front();
}

void vehicle::add_toggle_to_opts( std::vector<uimenu_entry> &options,
                                  std::vector<std::function<void()>> &actions, const std::string &name, char key,
                                  const std::string &flag )
{
    // fetch matching parts and abort early if none found
    auto found = get_parts( flag );
    if( found.empty() ) {
        return;
    }

    // can this menu option be selected by the user?
    bool allow = true;

    // determine target state - currently parts of similar type are all switched concurrently
    bool state = std::none_of( found.begin(), found.end(), []( const vehicle_part * e ) {
        return e->enabled;
    } );

    // if toggled part potentially usable check if could be enabled now (sufficient fuel etc.)
    if( state ) {
        allow = std::any_of( found.begin(), found.end(), [&]( const vehicle_part * e ) {
            return can_enable( *e );
        } );
    }

    auto msg = string_format( state ? _( "Turn on %s" ) : _( "Turn off %s" ), name.c_str() );
    options.emplace_back( -1, allow, key, msg );

    actions.push_back( [ = ] {
        for( vehicle_part *e : found )
        {
            if( e->enabled != state ) {
                add_msg( state ? _( "Turned on %s" ) : _( "Turned off %s." ), e->name().c_str() );
                e->enabled = state;
            }
        }
        refresh();
    } );
}

void vehicle::control_doors()
{
    const auto door_motors = parts_with_feature( "DOOR_MOTOR", true );
    std::vector< int > doors_with_motors; // Indices of doors
    std::vector< tripoint > locations; // Locations used to display the doors
    // it is possible to have one door to open and one to close for single motor
    if( empty( door_motors ) ) {
        debugmsg( "vehicle::control_doors called but no door motors found" );
        return;
    }

    uimenu pmenu;
    pmenu.title = _( "Select door to toggle" );
    for( const vpart_reference vp : door_motors ) {
        const size_t p = vp.part_index();
        if( parts[ p ].is_unavailable() ) {
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
            pmenu.addentry( val, true, MENU_AUTOASSIGN, "%s %s", actname, parts[ door ].name().c_str() );
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

    pmenu.addentry( doors_with_motors.size() + CANCEL, true, 'q', _( "Cancel" ) );
    pointmenu_cb callback( locations );
    pmenu.callback = &callback;
    pmenu.w_y = 0; // Move the menu so that we can see our vehicle
    pmenu.query();

    if( pmenu.ret >= 0 ) {
        if( pmenu.ret < ( int )doors_with_motors.size() ) {
            int part = doors_with_motors[pmenu.ret];
            open_or_close( part, !( parts[part].open ) );
        } else if( pmenu.ret < ( ( int )doors_with_motors.size() + CANCEL ) ) {
            int option = pmenu.ret - ( int )doors_with_motors.size();
            bool open = option == OPENBOTH || option == OPENCURTAINS;
            for( const vpart_reference vp : door_motors ) {
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

void vehicle::set_electronics_menu_options( std::vector<uimenu_entry> &options,
        std::vector<std::function<void()>> &actions )
{
    auto add_toggle = [&]( const std::string & name, char key, const std::string & flag ) {
        add_toggle_to_opts( options, actions, name, key, flag );
    };
    add_toggle( _( "reactor" ), keybind( "TOGGLE_REACTOR" ), "REACTOR" );
    add_toggle( _( "headlights" ), keybind( "TOGGLE_HEADLIGHT" ), "CONE_LIGHT" );
    add_toggle( _( "overhead lights" ), keybind( "TOGGLE_OVERHEAD_LIGHT" ), "CIRCLE_LIGHT" );
    add_toggle( _( "aisle lights" ), keybind( "TOGGLE_AISLE_LIGHT" ), "AISLE_LIGHT" );
    add_toggle( _( "dome lights" ), keybind( "TOGGLE_DOME_LIGHT" ), "DOME_LIGHT" );
    add_toggle( _( "atomic lights" ), keybind( "TOGGLE_ATOMIC_LIGHT" ), "ATOMIC_LIGHT" );
    add_toggle( _( "stereo" ), keybind( "TOGGLE_STEREO" ), "STEREO" );
    add_toggle( _( "chimes" ), keybind( "TOGGLE_CHIMES" ), "CHIMES" );
    add_toggle( _( "fridge" ), keybind( "TOGGLE_FRIDGE" ), "FRIDGE" );
    add_toggle( _( "freezer" ), keybind( "TOGGLE_FEEZER" ), "FREEZER" );
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
        options.emplace_back( camera_on ?  _( "Turn off camera system" ) : _( "Turn on camera system" ),
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
        std::vector<uimenu_entry> options;
        std::vector<std::function<void()>> actions;

        set_electronics_menu_options( options, actions );

        options.emplace_back( _( "Quit controlling electronics" ), keybind( "QUIT" ) );

        uimenu menu;
        menu.return_invalid = true;
        menu.text = _( "Electronics controls" );
        menu.entries = options;
        menu.query();
        valid_option = menu.ret >= 0 && menu.ret < ( int )actions.size();
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
    int active_count = 0;
    for( size_t e = 0; e < engines.size(); ++e ) {
        if( is_part_on( engines[e] ) ) {
            active_count++;
        }
    }

    //show menu until user finishes
    while( e_toggle >= 0 && e_toggle < ( int )engines.size() ) {
        e_toggle = select_engine();
        if( e_toggle >= 0 && e_toggle < ( int )engines.size() &&
            ( active_count > 1 || !is_part_on( engines[e_toggle] ) ) ) {
            active_count += ( !is_part_on( engines[e_toggle] ) ) ? 1 : -1;
            toggle_specific_engine( e_toggle, !is_part_on( engines[e_toggle] ) );
            dirty = true;
        }
    }

    if( !dirty ) {
        return;
    }

    // if current velocity greater than new configuration safe speed
    // drop down cruise velocity.
    int safe_vel = safe_velocity();
    if( velocity > safe_vel ) {
        cruise_velocity = safe_vel;
    }

    if( engine_on ) {
        add_msg( _( "You turn off the %s's engines to change their configurations." ), name.c_str() );
        engine_on = false;
    } else if( !g->u.controlling_vehicle ) {
        add_msg( _( "You change the %s's engine configuration." ), name.c_str() );
        return;
    }

    start_engines();
}

int vehicle::select_engine()
{
    uimenu tmenu;
    std::string name;
    tmenu.text = _( "Toggle which?" );
    for( size_t e = 0; e < engines.size(); ++e ) {
        if( parts[ engines[ e] ].is_unavailable() ) {
            continue;
        }
        name = parts[ engines[ e ] ].name();
        tmenu.addentry( e, true, -1, "[%s] %s",
                        ( ( parts[engines[e]].enabled ) ? "x" : " " ), name.c_str() );
    }

    tmenu.addentry( -1, true, 'q', _( "Finish" ) );
    tmenu.query();
    return tmenu.ret;
}

bool vehicle::interact_vehicle_locked()
{
    if( is_locked ) {
        const inventory &crafting_inv = g->u.crafting_inventory();
        add_msg( _( "You don't find any keys in the %s." ), name.c_str() );
        if( crafting_inv.has_quality( quality_id( "SCREW" ) ) ) {
            if( query_yn( _( "You don't find any keys in the %s. Attempt to hotwire vehicle?" ),
                          name.c_str() ) ) {
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
                if( has_security_working() && query_yn( _( "Trigger the %s's Alarm?" ), name.c_str() ) ) {
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
    for( size_t d = 0; d < speciality.size(); d++ ) {
        int p = speciality[d];
        if( part_flag( p, "SECURITY" ) && !parts[ p ].is_broken() ) {
            s = p;
            c = part_with_feature( s, "CONTROLS" );
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
    std::vector<uimenu_entry> options;
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
        has_electronic_controls = !get_parts( pos, "CTRL_ELECTRONIC" ).empty();
    }

    if( get_parts( pos, "CONTROLS" ).empty() && !has_electronic_controls ) {
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
                } else
                {
                    add_msg( _( "You let go of the controls." ) );
                }
                engine_on = false;
                g->u.controlling_vehicle = false;
                g->setremoteveh( nullptr );
                refresh();
            } );

        } else if( has_engine_type_not( fuel_type_muscle, true ) ) {
            options.emplace_back( engine_on ? _( "Turn off the engine" ) : _( "Turn on the engine" ),
                                  keybind( "TOGGLE_ENGINE" ) );
            actions.push_back( [&] {
                if( engine_on )
                {
                    engine_on = false;
                    add_msg( _( "You turn the engine off." ) );
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
        options.emplace_back( string_format( _( "Fold %s" ), name.c_str() ), keybind( "FOLD_VEHICLE" ) );
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

    uimenu menu;
    menu.return_invalid = true;
    menu.text = _( "Vehicle controls" );
    menu.entries = options;
    menu.query();
    if( menu.ret >= 0 ) {
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
        debugmsg( _( "Tried to fold non-folding vehicle %s" ), name.c_str() );
        return false;
    }

    if( g->u.controlling_vehicle ) {
        add_msg( m_warning,
                 _( "As the pitiless metal bars close on your nether regions, you reconsider trying to fold the %s while riding it." ),
                 name.c_str() );
        return false;
    }

    if( velocity > 0 ) {
        add_msg( m_warning, _( "You can't fold the %s while it's in motion." ), name.c_str() );
        return false;
    }

    add_msg( _( "You painstakingly pack the %s into a portable configuration." ), name.c_str() );

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
    for( const vpart_reference vp : parts_with_feature( "CARGO" ) ) {
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
        bicycle.set_var( "name", string_format( _( "folded %s" ), name.c_str() ) );
        bicycle.set_var( "vehicle_name", name );
        // TODO: a better description?
        bicycle.set_var( "description", string_format( _( "A folded %s." ), name.c_str() ) );
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

    int eff_temp = g->get_temperature( g->u.pos() );
    if( !parts[ engines[ e ] ].faults().count( fault_glowplug ) ) {
        eff_temp = std::min( eff_temp, 20 );
    }

    return 1.0 - ( std::max( 0, std::min( 30, eff_temp ) ) / 30.0 );
}

int vehicle::engine_start_time( const int e ) const
{
    if( !is_engine_on( e ) || part_info( engines[e] ).has_flag( "E_STARTS_INSTANTLY" ) ||
        !fuel_left( part_info( engines[e] ).fuel_type ) ) {
        return 0;
    }

    const double dmg = parts[engines[e]].damage_percent();

    // non-linear range [100-1000]; f(0.0) = 100, f(0.6) = 250, f(0.8) = 500, f(0.9) = 1000
    // diesel engines with working glow plugs always start with f = 0.6 (or better)
    const int cold = ( 1 / tanh( 1 - std::min( engine_cold_factor( e ), 0.9 ) ) ) * 100;

    return ( part_power( engines[ e ], true ) / 16 ) + ( 100 * dmg ) + cold;
}

bool vehicle::start_engine( const int e )
{
    if( !is_engine_on( e ) ) {
        return false;
    }

    const vpart_info &einfo = part_info( engines[e] );
    const vehicle_part &eng = parts[ engines[ e ] ];

    if( fuel_left( einfo.fuel_type ) <= 0 && einfo.fuel_type != fuel_type_none ) {
        if( einfo.fuel_type == fuel_type_muscle ) {
            add_msg( _( "The %s's mechanism is out of reach!" ), name.c_str() );
        } else {
            add_msg( _( "Looks like the %1$s is out of %2$s." ), eng.name().c_str(),
                     item::nname( einfo.fuel_type ).c_str() );
        }
        return false;
    }

    const double dmg = parts[engines[e]].damage_percent();
    const int engine_power = part_power( engines[e], true );
    const double cold_factor = engine_cold_factor( e );

    if( einfo.engine_backfire_threshold() ) {
        if( ( 1 - dmg ) < einfo.engine_backfire_threshold() && one_in( einfo.engine_backfire_freq() ) ) {
            backfire( e );
        } else {
            const tripoint pos = global_part_pos3( engines[e] );
            sounds::ambient_sound( pos, engine_start_time( e ) / 10, "" );
        }
    }

    // Immobilizers need removing before the vehicle can be started
    if( eng.faults().count( fault_immobiliser ) ) {
        add_msg( _( "The %s makes a long beeping sound." ), eng.name().c_str() );
        return false;
    }

    // Engine with starter motors can fail on both battery and starter motor
    if( eng.faults_potential().count( fault_starter ) ) {
        if( eng.faults().count( fault_starter ) ) {
            add_msg( _( "The %s makes a single clicking sound." ), eng.name().c_str() );
            return false;
        }
        const int penalty = ( engine_power * dmg / 2 ) + ( engine_power * cold_factor / 5 );
        if( discharge_battery( ( engine_power + penalty ) / 10, true ) != 0 ) {
            add_msg( _( "The %s makes a rapid clicking sound." ), eng.name().c_str() );
            return false;
        }
    }

    // Engines always fail to start with faulty fuel pumps
    if( eng.faults().count( fault_pump ) || eng.faults().count( fault_diesel ) ) {
        add_msg( _( "The %s quickly stutters out." ), eng.name().c_str() );
        return false;
    }

    // Damaged engines have a chance of failing to start
    if( x_in_y( dmg * 100, 120 ) ) {
        add_msg( _( "The %s makes a terrible clanking sound." ), eng.name().c_str() );
        return false;
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
        add_msg( m_info, _( "The %s doesn't have an engine!" ), name.c_str() );
        return;
    }

    if( take_control && !g->u.controlling_vehicle ) {
        g->u.controlling_vehicle = true;
        add_msg( _( "You take control of the %s." ), name.c_str() );
    }

    g->u.assign_activity( activity_id( "ACT_START_ENGINES" ), start_time );
    g->u.activity.placement = starting_engine_position - g->u.pos();
    g->u.activity.values.push_back( take_control );
}

void vehicle::honk_horn()
{
    const bool no_power = ! fuel_left( fuel_type_battery, true );
    bool honked = false;

    for( const vpart_reference vp : parts_with_feature( "HORN" ) ) {
        const size_t p = vp.part_index();
        //Only bicycle horn doesn't need electricity to work
        const vpart_info &horn_type = part_info( p );
        if( ( horn_type.get_id() != vpart_id( "horn_bicycle" ) ) && no_power ) {
            continue;
        }
        if( ! honked ) {
            add_msg( _( "You honk the horn!" ) );
            honked = true;
        }
        //Get global position of horn
        const auto horn_pos = global_part_pos3( p );
        //Determine sound
        if( horn_type.bonus >= 110 ) {
            //~ Loud horn sound
            sounds::sound( horn_pos, horn_type.bonus, _( "HOOOOORNK!" ) );
        } else if( horn_type.bonus >= 80 ) {
            //~ Moderate horn sound
            sounds::sound( horn_pos, horn_type.bonus, _( "BEEEP!" ) );
        } else {
            //~ Weak horn sound
            sounds::sound( horn_pos, horn_type.bonus, _( "honk." ) );
        }
    }

    if( ! honked ) {
        add_msg( _( "You honk the horn, but nothing happens." ) );
    }
}

void vehicle::beeper_sound()
{
    // No power = no sound
    if( fuel_left( fuel_type_battery, true ) == 0 ) {
        return;
    }

    const bool odd_turn = calendar::once_every( 2_turns );
    for( const vpart_reference vp : parts_with_feature( "BEEPER" ) ) {
        const size_t p = vp.part_index();
        if( ( odd_turn && part_flag( p, VPFLAG_EVENTURN ) ) ||
            ( !odd_turn && part_flag( p, VPFLAG_ODDTURN ) ) ) {
            continue;
        }

        const vpart_info &beeper_type = part_info( p );
        //~ Beeper sound
        sounds::sound( global_part_pos3( p ), beeper_type.bonus, _( "beep!" ) );
    }
}

void vehicle::play_music()
{
    for( auto e : get_parts( "STEREO", true ) ) {
        iuse::play_music( g->u, global_part_pos3( *e ), 15, 30 );
    }
}

void vehicle::play_chimes()
{
    if( !one_in( 3 ) ) {
        return;
    }

    for( auto e : get_parts( "CHIMES", true ) ) {
        sounds::sound( global_part_pos3( *e ), 40, _( "a simple melody blaring from the loudspeakers." ) );
    }
}

void vehicle::operate_plow()
{
    for( const vpart_reference vp : parts_with_feature( "PLOW" ) ) {
        const size_t plow_id = vp.part_index();
        const tripoint start_plow = global_part_pos3( plow_id );
        if( g->m.has_flag( "DIGGABLE", start_plow ) ) {
            g->m.ter_set( start_plow, t_dirtmound );
        } else {
            const int speed = velocity;
            const int v_damage = rng( 3, speed );
            damage( plow_id, v_damage, DT_BASH, false );
            sounds::sound( start_plow, v_damage, _( "Clanggggg!" ) );
        }
    }
}

void vehicle::operate_rockwheel()
{
    for( const vpart_reference vp : parts_with_feature( "ROCKWHEEL" ) ) {
        const size_t rockwheel_id = vp.part_index();
        const tripoint start_dig = global_part_pos3( rockwheel_id );
        if( g->m.has_flag( "DIGGABLE", start_dig ) ) {
            g->m.ter_set( start_dig, t_pit_shallow );
        } else {
            const int speed = velocity;
            const int v_damage = rng( 3, speed );
            damage( rockwheel_id, v_damage, DT_BASH, false );
            sounds::sound( start_dig, v_damage, _( "Clanggggg!" ) );
        }
    }
}

void vehicle::operate_reaper()
{
    for( const vpart_reference vp : parts_with_feature( "REAPER" ) ) {
        const size_t reaper_id = vp.part_index();
        const tripoint reaper_pos = global_part_pos3( reaper_id );
        const int plant_produced =  rng( 1, parts[ reaper_id ].info().bonus );
        const int seed_produced = rng( 1, 3 );
        const units::volume max_pickup_volume = parts[ reaper_id ].info().size / 20;
        if( g->m.furn( reaper_pos ) != f_plant_harvest ||
            !g->m.has_items( reaper_pos ) ) {
            continue;
        }
        const item &seed = g->m.i_at( reaper_pos ).front();
        if( seed.typeId() == "fungal_seeds" ||
            seed.typeId() == "marloss_seed" ) {
            // Otherworldly plants, the earth-made reaper can not handle those.
            continue;
        }
        g->m.furn_set( reaper_pos, f_null );
        g->m.i_clear( reaper_pos );
        for( auto &i : iexamine::get_harvest_items(
                 *seed.type, plant_produced, seed_produced, false ) ) {
            g->m.add_item_or_charges( reaper_pos, i );
        }
        sounds::sound( reaper_pos, rng( 10, 25 ), _( "Swish" ) );
        if( part_flag( reaper_id, "CARGO" ) ) {
            map_stack stack( g->m.i_at( reaper_pos ) );
            for( auto iter = stack.begin(); iter != stack.end(); ) {
                if( ( iter->volume() <= max_pickup_volume ) &&
                    add_item( reaper_id, *iter ) ) {
                    iter = stack.erase( iter );
                } else {
                    ++iter;
                }
            }
        }
    }
}

void vehicle::operate_planter()
{
    for( const vpart_reference vp : parts_with_feature( "PLANTER" ) ) {
        const size_t planter_id = vp.part_index();
        const tripoint &loc = global_part_pos3( planter_id );
        vehicle_stack v = get_items( planter_id );
        for( auto i = v.begin(); i != v.end(); i++ ) {
            if( i->is_seed() ) {
                // If it is an "advanced model" then it will avoid damaging itself or becoming damaged. It's a real feature.
                if( g->m.ter( loc ) != t_dirtmound && part_flag( planter_id,  "ADVANCED_PLANTER" ) ) {
                    //then don't put the item there.
                    break;
                } else if( g->m.ter( loc ) == t_dirtmound ) {
                    g->m.furn_set( loc, f_plant_seed );
                } else if( !g->m.has_flag( "DIGGABLE", loc ) ) {
                    //If it isn't diggable terrain, then it will most likely be damaged.
                    damage( planter_id, rng( 1, 10 ), DT_BASH, false );
                    sounds::sound( loc, rng( 10, 20 ), _( "Clink" ) );
                }
                if( !i->count_by_charges() || i->charges == 1 ) {
                    i->set_age( 0 );
                    g->m.add_item( loc, *i );
                    v.erase( i );
                } else {
                    item tmp = *i;
                    tmp.charges = 1;
                    tmp.set_age( 0 );
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
    for( const vpart_reference vp : parts_with_feature( "SCOOP" ) ) {
        const size_t scoop = vp.part_index();
        const int chance_to_damage_item = 9;
        const units::volume max_pickup_volume = parts[scoop].info().size / 10;
        const std::array<std::string, 4> sound_msgs = {{
                _( "Whirrrr" ), _( "Ker-chunk" ), _( "Swish" ), _( "Cugugugugug" )
            }
        };
        sounds::sound( global_part_pos3( scoop ), rng( 20, 35 ), random_entry_ref( sound_msgs ) );
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
            const map_stack q = g->m.i_at( position );
            if( g->m.has_flag( "SEALED", position ) ) {
                continue;//ignore it. Street sweepers are not known for their ability to harvest crops.
            }
            size_t itemdex = 0;
            for( auto it : q ) {
                if( it.volume() < max_pickup_volume ) {
                    that_item_there = g->m.item_from( position, itemdex );
                    break;
                }
                itemdex++;
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
                               _( "BEEEThump" ) );
            }
            const int battery_deficit = discharge_battery( that_item_there->weight() / 1_gram *
                                        -part_epower( scoop ) / rng( 8, 15 ) );
            if( battery_deficit == 0 && add_item( scoop, *that_item_there ) ) {
                g->m.i_rem( position, itemdex );
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
            sounds::sound( global_pos3(), ( int ) rng( 45, 80 ), random_entry_ref( sound_msgs ) );
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
                  parts[ part_index ].name().c_str(), name.c_str() );
    } else {
        open_or_close( part_index, true );
    }
}

/**
 * Opens an openable part at the specified index. If it's a multipart, opens
 * all attached parts as well.
 * @param part_index The index in the parts list of the part to open.
 */
void vehicle::close( int part_index )
{
    if( !part_info( part_index ).has_flag( "OPENABLE" ) ) {
        debugmsg( "Attempted to close non-closeable part %d (%s) on a %s!", part_index,
                  parts[ part_index ].name().c_str(), name.c_str() );
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
    std::vector<int> parts_here = parts_at_relative( parts[p].mount.x, parts[p].mount.y );
    for( auto &elem : parts_here ) {
        if( part_flag( elem, VPFLAG_OPENABLE ) ) {
            // Note that this will open multi-square and non-multipart parts in the tile. This
            // means that adjacent open multi-square openables can still have closed stuff
            // on same tile after this function returns
            open( elem );
        }
    }
}

void vehicle::open_or_close( int const part_index, bool const opening )
{
    parts[part_index].open = opening ? 1 : 0;
    insides_dirty = true;
    g->m.set_transparency_cache_dirty( smz );

    if( !part_info( part_index ).has_flag( "MULTISQUARE" ) ) {
        return;
    }

    /* Find all other closed parts with the same ID in adjacent squares.
     * This is a tighter restriction than just looking for other Multisquare
     * Openable parts, and stops trunks from opening side doors and the like. */
    for( const vpart_reference vp : get_parts() ) {
        const size_t next_index = vp.part_index();
        if( parts[next_index].removed ) {
            continue;
        }

        //Look for parts 1 square off in any cardinal direction
        const int dx = parts[next_index].mount.x - parts[part_index].mount.x;
        const int dy = parts[next_index].mount.y - parts[part_index].mount.y;
        const int delta = dx * dx + dy * dy;

        const bool is_near = ( delta == 1 );
        const bool is_id = part_info( next_index ).get_id() == part_info( part_index ).get_id();
        const bool do_next = !!parts[next_index].open ^ opening;

        if( is_near && is_id && do_next ) {
            open_or_close( next_index, opening );
        }
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

    if( parts[p].enabled ) {
        parts[p].enabled = false;
        add_msg( m_bad,
                 _( "You turn the washing machine off before it's finished the program, and open its lid." ) );
    } else if( fuel_left( "water" ) < 24 && fuel_left( "water_clean" ) < 24 ) {
        add_msg( m_bad, _( "You need 24 charges of water in tanks of the %s to fill the washing machine." ),
                 name.c_str() );
    } else if( !detergent_is_enough ) {
        add_msg( m_bad, _( "You need 5 charges of detergent for the washing machine." ) );
    } else if( !filthy_items ) {
        add_msg( m_bad,
                 _( "You need to remove all non-filthy items from the washing machine to start the washing program." ) );
    } else {
        parts[p].enabled = true;
        for( auto &n : items ) {
            n.set_age( 0 );
        }

        if( fuel_left( "water" ) >= 24 ) {
            drain( "water", 24 );
        } else {
            drain( "water_clean", 24 );
        }

        std::vector<item_comp> detergent;
        detergent.push_back( item_comp( "detergent", 5 ) );
        g->u.consume_items( detergent );

        add_msg( m_good,
                 _( "You pour some detergent into the washing machine, close its lid, and turn it on.  The washing machine is being filled with water from vehicle tanks." ) );
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
    for( auto rack_parts : racks_parts ) {
        for( auto rack_part : rack_parts ) {
            // skip parts that aren't carrying anything
            if( !parts[ rack_part ].has_flag( vehicle_part::carrying_flag ) ) {
                continue;
            }
            for( int i = 0; i < 4; i++ ) {
                point near_loc = parts[ rack_part ].mount + vehicles::cardinal_d[ i ];
                std::vector<int> near_parts = parts_at_relative( near_loc.x, near_loc.y );
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
        success = find_rackable_vehicle( racks_parts );
    }
    if( success ) {
        g->refresh_all();
    }
}
