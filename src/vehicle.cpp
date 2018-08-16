#include "vehicle.h"

#include "coordinate_conversions.h"
#include "map.h"
#include "mapbuffer.h"
#include "output.h"
#include "game.h"
#include "map.h"
#include "item.h"
#include "item_group.h"
#include "veh_interact.h"
#include "cursesdef.h"
#include "catacharset.h"
#include "overmapbuffer.h"
#include "messages.h"
#include "iexamine.h"
#include "vpart_position.h"
#include "vpart_reference.h"
#include "string_formatter.h"
#include "ui.h"
#include "debug.h"
#include "sounds.h"
#include "translations.h"
#include "ammo.h"
#include "options.h"
#include "material.h"
#include "monster.h"
#include "npc.h"
#include "veh_type.h"
#include "trap.h"
#include "itype.h"
#include "submap.h"
#include "mapdata.h"
#include "mtype.h"
#include "weather.h"
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

/*
 * Speed up all those if ( blarg == "structure" ) statements that are used everywhere;
 *   assemble "structure" once here instead of repeatedly later.
 */
static const itype_id fuel_type_none("null");
static const itype_id fuel_type_gasoline("gasoline");
static const itype_id fuel_type_diesel("diesel");
static const itype_id fuel_type_battery("battery");
static const itype_id fuel_type_water("water_clean");
static const itype_id fuel_type_muscle("muscle");
static const std::string part_location_structure("structure");

static const fault_id fault_belt( "fault_engine_belt_drive" );
static const fault_id fault_diesel( "fault_engine_pump_diesel" );
static const fault_id fault_glowplug( "fault_engine_glow_plug" );
static const fault_id fault_immobiliser( "fault_engine_immobiliser" );
static const fault_id fault_pump( "fault_engine_pump_fuel" );
static const fault_id fault_starter( "fault_engine_starter" );
static const fault_id fault_filter_air( "fault_engine_filter_air" );
static const fault_id fault_filter_fuel( "fault_engine_filter_fuel" );

const skill_id skill_mechanics( "mechanics" );

const efftype_id effect_stunned( "stunned" );

// Vehicle stack methods.
std::list<item>::iterator vehicle_stack::erase( std::list<item>::iterator it )
{
    return myorigin->remove_item(part_num, it);
}

void vehicle_stack::push_back( const item &newitem )
{
    myorigin->add_item(part_num, newitem);
}

void vehicle_stack::insert_at( std::list<item>::iterator index,
                                   const item &newitem )
{
    myorigin->add_item_at(part_num, index, newitem);
}

units::volume vehicle_stack::max_volume() const
{
    if( myorigin->part_flag( part_num, "CARGO" ) && !myorigin->parts[part_num].is_broken() ) {
        return myorigin->parts[part_num].info().size;
    }
    return 0;
}

// Vehicle class methods.

vehicle::vehicle(const vproto_id &type_id, int init_veh_fuel, int init_veh_status): type(type_id)
{
    turn_dir = 0;
    face.init(0);
    move.init(0);
    of_turn_carry = 0;

    if( !type.str().empty() && type.is_valid() ) {
        const vehicle_prototype &proto = type.obj();
        // Copy the already made vehicle. The blueprint is created when the json data is loaded
        // and is guaranteed to be valid (has valid parts etc.).
        *this = *proto.blueprint;
        init_state(init_veh_fuel, init_veh_status);
    }
    precalc_mounts(0, pivot_rotation[0], pivot_anchor[0]);
    refresh();
}

vehicle::vehicle() : vehicle( vproto_id() )
{
    smx = 0;
    smy = 0;
    smz = 0;
}

vehicle::~vehicle() = default;

void vehicle::set_hp( vehicle_part &pt, int qty )
{
    if( qty == pt.info().durability ) {
        pt.base.set_damage( 0 );

    } else if( qty == 0 ) {
        pt.base.set_damage( pt.base.max_damage() );

    } else {
        double k = pt.base.max_damage() / double( pt.info().durability );
        pt.base.set_damage( pt.base.max_damage() - ( qty * k ) );
    }
}

bool vehicle::mod_hp( vehicle_part &pt, int qty, damage_type dt )
{
    double k = pt.base.max_damage() / double( pt.info().durability );
    return pt.base.mod_damage( - qty * k, dt );
}

bool vehicle::player_in_control(player const& p) const
{
    // Debug switch to prevent vehicles from skidding
    // without having to place the player in them.
    if( tags.count( "IN_CONTROL_OVERRIDE" ) ) {
        return true;
    }

    const optional_vpart_position vp = g->m.veh_at( p.pos() );
    if( vp && &vp->vehicle() == this &&
        part_with_feature( vp->part_index(), VPFLAG_CONTROLS, false ) >= 0 && p.controlling_vehicle ) {
        return true;
    }

    return remote_controlled( p );
}

bool vehicle::remote_controlled(player const &p) const
{
    vehicle *veh = g->remoteveh();
    if( veh != this ) {
        return false;
    }

    auto remote = all_parts_with_feature( "REMOTE_CONTROLS", true );
    for( int part : remote ) {
        if( rl_dist( p.pos(), tripoint( global_pos() + parts[part].precalc[0], p.posz() ) ) <= 40 ) {
            return true;
        }
    }

    add_msg(m_bad, _("Lost connection with the vehicle due to distance!"));
    g->setremoteveh( nullptr );
    return false;
}

/** Checks all parts to see if frames are missing (as they might be when
 * loading from a game saved before the vehicle construction rules overhaul). */
void vehicle::add_missing_frames()
{
    static const vpart_id frame_id( "frame_vertical" );
    const vpart_info &frame_part = frame_id.obj(); // NOT static, could be different each time
    //No need to check the same (x, y) spot more than once
    std::set< std::pair<int, int> > locations_checked;
    for (auto &i : parts) {
        int next_x = i.mount.x;
        int next_y = i.mount.y;
        std::pair<int, int> mount_location = std::make_pair(next_x, next_y);

        if(locations_checked.count(mount_location) == 0) {
            std::vector<int> parts_here = parts_at_relative(next_x, next_y, false);
            bool found = false;
            for( auto &elem : parts_here ) {
                if( part_info( elem ).location == part_location_structure ) {
                    found = true;
                    break;
                }
            }
            if( !found ) {
                // Install missing frame
                parts.emplace_back( frame_part.get_id(), next_x, next_y, item( frame_part.item ) );
            }
        }

        locations_checked.insert(mount_location);
    }
}

// Called when loading a vehicle that predates steerable wheels.
// Tries to convert some wheels to steerable versions on the front axle.
void vehicle::add_steerable_wheels()
{
    int axle = INT_MIN;
    std::vector< std::pair<int, vpart_id> > wheels;

    // Find wheels that have steerable versions.
    // Convert the wheel(s) with the largest x value.
    for (size_t p = 0; p < parts.size(); ++p) {
        if (part_flag(p, "STEERABLE") || part_flag(p, "TRACKED")) {
            // Has a wheel that is inherently steerable
            // (e.g. unicycle, casters), this vehicle doesn't
            // need conversion.
            return;
        }

        if (parts[p].mount.x < axle) {
            // there is another axle in front of this
            continue;
        }

        if( part_flag( p, VPFLAG_WHEEL ) ) {
            vpart_id steerable_id( part_info( p ).get_id().str() + "_steerable" );
            if( steerable_id.is_valid() ) {
                // We can convert this.
                if (parts[p].mount.x != axle) {
                    // Found a new axle further forward than the
                    // existing one.
                    wheels.clear();
                    axle = parts[p].mount.x;
                }

                wheels.push_back(std::make_pair(static_cast<int>( p ), steerable_id));
            }
        }
    }

    // Now convert the wheels to their new types.
    for (auto &wheel : wheels) {
        parts[ wheel.first ].id = wheel.second;
    }
}

void vehicle::init_state(int init_veh_fuel, int init_veh_status)
{
    // vehicle parts excluding engines are by default turned off
    for( auto &pt : parts ) {
        pt.enabled = pt.base.is_engine();
    }

    bool destroySeats = false;
    bool destroyControls = false;
    bool destroyTank = false;
    bool destroyEngine = false;
    bool destroyTires = false;
    bool blood_covered = false;
    bool blood_inside = false;
    bool has_no_key = false;
    bool destroyAlarm = false;

    // More realistically it should be -5 days old
    last_update = 0;

    // veh_fuel_multiplier is percentage of fuel
    // 0 is empty, 100 is full tank, -1 is random 7% to 35%
    int veh_fuel_mult = init_veh_fuel;
    if (init_veh_fuel == - 1) {
        veh_fuel_mult = rng (1,7);
    }
    if (init_veh_fuel > 100) {
        veh_fuel_mult = 100;
    }

    // veh_status is initial vehicle damage
    // -1 = light damage (DEFAULT)
    //  0 = undamaged
    //  1 = disabled, destroyed tires OR engine
    int veh_status = -1;
    if (init_veh_status == 0) {
        veh_status = 0;
    }
    if (init_veh_status == 1) {
        int rand = rng( 1, 100 );
        veh_status = 1;

        if( rand <= 5 ) {          //  seats are destroyed 5%
            destroySeats = true;
        } else if( rand <= 15 ) {  // controls are destroyed 10%
            destroyControls = true;
            veh_fuel_mult += rng (0, 7);    // add 0-7% more fuel if controls are destroyed
        } else if( rand <= 23 ) {  // battery, minireactor or gasoline tank are destroyed 8%
            destroyTank = true;
        } else if( rand <= 29 ) {  // engine are destroyed 6%
            destroyEngine = true;
            veh_fuel_mult += rng (3, 12);   // add 3-12% more fuel if engine is destroyed
        } else if( rand <= 66 ) {  // tires are destroyed 37%
            destroyTires = true;
            veh_fuel_mult += rng (0, 18);   // add 0-18% more fuel if tires are destroyed
        } else {                   // vehicle locked 34%
            has_no_key = true;
        }
    }
    // if locked, 16% chance something damaged
    if( one_in(6) && has_no_key ) {
        if( one_in(3) ) {
            destroyTank = true;
        } else if( one_in(2) ) {
            destroyEngine = true;
        } else {
            destroyTires = true;
        }
    } else if( !one_in(3) ){
        //most cars should have a destroyed alarm
        destroyAlarm = true;
    }

    //Provide some variety to non-mint vehicles
    if( veh_status != 0 ) {
        //Leave engine running in some vehicles, if the engine has not been destroyed
        if( veh_fuel_mult > 0 && all_parts_with_feature("ENGINE", true).size() > 0 &&
            one_in(8) && !destroyEngine && !has_no_key && has_engine_type_not(fuel_type_muscle, true) ) {
            engine_on = true;
        }

        auto light_head  = one_in( 20 );
        auto light_dome  = one_in( 16 );
        auto light_aisle = one_in(  8 );
        auto light_overh = one_in(  4 );
        auto light_atom  = one_in(  2 );
        for( auto &pt : parts ) {
            if( pt.has_flag( VPFLAG_CONE_LIGHT ) ) {
                pt.enabled = light_head;
            } else if( pt.has_flag( VPFLAG_DOME_LIGHT ) ) {
                pt.enabled = light_dome;
            } else if( pt.has_flag( VPFLAG_AISLE_LIGHT ) ) {
                pt.enabled = light_aisle;
            } else if( pt.has_flag( VPFLAG_CIRCLE_LIGHT ) ) {
                pt.enabled = light_overh;
            } else if( pt.has_flag( VPFLAG_ATOMIC_LIGHT ) ) {
                pt.enabled = light_atom;
            }
        }

        if( one_in(10) ) {
            blood_covered = true;
        }

        if( one_in(8) ) {
            blood_inside = true;
        }

        for( auto e : get_parts( "FRIDGE" ) ) {
            e->enabled = true;
        }

        for( auto e : get_parts( "WATER_PURIFIER" ) ) {
            e->enabled = true;
        }
    }

    bool blood_inside_set = false;
    int blood_inside_x = 0;
    int blood_inside_y = 0;
    for( size_t p = 0; p < parts.size(); p++ ) {
        auto &pt = parts[ p ];

        if( part_flag( p, "REACTOR" ) ) {
            // De-hardcoded reactors. Should always start active
            parts[ p ].enabled = true;
        }

        if( pt.is_battery() ) {
            if( veh_fuel_mult == 100 ) { // Mint condition vehicle
                pt.ammo_set( "battery", pt.ammo_capacity() );
            } else if( one_in( 2 ) && veh_fuel_mult > 0 ) { // Randomize battery ammo a bit
                pt.ammo_set( "battery", pt.ammo_capacity() * ( veh_fuel_mult + rng( 0, 10 ) ) / 100 );
            } else if( one_in( 2 ) && veh_fuel_mult > 0 ) {
                pt.ammo_set( "battery", pt.ammo_capacity() * ( veh_fuel_mult - rng( 0, 10 ) ) / 100 );
            } else {
                pt.ammo_set( "battery", pt.ammo_capacity() * veh_fuel_mult / 100 );
            }
        }

        if( pt.is_tank() && type->parts[p].fuel != "null" ) {
            int qty = pt.ammo_capacity() * veh_fuel_mult / 100;
            qty *= std::max( item::find_type( type->parts[p].fuel )->stack_size, 1 );
            qty /= to_milliliter( units::legacy_volume_factor );
            pt.ammo_set( type->parts[ p ].fuel, qty );
        }

        if (part_flag(p, "OPENABLE")) {    // doors are closed
            if(!parts[p].open && one_in(4)) {
              open(p);
            }
        }
        if (part_flag(p, "BOARDABLE")) {      // no passengers
            parts[p].remove_flag(vehicle_part::passenger_flag);
        }

        // initial vehicle damage
        if (veh_status == 0) {
            // Completely mint condition vehicle
            set_hp( parts[ p ], part_info( p ).durability );
        } else {
            //a bit of initial damage :)
            //clamp 4d8 to the range of [8,20]. 8=broken, 20=undamaged.
            int broken = 8;
            int unhurt = 20;
            int roll = dice( 4, 8 );
            if(roll < unhurt){
                if (roll <= broken) {
                    set_hp( parts[ p ], 0 );
                    parts[ p ].ammo_unset(); //empty broken batteries and fuel tanks
                } else {
                    set_hp( parts[ p ], ( roll - broken ) / double( unhurt - broken ) * part_info( p ).durability );
                }
            } else {
                set_hp( parts[ p ], part_info( p ).durability );
            }

            if( part_flag( p, VPFLAG_ENGINE ) ) {
                // If possible set an engine fault rather than destroying the engine outright
                if( destroyEngine && parts[ p ].faults_potential().empty() ) {
                    set_hp( parts[ p ], 0 );
                } else if( destroyEngine || one_in( 3 ) ) {
                    do {
                        parts[ p ].fault_set( random_entry( parts[ p ].faults_potential() ) );
                    } while( one_in( 3 ) );
                }

            } else if ((destroySeats && (part_flag(p, "SEAT") || part_flag(p, "SEATBELT"))) ||
                (destroyControls && (part_flag(p, "CONTROLS") || part_flag(p, "SECURITY"))) ||
                (destroyAlarm && part_flag(p, "SECURITY"))) {
                set_hp( parts[ p ], 0 );
            }

            // Fuel tanks should be emptied as well
            if( destroyTank && pt.is_tank() ) {
                set_hp( pt, 0 );
                pt.ammo_unset();
            }

            //Solar panels have 25% of being destroyed
            if (part_flag(p, "SOLAR_PANEL") && one_in(4)) {
                set_hp( parts[ p ], 0 );
            }


            /* Bloodsplatter the front-end parts. Assume anything with x > 0 is
            * the "front" of the vehicle (since the driver's seat is at (0, 0).
            * We'll be generous with the blood, since some may disappear before
            * the player gets a chance to see the vehicle. */
            if(blood_covered && parts[p].mount.x > 0) {
                if(one_in(3)) {
                    //Loads of blood. (200 = completely red vehicle part)
                    parts[p].blood = rng(200, 600);
                } else {
                    //Some blood
                    parts[p].blood = rng(50, 200);
                }
            }

            if(blood_inside) {
                // blood is splattered around (blood_inside_x, blood_inside_y),
                // coordinates relative to mount point; the center is always a seat
                if (blood_inside_set) {
                    int distSq = std::pow((blood_inside_x - parts[p].mount.x), 2) +
                        std::pow((blood_inside_y - parts[p].mount.y), 2);
                    if (distSq <= 1) {
                        parts[p].blood = rng(200, 400) - distSq * 100;
                    }
                } else if (part_flag(p, "SEAT")) {
                    // Set the center of the bloody mess inside
                    blood_inside_x = parts[p].mount.x;
                    blood_inside_y = parts[p].mount.y;
                    blood_inside_set = true;
                }
            }
        }
        //sets the vehicle to locked, if there is no key and an alarm part exists
        if( part_flag( p, "SECURITY" ) && has_no_key && !parts[p].is_broken() ) {
            is_locked = true;

            if( one_in( 2 ) ) {
                // if vehicle has immobilizer 50% chance to add additional fault
                parts[ p ].fault_set( fault_immobiliser );
            }
        }
    }
    // destroy tires until the vehicle is not drivable
    if( destroyTires && !wheelcache.empty() ) {
        int tries = 0;
        while( valid_wheel_config( false ) && tries < 100 ) {
            // wheel config is still valid, destroy the tire.
            set_hp( parts[random_entry( wheelcache )], 0 );
            tries++;
        }
    }

    invalidate_mass();
}
/**
 * Smashes up a vehicle that has already been placed; used for generating
 * very damaged vehicles. Additionally, any spot where two vehicles overlapped
 * (ie, any spot with multiple frames) will be completely destroyed, as that
 * was the collision point.
 */
void vehicle::smash() {
    for( auto &part : parts ) {
        //Skip any parts already mashed up or removed.
        if( part.is_broken() || part.removed ) {
            continue;
        }

        std::vector<int> parts_in_square = parts_at_relative( part.mount.x, part.mount.y );
        int structures_found = 0;
        for( auto &square_part_index : parts_in_square ) {
            if (part_info(square_part_index).location == part_location_structure) {
                structures_found++;
            }
        }

        if(structures_found > 1) {
            //Destroy everything in the square
            for( int idx : parts_in_square ) {
                mod_hp( parts[ idx ], 0 - parts[ idx ].hp(), DT_BASH );
                parts[ idx ].ammo_unset();
            }
            continue;
        }

        //Everywhere else, drop by 10-120% of max HP (anything over 100 = broken)
        if( mod_hp( part, 0 - ( rng_float( 0.1f, 1.2f ) * part.info().durability ), DT_BASH ) ) {
            part.ammo_unset();
        }
    }
}

const std::string vehicle::disp_name() const
{
    return string_format( _("the %s"), name.c_str() );
}


int vehicle::lift_strength() const
{
    units::mass mass = total_mass();
    return std::max( mass / 10000_gram, 1 );
}

enum change_types : int {
    OPENCURTAINS = 0,
    OPENBOTH,
    CLOSEDOORS,
    CLOSEBOTH,
    CANCEL
};

void vehicle::control_doors()
{
    std::vector< int > door_motors = all_parts_with_feature( "DOOR_MOTOR", true );
    std::vector< int > doors_with_motors; // Indices of doors
    std::vector< tripoint > locations; // Locations used to display the doors
    // it is possible to have one door to open and one to close for single motor
    doors_with_motors.reserve( door_motors.size() * 2 );
    locations.reserve( door_motors.size() * 2 );
    if( door_motors.empty() ) {
        debugmsg( "vehicle::control_doors called but no door motors found" );
        return;
    }

    uimenu pmenu;
    pmenu.title = _( "Select door to toggle" );
    int doors[2]; // one door to open and one to close
    for( int p : door_motors ) {
        doors[0] = next_part_to_open( p );
        doors[1] = next_part_to_close( p );
        for( int door : doors ) {
            if( door == -1 ) {
                continue;
            }

            int val = doors_with_motors.size();
            doors_with_motors.push_back( door );
            locations.push_back( tripoint( global_pos() + parts[p].precalc[0], smz ) );
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
            for( int motor : door_motors ) {
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
                        if( next_part != -1) {
                            open_or_close( next_part, open );
                        }
                    }
                } else {
                    int part = next_part_to_close( motor );
                    if( part != -1) {
                        if( part_flag( part, "CURTAIN" ) &&  option == CLOSEDOORS ) {
                            continue;
                        }
                        open_or_close( part, open );
                        if( option == CLOSEBOTH ) {
                            next_part = next_part_to_close( motor );
                        }
                        if( next_part != -1) {
                            open_or_close( next_part, open );
                        }
                    }
                }
            }
        }
    }
}

char keybind( const std::string &opt, const std::string &context )
{
    auto const keys = input_context( context ).keys_bound_to( opt );
    return keys.empty() ? ' ' : keys.front();
}

void vehicle::add_toggle_to_opts(std::vector<uimenu_entry> &options, std::vector<std::function<void()>> &actions, const std::string &name, char key, const std::string &flag )
{
    // fetch matching parts and abort early if none found
    auto found = get_parts( flag );
    if( found.empty() ) {
        return;
    }

    // can this menu option be selected by the user?
    bool allow = true;

    // determine target state - currently parts of similar type are all switched concurrently
    bool state = std::none_of( found.begin(), found.end(), []( const vehicle_part *e ) {
        return e->enabled;
    } );

    // if toggled part potentially usable check if could be enabled now (sufficient fuel etc.)
    if( state ) {
        allow = std::any_of( found.begin(), found.end(), [&]( const vehicle_part *e ) {
            return can_enable( *e );
       } );
    }

    auto msg = string_format( state ? _( "Turn on %s" ) : _( "Turn off %s" ), name.c_str() );
    options.emplace_back( -1, allow, key, msg );

    actions.push_back( [=]{
        for( vehicle_part *e : found ) {
            if( e->enabled != state ) {
                add_msg( state ? _( "Turned on %s" ) : _( "Turned off %s." ), e->name().c_str() );
                e->enabled = state;
            }
        }
        refresh();
    } );
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
            if( camera_on ) {
                camera_on = false;
                add_msg( _( "Camera system disabled" ) );
            } else if( fuel_left( fuel_type_battery, true ) ) {
                camera_on = true;
                add_msg( _( "Camera system enabled" ) );
            } else {
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

        options.emplace_back( _( "Quit controlling electronics" ), "q" );

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
        name = parts[ engines[ e ] ].name();
        tmenu.addentry( e, true, -1, "[%s] %s",
                        ( ( parts[engines[e]].enabled ) ? "x" : " " ) , name.c_str() );
    }

    tmenu.addentry( -1, true, 'q', _( "Finish" ) );
    tmenu.query();
    return tmenu.ret;
}

void vehicle::toggle_specific_engine( int e, bool on )
{
    toggle_specific_part( engines[e], on );
}
void vehicle::toggle_specific_part( int p, bool on )
{
    parts[p].enabled = on;
}
bool vehicle::is_engine_type_on( int e, const itype_id &ft ) const
{
    return is_engine_on( e ) && is_engine_type( e, ft );
}

bool vehicle::has_engine_type( const itype_id &ft, bool const enabled ) const
{
    for( size_t e = 0; e < engines.size(); ++e ) {
        if( is_engine_type( e, ft ) && ( !enabled || is_engine_on( e ) ) ) {
            return true;
        }
    }
    return false;
}
bool vehicle::has_engine_type_not( const itype_id &ft, bool const enabled ) const
{
    for( size_t e = 0; e < engines.size(); ++e ) {
        if( !is_engine_type( e, ft ) && ( !enabled || is_engine_on( e ) ) ) {
            return true;
        }
    }
    return false;
}

bool vehicle::is_engine_type( const int e, const itype_id  &ft ) const
{
    return part_info( engines[e] ).fuel_type == ft;
}

bool vehicle::is_engine_on( int const e ) const
{
    return !parts[ engines[ e ] ].is_broken() && is_part_on( engines[ e ] );
}

bool vehicle::is_part_on( int const p ) const
{
    return parts[p].enabled;
}

bool vehicle::is_alternator_on( int const a ) const
{
    auto alt = parts[ alternators [ a ] ];
    if( alt.is_broken() ) {
        return false;
    }

    return std::any_of( engines.begin(), engines.end(), [this, &alt]( int idx ) {
        auto &eng = parts [ idx ];
        return eng.enabled && !eng.is_broken() && eng.mount == alt.mount &&
               !eng.faults().count( fault_belt );
    } );
}

bool vehicle::has_security_working() const
{
    bool found_security = false;
    for( size_t s = 0; s < speciality.size(); s++ ) {
        if( part_flag( speciality[ s ], "SECURITY" ) && !parts[ speciality[ s ] ].is_broken() ) {
            found_security = true;
            break;
        }
    }
    return found_security;
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
                g->u.activity.values.push_back( global_x() + q.x ); //[0]
                g->u.activity.values.push_back( global_y() + q.y ); //[1]
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
                } else {
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
                } else {
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

bool vehicle::fold_up() {
    const bool can_be_folded = is_foldable();
    const bool is_convertible = (tags.count("convertible") > 0);
    if( ! ( can_be_folded || is_convertible ) ) {
        debugmsg(_("Tried to fold non-folding vehicle %s"), name.c_str());
        return false;
    }

    if( g->u.controlling_vehicle ) {
        add_msg(m_warning, _("As the pitiless metal bars close on your nether regions, you reconsider trying to fold the %s while riding it."), name.c_str());
        return false;
    }

    if( velocity > 0 ) {
        add_msg(m_warning, _("You can't fold the %s while it's in motion."), name.c_str());
        return false;
    }

    add_msg(_("You painstakingly pack the %s into a portable configuration."), name.c_str());

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
    for (size_t p = 0; p < parts.size(); p++) {
        if( part_flag( p, "CARGO" ) ) {
            for( auto &elem : get_items(p) ) {
                g->m.add_item_or_charges( g->u.pos(), elem );
            }
            while( !get_items(p).empty() ) {
                get_items(p).erase( get_items(p).begin() );
            }
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
        JsonOut json(veh_data);
        json.write(parts);
        bicycle.set_var( "folding_bicycle_parts", veh_data.str() );
    } catch( const JsonError &e ) {
        debugmsg("Error storing vehicle: %s", e.c_str());
    }

    if (can_be_folded) {
        bicycle.set_var( "weight", to_gram( total_mass() ) );
        bicycle.set_var( "volume", total_folded_volume() / units::legacy_volume_factor );
        bicycle.set_var( "name", string_format(_("folded %s"), name.c_str()) );
        bicycle.set_var( "vehicle_name", name );
        // TODO: a better description?
        bicycle.set_var( "description", string_format(_("A folded %s."), name.c_str()) );
    }

    g->m.add_item_or_charges( g->u.pos(), bicycle );
    g->m.destroy_vehicle(this);

    // TODO: take longer to fold bigger vehicles
    // TODO: make this interruptible
    g->u.moves -= 500;
    return true;
}

double vehicle::engine_cold_factor( const int e ) const
{
    if( !is_engine_type( e, fuel_type_diesel ) ) { return 0.0; }

    int eff_temp = g->get_temperature( g->u.pos() );
    if( !parts[ engines[ e ] ].faults().count( fault_glowplug ) ) {
        eff_temp = std::min( eff_temp, 20 );
    }

    return 1.0 - (std::max( 0, std::min( 30, eff_temp ) ) / 30.0);
}

int vehicle::engine_start_time( const int e ) const
{
    if( !is_engine_on( e ) || is_engine_type( e, fuel_type_muscle ) ||
        !fuel_left( part_info( engines[e] ).fuel_type ) ) { return 0; }

    const double dmg = 1.0 - double( parts[engines[e]].hp() ) / part_info( engines[e] ).durability;

    // non-linear range [100-1000]; f(0.0) = 100, f(0.6) = 250, f(0.8) = 500, f(0.9) = 1000
    // diesel engines with working glow plugs always start with f = 0.6 (or better)
    const int cold = ( 1 / tanh( 1 - std::min( engine_cold_factor( e ), 0.9 ) ) ) * 100;

    return ( part_power( engines[ e ], true ) / 16 ) + ( 100 * dmg ) + cold;
}

bool vehicle::start_engine( const int e )
{
    if( !is_engine_on( e ) ) { return false; }

    const vpart_info &einfo = part_info( engines[e] );
    const vehicle_part &eng = parts[ engines[ e ] ];

    if( fuel_left( einfo.fuel_type ) <= 0 && einfo.fuel_type != fuel_type_none ) {
        if( einfo.fuel_type == fuel_type_muscle ) {
            add_msg( _("The %s's mechanism is out of reach!"), name.c_str() );
        } else {
            add_msg( _("Looks like the %1$s is out of %2$s."), eng.name().c_str(),
                item::nname( einfo.fuel_type ).c_str() );
        }
        return false;
    }

    const double dmg = 1.0 - ((double)parts[engines[e]].hp() / einfo.durability);
    const int engine_power = part_power( engines[e], true );
    const double cold_factor = engine_cold_factor( e );

    if( einfo.fuel_type != fuel_type_muscle ) {
        if( einfo.fuel_type == fuel_type_gasoline && dmg > 0.75 && one_in( 20 ) ) {
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
        if( !has_starting_engine_position && !parts[ engines[ e ] ].is_broken() && parts[ engines[ e ] ].enabled ) {
            starting_engine_position = global_part_pos3( engines[ e ] );
            has_starting_engine_position = true;
        }
        has_engine = has_engine || is_engine_on( e );
        start_time = std::max( start_time, engine_start_time( e ) );
    }

    if(!has_starting_engine_position){
        starting_engine_position = global_pos3();
    }

    if( !has_engine ) {
        add_msg( m_info, _("The %s doesn't have an engine!"), name.c_str() );
        return;
    }

    if( take_control && !g->u.controlling_vehicle ) {
        g->u.controlling_vehicle = true;
        add_msg( _("You take control of the %s."), name.c_str() );
    }

    g->u.assign_activity( activity_id( "ACT_START_ENGINES" ), start_time );
    g->u.activity.placement = starting_engine_position - g->u.pos();
    g->u.activity.values.push_back( take_control );
}

void vehicle::backfire( const int e ) const
{
    const int power = part_power( engines[e], true );
    const tripoint pos = global_part_pos3( engines[e] );
    //~ backfire sound
    sounds::ambient_sound( pos, 40 + (power / 30), _( "BANG!" ) );
}

void vehicle::honk_horn()
{
    const bool no_power = ! fuel_left( fuel_type_battery, true );
    bool honked = false;

    for( size_t p = 0; p < parts.size(); ++p ) {
        if( ! part_flag( p, "HORN" ) ) {
            continue;
        }
        //Only bicycle horn doesn't need electricity to work
        const vpart_info &horn_type = part_info( p );
        if( ( horn_type.get_id() != vpart_id( "horn_bicycle" ) ) && no_power ) {
            continue;
        }
        if( ! honked ) {
            add_msg( _("You honk the horn!") );
            honked = true;
        }
        //Get global position of horn
        const auto horn_pos = global_part_pos3( p );
        //Determine sound
        if( horn_type.bonus >= 110 ) {
            //~ Loud horn sound
            sounds::sound( horn_pos, horn_type.bonus, _("HOOOOORNK!") );
        } else if( horn_type.bonus >= 80 ) {
            //~ Moderate horn sound
            sounds::sound( horn_pos, horn_type.bonus, _("BEEEP!") );
        } else {
            //~ Weak horn sound
            sounds::sound( horn_pos, horn_type.bonus, _("honk.") );
        }
    }

    if( ! honked ) {
        add_msg( _("You honk the horn, but nothing happens.") );
    }
}

void vehicle::beeper_sound()
{
    // No power = no sound
    if( fuel_left( fuel_type_battery, true ) == 0 ) {
        return;
    }

    const bool odd_turn = calendar::once_every( 2_turns );
    for( size_t p = 0; p < parts.size(); ++p ) {
        if( !part_flag( p, "BEEPER" ) ) {
            continue;
        }
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

const vpart_info& vehicle::part_info (int index, bool include_removed) const
{
    if (index < (int)parts.size()) {
        if (!parts[index].removed || include_removed) {
            return parts[index].info();
        }
    }
    return vpart_id::NULL_ID().obj();
}

// engines & alternators all have power.
// engines provide, whilst alternators consume.
int vehicle::part_power(int const index, bool const at_full_hp) const
{
    if( !part_flag(index, VPFLAG_ENGINE) &&
        !part_flag(index, VPFLAG_ALTERNATOR) ) {
       return 0; // not an engine.
    }

    const vehicle_part& vp = parts[ index ];

    int pwr = vp.info().power;
    if( pwr == 0 ) {
        pwr = vp.base.engine_displacement();
    }

    if (part_info(index).fuel_type == fuel_type_muscle) {
        int pwr_factor = (part_flag(index, "MUSCLE_LEGS") ? 5 : 0) +
                         (part_flag(index, "MUSCLE_ARMS") ? 2 : 0);
        ///\EFFECT_STR increases power produced for MUSCLE_* vehicles
        pwr += int(((g->u).str_cur - 8) * pwr_factor);
    }

    if( pwr < 0 ) {
        return pwr; // Consumers always draw full power, even if broken
    }
    if( at_full_hp ) {
        return pwr; // Assume full hp
    }
    // Damaged engines give less power, but gas/diesel handle it better
    if( part_info(index).fuel_type == fuel_type_gasoline ||
        part_info(index).fuel_type == fuel_type_diesel ) {
        return pwr * (0.25 + (0.75 * ((double)parts[index].hp() / part_info(index).durability)));
    } else {
        return double( pwr * parts[index].hp() ) / part_info(index).durability;
    }
 }

// alternators, solar panels, reactors, and accessories all have epower.
// alternators, solar panels, and reactors provide, whilst accessories consume.
int vehicle::part_epower(int const index) const
{
    int e = part_info(index).epower;
    if( e < 0 ) {
        return e; // Consumers always draw full power, even if broken
    }
    return e * parts[ index ].hp() / part_info(index).durability;
}

int vehicle::epower_to_power(int const epower)
{
    // Convert epower units (watts) to power units
    // Used primarily for calculating battery charge/discharge
    // TODO: convert batteries to use energy units based on watts (watt-ticks?)
    constexpr int conversion_factor = 373; // 373 epower == 373 watts == 1 power == 0.5 HP
    int power = epower / conversion_factor;
    // epower remainder results in chance at additional charge/discharge
    if (x_in_y(abs(epower % conversion_factor), conversion_factor)) {
        power += epower >= 0 ? 1 : -1;
    }
    return power;
}

int vehicle::power_to_epower(int const power)
{
    // Convert power units to epower units (watts)
    // Used primarily for calculating battery charge/discharge
    // TODO: convert batteries to use energy units based on watts (watt-ticks?)
    constexpr int conversion_factor = 373; // 373 epower == 373 watts == 1 power == 0.5 HP
    return power * conversion_factor;
}

bool vehicle::has_structural_part(int const dx, int const dy) const
{
    std::vector<int> parts_here = parts_at_relative(dx, dy, false);

    for( auto &elem : parts_here ) {
        if( part_info( elem ).location == part_location_structure &&
            !part_info( elem ).has_flag( "PROTRUSION" ) ) {
            return true;
        }
    }
    return false;
}

/**
 * Returns whether or not the vehicle has a structural part queued for removal,
 * @return true if a structural is queue for removal, false if not.
 * */
bool vehicle::is_structural_part_removed() const
{
    for( size_t i = 0; i < parts.size(); ++i ) {
        if (parts[i].removed &&
                part_info( i, true ).location == part_location_structure) {
            return true;
        }
    }
    return false;
}

/**
 * Returns whether or not the vehicle part with the given id can be mounted in
 * the specified square.
 * @param dx The local x-coordinate to mount in.
 * @param dy The local y-coordinate to mount in.
 * @param id The id of the part to install.
 * @return true if the part can be mounted, false if not.
 */
bool vehicle::can_mount(int const dx, int const dy, const vpart_id &id) const
{
    //The part has to actually exist.
    if( !id.is_valid() ) {
        return false;
    }

    //It also has to be a real part, not the null part
    const vpart_info &part = id.obj();
    if(part.has_flag("NOINSTALL")) {
        return false;
    }

    const std::vector<int> parts_in_square = parts_at_relative(dx, dy, false);

    //First part in an empty square MUST be a structural part
    if(parts_in_square.empty() && part.location != part_location_structure) {
        return false;
    }

    //No other part can be placed on a protrusion
    if(!parts_in_square.empty() && part_info(parts_in_square[0]).has_flag("PROTRUSION")) {
        return false;
    }

    //No part type can stack with itself, or any other part in the same slot
    for( const auto &elem : parts_in_square ) {
        const vpart_info &other_part = parts[elem].info();

        //Parts with no location can stack with each other (but not themselves)
        if( part.get_id() == other_part.get_id() ||
                (!part.location.empty() && part.location == other_part.location)) {
            return false;
        }
        // Until we have an interface for handling multiple components with CARGO space,
        // exclude them from being mounted in the same tile.
        if( part.has_flag( "CARGO" ) && other_part.has_flag( "CARGO" ) ) {
            return false;
        }

    }

    // All parts after the first must be installed on or next to an existing part
    // the exception is when a single tile only structural object is being repaired
    if(!parts.empty()) {
        if(!is_structural_part_removed() &&
                !has_structural_part(dx, dy) &&
                !has_structural_part(dx+1, dy) &&
                !has_structural_part(dx, dy+1) &&
                !has_structural_part(dx-1, dy) &&
                !has_structural_part(dx, dy-1)) {
            return false;
        }
    }

    // only one muscle engine allowed
    if( part.has_flag(VPFLAG_ENGINE) && part.fuel_type == fuel_type_muscle &&
        has_engine_type(fuel_type_muscle, false) ) {
        return false;
    }

    // Alternators must be installed on a gas engine
    if(part.has_flag(VPFLAG_ALTERNATOR)) {
        bool anchor_found = false;
        for( const auto &elem : parts_in_square ) {
            if( part_info( elem ).has_flag( VPFLAG_ENGINE ) &&
                ( part_info( elem ).fuel_type == fuel_type_gasoline ||
                  part_info( elem ).fuel_type == fuel_type_diesel ||
                  part_info( elem ).fuel_type == fuel_type_muscle)) {
                anchor_found = true;
            }
        }
        if(!anchor_found) {
            return false;
        }
    }

    //Seatbelts must be installed on a seat
    if(part.has_flag("SEATBELT")) {
        bool anchor_found = false;
        for( const auto &elem : parts_in_square ) {
            if( part_info( elem ).has_flag( "BELTABLE" ) ) {
                anchor_found = true;
            }
        }
        if(!anchor_found) {
            return false;
        }
    }

    //Internal must be installed into a cargo area.
    if(part.has_flag("INTERNAL")) {
        bool anchor_found = false;
        for( const auto &elem : parts_in_square ) {
            if( part_info( elem ).has_flag( "CARGO" ) ) {
                anchor_found = true;
            }
        }
        if(!anchor_found) {
            return false;
        }
    }

    // curtains must be installed on (reinforced)windshields
    // TODO: do this automatically using "location":"on_mountpoint"
    if ( part.has_flag( "WINDOW_CURTAIN" ) ) {
        bool anchor_found = false;
        for( const auto &elem : parts_in_square ) {
            if( part_info( elem ).has_flag( "WINDOW" ) ) {
                anchor_found = true;
            }
        }
        if (!anchor_found) {
            return false;
        }
    }

    // Security system must be installed on controls
    if(part.has_flag("ON_CONTROLS")) {
        bool anchor_found = false;
        for( std::vector<int>::const_iterator it = parts_in_square.begin();
             it != parts_in_square.end(); ++it ) {
            if(part_info(*it).has_flag("CONTROLS")) {
                anchor_found = true;
            }
        }
        if(!anchor_found) {
            return false;
        }
    }

    // Cargo locks must go on lockable cargo containers
    // TODO: do this automatically using "location":"on_mountpoint"
    if(part.has_flag("CARGO_LOCKING")) {
        bool anchor_found = false;
        for( std::vector<int>::const_iterator it = parts_in_square.begin();
             it != parts_in_square.end(); ++it ) {
            if(part_info(*it).has_flag("LOCKABLE_CARGO")) {
                anchor_found = true;
            }
        }
        if(!anchor_found) {
            return false;
        }
    }

    //Swappable storage battery must be installed on a BATTERY_MOUNT
    if(part.has_flag("NEEDS_BATTERY_MOUNT")) {
        bool anchor_found = false;
        for( const auto &elem : parts_in_square ) {
            if( part_info( elem ).has_flag( "BATTERY_MOUNT" ) ) {
                anchor_found = true;
            }
        }
        if(!anchor_found) {
            return false;
        }
    }

    //Door motors need OPENABLE
    if( part.has_flag( "DOOR_MOTOR" ) ) {
        bool anchor_found = false;
        for( const auto &elem : parts_in_square ) {
            if( part_info( elem ).has_flag( "OPENABLE" ) ) {
                anchor_found = true;
            }
        }
        if(!anchor_found) {
            return false;
        }
    }

    //Mirrors cannot be mounted on OPAQUE parts
    if( part.has_flag( "VISION" ) && !part.has_flag( "CAMERA" ) ) {
        for( const auto &elem : parts_in_square ) {
            if( part_info( elem ).has_flag( "OPAQUE" ) ) {
                return false;
            }
        }
    }
    //and vice versa
    if( part.has_flag( "OPAQUE" ) ) {
        for( const auto &elem : parts_in_square ) {
            if( part_info( elem ).has_flag( "VISION" ) &&
                !part_info( elem ).has_flag( "CAMERA" ) ) {
                return false;
            }
        }
    }

    //Turrets must be installed on a turret mount
    if( part.has_flag( "TURRET" ) ) {
        bool anchor_found = false;
        for( const auto &elem : parts_in_square ) {
            if( part_info( elem ).has_flag( "TURRET_MOUNT" ) ) {
                anchor_found = true;
                break;
            }
        }
        if( !anchor_found ) {
            return false;
        }
    }

    //Anything not explicitly denied is permitted
    return true;
}

bool vehicle::can_unmount(int const p) const
{
    if(p < 0 || p > (int)parts.size()) {
        return false;
    }

    int dx = parts[p].mount.x;
    int dy = parts[p].mount.y;

    // Can't remove an engine if there's still an alternator there
    if(part_flag(p, VPFLAG_ENGINE) && part_with_feature(p, VPFLAG_ALTERNATOR) >= 0) {
        return false;
    }

    //Can't remove a seat if there's still a seatbelt there
    if(part_flag(p, "BELTABLE") && part_with_feature(p, "SEATBELT") >= 0) {
        return false;
    }

    // Can't remove a window with curtains still on it
    if(part_flag(p, "WINDOW") && part_with_feature(p, "CURTAIN") >=0) {
        return false;
    }

    //Can't remove controls if there's something attached
    if(part_flag(p, "CONTROLS") && part_with_feature(p, "ON_CONTROLS") >= 0) {
        return false;
    }

    //Can't remove a battery mount if there's still a battery there
    if(part_flag(p, "BATTERY_MOUNT") && part_with_feature(p, "NEEDS_BATTERY_MOUNT") >= 0) {
        return false;
    }

    //Can't remove a turret mount if there's still a turret there
    if( part_flag( p, "TURRET_MOUNT" ) && part_with_feature( p, "TURRET" ) >= 0 ) {
        return false;
    }

    //Structural parts have extra requirements
    if(part_info(p).location == part_location_structure) {

        std::vector<int> parts_in_square = parts_at_relative(dx, dy, false);
        /* To remove a structural part, there can be only structural parts left
         * in that square (might be more than one in the case of wreckage) */
        for( auto &elem : parts_in_square ) {
            if( part_info( elem ).location != part_location_structure ) {
                return false;
            }
        }

        //If it's the last part in the square...
        if(parts_in_square.size() == 1) {

            /* This is the tricky part: We can't remove a part that would cause
             * the vehicle to 'break into two' (like removing the middle section
             * of a quad bike, for instance). This basically requires doing some
             * breadth-first searches to ensure previously connected parts are
             * still connected. */

            //First, find all the squares connected to the one we're removing
            std::vector<vehicle_part> connected_parts;

            for(int i = 0; i < 4; i++) {
                int next_x = i < 2 ? (i == 0 ? -1 : 1) : 0;
                int next_y = i < 2 ? 0 : (i == 2 ? -1 : 1);
                std::vector<int> parts_over_there = parts_at_relative(dx + next_x, dy + next_y, false);
                //Ignore empty squares
                if(!parts_over_there.empty()) {
                    //Just need one part from the square to track the x/y
                    connected_parts.push_back(parts[parts_over_there[0]]);
                }
            }

            /* If size = 0, it's the last part of the whole vehicle, so we're OK
             * If size = 1, it's one protruding part (ie, bicycle wheel), so OK
             * Otherwise, it gets complicated... */
            if(connected_parts.size() > 1) {

                /* We'll take connected_parts[0] to be the target part.
                 * Every other part must have some path (that doesn't involve
                 * the part about to be removed) to the target part, in order
                 * for the part to be legally removable. */
                for(auto const &next_part : connected_parts) {
                    if(!is_connected(connected_parts[0], next_part, parts[p])) {
                        //Removing that part would break the vehicle in two
                        return false;
                    }
                }

            }

        }
    }
    //Anything not explicitly denied is permitted
    return true;
}

/**
 * Performs a breadth-first search from one part to another, to see if a path
 * exists between the two without going through the excluded part. Used to see
 * if a part can be legally removed.
 * @param to The part to reach.
 * @param from The part to start the search from.
 * @param excluded_part The part that is being removed and, therefore, should not
 *        be included in the path.
 * @return true if a path exists without the excluded part, false otherwise.
 */
bool vehicle::is_connected(vehicle_part const &to, vehicle_part const &from, vehicle_part const &excluded_part) const
{
    const auto target = to.mount;
    const auto excluded = excluded_part.mount;

    //Breadth-first-search components
    std::list<vehicle_part> discovered;
    vehicle_part current_part;
    std::list<vehicle_part> searched;

    //We begin with just the start point
    discovered.push_back(from);

    while(!discovered.empty()) {
        current_part = discovered.front();
        discovered.pop_front();
        auto current = current_part.mount;

        for(int i = 0; i < 4; i++) {
            point next( current.x + (i < 2 ? (i == 0 ? -1 : 1) : 0),
                        current.y + (i < 2 ? 0 : (i == 2 ? -1 : 1)) );

            if( next == target ) {
                //Success!
                return true;
            } else if( next == excluded ) {
                //There might be a path, but we're not allowed to go that way
                continue;
            }

            std::vector<int> parts_there = parts_at_relative(next.x, next.y);

            if(!parts_there.empty()) {
                //Only add the part if we haven't been here before
                bool found = false;
                for( auto &elem : discovered ) {
                    if( elem.mount == next ) {
                        found = true;
                        break;
                    }
                }
                if(!found) {
                    for( auto &elem : searched ) {
                        if( elem.mount == next ) {
                            found = true;
                            break;
                        }
                    }
                }
                if(!found) {
                    vehicle_part next_part = parts[parts_there[0]];
                    discovered.push_back(next_part);
                }
            }
        }
        //Now that that's done, we've finished exploring here
        searched.push_back(current_part);
    }
    //If we completely exhaust the discovered list, there's no path
    return false;
}

/**
 * Installs a part into this vehicle.
 * @param dx The x coordinate of where to install the part.
 * @param dy The y coordinate of where to install the part.
 * @param id The string ID of the part to install. (see vehicle_parts.json)
 * @param force Skip check of whether we can mount the part here.
 * @return false if the part could not be installed, true otherwise.
 */
int vehicle::install_part( int dx, int dy, const vpart_id &id, bool force )
{
    if( !( force || can_mount( dx, dy, id ) ) ) {
        return -1;
    }
    return install_part( dx, dy, vehicle_part( id, dx, dy, item( id.obj().item ) ) );
}

int vehicle::install_part( int dx, int dy, const vpart_id &id, item&& obj, bool force )
{
    if( !( force || can_mount ( dx, dy, id ) ) ) {
        return -1;
    }
    return install_part(dx, dy, vehicle_part( id, dx, dy, std::move( obj ) ) );
}

int vehicle::install_part( int dx, int dy, const vehicle_part &new_part )
{
    // Should be checked before installing the part
    bool enable = false;
    if( new_part.is_engine() ) {
        enable = true;
    } else {
        // @todo: read toggle groups from JSON
        static const std::vector<std::string> enable_like = {{
            "CONE_LIGHT",
            "CIRCLE_LIGHT",
            "AISLE_LIGHT",
            "DOME_LIGHT",
            "ATOMIC_LIGHT",
            "STEREO",
            "CHIMES",
            "FRIDGE",
            "RECHARGE",
            "PLOW",
            "REAPER",
            "PLANTER",
            "SCOOP",
            "WATER_PURIFIER",
            "ROCKWHEEL"
        }};

        for( const std::string &flag : enable_like ) {
            if( new_part.info().has_flag( flag ) ) {
                enable = has_part( flag, true );
                break;
            }
        }
    }

    parts.push_back( new_part );
    auto &pt = parts.back();

    pt.enabled = enable;

    pt.mount.x = dx;
    pt.mount.y = dy;

    refresh();
    return parts.size() - 1;
}

/**
 * Mark a part as removed from the vehicle.
 * @return bool true if the vehicle's 0,0 point shifted.
 */
bool vehicle::remove_part( int p )
{
    if( p >= (int)parts.size() ) {
        debugmsg("Tried to remove part %d but only %d parts!", p, parts.size());
        return false;
    }
    if( parts[p].removed ) {
        /* This happens only when we had to remove part, because it was depending on
         * other part (using recursive remove_part() call) - currently curtain
         * depending on presence of window and seatbelt depending on presence of seat.
         */
        return false;
    }

    int x = parts[p].precalc[0].x;
    int y = parts[p].precalc[0].y;
    tripoint part_loc( global_x() + x, global_y() + y, smz );

    // If `p` has flag `parent_flag`, remove child with flag `child_flag`
    // Returns true if removal occurs
    const auto remove_dependent_part = [&]( const std::string& parent_flag,
            const std::string& child_flag ) {
        if( part_flag( p, parent_flag ) ) {
            int dep = part_with_feature( p, child_flag, false );
            if( dep >= 0 ) {
                item it = parts[dep].properties_to_item();
                g->m.add_item_or_charges( part_loc, it );
                remove_part( dep );
                return true;
            }
        }
        return false;
    };

    // if a windshield is removed (usually destroyed) also remove curtains
    // attached to it.
    if( remove_dependent_part( "WINDOW", "CURTAIN" ) || part_flag( p, VPFLAG_OPAQUE ) ) {
        g->m.set_transparency_cache_dirty( smz );
    }

    remove_dependent_part( "SEAT", "SEATBELT" );
    remove_dependent_part( "BATTERY_MOUNT", "NEEDS_BATTERY_MOUNT" );

    // Unboard any entities standing on removed boardable parts
    if( part_flag( p, "BOARDABLE" ) ) {
        std::vector<int> bp = boarded_parts();
        for( auto &elem : bp ) {
            if( elem == p ) {
                g->m.unboard_vehicle( part_loc );
            }
        }
    }

    // Update current engine configuration if needed
    if( part_flag( p, "ENGINE" ) && engines.size() > 1 ) {
        bool any_engine_on = false;

        for( auto &e : engines ) {
            if( e != p && is_part_on( e ) ) {
                any_engine_on = true;
                break;
            }
        }

        if( !any_engine_on ) {
            engine_on = false;
            for( auto &e : engines ) {
                toggle_specific_part( e, true );
            }
        }
    }

    parts[p].removed = true;
    removed_part_count++;

    // If the player is currently working on the removed part, stop them as it's futile now.
    const player_activity &act = g->u.activity;
    if( act.id() == activity_id( "ACT_VEHICLE" ) && act.moves_left > 0 && act.values.size() > 6 ) {
        if( veh_pointer_or_null( g->m.veh_at( tripoint( act.values[0], act.values[1], g->u.posz() ) ) ) == this ) {
            if( act.values[6] >= p ) {
                g->u.cancel_activity();
                add_msg( m_info, _( "The vehicle part you were working on has gone!" ) );
            }
        }
    }

    const auto iter = labels.find( label( parts[p].mount.x, parts[p].mount.y ) );
    const bool no_label = iter != labels.end();
    const bool grab_found = g->u.grab_type == OBJECT_VEHICLE && g->u.grab_point == part_loc;
    // Checking these twice to avoid calling the relatively expensive parts_at_relative() unnecessarily.
    if( no_label || grab_found ) {
        if( parts_at_relative( parts[p].mount.x, parts[p].mount.y, false ).empty() ) {
            if( no_label ) {
                labels.erase( iter );
            }
            if( grab_found ) {
                add_msg( m_info, _( "The vehicle part you were holding has been destroyed!" ) );
                g->u.grab_type = OBJECT_NONE;
                g->u.grab_point = tripoint_zero;
            }
        }
    }

    for( auto &i : get_items( p ) ) {
        // Note: this can spawn items on the other side of the wall!
        tripoint dest( part_loc.x + rng( -3, 3 ), part_loc.y + rng( -3, 3 ), smz );
        g->m.add_item_or_charges( dest, i );
    }
    g->m.dirty_vehicle_list.insert( this );
    refresh();
    return shift_if_needed();
}

void vehicle::part_removal_cleanup() {
    bool changed = false;
    for( std::vector<vehicle_part>::iterator it = parts.begin(); it != parts.end(); /* noop */ ) {
        if( it->removed ) {
            auto items = get_items( std::distance( parts.begin(), it ) );
            while( !items.empty() ) {
                items.erase( items.begin() );
            }
            it = parts.erase( it );
            changed = true;
        }
        else {
            ++it;
        }
    }
    removed_part_count = 0;
    if( changed || parts.empty() ) {
        refresh();
        if( parts.empty() ) {
            g->m.destroy_vehicle( this );
            return;
        } else {
            g->m.update_vehicle_cache( this, smz );
        }
    }
    shift_if_needed();
    refresh(); // Rebuild cached indices
}

item_location vehicle::part_base( int p )
{
    return item_location( vehicle_cursor( *this, p ), &parts[ p ].base );
}

int vehicle::find_part( const item& it ) const
{
    auto idx = std::find_if( parts.begin(), parts.end(), [&it]( const vehicle_part& e ) {
        return &e.base == &it;
    } );
    return idx != parts.end() ? std::distance( parts.begin(), idx ) : INT_MIN;
}

/**
 * Breaks the specified part into the pieces defined by its breaks_into entry.
 * @param p The index of the part to break.
 * @param x The map x-coordinate to place pieces at (give or take).
 * @param y The map y-coordinate to place pieces at (give or take).
 * @param scatter If true, pieces are scattered near the target square.
 */
void vehicle::break_part_into_pieces(int p, int x, int y, bool scatter) {
    const std::string& group = part_info(p).breaks_into_group;
    if( group.empty() ) {
        return;
    }
    for( item& piece : item_group::items_from( group, calendar::turn ) ) {
        // TODO: balance audit, ensure that less pieces are generated than one would need
        // to build the component (smash a vehicle box that took 10 lumps of steel,
        // find 12 steel lumps scattered after atom-smashing it with a tree trunk)
            const int actual_x = scatter ? x + rng(-SCATTER_DISTANCE, SCATTER_DISTANCE) : x;
            const int actual_y = scatter ? y + rng(-SCATTER_DISTANCE, SCATTER_DISTANCE) : y;
            tripoint dest( actual_x, actual_y, smz );
            g->m.add_item_or_charges( dest, piece );
    }
}

std::vector<int> vehicle::parts_at_relative (const int dx, const int dy, bool const use_cache) const
{
    if( !use_cache ) {
        std::vector<int> res;
        for (size_t i = 0; i < parts.size(); i++) {
            if (parts[i].mount.x == dx && parts[i].mount.y == dy && !parts[i].removed) {
                res.push_back ((int)i);
            }
        }
        return res;
    } else {
        const auto &iter = relative_parts.find( point( dx, dy ) );
        if ( iter != relative_parts.end() ) {
            return iter->second;
        } else {
            std::vector<int> res;
            return res;
        }
    }
}

cata::optional<vpart_reference> vpart_position::obstacle_at_part() const
{
    const cata::optional<vpart_reference> part = part_with_feature( VPFLAG_OBSTACLE, true );
    if( !part ) {
        return cata::nullopt; // No obstacle here
    }

    const ::vehicle &v = vehicle();
    if( v.part_flag( part->part_index(), VPFLAG_OPENABLE ) && v.parts[part->part_index()].open ) {
        return cata::nullopt; // Open door here
    }

    return part;
}

cata::optional<vpart_reference> vpart_position::part_with_feature( const std::string &f, const bool unbroken ) const
{
    const int i = vehicle().part_with_feature( part_index(), f, unbroken );
    if( i < 0 ) {
        return cata::nullopt;
    }
    return vpart_reference( vehicle(), i );
}

cata::optional<vpart_reference> vpart_position::part_with_feature( const vpart_bitflags f, const bool unbroken ) const
{
    const int i = vehicle().part_with_feature( part_index(), f, unbroken );
    if( i < 0 ) {
        return cata::nullopt;
    }
    return vpart_reference( vehicle(), i );
}

cata::optional<vpart_reference> optional_vpart_position::part_with_feature( const std::string &f,
        const bool unbroken ) const
{
    return has_value() ? value().part_with_feature( f, unbroken ) : cata::nullopt;
}

cata::optional<vpart_reference> optional_vpart_position::part_with_feature( const vpart_bitflags f,
        const bool unbroken ) const
{
    return has_value() ? value().part_with_feature( f, unbroken ) : cata::nullopt;
}

cata::optional<vpart_reference> optional_vpart_position::obstacle_at_part() const
{
    return has_value() ? value().obstacle_at_part() : cata::nullopt;
}

int vehicle::part_with_feature (int part, vpart_bitflags const flag, bool unbroken) const
{
    if (part_flag(part, flag)) {
        return part;
    }
    const auto it = relative_parts.find( parts[part].mount );
    if ( it != relative_parts.end() ) {
        const std::vector<int> & parts_here = it->second;
        for (auto &i : parts_here) {
            if( part_flag( i, flag ) && ( !unbroken || !parts[i].is_broken() ) ) {
                return i;
            }
        }
    }
    return -1;
}

int vehicle::part_with_feature (int part, const std::string &flag, bool unbroken) const
{
    return part_with_feature_at_relative(parts[part].mount, flag, unbroken);
}

int vehicle::part_with_feature_at_relative (const point &pt, const std::string &flag, bool unbroken) const
{
    std::vector<int> parts_here = parts_at_relative(pt.x, pt.y, false);
    for( auto &elem : parts_here ) {
        if( part_flag( elem, flag ) && ( !unbroken || !parts[ elem ].is_broken() ) ) {
            return elem;
        }
    }
    return -1;
}

bool vehicle::has_part( const std::string &flag, bool enabled ) const
{
    return std::any_of( parts.begin(), parts.end(), [&flag,&enabled]( const vehicle_part &e ) {
        return !e.removed && ( !enabled || e.enabled ) && !e.is_broken() && e.info().has_flag( flag );
    } );
}

bool vehicle::has_part( const tripoint &pos, const std::string &flag, bool enabled ) const
{
    auto px = pos.x - global_x();
    auto py = pos.y - global_y();

    for( const auto &e : parts ) {
        if( e.precalc[0].x != px || e.precalc[0].y != py ) {
            continue;
        }
        if( !e.removed && ( !enabled || e.enabled ) && !e.is_broken() && e.info().has_flag( flag ) ) {
            return true;
        }
    }
    return false;
}

// All 4 functions below look identical except for flag type and consts
template<typename Vehicle, typename Flag, typename Vector>
void get_parts_helper( Vehicle &veh, const Flag &flag, Vector &ret, bool enabled )
{
    for( auto &e : veh.parts ) {
        if( !e.removed && ( !enabled || e.enabled ) && !e.is_broken() && e.info().has_flag( flag ) ) {
            ret.emplace_back( &e );
        }
    }
}

std::vector<vehicle_part *> vehicle::get_parts( const std::string &flag, bool enabled )
{
    std::vector<vehicle_part *> res;
    get_parts_helper( *this, flag, res, enabled );
    return res;
}

std::vector<const vehicle_part *> vehicle::get_parts( const std::string &flag, bool enabled ) const
{
    std::vector<const vehicle_part *> res;
    get_parts_helper( *this, flag, res, enabled );
    return res;
}

std::vector<vehicle_part *> vehicle::get_parts( vpart_bitflags flag, bool enabled )
{
    std::vector<vehicle_part *> res;
    get_parts_helper( *this, flag, res, enabled );
    return res;
}

std::vector<const vehicle_part *> vehicle::get_parts( vpart_bitflags flag, bool enabled ) const
{
    std::vector<const vehicle_part *> res;
    get_parts_helper( *this, flag, res, enabled );
    return res;
}

std::vector<vehicle_part *> vehicle::get_parts( const tripoint &pos, const std::string &flag, bool enabled )
{
    std::vector<vehicle_part *> res;
    for( auto &e : parts ) {
        if( e.precalc[ 0 ].x != pos.x - global_x() ||
            e.precalc[ 0 ].y != pos.y - global_y() ) {
            continue;
        }
        if( !e.removed && ( !enabled || e.enabled ) && !e.is_broken() && ( flag.empty() || e.info().has_flag( flag ) ) ) {
            res.push_back( &e );
        }
    }
    return res;
}

std::vector<const vehicle_part *> vehicle::get_parts( const tripoint &pos, const std::string &flag, bool enabled ) const
{
    std::vector<const vehicle_part *> res;
    for( const auto &e : parts ) {
        if( e.precalc[ 0 ].x != pos.x - global_x() ||
            e.precalc[ 0 ].y != pos.y - global_y() ) {
            continue;
        }
        if( !e.removed && ( !enabled || e.enabled ) && !e.is_broken() && ( flag.empty() || e.info().has_flag( flag ) ) ) {
            res.push_back( &e );
        }
    }
    return res;
}

bool vehicle::can_enable( const vehicle_part &pt, bool alert ) const
{
    if( std::none_of( parts.begin(), parts.end(), [&pt]( const vehicle_part &e ) { return &e == &pt; } ) || pt.removed ) {
        debugmsg( "Cannot enable removed or non-existent part" );
    }

    if( pt.is_broken() ) {
        return false;
    }

    if( pt.info().has_flag( "PLANTER" ) && !warm_enough_to_plant() ) {
        if( alert ) {
            add_msg( m_bad, _( "It is too cold to plant anything now." ) );
        }
        return false;
    }

    // @todo: check fuel for combustion engines

    if( pt.info().epower < 0 && fuel_left( fuel_type_battery, true ) <= 0 ) {
        if( alert ) {
            add_msg( m_bad, _( "Insufficient power to enable %s" ), pt.name().c_str() );
        }
        return false;
    }

    return true;
}

cata::optional<std::string> vpart_position::get_label() const
{
    const vehicle_part &part = vehicle().parts[part_index()];
    const auto it = vehicle().labels.find( label( part.mount.x, part.mount.y ) );
    if( it == vehicle().labels.end() ) {
        return cata::nullopt;
    }
    if( it->text.empty() ) {
        // legacy support @todo change labels into a map and keep track of deleted labels
        return cata::nullopt;
    }
    return it->text;
}

void vpart_position::set_label( const std::string &text ) const
{
    auto &labels = vehicle().labels;
    const vehicle_part &part = vehicle().parts[part_index()];
    const auto it = labels.find( label( part.mount.x, part.mount.y ) );
    //@todo empty text should remove the label instead of just storing an empty string, see get_label
    if( it == labels.end() ) {
        labels.insert( label( part.mount.x, part.mount.y, text ) );
    } else {
        // labels should really be a map
        labels.insert( labels.erase( it ), label( part.mount.x, part.mount.y, text ) );
    }
}

int vehicle::next_part_to_close( int p, bool outside ) const
{
    std::vector<int> parts_here = parts_at_relative(parts[p].mount.x, parts[p].mount.y);

    // We want reverse, since we close the outermost thing first (curtains), and then the innermost thing (door)
    for(std::vector<int>::reverse_iterator part_it = parts_here.rbegin();
        part_it != parts_here.rend();
        ++part_it)
    {

        if(part_flag(*part_it, VPFLAG_OPENABLE)
           && !parts[ *part_it ].is_broken()
           && parts[*part_it].open == 1
           && (!outside || !part_flag(*part_it, "OPENCLOSE_INSIDE")) )
        {
            return *part_it;
        }
    }
    return -1;
}

int vehicle::next_part_to_open(int p, bool outside) const
{
    std::vector<int> parts_here = parts_at_relative(parts[p].mount.x, parts[p].mount.y);

    // We want forwards, since we open the innermost thing first (curtains), and then the innermost thing (door)
    for( auto &elem : parts_here ) {
        if( part_flag( elem, VPFLAG_OPENABLE ) && !parts[ elem ].is_broken() && parts[elem].open == 0 &&
            ( !outside || !part_flag( elem, "OPENCLOSE_INSIDE" ) ) ) {
            return elem;
        }
    }
    return -1;
}

/**
 * Returns all parts in the vehicle with the given flag, optionally checking
 * to only return unbroken parts.
 * If performance becomes an issue, certain lists (such as wheels) could be
 * cached and fast-returned here, but this is currently linear-time with
 * respect to the number of parts in the vehicle.
 * @param feature The flag (such as "WHEEL" or "CONE_LIGHT") to find.
 * @param unbroken true if only unbroken parts should be returned, false to
 *        return all matching parts.
 * @return A list of indices to all the parts with the specified feature.
 */
std::vector<int> vehicle::all_parts_with_feature(const std::string& feature, bool const unbroken) const
{
    std::vector<int> parts_found;
    for( size_t part_index = 0; part_index < parts.size(); ++part_index ) {
        if(part_info(part_index).has_flag(feature) &&
                ( !unbroken || !parts[ part_index ].is_broken() ) ) {
            parts_found.push_back(part_index);
        }
    }
    return parts_found;
}

std::vector<int> vehicle::all_parts_with_feature(vpart_bitflags feature, bool const unbroken) const
{
    std::vector<int> parts_found;
    for( size_t part_index = 0; part_index < parts.size(); ++part_index ) {
        if(part_info(part_index).has_flag(feature) &&
                ( !unbroken || !parts[ part_index ].is_broken() ) ) {
            parts_found.push_back(part_index);
        }
    }
    return parts_found;
}

/**
 * Returns all parts in the vehicle that exist in the given location slot. If
 * the empty string is passed in, returns all parts with no slot.
 * @param location The location slot to get parts for.
 * @return A list of indices to all parts with the specified location.
 */
std::vector<int> vehicle::all_parts_at_location(const std::string& location) const
{
    std::vector<int> parts_found;
    for( size_t part_index = 0; part_index < parts.size(); ++part_index ) {
        if(part_info(part_index).location == location && !parts[part_index].removed) {
            parts_found.push_back(part_index);
        }
    }
    return parts_found;
}

bool vehicle::part_flag (int part, const std::string &flag) const
{
    if (part < 0 || part >= (int)parts.size() || parts[part].removed) {
        return false;
    } else {
        return part_info(part).has_flag(flag);
    }
}

bool vehicle::part_flag( int part, const vpart_bitflags flag) const
{
   if (part < 0 || part >= (int)parts.size() || parts[part].removed) {
        return false;
    } else {
        return part_info(part).has_flag(flag);
    }
}

int vehicle::part_at(int const dx, int const dy) const
{
    for (size_t p = 0; p < parts.size(); p++) {
        if (parts[p].precalc[0].x == dx && parts[p].precalc[0].y == dy && !parts[p].removed) {
            return (int)p;
        }
    }
    return -1;
}

int vehicle::global_part_at(int const x, int const y) const
{
    return part_at(x - global_x(), y - global_y());
}

/**
 * Given a vehicle part which is inside of this vehicle, returns the index of
 * that part. This exists solely because activities relating to vehicle editing
 * require the index of the vehicle part to be passed around.
 * @param part The part to find.
 * @param check_removed Check whether this part can be removed
 * @return The part index, -1 if it is not part of this vehicle.
 */
int vehicle::index_of_part(const vehicle_part *const part, bool const check_removed) const
{
  if(part != NULL) {
    for( size_t index = 0; index < parts.size(); ++index ) {
      // @note Doesn't this have a bunch of copy overhead?
      vehicle_part next_part = parts[index];
      if (!check_removed && next_part.removed) {
        continue;
      }
      if( part->id == next_part.id && part->mount == next_part.mount ) {
        return index;
      }
    }
  }
  return -1;
}

/**
 * Returns which part (as an index into the parts list) is the one that will be
 * displayed for the given square. Returns -1 if there are no parts in that
 * square.
 * @param local_x The local x-coordinate.
 * @param local_y The local y-coordinate.
 * @return The index of the part that will be displayed.
 */
int vehicle::part_displayed_at(int const local_x, int const local_y) const
{
    // Z-order is implicitly defined in game::load_vehiclepart, but as
    // numbers directly set on parts rather than constants that can be
    // used elsewhere. A future refactor might be nice but this way
    // it's clear where the magic number comes from.
    const int ON_ROOF_Z = 9;

    std::vector<int> parts_in_square = parts_at_relative(local_x, local_y);

    if(parts_in_square.empty()) {
        return -1;
    }

    bool in_vehicle = g->u.in_vehicle;
    if (in_vehicle) {
        // They're in a vehicle, but are they in /this/ vehicle?
        std::vector<int> psg_parts = boarded_parts();
        in_vehicle = false;
        for( auto &psg_part : psg_parts ) {
            if( get_passenger( psg_part ) == &( g->u ) ) {
                in_vehicle = true;
                break;
            }
        }
    }

    int hide_z_at_or_above = (in_vehicle) ? (ON_ROOF_Z) : INT_MAX;

    int top_part = 0;
    for(size_t index = 1; index < parts_in_square.size(); index++) {
        if((part_info(parts_in_square[top_part]).z_order <
            part_info(parts_in_square[index]).z_order) &&
           (part_info(parts_in_square[index]).z_order <
            hide_z_at_or_above)) {
            top_part = index;
        }
    }

    return parts_in_square[top_part];
}

int vehicle::roof_at_part( const int part ) const
{
    std::vector<int> parts_in_square = parts_at_relative( parts[part].mount.x, parts[part].mount.y );
    for( const int p : parts_in_square ) {
        if( part_info( p ).location == "on_roof" || part_flag( p, "ROOF" ) ) {
            return p;
        }
    }

    return -1;
}

char vehicle::part_sym( const int p, const bool exact ) const
{
    if (p < 0 || p >= (int)parts.size() || parts[p].removed) {
        return ' ';
    }

    const int displayed_part = exact ? p : part_displayed_at(parts[p].mount.x, parts[p].mount.y);

    if (part_flag (displayed_part, VPFLAG_OPENABLE) && parts[displayed_part].open) {
        return '\''; // open door
    } else {
        return parts[ displayed_part ].is_broken() ?
            part_info(displayed_part).sym_broken : part_info(displayed_part).sym;
    }
}

// similar to part_sym(int p) but for use when drawing SDL tiles. Called only by cata_tiles during draw_vpart
// vector returns at least 1 element, max of 2 elements. If 2 elements the second denotes if it is open or damaged
vpart_id vehicle::part_id_string(int const p, char &part_mod) const
{
    part_mod = 0;
    if( p < 0 || p >= (int)parts.size() || parts[p].removed ) {
        return vpart_id::NULL_ID();
    }

    int displayed_part = part_displayed_at(parts[p].mount.x, parts[p].mount.y);
    const vpart_id idinfo = parts[displayed_part].id;

    if (part_flag (displayed_part, VPFLAG_OPENABLE) && parts[displayed_part].open) {
        part_mod = 1; // open
    } else if( parts[ displayed_part ].is_broken() ){
        part_mod = 2; // broken
    }

    return idinfo;
}

nc_color vehicle::part_color( const int p, const bool exact ) const
{
    if (p < 0 || p >= (int)parts.size()) {
        return c_black;
    }

    nc_color col;

    int parm = -1;

    //If armoring is present and the option is set, it colors the visible part
    if( get_option<bool>( "VEHICLE_ARMOR_COLOR" ) ) {
        parm = part_with_feature(p, VPFLAG_ARMOR, false);
    }

    if( parm >= 0 ) {
        col = part_info(parm).color;
    } else {
        const int displayed_part = exact ? p : part_displayed_at(parts[p].mount.x, parts[p].mount.y);

        if (displayed_part < 0 || displayed_part >= (int)parts.size()) {
            return c_black;
        }
        if (parts[displayed_part].blood > 200) {
            col = c_red;
        } else if (parts[displayed_part].blood > 0) {
            col = c_light_red;
        } else if (parts[displayed_part].is_broken()) {
            col = part_info(displayed_part).color_broken;
        } else {
            col = part_info(displayed_part).color;
        }

    }

    if( exact ) {
        return col;
    }

    // curtains turn windshields gray
    int curtains = part_with_feature(p, VPFLAG_CURTAIN, false);
    if (curtains >= 0) {
        if (part_with_feature(p, VPFLAG_WINDOW, true) >= 0 && !parts[curtains].open)
            col = part_info(curtains).color;
    }

    //Invert colors for cargo parts with stuff in them
    int cargo_part = part_with_feature(p, VPFLAG_CARGO);
    if(cargo_part > 0 && !get_items(cargo_part).empty()) {
        return invert_color(col);
    } else {
        return col;
    }
}

/**
 * Prints a list of all parts to the screen inside of a boxed window, possibly
 * highlighting a selected one.
 * @param win The window to draw in.
 * @param y1 The y-coordinate to start drawing at.
 * @param max_y Draw no further than this y-coordinate.
 * @param width The width of the window.
 * @param p The index of the part being examined.
 * @param hl The index of the part to highlight (if any).
 */
int vehicle::print_part_list( const catacurses::window &win, int y1, const int max_y, int width,
                              int p, int hl /*= -1*/ ) const
{
    if( p < 0 || p >= ( int )parts.size() ) {
        return y1;
    }
    std::vector<int> pl = this->parts_at_relative( parts[p].mount.x, parts[p].mount.y );
    int y = y1;
    for( size_t i = 0; i < pl.size(); i++ ) {
        if( y >= max_y ) {
            mvwprintz( win, y, 1, c_yellow, _( "More parts here..." ) );
            ++y;
            break;
        }

        const vehicle_part &vp = parts[ pl [ i ] ];
        nc_color col_cond = vp.is_broken() ? c_dark_gray : vp.base.damage_color();

        std::string partname = vp.name();

        if( vp.is_tank() && vp.ammo_current() != "null" ) {
            partname += string_format( " (%s)", item::nname( vp.ammo_current() ).c_str() );
        }

        if( part_flag( pl[i], "CARGO" ) ) {
            //~ used/total volume of a cargo vehicle part
            partname += string_format( _( " (vol: %s/%s %s)" ),
                                       format_volume( stored_volume( pl[i] ) ).c_str(),
                                       format_volume( max_volume( pl[i] ) ).c_str(),
                                       volume_units_abbr() );
        }

        bool armor = part_flag( pl[i], "ARMOR" );
        std::string left_sym;
        std::string right_sym;
        if( armor ) {
            left_sym = "(";
            right_sym = ")";
        } else if( part_info( pl[i] ).location == part_location_structure ) {
            left_sym = "[";
            right_sym = "]";
        } else {
            left_sym = "-";
            right_sym = "-";
        }
        nc_color sym_color = ( int )i == hl ? hilite( c_light_gray ) : c_light_gray;
        mvwprintz( win, y, 1, sym_color, left_sym );
        trim_and_print( win, y, 2, getmaxx( win ) - 4,
                        ( int )i == hl ? hilite( col_cond ) : col_cond, partname );
        wprintz( win, sym_color, right_sym );

        if( i == 0 && vpart_position( const_cast<vehicle &>( *this ), pl[i] ).is_inside() ) {
            //~ indicates that a vehicle part is inside
            mvwprintz( win, y, width - 2 - utf8_width( _( "Interior" ) ), c_light_gray, _( "Interior" ) );
        } else if( i == 0 ) {
            //~ indicates that a vehicle part is outside
            mvwprintz( win, y, width - 2 - utf8_width( _( "Exterior" ) ), c_light_gray, _( "Exterior" ) );
        }
        y++;
    }

    // print the label for this location
    const cata::optional<std::string> label = vpart_position( const_cast<vehicle &>( *this ),
            p ).get_label();
    if( label && y <= max_y ) {
        mvwprintz( win, y++, 1, c_light_red, _( "Label: %s" ), label->c_str() );
    }

    return y;
}

/**
 * Prints a list of descriptions for all parts to the screen inside of a boxed window
 * @param win The window to draw in.
 * @param max_y Draw no further than this y-coordinate.
 * @param width The width of the window.
 * @param &p The index of the part being examined.
 * @param start_at Which vehicle part to start printing at.
 * @param start_limit the part index beyond which the display is full
 */
void vehicle::print_vparts_descs( const catacurses::window &win, int max_y, int width, int &p,
                                  int &start_at, int &start_limit ) const
{
    if( p < 0 || p >= ( int )parts.size() ) {
        return;
    }

    std::vector<int> pl = this->parts_at_relative( parts[p].mount.x, parts[p].mount.y );
    std::ostringstream msg;

    int lines = 0;
    /*
     * start_at and start_limit interaction is little tricky
     * start_at and start_limit start at 0 when moving to a new frame
     * if all the descriptions are displayed in the window, start_limit stays at 0 and
     *    start_at is capped at 0 - so no scrolling at all.
     * if all the descriptions aren't displayed, start_limit jumps to the last displayed part
     *    and the next scrollthrough can start there - so scrolling down happens.
     * when the scroll reaches the point where all the remaining descriptions are displayed in
     *    the window, start_limit is set to start_at again.
     * on the next attempted scrolldown, start_limit is set to the nth item, and start_at is
     *    capped to the nth item, so no more scrolling down.
     * start_at can always go down, but never below 0, so scrolling up is only possible after
     *    some scrolling down has occurred.
     * important! the calling function needs to track p, start_at, and start_limit, and set
     *    start_limit to 0 if p changes.
     */
    start_at = std::max( 0, std::min( start_at, start_limit ) );
    if( start_at ) {
           msg << "<color_yellow>" << "<  " << _( "More parts here..." ) << "</color>\n";
           lines += 1;
    }
    for( size_t i = start_at; i < pl.size(); i++ ) {
        const vehicle_part &vp = parts[ pl [ i ] ];
        std::ostringstream possible_msg;
        std::string name_color = string_format( "<color_%1$s>",
                                                string_from_color( vp.is_broken() ? c_dark_gray : c_light_green ) );
        possible_msg << name_color << vp.name() << "</color>\n";
        std::string desc_color = string_format( "<color_%1$s>",
                                                string_from_color( vp.is_broken() ? c_dark_gray : c_light_gray ) );
        int new_lines = 2 + vp.info().format_description( possible_msg, desc_color, width - 2 );
        possible_msg << "</color>\n";
        if( lines + new_lines <= max_y ) {
           msg << possible_msg.str();
           lines += new_lines;
	   start_limit = start_at;
        } else {
           msg << "<color_yellow>" << _( "More parts here..." ) << "  >" << "</color>\n";
           start_limit = i;
           break;
        }
    }
    werase( win );
    fold_and_print( win, 0, 1, width, c_light_gray, msg.str() );
    wrefresh( win );
}

/**
 * Returns an array of fuel types that can be printed
 * @return An array of printable fuel type ids
 */
std::vector<itype_id> vehicle::get_printable_fuel_types() const
{
    std::set<itype_id> opts;
    for( const auto &pt : parts ) {
        if( ( pt.is_tank() || pt.is_battery() || pt.is_reactor() ) && pt.ammo_current() != "null" ) {
            opts.emplace( pt.ammo_current() );
        }
    }

    std::vector<itype_id> res( opts.begin(), opts.end() );

    std::sort( res.begin(), res.end(), [&]( const itype_id &lhs, const itype_id &rhs ) {
        return basic_consumption( rhs ) < basic_consumption( lhs );
    } );

    return res;
}

/**
 * Prints all of the fuel indicators of the vehicle
 * @param win Pointer to the window to draw in.
 * @param y Y location to draw at.
 * @param x X location to draw at.
 * @param start_index Starting index in array of fuel gauges to start reading from
 * @param fullsize true if it's expected to print multiple rows
 * @param verbose true if there should be anything after the gauge (either the %, or number)
 * @param desc true if the name of the fuel should be at the end
 * @param isHorizontal true if the menu is not vertical
 */
void vehicle::print_fuel_indicators( const catacurses::window &win, int y, int x, int start_index, bool fullsize, bool verbose, bool desc, bool isHorizontal ) const
{
    auto fuels = get_printable_fuel_types();

    if( !fullsize ) {
        if( !fuels.empty() ) {
            print_fuel_indicator( win, y, x, fuels.front(), verbose, desc );
        }
        return;
    }

    int yofs = 0;
    int max_gauge = ((isHorizontal) ? 12 : 5) + start_index;
    int max_size = std::min((int)fuels.size(), max_gauge);

    for( int i = start_index; i < max_size; i++ ) {
        const itype_id &f = fuels[i];
        print_fuel_indicator( win, y + yofs, x, f, verbose, desc );
        yofs++;
    }

    // check if the current index is less than the max size minus 12 or 5, to indicate that there's more
    if((start_index < (int)fuels.size() -  ((isHorizontal) ? 12 : 5)) ) {
        mvwprintz( win, y + yofs, x, c_light_green, ">" );
        wprintz( win, c_light_gray, " for more" );
    }
}

/**
 * Prints a fuel gauge for a vehicle
 * @param w Pointer to the window to draw in.
 * @param y Y location to draw at.
 * @param x X location to draw at.
 * @param fuel_type ID of the fuel type to draw
 * @param verbose true if there should be anything after the gauge (either the %, or number)
 * @param desc true if the name of the fuel should be at the end
 */
void vehicle::print_fuel_indicator( const catacurses::window &win, int y, int x, const itype_id &fuel_type, bool verbose, bool desc ) const
{
    const char fsyms[5] = { 'E', '\\', '|', '/', 'F' };
    nc_color col_indf1 = c_light_gray;
    int cap = fuel_capacity( fuel_type );
    int f_left = fuel_left( fuel_type );
    nc_color f_color = item::find_type( fuel_type )->color;
    mvwprintz(win, y, x, col_indf1, "E...F");
    int amnt = cap > 0 ? f_left * 99 / cap : 0;
    int indf = (amnt / 20) % 5;
    mvwprintz( win, y, x + indf, f_color, "%c", fsyms[indf] );
    if (verbose) {
        if( debug_mode ) {
            mvwprintz( win, y, x + 6, f_color, "%d/%d", f_left, cap );
        } else {
            mvwprintz( win, y, x + 6, f_color, "%d", (f_left * 100) / cap );
            wprintz( win, c_light_gray, "%c", 045 );
        }
    }
    if (desc) {
        wprintz(win, c_light_gray, " - %s", item::nname( fuel_type ).c_str() );
    }
}

point vehicle::coord_translate (const point &p) const
{
    point q;
    coord_translate(pivot_rotation[0], pivot_anchor[0], p, q);
    return q;
}

void vehicle::coord_translate (int dir, const point &pivot, const point &p, point &q) const
{
    tileray tdir (dir);
    tdir.advance (p.x - pivot.x);
    q.x = tdir.dx() + tdir.ortho_dx(p.y - pivot.y);
    q.y = tdir.dy() + tdir.ortho_dy(p.y - pivot.y);
}

void vehicle::precalc_mounts (int idir, int dir, const point &pivot)
{
    if (idir < 0 || idir > 1)
        idir = 0;
    for (auto &p : parts)
    {
        if (p.removed) {
            continue;
        }
        coord_translate (dir, pivot, p.mount, p.precalc[idir]);
    }
    pivot_anchor[idir] = pivot;
    pivot_rotation[idir] = dir;
}

std::vector<int> vehicle::boarded_parts() const
{
    std::vector<int> res;
    for (size_t p = 0; p < parts.size(); p++) {
        if (part_flag (p, VPFLAG_BOARDABLE) &&
                parts[p].has_flag(vehicle_part::passenger_flag)) {
            res.push_back ((int)p);
        }
    }
    return res;
}

player *vehicle::get_passenger(int p) const
{
    p = part_with_feature (p, VPFLAG_BOARDABLE, false);
    if (p >= 0 && parts[p].has_flag(vehicle_part::passenger_flag))
    {
        return g->critter_by_id<player>( parts[p].passenger_id );
    }
    return 0;
}

int vehicle::global_x() const
{
    return smx * SEEX + posx;
}

int vehicle::global_y() const
{
    return smy * SEEY + posy;
}

point vehicle::global_pos() const
{
    return point( smx * SEEX + posx, smy * SEEY + posy );
}

tripoint vehicle::global_pos3() const
{
    return tripoint( smx * SEEX + posx, smy * SEEY + posy, smz );
}

tripoint vehicle::global_part_pos3( const int &index ) const
{
    return global_part_pos3( parts[ index ] );
}

tripoint vehicle::global_part_pos3( const vehicle_part &pt ) const
{
    return global_pos3() + pt.precalc[ 0 ];
}

point vehicle::real_global_pos() const
{
    return g->m.getabs( global_x(), global_y() );
}

tripoint vehicle::real_global_pos3() const
{
    return g->m.getabs( tripoint( global_x(), global_y(), smz ) );
}

void vehicle::set_submap_moved( int x, int y )
{
    const point old_msp = real_global_pos();
    smx = x;
    smy = y;
    if( !tracking_on ) {
        return;
    }
    overmap_buffer.move_vehicle( this, old_msp );
}

units::mass vehicle::total_mass() const
{
    if( mass_dirty ) {
        refresh_mass();
    }

    return mass_cache;
}

units::volume vehicle::total_folded_volume() const
{
    units::volume m = 0;
    for( size_t i = 0; i < parts.size(); i++ ) {
        if( parts[i].removed ) {
            continue;
        }
        m += part_info(i).folded_volume;
    }
    return m;
}

const point &vehicle::rotated_center_of_mass() const
{
    // @todo: Bring back caching of this point
    calc_mass_center( true );

    return mass_center_precalc;
}

const point &vehicle::local_center_of_mass() const
{
    if( mass_center_no_precalc_dirty ) {
        calc_mass_center( false );
    }

    return mass_center_no_precalc;
}

point vehicle::pivot_displacement() const
{
    // precalc_mounts always produces a result that puts the pivot point at (0,0).
    // If the pivot point changes, this artificially moves the vehicle, as the position
    // of the old pivot point will appear to move from (posx+0, posy+0) to some other point
    // (posx+dx,posy+dy) even if there is no change in vehicle position or rotation.
    // This method finds that movement so it can be canceled out when actually moving
    // the vehicle.

    // rotate the old pivot point around the new pivot point with the old rotation angle
    point dp;
    coord_translate(pivot_rotation[0], pivot_anchor[1], pivot_anchor[0], dp);
    return dp;
}

int vehicle::fuel_left (const itype_id & ftype, bool recurse) const
{
    int fl = std::accumulate( parts.begin(), parts.end(), 0, [&ftype]( const int &lhs, const vehicle_part &rhs ) {
        return lhs + ( rhs.ammo_current() == ftype ? rhs.ammo_remaining() : 0 );
    } );

    if(recurse && ftype == fuel_type_battery) {
        auto fuel_counting_visitor = [&] (vehicle const* veh, int amount, int) {
            return amount + veh->fuel_left(ftype, false);
        };

        // HAX: add 1 to the initial amount so traversal doesn't immediately stop just
        // 'cause we have 0 fuel left in the current vehicle. Subtract the 1 immediately
        // after traversal.
        fl = traverse_vehicle_graph(this, fl + 1, fuel_counting_visitor) - 1;
    }

    //muscle engines have infinite fuel
    if (ftype == fuel_type_muscle) {
        // @todo: Allow NPCs to power those
        const optional_vpart_position vp = g->m.veh_at( g->u.pos() );
        bool player_controlling = player_in_control(g->u);

        //if the engine in the player tile is a muscle engine, and player is controlling vehicle
        if( vp && &vp->vehicle() == this && player_controlling ) {
            const int p = part_with_feature( vp->part_index(), VPFLAG_ENGINE );
            if( p >= 0 && part_info(p).fuel_type == fuel_type_muscle && is_part_on( p ) ) {
                fl += 10;
            }
        }
    }

    return fl;
}

int vehicle::fuel_capacity (const itype_id &ftype) const
{
    return std::accumulate( parts.begin(), parts.end(), 0, [&ftype]( const int &lhs, const vehicle_part &rhs ) {
        return lhs + ( rhs.ammo_current() == ftype ? rhs.ammo_capacity() : 0 );
    } );
}

int vehicle::drain( const itype_id & ftype, int amount )
{
    if( ftype == fuel_type_battery ) {
        // Batteries get special handling to take advantage of jumper
        // cables -- discharge_battery knows how to recurse properly
        // (including taking cable power loss into account).
        int remnant = discharge_battery(amount, true);

        // discharge_battery returns amount of charges that were not
        // found anywhere in the power network, whereas this function
        // returns amount of charges consumed; simple subtraction.
        return amount - remnant;
    }

    int drained = 0;
    for( auto &p : parts ) {
        if( amount <= 0 ) {
            break;
        }
        if( p.ammo_current() == ftype ) {
            int qty = p.ammo_consume( amount, global_part_pos3( p ) );
            drained += qty;
            amount -= qty;
        }
    }

    invalidate_mass();
    return drained;
}

int vehicle::basic_consumption(const itype_id &ftype) const
{
    int fcon = 0;
    for( size_t e = 0; e < engines.size(); ++e ) {
        if(is_engine_type_on(e, ftype)) {
            if( part_info( engines[e] ).fuel_type == fuel_type_battery &&
                part_epower( engines[e] ) >= 0 ) {
                // Electric engine - use epower instead
                fcon -= epower_to_power( part_epower( engines[e] ) );

            } else if( !is_engine_type( e, fuel_type_muscle ) ) {
                fcon += part_power( engines[e] );
                if( parts[ e ].faults().count( fault_filter_air ) ) {
                    fcon *= 2;
                }
            }
        }
    }
    return fcon;
}

int vehicle::total_power(bool const fueled) const
{
    int pwr = 0;
    int cnt = 0;

    for (size_t e = 0; e < engines.size(); e++) {
        int p = engines[e];
        if (is_engine_on(e) && (fuel_left (part_info(p).fuel_type) || !fueled)) {
            pwr += part_power(p);
            cnt++;
        }
    }

    for (size_t a = 0; a < alternators.size();a++){
        int p = alternators[a];
        if (is_alternator_on(a)) {
            pwr += part_power(p); // alternators have negative power
        }
    }
    if (cnt > 1) {
        pwr = pwr * 4 / (4 + cnt -1);
    }
    return pwr;
}

int vehicle::acceleration(bool const fueled) const
{
    if( ( engine_on && has_engine_type_not( fuel_type_muscle, true ) ) || skidding ) {
        return safe_velocity( fueled ) * k_mass() / ( 1 + strain () ) / 10;

    } else if ((has_engine_type(fuel_type_muscle, true))){
        //limit vehicle weight for muscle engines
        const units::mass mass = total_mass() / 1000;
        ///\EFFECT_STR caps vehicle weight for muscle engines
        const units::mass move_mass = std::max( g->u.str_cur * 25_gram, 150_gram ) * 1000;
        if (mass <= move_mass) {
            return (int) (safe_velocity (fueled) * k_mass() / (1 + strain ()) / 10);
        } else {
            return 0;
        }
    }
    else {
        return 0;
    }
}

int vehicle::max_velocity(bool const fueled) const
{
    return total_power (fueled) * 80;
}

bool vehicle::do_environmental_effects()
{
    bool needed = false;
    // check for smoking parts
    for( size_t p = 0; p < parts.size(); p++ ) {
        auto part_pos = global_pos3() + parts[p].precalc[0];

        /* Only lower blood level if:
         * - The part is outside.
         * - The weather is any effect that would cause the player to be wet. */
        if( parts[p].blood > 0 && g->m.is_outside( part_pos ) ) {
            needed = true;
            if( g->weather >= WEATHER_DRIZZLE && g->weather <= WEATHER_ACID_RAIN ) {
                parts[p].blood--;
            }
        }
    }
    return needed;
}

int vehicle::safe_velocity(bool const fueled) const
{
    int pwrs = 0;
    int cnt = 0;
    for (size_t e = 0; e < engines.size(); e++){
        if (is_engine_on(e) &&
            (!fueled || is_engine_type(e, fuel_type_muscle) ||
            fuel_left (part_info(engines[e]).fuel_type))) {
            int m2c = 100;

            if (is_engine_type(e, fuel_type_gasoline)) {
                m2c = 60;
            } else if(is_engine_type(e, fuel_type_diesel)) {
                m2c = 65;
            } else if(is_engine_type(e, fuel_type_battery)) {
                m2c = 90;
            } else if(is_engine_type(e, fuel_type_muscle)) {
                m2c = 45;
            }

            if( parts[ engines[ e ] ].faults().count( fault_filter_fuel ) ) {
                m2c *= 0.6;
            }

            pwrs += part_power(engines[e]) * m2c / 100;
            cnt++;
        }
    }
    for (int a = 0; a < (int)alternators.size(); a++){
         if (is_alternator_on(a)){
            pwrs += part_power(alternators[a]); // alternator parts have negative power
         }
    }
    if (cnt > 0) {
        pwrs = pwrs * 4 / (4 + cnt -1);
    }
    return (int) (pwrs * k_dynamics() * k_mass()) * 80;
}

void vehicle::spew_smoke( double joules, int part, int density )
{
    if( rng( 1, 10000 ) > joules ) {
        return;
    }
    point p = parts[part].mount;
    density = std::max( joules / 10000, double( density ) );
    // Move back from engine/muffler until we find an open space
    while( relative_parts.find(p) != relative_parts.end() ) {
        p.x += ( velocity < 0 ? 1 : -1 );
    }
    point q = coord_translate(p);
    tripoint dest( global_x() + q.x, global_y() + q.y, smz );
    g->m.adjust_field_strength( dest, fd_smoke, density );
}

/**
 * Generate noise or smoke from a vehicle with engines turned on
 * load = how hard the engines are working, from 0.0 until 1.0
 * time = how many seconds to generated smoke for
 */
void vehicle::noise_and_smoke( double load, double time )
{
    const std::array<int, 8> sound_levels = {{ 0, 15, 30, 60, 100, 140, 180, INT_MAX }};
    const std::array<std::string, 8> sound_msgs = {{
        "", _("hummm!"), _("whirrr!"), _("vroom!"), _("roarrr!"), _("ROARRR!"),
        _("BRRROARRR!"), _("BRUMBRUMBRUMBRUM!")
    }};
    double noise = 0.0;
    double mufflesmoke = 0.0;
    double muffle = 1.0;
    double m = 0.0;
    int exhaust_part = -1;
    for( size_t p = 0; p < parts.size(); p++ ) {
        if( part_flag(p, "MUFFLER") ) {
            m = 1.0 - (1.0 - part_info(p).bonus / 100.0) * double( parts[p].hp() ) / part_info(p).durability;
            if( m < muffle ) {
                muffle = m;
                exhaust_part = int(p);
            }
        }
    }

    bool bad_filter = false;

    for( size_t e = 0; e < engines.size(); e++ ) {
        int p = engines[e];
        if( is_engine_on(e) &&
                (is_engine_type(e, fuel_type_muscle) || fuel_left (part_info(p).fuel_type)) ) {
            double pwr = 10.0; // Default noise if nothing else found, shouldn't happen
            double max_pwr = double(power_to_epower(part_power(p, true)))/40000;
            double cur_pwr = load * max_pwr;

            if( is_engine_type(e, fuel_type_gasoline) || is_engine_type(e, fuel_type_diesel)) {

                if( is_engine_type( e, fuel_type_gasoline ) ) {
                    double dmg = 1.0 - double( parts[p].hp() ) / part_info( p ).durability;
                    if( parts[ p ].base.faults.count( fault_filter_fuel ) ) {
                        dmg = 1.0;
                    }
                    if( dmg > 0.75 && one_in( 200 - ( 150 * dmg ) ) ) {
                        backfire( e );
                    }
                }
                double j = power_to_epower(part_power(p, true)) * load * time * muffle;

                if( parts[ p ].base.faults.count( fault_filter_air ) ) {
                    bad_filter = true;
                    j *= j;
                }

                if( (exhaust_part == -1) && engine_on ) {
                    spew_smoke( j, p, bad_filter ? MAX_FIELD_DENSITY : 1 );
                } else {
                    mufflesmoke += j;
                }
                pwr = (cur_pwr*15 + max_pwr*3 + 5) * muffle;
            } else if(is_engine_type(e, fuel_type_battery)) {
                pwr = cur_pwr*3;
            } else if(is_engine_type(e, fuel_type_muscle)) {
                pwr = cur_pwr*5;
            }
            noise = std::max(noise, pwr); // Only the loudest engine counts.
        }
    }

    if( (exhaust_part != -1) && engine_on &&
        has_engine_type_not(fuel_type_muscle, true)) { // No engine, no smoke
        spew_smoke( mufflesmoke, exhaust_part, bad_filter ? MAX_FIELD_DENSITY : 1 );
    }
    // Even a vehicle with engines off will make noise traveling at high speeds
    noise = std::max( noise, double(fabs(velocity/500.0)) );
    int lvl = 0;
    if( one_in(4) && rng(0, 30) < noise &&
        has_engine_type_not(fuel_type_muscle, true)) {
       while( noise > sound_levels[lvl] ) {
           lvl++;
       }
    }
    sounds::ambient_sound( global_pos3(), noise, sound_msgs[lvl] );
}

float vehicle::wheel_area( bool boat ) const
{
    float total_area = 0.0f;
    const auto &wheel_indices = boat ? floating : wheelcache;
    for( auto &wheel_index : wheel_indices ) {
        total_area += parts[ wheel_index ].base.wheel_area();
    }

    return total_area;
}

float vehicle::k_friction() const
{
    // calculate safe speed reduction due to wheel friction
    constexpr float fr0 = 9000.0;
    return fr0 / ( fr0 + wheel_area( false ) ) ;
}

float vehicle::k_aerodynamics() const
{
    const int max_obst = 13;
    int obst[max_obst];
    for( auto &elem : obst ) {
        elem = 0;
    }
    std::vector<int> structure_indices = all_parts_at_location(part_location_structure);
    for( auto &structure_indice : structure_indices ) {
        int p = structure_indice;
        int frame_size = part_with_feature(p, VPFLAG_OBSTACLE) ? 30 : 10;
        int pos = parts[p].mount.y + max_obst / 2;
        if (pos < 0) {
            pos = 0;
        }
        if (pos >= max_obst) {
            pos = max_obst -1;
        }
        if (obst[pos] < frame_size) {
            obst[pos] = frame_size;
        }
    }
    int frame_obst = 0;
    for( auto &elem : obst ) {
        frame_obst += elem;
    }
    float ae0 = 200.0;

    // calculate aerodynamic coefficient
    float ka = ( ae0 / (ae0 + frame_obst) );
    return ka;
}

float vehicle::k_dynamics() const
{
    return ( k_aerodynamics() * k_friction() );
}

float vehicle::k_mass() const
{
    // @todo: Remove this sum, apply only the relevant wheel type
    float wa = wheel_area( false ) + wheel_area( true );
    if( wa <= 0 ) {
       return 0;
    }

    float ma0 = 50.0;
    // calculate safe speed reduction due to mass
    float km = ma0 / ( ma0 + to_kilogram( total_mass() ) / ( wa * 8.0f / 9.0f ) );

    return km;
}

float vehicle::k_traction( float wheel_traction_area ) const
{
    if( wheel_traction_area <= 0.01f ) {
        return 0.0f;
    }

    const float mass_penalty = ( 1.0f - wheel_traction_area / wheel_area( !floating.empty() ) ) * to_kilogram( total_mass() );

    float traction = std::min( 1.0f, wheel_traction_area / mass_penalty );
    add_msg( m_debug, "%s has traction %.2f", name.c_str(), traction );
    // For now make it easy until it gets properly balanced: add a low cap of 0.1
    return std::max( 0.1f, traction );
}

float vehicle::drag() const
{
    return -extra_drag;
}

float vehicle::strain() const
{
    int mv = max_velocity();
    int sv = safe_velocity();
    if( mv <= sv ) {
        mv = sv + 1;
    }
    if( velocity < sv && velocity > -sv ) {
        return 0;
    } else {
        return (float) (abs(velocity) - sv) / (float) (mv - sv);
    }
}

bool vehicle::sufficient_wheel_config( bool boat ) const
{
    std::vector<int> floats = all_parts_with_feature(VPFLAG_FLOATS);
    // @todo: Remove the limitations that boats can't move on land
    if( boat || !floats.empty() ) {
        return boat && floats.size() > 2;
    }
    std::vector<int> wheel_indices = all_parts_with_feature(VPFLAG_WHEEL);
    if(wheel_indices.empty()) {
        // No wheels!
        return false;
    } else if(wheel_indices.size() == 1) {
        //Has to be a stable wheel, and one wheel can only support a 1-3 tile vehicle
        if( !part_info(wheel_indices[0]).has_flag("STABLE") ||
             all_parts_at_location(part_location_structure).size() > 3) {
            return false;
        }
    }
    return true;
}

bool vehicle::balanced_wheel_config( bool boat ) const
{
    int xmin = INT_MAX;
    int ymin = INT_MAX;
    int xmax = INT_MIN;
    int ymax = INT_MIN;
    // find the bounding box of the wheels
    // TODO: find convex hull instead
    const auto &indices = boat ? floating : wheelcache;
    for( auto &w : indices ) {
        const auto &pt = parts[ w ].mount;
        xmin = std::min( xmin, pt.x );
        ymin = std::min( ymin, pt.y );
        xmax = std::max( xmax, pt.x );
        ymax = std::max( ymax, pt.y );
    }

    const point &com = local_center_of_mass();
    if( com.x < xmin || com.x > xmax || com.y < ymin || com.y > ymax ) {
        return false; // center of mass not inside support of wheels (roughly)
    }
    return true;
}

bool vehicle::valid_wheel_config( bool boat ) const
{
    return sufficient_wheel_config( boat ) && balanced_wheel_config( boat );
}

float vehicle::steering_effectiveness() const
{
    if (!floating.empty()) {
        // I'M ON A BOAT
        return 1.0;
    }

    if (steering.empty()) {
        return -1.0; // No steering installed
    }

    // For now, you just need one wheel working for 100% effective steering.
    // TODO: return something less than 1.0 if the steering isn't so good
    // (unbalanced, long wheelbase, back-heavy vehicle with front wheel steering,
    // etc)
    for (int p : steering) {
        if( !parts[ p ].is_broken() ) {
            return 1.0;
        }
    }

    // We have steering, but it's all broken.
    return 0.0;
}

float vehicle::handling_difficulty() const
{
    const float steer = std::max( 0.0f, steering_effectiveness() );
    const float ktraction = k_traction( g->m.vehicle_wheel_traction( *this ) );
    const float kmass = k_mass();
    const float aligned = std::max( 0.0f, 1.0f - ( face_vec() - dir_vec() ).magnitude() );

    constexpr float tile_per_turn = 10 * 100;

    // TestVehicle: perfect steering, kmass, moving on road at 100 mph (10 tiles per turn) = 0.0
    // TestVehicle but on grass (0.75 friction) = 2.5
    // TestVehicle but overloaded (0.5 kmass) = 5
    // TestVehicle but with bad steering (0.5 steer) and overloaded (0.5 kmass) = 10
    // TestVehicle but on fungal bed (0.5 friction), bad steering and overloaded = 15
    // TestVehicle but turned 90 degrees during this turn (0 align) = 10
    const float diff_mod = ( ( 1.0f - steer ) + ( 1.0f - kmass ) + ( 1.0f - ktraction ) + ( 1.0f - aligned ) );
    return velocity * diff_mod / tile_per_turn;
}

std::map<itype_id, int> vehicle::fuel_usage() const
{
    std::map<itype_id, int> ret;
    for( size_t i = 0; i < engines.size(); i++ ) {
        // Note: functions with "engine" in name do NOT take part indices
        // @todo: Use part indices and not engine vector indices
        if( !is_engine_on( i ) ) {
            continue;
        }

        const size_t e = engines[ i ];
        const auto &info = part_info( e );
        static const itype_id null_fuel_type( "null" );
        if( info.fuel_type == null_fuel_type ) {
            continue;
        }

        // @todo: Get rid of this special case
        if( info.fuel_type == fuel_type_battery ) {
            // Motor epower is in negatives
            ret[ fuel_type_battery ] -= epower_to_power( part_epower( e ) );
        } else if( !is_engine_type( i, fuel_type_muscle ) ) {
            int usage = part_power( e );
            if( parts[ e ].faults().count( fault_filter_air ) ) {
                usage *= 2;
            }

            ret[ info.fuel_type ] += usage;
        }
    }

    return ret;
}

float vehicle::drain_energy( const itype_id &ftype, float energy )
{
    float drained = 0.0f;
    for( auto &p : parts ) {
        if( energy <= 0.0f ) {
            break;
        }

        float consumed = p.consume_energy( ftype, energy );
        drained += consumed;
        energy -= consumed;
    }

    invalidate_mass();
    return drained;
}

void vehicle::consume_fuel( double load = 1.0 )
{
    float st = strain();
    for( auto &fuel_pr : fuel_usage() ) {
        auto &ft = fuel_pr.first;
        int amnt_fuel_use = fuel_pr.second;

        // In kilojoules
        double amnt_precise = double( amnt_fuel_use );
        amnt_precise *= load * ( 1.0 + st * st * 100.0 );
        double remainder = fuel_remainder[ ft ];
        amnt_precise -= remainder;

        if( amnt_precise > 0.0f ) {
            fuel_remainder[ ft ] = amnt_precise - drain_energy( ft, amnt_precise );
        } else {
            fuel_remainder[ ft ] = -amnt_precise;
        }

        add_msg( m_debug, "%s consumes %s: amount %.2f, remainder %.2f",
                 name.c_str(), ft.c_str(), amnt_precise, fuel_remainder[ ft ] );
    }
    //do this with chance proportional to current load
    // But only if the player is actually there!
    if( load > 0 && one_in( (int) (1 / load) ) &&
        fuel_left( fuel_type_muscle ) > 0 ) {
        //charge bionics when using muscle engine
        if (g->u.has_bionic( bionic_id( "bio_torsionratchet" ) ) ) {
            g->u.charge_power(1);
        }
        //cost proportional to strain
        int mod = 1 + 4 * st;
        if (one_in(10)) {
            g->u.mod_hunger(mod);
            g->u.mod_thirst(mod);
            g->u.mod_fatigue(mod);
        }
        g->u.mod_stat( "stamina", -mod * 20);
    }
}

std::vector<vehicle_part *> vehicle::lights( bool active )
{
    std::vector<vehicle_part *> res;
    for( auto &e : parts ) {
        if( ( !active || e.enabled ) && !e.is_broken() && e.is_light() ) {
            res.push_back( &e );
        }
    }
    return res;
}

void vehicle::power_parts()
{
    int epower = 0;

    for( const auto *pt : get_parts( VPFLAG_ENABLED_DRAINS_EPOWER, true ) ) {
        epower += pt->info().epower;
    }

    // Consumers of epower
    if( is_alarm_on ) epower += alarm_epower;
    if( camera_on ) epower += camera_epower;
    // Engines: can both produce (plasma) or consume (gas, diesel)
    // Gas engines require epower to run for ignition system, ECU, etc.
    int engine_epower = 0;
    if( engine_on ) {
        for( size_t e = 0; e < engines.size(); ++e ) {
            // Electric engines consume power when actually used, not passively
            if( is_engine_on( e ) && !is_engine_type(e, fuel_type_battery) ) {
                engine_epower += part_epower( engines[e] );
            }
        }

        epower += engine_epower;

        // Producers of epower

        // If the engine is on, the alternators are working.
        int alternators_epower = 0;
        int alternators_power = 0;
        for( size_t p = 0; p < alternators.size(); ++p ) {
            if(is_alternator_on(p)) {
                alternators_epower += part_info(alternators[p]).epower;
                alternators_power += part_power(alternators[p]);
            }
        }
        if(alternators_epower > 0) {
            alternator_load = (float)abs(alternators_power);
            epower += alternators_epower;
        }
    }

    int epower_capacity_left = power_to_epower(fuel_capacity(fuel_type_battery) - fuel_left(fuel_type_battery));
    if( has_part( "REACTOR", true ) && epower_capacity_left - epower > 0 ) {
        // Still not enough surplus epower to fully charge battery
        // Produce additional epower from any reactors
        bool reactor_working = false;
        for( auto &elem : reactors ) {
            if( !parts[ elem ].is_broken() && parts[elem].ammo_remaining() > 0 ) {
                // Efficiency: one unit of fuel is this many units of battery
                // Note: One battery is roughly 373 units of epower
                const int efficiency = part_info( elem ).power;
                const int avail_fuel = parts[elem].ammo_remaining() * efficiency;

                const int elem_epower = std::min( part_epower( elem ), power_to_epower( avail_fuel ) );
                // Cap output at what we can achieve and utilize
                const int reactors_output = std::min( elem_epower, epower_capacity_left - epower );
                // Units of fuel consumed before adjustment for efficiency
                const int battery_consumed = epower_to_power( reactors_output );
                // Fuel consumed in actual units of the resource
                int fuel_consumed = battery_consumed / efficiency;
                // Remainder has a chance of resulting in more fuel consumption
                if( x_in_y( battery_consumed % efficiency, efficiency ) ) {
                    fuel_consumed++;
                }

                parts[ elem ].ammo_consume( fuel_consumed, global_part_pos3( elem ) );
                reactor_working = true;

                epower += reactors_output;
            }
        }

        if( !reactor_working ) {
            // All reactors out of fuel or destroyed
            for( auto pt : get_parts( "REACTOR" ) ) {
                pt->enabled = false;
            }
            if( player_in_control(g->u) || g->u.sees( global_pos3() ) ) {
                add_msg( _("The %s's reactor dies!"), name.c_str() );
            }
        }
    }

    int battery_deficit = 0;
    if(epower > 0) {
        // store epower surplus in battery
        charge_battery(epower_to_power(epower));
    } else if(epower < 0) {
        // draw epower deficit from battery
        battery_deficit = discharge_battery(abs(epower_to_power(epower)));
    }

    if( battery_deficit != 0 ) {
        // Scoops need a special case since they consume power during actual use
        for( auto *pt : get_parts( "SCOOP" ) ) {
            pt->enabled = false;
        }
        // Rechargers need special case since they consume power on demand
        for( auto *pt : get_parts( "RECHARGE" ) ) {
            pt->enabled = false;
        }

        for( auto *pt : get_parts( VPFLAG_ENABLED_DRAINS_EPOWER, true ) ) {
            if( pt->info().epower < 0 ) {
                pt->enabled = false;
            }
        }

        is_alarm_on = false;
        camera_on = false;
        if( player_in_control( g->u ) || g->u.sees( global_pos3() ) ) {
            add_msg( _("The %s's battery dies!"), name.c_str() );
        }
        if( engine_epower < 0 ) {
            // Not enough epower to run gas engine ignition system
            engine_on = false;
            if( player_in_control( g->u ) || g->u.sees( global_pos3() ) ) {
                add_msg( _("The %s's engine dies!"), name.c_str() );
            }
        }
    }
}

vehicle* vehicle::find_vehicle( const tripoint &where )
{
    // Is it in the reality bubble?
    tripoint veh_local = g->m.getlocal( where );
    if( const optional_vpart_position vp = g->m.veh_at( veh_local ) ) {
        return &vp->vehicle();
    }

    // Nope. Load up its submap...
    point veh_in_sm = point( where.x, where.y );
    point veh_sm = ms_to_sm_remain( veh_in_sm );

    auto sm = MAPBUFFER.lookup_submap( veh_sm.x, veh_sm.y, where.z );
    if( sm == nullptr ) {
        return nullptr;
    }

    for( auto &elem : sm->vehicles ) {
        vehicle *found_veh = elem;
        point veh_location( found_veh->posx, found_veh->posy );

        if( veh_in_sm == veh_location ) {
            return found_veh;
        }
    }

    return nullptr;
}

template <typename Func, typename Vehicle>
int vehicle::traverse_vehicle_graph(Vehicle *start_veh, int amount, Func action)
{
    // Breadth-first search! Initialize the queue with a pointer to ourselves and go!
    std::queue< std::pair<Vehicle*, int> > connected_vehs;
    std::set<Vehicle*> visited_vehs;
    connected_vehs.push(std::make_pair(start_veh, 0));

    while(amount > 0 && connected_vehs.size() > 0) {
        auto current_node = connected_vehs.front();
        Vehicle *current_veh = current_node.first;
        int current_loss = current_node.second;

        visited_vehs.insert(current_veh);
        connected_vehs.pop();

        g->u.add_msg_if_player(m_debug, "Traversing graph with %d power", amount);

        for(auto &p : current_veh->loose_parts) {
            if(!current_veh->part_info(p).has_flag("POWER_TRANSFER")) {
                continue; // ignore loose parts that aren't power transfer cables
            }

            auto target_veh = vehicle::find_vehicle(current_veh->parts[p].target.second);
            if(target_veh == nullptr || visited_vehs.count(target_veh) > 0) {
                // Either no destination here (that vehicle's rolled away or off-map) or
                // we've already looked at that vehicle.
                continue;
            }

            // Add this connected vehicle to the queue of vehicles to search next,
            // but only if we haven't seen this one before.
            if(visited_vehs.count(target_veh) < 1) {
                int target_loss = current_loss + current_veh->part_info(p).epower;
                connected_vehs.push(std::make_pair(target_veh, target_loss));

                float loss_amount = ((float)amount * (float)target_loss) / 100;
                g->u.add_msg_if_player(m_debug, "Visiting remote %p with %d power (loss %f, which is %d percent)",
                                        (void*)target_veh, amount, loss_amount, target_loss);

                amount = action(target_veh, amount, (int)loss_amount);
                g->u.add_msg_if_player(m_debug, "After remote %p, %d power", (void*)target_veh, amount);

                if(amount < 1) {
                    break; // No more charge to donate away.
                }
            }
        }
    }
    return amount;
}

int vehicle::charge_battery (int amount, bool include_other_vehicles)
{
    for( auto &p : parts ) {
        if( amount <= 0 ) {
            break;
        }
        if( !p.is_broken() && p.is_battery() ) {
            int qty = std::min( long( amount ), p.ammo_capacity() - p.ammo_remaining() );
            p.ammo_set( fuel_type_battery, p.ammo_remaining() + qty );
            amount -= qty;
        }
    }

    auto charge_visitor = [] (vehicle* veh, int amount, int lost) {
        g->u.add_msg_if_player(m_debug, "CH: %d", amount - lost);
        return veh->charge_battery(amount - lost, false);
    };

    if(amount > 0 && include_other_vehicles) { // still a bit of charge we could send out...
        amount = traverse_vehicle_graph(this, amount, charge_visitor);
    }

    return amount;
}

int vehicle::discharge_battery (int amount, bool recurse)
{
    for( auto &p : parts ) {
        if( amount <= 0 ) {
            break;
        }
        if( !p.is_broken() && p.is_battery() ) {
            amount -= p.ammo_consume( amount, global_part_pos3( p ) );
        }
    }

    auto discharge_visitor = [] (vehicle* veh, int amount, int lost) {
        g->u.add_msg_if_player(m_debug, "CH: %d", amount + lost);
        return veh->discharge_battery(amount + lost, false);
    };
    if(amount > 0 && recurse) { // need more power!
        amount = traverse_vehicle_graph(this, amount, discharge_visitor);
    }

    return amount; // non-zero if we weren't able to fulfill demand.
}

void vehicle::do_engine_damage(size_t e, int strain) {
     if( is_engine_on(e) && !is_engine_type(e, fuel_type_muscle) &&
         fuel_left(part_info(engines[e]).fuel_type) &&  rng (1, 100) < strain ) {
        int dmg = rng(strain * 2, strain * 4);
        damage_direct( engines[e], dmg );
        if(one_in(2)) {
            add_msg(_("Your engine emits a high pitched whine."));
        } else {
            add_msg(_("Your engine emits a loud grinding sound."));
        }
    }
}

void vehicle::idle(bool on_map) {
    int engines_power = 0;
    float idle_rate;

    if (engine_on && total_power() > 0) {
        for (size_t e = 0; e < engines.size(); e++){
            size_t p = engines[e];
            if (fuel_left(part_info(p).fuel_type) && is_engine_on(e)) {
                engines_power += part_power(engines[e]);
            }
        }

        idle_rate = (float)alternator_load / (float)engines_power;
        if (idle_rate < 0.01) idle_rate = 0.01; // minimum idle is 1% of full throttle
        consume_fuel(idle_rate);

        if (on_map) {
            noise_and_smoke( idle_rate, 6.0 );
        }
    } else {
        if( engine_on && g->u.sees( global_pos3() ) &&
            has_engine_type_not(fuel_type_muscle, true) ) {
            add_msg(_("The %s's engine dies!"), name.c_str());
        }
        engine_on = false;
    }

    if( !warm_enough_to_plant() ) {
        for( auto e : get_parts( "PLANTER", true ) ) {
            if( g->u.sees( global_pos3() ) ) {
                add_msg( _( "The %s's planter turns off due to low temperature." ), name.c_str() );
            }
            e->enabled = false;
        }
    }


    if( has_part( "STEREO", true ) ) {
        play_music();
    }

    if( has_part( "CHIMES", true ) ) {
        play_chimes();
    }

    if (on_map && is_alarm_on) {
        alarm();
    }

    if( on_map ) {
        update_time( calendar::turn );
    }
}

void vehicle::on_move(){
    if( has_part( "SCOOP", true ) ) {
        operate_scoop();
    }
    if( has_part( "PLANTER", true ) ) {
        operate_planter();
    }
    if( has_part( "PLOW", true ) ) {
        operate_plow();
    }
    if( has_part( "REAPER", true ) ) {
        operate_reaper();
    }
    if( has_part( "ROCKWHEEL", true ) ) {
        operate_rockwheel();
    }

    occupied_cache_time = calendar::before_time_starts;
}

void vehicle::operate_plow(){
    for( const int plow_id : all_parts_with_feature( "PLOW" ) ){
        const tripoint start_plow = global_pos3() + parts[plow_id].precalc[0];
        if( g->m.has_flag("DIGGABLE", start_plow) ){
            g->m.ter_set( start_plow, t_dirtmound );
        } else {
            const int speed = velocity;
            const int v_damage = rng( 3, speed );
            damage( plow_id, v_damage, DT_BASH, false );
            sounds::sound( start_plow, v_damage, _("Clanggggg!") );
        }
    }
}

void vehicle::operate_rockwheel() {
    for( const int rockwheel_id : all_parts_with_feature( "ROCKWHEEL" ) ) {
        const tripoint start_dig = global_pos3() + parts[rockwheel_id].precalc[0];
        if( g->m.has_flag( "DIGGABLE", start_dig ) ) {
            g->m.ter_set( start_dig, t_pit_shallow );
        } else {
            const int speed = velocity;
            const int v_damage = rng( 3, speed );
            damage( rockwheel_id, v_damage, DT_BASH, false );
            sounds::sound( start_dig, v_damage, _("Clanggggg!") );
        }
    }
}

void vehicle::operate_reaper(){
    const tripoint &veh_start = global_pos3();
    for( const int reaper_id : all_parts_with_feature( "REAPER" ) ){
        const tripoint reaper_pos = veh_start + parts[ reaper_id ].precalc[ 0 ];
        const int plant_produced =  rng( 1, parts[ reaper_id ].info().bonus );
        const int seed_produced = rng( 1, 3 );
        const units::volume max_pickup_volume = parts[ reaper_id ].info().size / 20;
        if( g->m.furn( reaper_pos ) != f_plant_harvest ||
            !g->m.has_items( reaper_pos ) ) {
            continue;
        }
        const item& seed = g->m.i_at( reaper_pos ).front();
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
        sounds::sound( reaper_pos, rng( 10, 25 ), _("Swish") );
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

void vehicle::operate_planter(){
    std::vector<int> planters = all_parts_with_feature("PLANTER");
    for( int planter_id : planters ){
        const tripoint &loc = global_pos3() + parts[planter_id].precalc[0];
        vehicle_stack v = get_items(planter_id);
        for( auto i = v.begin(); i != v.end(); i++ ){
            if( i->is_seed() ){
                // If it is an "advanced model" then it will avoid damaging itself or becoming damaged. It's a real feature.
                if( g->m.ter(loc) != t_dirtmound && part_flag(planter_id,  "ADVANCED_PLANTER" ) ) {
                    //then don't put the item there.
                    break;
                } else if( g->m.ter(loc) == t_dirtmound ) {
                    g->m.furn_set(loc, f_plant_seed);
                } else if( !g->m.has_flag( "DIGGABLE", loc ) ) {
                    //If it isn't diggable terrain, then it will most likely be damaged.
                    damage( planter_id, rng(1, 10), DT_BASH, false );
                    sounds::sound( loc, rng(10,20), _("Clink"));
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
    std::vector<int> scoops = all_parts_with_feature( "SCOOP" );
    for( int scoop : scoops ) {
        const int chance_to_damage_item = 9;
        const units::volume max_pickup_volume = parts[scoop].info().size / 10;
        const std::array<std::string, 4> sound_msgs = {{
            _("Whirrrr"), _("Ker-chunk"), _("Swish"), _("Cugugugugug")
        }};
        sounds::sound( global_pos3() + parts[scoop].precalc[0], rng( 20, 35 ),
                       random_entry_ref( sound_msgs ) );
        std::vector<tripoint> parts_points;
        for( const tripoint &current :
                 g->m.points_in_radius( global_pos3() + parts[scoop].precalc[0], 1 ) ) {
            parts_points.push_back( current );
        }
        for( const tripoint &position : parts_points ) {
            g->m.mop_spills( position );
            if( !g->m.has_items( position ) ) {
                continue;
            }
            item *that_item_there = nullptr;
            const map_stack q = g->m.i_at( position );
            if( g->m.has_flag( "SEALED", position) ) {
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
                sounds::sound( position, rng(10, that_item_there->volume() / units::legacy_volume_factor * 2 + 10),
                               _("BEEEThump") );
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

void vehicle::alarm() {
    if( one_in(4) ) {
        //first check if the alarm is still installed
        bool found_alarm = has_security_working();

        //if alarm found, make noise, else set alarm disabled
        if( found_alarm ) {
            const std::array<std::string, 4> sound_msgs = {{
                _("WHOOP WHOOP"), _("NEEeu NEEeu NEEeu"), _("BLEEEEEEP"), _("WREEP")
            }};
            sounds::sound( global_pos3(), (int) rng(45,80), random_entry_ref( sound_msgs ) );
            if( one_in(1000) ) {
                is_alarm_on = false;
            }
        } else {
            is_alarm_on = false;
        }
    }
}

void vehicle::slow_leak()
{
    // for each badly damaged tanks (lower than 50% health), leak a small amount
    for( auto &p : parts ) {
        auto dmg = double( p.hp() ) / p.info().durability;
        if( dmg > 0.5 || p.ammo_remaining() <= 0 ) {
            continue;
        }

        auto fuel = p.ammo_current();
        if( fuel != fuel_type_gasoline && fuel != fuel_type_diesel &&
            fuel != fuel_type_battery && fuel != fuel_type_water ) {
            continue; // not a liquid fuel or battery
        }

        int qty = std::max( ( 0.5 - dmg ) * ( 0.5 - dmg) * p.ammo_remaining() / 10, 1.0 );

        // damaged batteries self-discharge without leaking
        if( fuel != fuel_type_battery ) {
            item leak( fuel, calendar::turn, qty );
            point q = coord_translate( p.mount );
            tripoint dest( global_x() + q.x, global_y() + q.y, smz );
            g->m.add_item_or_charges( dest, leak );
        }

        p.ammo_consume( qty, global_part_pos3( p ) );
    }
}

void vehicle::thrust( int thd ) {
    //if vehicle is stopped, set target direction to forward.
    //ensure it is not skidding. Set turns used to 0.
    if( velocity == 0 ) {
        turn_dir = face.dir();
        move = face;
        of_turn_carry = 0;
        last_turn = 0;
        skidding = false;
    }

    if( has_part( "STEREO", true ) ) {
        play_music();
    }

    if( has_part( "CHIMES", true ) ) {
        play_chimes();
    }

    // No need to change velocity
    if( !thd ) {
        return;
    }

    bool pl_ctrl = player_in_control( g->u );

    // No need to change velocity if there are no wheels
    if( !valid_wheel_config( !floating.empty() ) && velocity == 0 ) {
        if( pl_ctrl ) {
            if( floating.empty() ) {
                add_msg(_("The %s doesn't have enough wheels to move!"), name.c_str());
            } else {
                add_msg(_("The %s is too leaky!"), name.c_str());
            }
        }
        return;
    }

    // Accelerate (true) or brake (false)
    bool thrusting = true;
    if( velocity ) {
       int sgn = (velocity < 0) ? -1 : 1;
       thrusting = (sgn == thd);
    }

    // @todo: Pass this as an argument to avoid recalculating
    float traction = k_traction( g->m.vehicle_wheel_traction( *this ) );
    int accel = acceleration() * traction;
    if( thrusting && accel == 0 ) {
        if( pl_ctrl ) {
            add_msg( _("The %s is too heavy for its engine(s)!"), name.c_str() );
        }

        return;
    }

    int max_vel = max_velocity() * traction;
    // Get braking power
    int brake = 30 * k_mass();
    int brk = abs(velocity) * brake / 100;
    if (brk < accel) {
        brk = accel;
    }
    if (brk < 10 * 100) {
        brk = 10 * 100;
    }
    //pos or neg if accelerator or brake
    int vel_inc = ((thrusting) ? accel : brk) * thd;
    if( thd == -1 && thrusting ) {
        //accelerate 60% if going backward
        vel_inc = .6 * vel_inc;
    }

    // Keep exact cruise control speed
    if( cruise_on ) {
        if( thd > 0 ) {
            vel_inc = std::min( vel_inc, cruise_velocity - velocity );
        } else {
            vel_inc = std::max( vel_inc, cruise_velocity - velocity );
        }
    }

    //find power ratio used of engines max
    double load;
    if( cruise_on ) {
        load = ((float)abs(vel_inc)) / std::max((thrusting ? accel : brk),1);
    } else {
        load = (thrusting ? 1.0 : 0.0);
    }

    // only consume resources if engine accelerating
    if (load >= 0.01 && thrusting) {
        //abort if engines not operational
        if( total_power () <= 0 || !engine_on || accel == 0 ) {
            if (pl_ctrl) {
                if( total_power( false ) <= 0 ) {
                    add_msg( m_info, _("The %s doesn't have an engine!"), name.c_str() );
                } else if( has_engine_type( fuel_type_muscle, true ) ) {
                    add_msg( m_info, _("The %s's mechanism is out of reach!"), name.c_str() );
                } else if( !engine_on ) {
                    add_msg( _("The %s's engine isn't on!"), name.c_str() );
                } else if( traction < 0.01f ) {
                    add_msg( _("The %s is stuck."), name.c_str() );
                } else {
                    add_msg( _("The %s's engine emits a sneezing sound."), name.c_str() );
                }
            }
            cruise_velocity = 0;
            return;
        }

        //make noise and consume fuel
        noise_and_smoke (load);
        consume_fuel (load);

        //break the engines a bit, if going too fast.
        int strn = (int) (strain () * strain() * 100);
        for (size_t e = 0; e < engines.size(); e++){
            do_engine_damage(e, strn);
        }
    }

    //wheels aren't facing the right way to change velocity properly
    //lower down, since engines should be getting damaged anyway
    if( skidding ) {
        return;
    }

    //change vehicles velocity
    if( (velocity > 0 && velocity + vel_inc < 0) ||
        (velocity < 0 && velocity + vel_inc > 0) ) {
        //velocity within braking distance of 0
        stop ();
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
}

void vehicle::cruise_thrust (int amount)
{
    if( amount == 0 ) {
        return;
    }
    int safe_vel = safe_velocity();
    int max_vel = max_velocity();
    int max_rev_vel = -max_vel / 4;

    //if the safe velocity is between the cruise velocity and its next value, set to safe velocity
    if( (cruise_velocity < safe_vel && safe_vel < (cruise_velocity + amount)) ||
        (cruise_velocity > safe_vel && safe_vel > (cruise_velocity + amount)) ){
        cruise_velocity = safe_vel;
    } else {
        if (amount < 0 && (cruise_velocity == safe_vel || cruise_velocity == max_vel)){
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
        cruise_velocity = (cruise_velocity / abs(amount)) * abs(amount);
    }
    // Can't have a cruise speed faster than max speed
    // or reverse speed faster than max reverse speed.
    if (cruise_velocity > max_vel) {
        cruise_velocity = max_vel;
    } else if (cruise_velocity < max_rev_vel) {
        cruise_velocity = max_rev_vel;
    }
}

void vehicle::turn( int deg )
{
    if (deg == 0) {
        return;
    }
    if (velocity < 0) {
        deg = -deg;
    }
    last_turn = deg;
    turn_dir += deg;
    if (turn_dir < 0) {
        turn_dir += 360;
    }
    if (turn_dir >= 360) {
        turn_dir -= 360;
    }
}

void vehicle::stop ()
{
    velocity = 0;
    skidding = false;
    move = face;
    last_turn = 0;
    of_turn_carry = 0;
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
        return collision( colls, tripoint( dp.x, dp.y, 0    ), just_detect, bash_floor ) ||
               collision( colls, tripoint( 0,    0,    dp.z ), just_detect, bash_floor );
    }

    if( dp.z == -1 && !bash_floor ) {
        // First check current level, then the one below if current had no collisions
        // Bash floors on the current one, but not on the one below.
        if( collision( colls, tripoint( 0, 0, 0 ), just_detect, true ) ) {
            return true;
        }
    }

    const bool vertical = bash_floor || dp.z != 0;
    const int &coll_velocity = vertical ? vertical_velocity : velocity;
    if( !just_detect && coll_velocity == 0 ) {
        debugmsg( "Collision check on stationary vehicle %s", name.c_str() );
        just_detect = true;
    }

    const int velocity_before = coll_velocity;
    const int sign_before = sgn( velocity_before );
    std::vector<int> structural_indices = all_parts_at_location(part_location_structure);
    for( size_t i = 0; i < structural_indices.size(); i++ ) {
        const int p = structural_indices[i];
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

    if( structural_indices.empty() ) {
        // Hack for dirty vehicles that didn't yet get properly removed
        veh_collision fake_coll;
        fake_coll.type = veh_coll_other;
        colls.push_back( fake_coll );
        velocity = 0;
        vertical_velocity = 0;
        add_msg( m_debug, "Collision check on a dirty vehicle %s", name.c_str() );
        return true;
    }

    return !colls.empty();
}

// A helper to make sure mass and density is always calculated the same way
void terrain_collision_data( const tripoint &p, bool bash_floor,
                             float &mass, float &density, float &elastic )
{
    elastic = 0.30;
    // Just a rough rescale for now to obtain approximately equal numbers
    const int bash_min = g->m.bash_resistance( p, bash_floor );
    const int bash_max = g->m.bash_strength( p, bash_floor );
    mass = ( bash_min + bash_max ) / 2;
    density = bash_min;
}

veh_collision vehicle::part_collision( int part, const tripoint &p,
                                       bool just_detect, bool bash_floor )
{
    // Vertical collisions need to be handled differently
    // All collisions have to be either fully vertical or fully horizontal for now
    const bool vert_coll = bash_floor || p.z != smz;
    const bool pl_ctrl = player_in_control( g->u );
    Creature *critter = g->critter_at( p, true );
    player *ph = dynamic_cast<player*>( critter );

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

    // Damage armor before damaging any other parts
    // Actually target, not just damage - spiked plating will "hit back", for example
    const int armor_part = part_with_feature( ret.part, VPFLAG_ARMOR );
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
        switch( critter->get_size() ) {
        case MS_TINY:    // Rodent
            mass2 = 1;
            break;
        case MS_SMALL:   // Half human
            mass2 = 41;
            break;
        default:
        case MS_MEDIUM:  // Human
            mass2 = 82;
            break;
        case MS_LARGE:   // Cow
            mass2 = 400;
            break;
        case MS_HUGE:     // TAAAANK
            mass2 = 1000;
            break;
        }
        ret.target_name = critter->disp_name();
    } else if( ( bash_floor && g->m.is_bashable_ter_furn( p, true ) ) ||
               ( g->m.is_bashable_ter_furn( p, false ) && g->m.move_cost_ter_furn( p ) != 2 &&
                // Don't collide with tiny things, like flowers, unless we have a wheel in our space.
                (part_with_feature(ret.part, VPFLAG_WHEEL) >= 0 ||
                 !g->m.has_flag_ter_or_furn("TINY", p)) &&
                // Protrusions don't collide with short terrain.
                // Tiny also doesn't, but it's already excluded unless there's a wheel present.
                !(part_with_feature(ret.part, "PROTRUSION") >= 0 &&
                  g->m.has_flag_ter_or_furn("SHORT", p)) &&
                // These are bashable, but don't interact with vehicles.
                !g->m.has_flag_ter_or_furn("NOCOLLIDE", p) ) ) {
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
    float material_factor = (part_dens - vpart_dens)*0.5;
    material_factor = std::max( -25.0f, std::min( 25.0f, material_factor ) );
    // factor = -25 if mass is much greater than mass2
    // factor = +25 if mass2 is much greater than mass
    const float weight_factor = mass >= mass2 ?
        -25 * ( log(mass) - log(mass2) ) / log(mass) :
         25 * ( log(mass2) - log(mass) ) / log(mass2);

    float k = 50 + material_factor + weight_factor;
    k = std::max( 10.0f, std::min( 90.0f, k ) );

    bool smashed = true;
    std::string snd; // NOTE: Unused!
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
        const float vel1_a = (mass*vel1 + mass2*vel2 + e*mass2*(vel2 - vel1)) / (mass + mass2);
        // Velocity of object after collision
        const float vel2_a = (mass*vel1 + mass2*vel2 + e*mass *(vel1 - vel2)) / (mass + mass2);
        // Lost energy at collision -> deformation energy -> damage
        const float E_before = 0.5f * (mass * vel1 * vel1)   + 0.5f * (mass2 * vel2 * vel2);
        const float E_after =  0.5f * (mass * vel1_a*vel1_a) + 0.5f * (mass2 * vel2_a*vel2_a);
        const float d_E = E_before - E_after;
        if( d_E <= 0 ) {
            // Deformation energy is signed
            // If it's negative, it means something went wrong
            // But it still does happen sometimes...
            if( fabs(vel1_a) < fabs(vel1) ) {
                // Lower vehicle's speed to prevent infinite loops
                coll_velocity = vel1_a * 90;
            }
            if( fabs(vel2_a) > fabs(vel2) ) {
                vel2 = vel2_a;
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
        const float obj_dmg = dmg * (100-k)/100;

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
            int dam = obj_dmg*dmg_mod/100;

            // No blood from hallucinations
            if( !critter->is_hallucination() ) {
                if( part_flag( ret.part, "SHARP" ) ) {
                    parts[ret.part].blood += (20 + dam) * 5;
                } else if( dam > rng ( 10, 30 ) ) {
                    parts[ret.part].blood += (10 + dam / 2) * 5;
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
                if( fabs(vel2_a) > 10.0f ||
                    fabs(e * mass * vel1_a) > fabs(mass2 * (10.0f - vel2_a)) ) {
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
                } else {
                    // Only count critter as pushed away if it actually changed position
                    smashed = (critter->pos() != p);
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
                    add_msg (m_warning, _("Your %1$s's %2$s rams into %3$s and stuns it!"),
                             name.c_str(), parts[ ret.part ].name().c_str(), ret.target_name.c_str());
                } else {
                    //~ 1$s - vehicle name, 2$s - part name, 3$s - NPC or monster
                    add_msg (m_warning, _("Your %1$s's %2$s rams into %3$s!"),
                             name.c_str(), parts[ ret.part ].name().c_str(), ret.target_name.c_str());
                }
            }

            if( part_flag( ret.part, "SHARP" ) ) {
                critter->bleed();
            } else {
                sounds::sound( p, 20, snd );
            }
        }
    } else {
        if( pl_ctrl ) {
            if( snd.length() > 0 ) { // @todo: that is always false!
                //~ 1$s - vehicle name, 2$s - part name, 3$s - collision object name, 4$s - sound message
                add_msg (m_warning, _("Your %1$s's %2$s rams into %3$s with a %4$s"),
                         name.c_str(), parts[ ret.part ].name().c_str(), ret.target_name.c_str(), snd.c_str());
            } else {
                //~ 1$s - vehicle name, 2$s - part name, 3$s - collision object name
                add_msg (m_warning, _("Your %1$s's %2$s rams into %3$s."),
                         name.c_str(), parts[ ret.part ].name().c_str(), ret.target_name.c_str());
            }
        }

        sounds::sound(p, smashed ? 80 : 50, snd );
    }

    if( smashed && !vert_coll ) {
        int turn_amount = rng( 1, 3 ) * sqrt((double)part_dmg);
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
        if( turn_roll < std::abs((prev_velocity - coll_velocity) / 100.0f * 2.0f) ) {
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
    int pwh = part_with_feature( part, VPFLAG_WHEEL );
    if( pwh < 0 ) {
        return;
    }
    const trap &tr = g->m.tr_at(p);
    const trap_id t = tr.loadid;
    int noise = 0;
    int chance = 100;
    int expl = 0;
    int shrap = 0;
    int part_damage = 0;
    std::string snd;
    // @todo: make trapfuncv?

    if ( t == tr_bubblewrap ) {
        noise = 18;
        snd = _("Pop!");
    } else if ( t == tr_beartrap || t == tr_beartrap_buried ) {
        noise = 8;
        snd = _("SNAP!");
        part_damage = 300;
        g->m.remove_trap(p);
        g->m.spawn_item(p, "beartrap");
    } else if ( t == tr_nailboard || t == tr_caltrops ) {
        part_damage = 300;
    } else if ( t == tr_blade ) {
        noise = 1;
        snd = _("Swinnng!");
        part_damage = 300;
    } else if ( t == tr_crossbow ) {
        chance = 30;
        noise = 1;
        snd = _("Clank!");
        part_damage = 300;
        g->m.remove_trap(p);
        g->m.spawn_item(p, "crossbow");
        g->m.spawn_item(p, "string_6");
        if (!one_in(10)) {
            g->m.spawn_item(p, "bolt_steel");
        }
    } else if ( t == tr_shotgun_2 || t == tr_shotgun_1 ) {
        noise = 60;
        snd = _("Bang!");
        chance = 70;
        part_damage = 300;
        if (t == tr_shotgun_2) {
            g->m.trap_set(p, tr_shotgun_1);
        } else {
            g->m.remove_trap(p);
            g->m.spawn_item(p, "shotgun_s");
            g->m.spawn_item(p, "string_6");
        }
    } else if ( t == tr_landmine_buried || t == tr_landmine ) {
        expl = 10;
        shrap = 8;
        g->m.remove_trap(p);
        part_damage = 1000;
    } else if ( t == tr_boobytrap ) {
        expl = 18;
        shrap = 12;
        part_damage = 1000;
    } else if ( t == tr_dissector ) {
        noise = 10;
        snd = _("BRZZZAP!");
        part_damage = 500;
    } else if( t == tr_sinkhole || t == tr_pit || t == tr_spike_pit || t == tr_glass_pit ) {
        part_damage = 500;
    } else if( t == tr_ledge ) {
        falling = true;
        // Don't print message
        return;
    } else if( t == tr_lava ) {
        part_damage = 500;
        //@todo Make this damage not only wheels, but other parts too, especially tanks with flammable fuel
    } else {
        return;
    }
    if( g->u.sees(p) ) {
        if( g->u.knows_trap( p ) ) {
            //~ %1$s: name of the vehicle; %2$s: name of the related vehicle part; %3$s: trap name
            add_msg(m_bad, _("The %1$s's %2$s runs over %3$s."), name.c_str(),
                    parts[ part ].name().c_str(), tr.name().c_str() );
        } else {
            add_msg(m_bad, _("The %1$s's %2$s runs over something."), name.c_str(),
                    parts[ part ].name().c_str() );
        }
    }
    if (noise > 0) {
        sounds::sound(p, noise, snd);
    }
    if( part_damage && chance >= rng (1, 100) ) {
        // Hit the wheel directly since it ran right over the trap.
        damage_direct( pwh, part_damage );
    }
    if( expl > 0 ) {
        g->explosion( p, expl, 0.5f, false, shrap );
    }
}

// total volume of all the things
units::volume vehicle::stored_volume(int const part) const
{
    return get_items( part ).stored_volume();
}

units::volume vehicle::max_volume(int const part) const
{
    return get_items( part ).max_volume();
}

units::volume vehicle::free_volume(int const part) const
{
    return get_items( part ).free_volume();
}

void vehicle::make_active( item_location &loc )
{
    item *target = loc.get_item();
    if( !target->needs_processing() ) {
        return;
    }
    auto cargo_parts = get_parts( loc.position(), "CARGO" );
    if( cargo_parts.empty() ) {
        return;
    }
    // System insures that there is only one part in this vector.
    vehicle_part *cargo_part = cargo_parts.front();
    auto &item_stack = cargo_part->items;
    auto item_index = std::find_if( item_stack.begin(), item_stack.end(),
                                    [&target]( const item &i ) { return &i == target; } );
    active_items.add( item_index, cargo_part->mount );
}

long vehicle::add_charges( int part, const item &itm )
{
    if( !itm.count_by_charges() ) {
        debugmsg( "Add charges was called for an item not counted by charges!" );
        return 0;
    }
    const long ret = get_items( part ).amount_can_fit( itm );
    if( ret == 0 ) {
        return 0;
    }

    item itm_copy = itm;
    itm_copy.charges = ret;
    return add_item( part, itm_copy ) ? ret : 0;
}

bool vehicle::add_item( int part, const item &itm )
{
    if( part < 0 || part >= ( int )parts.size() ) {
        debugmsg( "int part (%d) is out of range", part );
        return false;
    }
    // const int max_weight = ?! // TODO: weight limit, calculation per vpart & vehicle stats, not a hard user limit.
    // add creaking sounds and damage to overloaded vpart, outright break it past a certain point, or when hitting bumps etc

    if( parts[ part ].base.is_gun() ) {
        if( !itm.is_ammo() || itm.ammo_type() != parts[ part ].base.ammo_type() ) {
            return false;
        }
    }
    bool charge = itm.count_by_charges();
    vehicle_stack istack = get_items( part );
    const long to_move = istack.amount_can_fit( itm );
    if( to_move == 0 || ( charge && to_move < itm.charges ) ) {
        return false; // @add_charges should be used in the latter case
    }
    if( charge ) {
        item *here = istack.stacks_with( itm );
        if( here ) {
            invalidate_mass();
            return here->merge_charges( itm );
        }
    }
    return add_item_at( part, parts[part].items.end(), itm );
}

bool vehicle::add_item( vehicle_part &pt, const item &obj )
{
    int idx = index_of_part( &pt );
    if( idx < 0 ) {
        debugmsg( "Tried to add item to invalid part" );
        return false;
    }
    return add_item( idx, obj );
}

bool vehicle::add_item_at(int part, std::list<item>::iterator index, item itm)
{
    if( itm.is_bucket_nonempty() ) {
        for( auto &elem : itm.contents ) {
            g->m.add_item_or_charges( global_part_pos3( part ), elem );
        }

        itm.contents.clear();
    }

    const auto new_pos = parts[part].items.insert( index, itm );
    if( itm.needs_processing() ) {
        active_items.add( new_pos, parts[part].mount );
    }

    invalidate_mass();
    return true;
}

bool vehicle::remove_item( int part, int itemdex )
{
    if( itemdex < 0 || itemdex >= (int)parts[part].items.size() ) {
        return false;
    }

    remove_item( part, std::next(parts[part].items.begin(), itemdex) );
    return true;
}

bool vehicle::remove_item( int part, const item *it )
{
    bool rc = false;
    std::list<item>& veh_items = parts[part].items;

    for( auto iter = veh_items.begin(); iter != veh_items.end(); iter++ ) {
        //delete the item if the pointer memory addresses are the same
        if( it == &*iter ) {
            remove_item(part, iter);
            rc = true;
            break;
        }
    }
    return rc;
}

std::list<item>::iterator vehicle::remove_item( int part, std::list<item>::iterator it )
{
    std::list<item>& veh_items = parts[part].items;

    if( active_items.has( it, parts[part].mount ) ) {
        active_items.remove( it, parts[part].mount );
    }

    invalidate_mass();
    return veh_items.erase(it);
}

vehicle_stack vehicle::get_items(int const part)
{
    return vehicle_stack( &parts[part].items, global_pos() + parts[part].precalc[0],
                          this, part );
}

vehicle_stack vehicle::get_items( int const part ) const
{
    // HACK: callers could modify items through this
    // TODO: a const version of vehicle_stack is needed
    return const_cast<vehicle*>(this)->get_items(part);
}

void vehicle::place_spawn_items()
{
    if( !type.is_valid() ) {
        return;
    }

    for( const auto &pt : type->parts ) {
        if( pt.with_ammo ) {
            int turret = part_with_feature_at_relative( pt.pos, "TURRET" );
            if( turret >= 0 && x_in_y( pt.with_ammo, 100 ) ) {
                parts[ turret ].ammo_set( random_entry( pt.ammo_types ), rng( pt.ammo_qty.first, pt.ammo_qty.second ) );
            }
        }
    }

    for( const auto& spawn : type.obj().item_spawns ) {
        if( rng( 1, 100 ) <= spawn.chance ) {
            int part = part_with_feature_at_relative( spawn.pos, "CARGO", false );
            if( part < 0 ) {
                debugmsg( "No CARGO parts at (%d, %d) of %s!", spawn.pos.x, spawn.pos.y, name.c_str() );

            } else {
                // if vehicle part is broken only 50% of items spawn and they will be variably damaged
                bool broken = parts[ part ].is_broken();
                if( broken && one_in( 2 ) ) {
                    continue;
                }

                std::vector<item> created;
                for( const itype_id& e : spawn.item_ids ) {
                    created.emplace_back( item( e ).in_its_container() );
                }
                for( const std::string& e : spawn.item_groups ) {
                    created.emplace_back( item_group::item_from( e, calendar::time_of_cataclysm ) );
                }

                for( item& e : created ) {
                    if( e.is_null() ) {
                        continue;
                    }
                    if( broken && e.mod_damage( rng( 1, e.max_damage() ) ) ) {
                        continue; // we destroyed the item
                    }
                    if( e.is_tool() || e.is_gun() || e.is_magazine() ) {
                        bool spawn_ammo = rng( 0, 99 ) < spawn.with_ammo && e.ammo_remaining() == 0;
                        bool spawn_mag  = rng( 0, 99 ) < spawn.with_magazine && !e.magazine_integral() && !e.magazine_current();

                        if( spawn_mag ) {
                            e.contents.emplace_back( e.magazine_default(), e.birthday() );
                        }
                        if( spawn_ammo ) {
                            e.ammo_set( e.ammo_type()->default_ammotype() );
                        }
                    }
                    add_item( part, e);
                }
            }
        }
    }
}

void vehicle::gain_moves()
{
    if( velocity != 0 || falling ) {
        if( loose_parts.size() > 0 ) {
            shed_loose_parts();
        }
        of_turn = 1 + of_turn_carry;
    } else {
        of_turn = 0;
    }
    of_turn_carry = 0;

    // cruise control TODO: enable for NPC?
    if( player_in_control(g->u) && cruise_on && cruise_velocity != velocity ) {
        thrust( (cruise_velocity) > velocity ? 1 : -1 );
    }

    // Force off-map vehicles to load by visiting them every time we gain moves.
    // Shouldn't be too expensive if there aren't fifty trillion vehicles in the graph...
    // ...and if there are, it's the player's fault for putting them there.
    auto nil_visitor = [] (vehicle*, int amount, int) { return amount; };
    traverse_vehicle_graph(this, 1, nil_visitor);

    if( check_environmental_effects ) {
        check_environmental_effects = do_environmental_effects();
    }

    // turrets which are enabled will try to reload and then automatically fire
    // Turrets which are disabled but have targets set are a special case
    for( auto e : turrets() ) {
        if( e->enabled || e->target.second != e->target.first ) {
            automatic_fire_turret( *e );
        }
    }

    if( velocity < 0 ) {
        beeper_sound();
    }
}

/**
 * Refreshes all caches and refinds all parts. Used after the vehicle has had a part added or removed.
 * Makes indices of different part types so they're easy to find. Also calculates power drain.
 */
void vehicle::refresh()
{
    alternators.clear();
    engines.clear();
    reactors.clear();
    solar_panels.clear();
    funnels.clear();
    relative_parts.clear();
    loose_parts.clear();
    wheelcache.clear();
    steering.clear();
    speciality.clear();
    floating.clear();
    tracking_epower = 0;
    alternator_load = 0;
    camera_epower = 0;
    extra_drag = 0;
    // Used to sort part list so it displays properly when examining
    struct sort_veh_part_vector {
        vehicle *veh;
        inline bool operator() (const int p1, const int p2) {
            return veh->part_info(p1).list_order < veh->part_info(p2).list_order;
        }
    } svpv = { this };
    std::vector<int>::iterator vii;

    // Main loop over all vehicle parts.
    for( size_t p = 0; p < parts.size(); p++ ) {
        const vpart_info& vpi = part_info( p );
        if( parts[p].removed ) {
            continue;
        }
        if( vpi.has_flag(VPFLAG_ALTERNATOR) ) {
            alternators.push_back( p );
        }
        if( vpi.has_flag(VPFLAG_ENGINE) ) {
            engines.push_back( p );
        }
        if( vpi.has_flag("REACTOR") ) {
            reactors.push_back( p );
        }
        if( vpi.has_flag(VPFLAG_SOLAR_PANEL) ) {
            solar_panels.push_back( p );
        }
        if( vpi.has_flag("FUNNEL") ) {
            funnels.push_back( p );
        }
        if( vpi.has_flag("UNMOUNT_ON_MOVE") ) {
            loose_parts.push_back(p);
        }
        if( vpi.has_flag( VPFLAG_WHEEL ) ) {
            wheelcache.push_back( p );
        }
        if (vpi.has_flag("STEERABLE") || vpi.has_flag("TRACKED")) {
            // TRACKED contributes to steering effectiveness but
            //  (a) doesn't count as a steering axle for install difficulty
            //  (b) still contributes to drag for the center of steering calculation
            steering.push_back(p);
        }
        if (vpi.has_flag("SECURITY")){
            speciality.push_back(p);
        }
        if( vpi.has_flag( "CAMERA" ) ) {
            camera_epower += vpi.epower;
        }
        if( vpi.has_flag( VPFLAG_FLOATS ) ) {
            floating.push_back( p );
        }
        if( parts[ p ].enabled ) {
            if( vpi.has_flag( "PLOW" ) ) {
                extra_drag += vpi.power;
            }
            if( vpi.has_flag( "ROCKWHEEL" ) ) {
                extra_drag += vpi.power;
            }
            if( vpi.has_flag( "PLANTER" ) ) {
                extra_drag += vpi.power;
            }
            if( vpi.has_flag( "REAPER" ) ) {
                extra_drag += vpi.power;
            }
        }
        // Build map of point -> all parts in that point
        const point pt = parts[p].mount;
        // This will keep the parts at point pt sorted
        vii = std::lower_bound( relative_parts[pt].begin(), relative_parts[pt].end(), static_cast<int>( p ), svpv );
        relative_parts[pt].insert( vii, p );
    }

    // NB: using the _old_ pivot point, don't recalc here, we only do that when moving!
    precalc_mounts( 0, pivot_rotation[0], pivot_anchor[0] );
    check_environmental_effects = true;
    insides_dirty = true;
    invalidate_mass();
}

const point &vehicle::pivot_point() const {
    if (pivot_dirty) {
        refresh_pivot();
    }

    return pivot_cache;
}

void vehicle::refresh_pivot() const {
    // Const method, but messes with mutable fields
    pivot_dirty = false;

    if( wheelcache.empty() || !valid_wheel_config( false ) ) {
        // No usable wheels, use CoM (dragging)
        pivot_cache = local_center_of_mass();
        return;
    }

    // The model here is:
    //
    //  We are trying to rotate around some point (xc,yc)
    //  This produces a friction force / moment from each wheel resisting the
    //  rotation. We want to find the point that minimizes that resistance.
    //
    //  For a given wheel w at (xw,yw), find:
    //   weight(w): a scaling factor for the friction force based on wheel
    //              size, brokenness, steerability/orientation
    //   center_dist: the distance from (xw,yw) to (xc,yc)
    //   centerline_angle: the angle between the X axis and a line through
    //                     (xw,yw) and (xc,yc)
    //
    //  Decompose the force into two components, assuming that the wheel is
    //  aligned along the X axis and we want to apply different weightings to
    //  the in-line vs perpendicular parts of the force:
    //
    //   Resistance force in line with the wheel (X axis)
    //    Fi = weightI(w) * center_dist * sin(centerline_angle)
    //   Resistance force perpendicular to the wheel (Y axis):
    //    Fp = weightP(w) * center_dist * cos(centerline_angle);
    //
    //  Then find the moment that these two forces would apply around (xc,yc)
    //    moment(w) = center_dist * cos(centerline_angle) * Fi +
    //                center_dist * sin(centerline_angle) * Fp
    //
    //  Note that:
    //    cos(centerline_angle) = (xw-xc) / center_dist
    //    sin(centerline_angle) = (yw-yc) / center_dist
    // -> moment(w) = weightP(w)*(xw-xc)^2 + weightI(w)*(yw-yc)^2
    //              = weightP(w)*xc^2 - 2*weightP(w)*xc*xw + weightP(w)*xw^2 +
    //                weightI(w)*yc^2 - 2*weightI(w)*yc*yw + weightI(w)*yw^2
    //
    //  which happily means that the X and Y axes can be handled independently.
    //  We want to minimize sum(moment(w)) due to wheels w=0,1,..., which
    //  occurs when:
    //
    //    sum( 2*xc*weightP(w) - 2*weightP(w)*xw ) = 0
    //     -> xc = (weightP(0)*x0 + weightP(1)*x1 + ...) /
    //             (weightP(0) + weightP(1) + ...)
    //    sum( 2*yc*weightI(w) - 2*weightI(w)*yw ) = 0
    //     -> yc = (weightI(0)*y0 + weightI(1)*y1 + ...) /
    //             (weightI(0) + weightI(1) + ...)
    //
    // so it turns into a fairly simple weighted average of the wheel positions.

    float xc_numerator = 0;
    float xc_denominator = 0;
    float yc_numerator = 0;
    float yc_denominator = 0;

    for (int p : wheelcache) {
        const auto &wheel = parts[p];

        // @todo: load on tire?
        float contact_area = wheel.wheel_area();
        float weight_i;  // weighting for the in-line part
        float weight_p;  // weighting for the perpendicular part
        if( wheel.is_broken() ) {
            // broken wheels don't roll on either axis
            weight_i = contact_area * 2;
            weight_p = contact_area * 2;
        } else if (wheel.info().has_flag("STEERABLE")) {
            // Unbroken steerable wheels can handle motion on both axes
            // (but roll a little more easily inline)
            weight_i = contact_area * 0.1;
            weight_p = contact_area * 0.2;
        } else {
            // Regular wheels resist perpendicular motion
            weight_i = contact_area * 0.1;
            weight_p = contact_area;
        }

        xc_numerator += weight_p * wheel.mount.x;
        yc_numerator += weight_i * wheel.mount.y;
        xc_denominator += weight_p;
        yc_denominator += weight_i;
    }

    if (xc_denominator < 0.1 || yc_denominator < 0.1) {
        debugmsg("vehicle::refresh_pivot had a bad weight: xc=%.3f/%.3f yc=%.3f/%.3f",
                 xc_numerator, xc_denominator, yc_numerator, yc_denominator);
        pivot_cache = local_center_of_mass();
    } else {
        pivot_cache.x = round(xc_numerator / xc_denominator);
        pivot_cache.y = round(yc_numerator / yc_denominator);
    }
}

void vehicle::remove_remote_part(int part_num) {
    auto veh = find_vehicle(parts[part_num].target.second);

    // If the target vehicle is still there, ask it to remove its part
    if (veh != nullptr) {
        auto pos = global_pos3() + parts[part_num].precalc[0];
        tripoint local_abs = g->m.getabs( pos );

        for( size_t j = 0; j < veh->loose_parts.size(); j++) {
            int remote_partnum = veh->loose_parts[j];
            auto remote_part = &veh->parts[remote_partnum];

            if( veh->part_flag(remote_partnum, "POWER_TRANSFER") && remote_part->target.first == local_abs) {
                veh->remove_part(remote_partnum);
                return;
            }
        }
    }
}

void vehicle::shed_loose_parts() {
    // remove_part rebuilds the loose_parts vector, when all of those parts have been removed,
    // it will stay empty.
    while( !loose_parts.empty() ) {
        int const elem = loose_parts.front();
        if( part_flag( elem, "POWER_TRANSFER" ) ) {
            remove_remote_part( elem );
        }

        auto part = &parts[elem];
        auto pos = global_pos3() + part->precalc[0];
        item drop = part->properties_to_item();
        g->m.add_item_or_charges( pos, drop );

        remove_part( elem );
    }
}

void vehicle::refresh_insides ()
{
    insides_dirty = false;
    for (size_t p = 0; p < parts.size(); p++) {
        if (parts[p].removed) {
          continue;
        }
        /* If there's no roof, or there is a roof but it's broken, it's outside.
         * (Use short-circuiting && so broken frames don't screw this up) */
        if ( !( part_with_feature( p, "ROOF" ) >= 0 && !parts[ p ].is_broken() ) ) {
            parts[p].inside = false;
            continue;
        }

        parts[p].inside = true; // inside if not otherwise
        for (int i = 0; i < 4; i++) { // let's check four neighbor parts
            int ndx = i < 2? (i == 0? -1 : 1) : 0;
            int ndy = i < 2? 0 : (i == 2? - 1: 1);
            std::vector<int> parts_n3ar = parts_at_relative (parts[p].mount.x + ndx,
                                                             parts[p].mount.y + ndy);
            bool cover = false; // if we aren't covered from sides, the roof at p won't save us
            for (auto &j : parts_n3ar) {
                if( part_flag( j, "ROOF" ) && !parts[ j ].is_broken() ) { // another roof -- cover
                    cover = true;
                    break;
                }
                else
                if( part_flag( j, "OBSTACLE" ) && !parts[ j ].is_broken() ) {
                    // found an obstacle, like board or windshield or door
                    if (parts[j].inside || (part_flag(j, "OPENABLE") && parts[j].open)) {
                        continue; // door and it's open -- can't cover
                    }
                    cover = true;
                    break;
                }
                //Otherwise keep looking, there might be another part in that square
            }
            if (!cover) {
                parts[p].inside = false;
                break;
            }
        }
    }
}

bool vpart_position::is_inside() const
{
    if( vehicle().insides_dirty ) {
        // TODO: this is a bit of a hack as refresh_insides has side effects
        // this should be called elsewhere and not in a function that intends to just query
        vehicle().refresh_insides();
    }
    return vehicle().parts[part_index()].inside;
}

void vehicle::unboard_all ()
{
    std::vector<int> bp = boarded_parts();
    for( auto &i : bp ) {
        g->m.unboard_vehicle( tripoint( global_x() + parts[i].precalc[0].x,
                                        global_y() + parts[i].precalc[0].y,
                                        smz ) );
    }
}

int vehicle::damage( int p, int dmg, damage_type type, bool aimed )
{
    if( dmg < 1 ) {
        return dmg;
    }

    std::vector<int> pl = parts_at_relative( parts[p].mount.x, parts[p].mount.y );
    if( pl.empty() ) {
      // We ran out of non removed parts at this location already.
      return dmg;
    }

    if( !aimed ) {
        bool found_obs = false;
        for( auto &i : pl ) {
            if( part_flag( i, "OBSTACLE" ) &&
                (!part_flag( i, "OPENABLE" ) || !parts[i].open) ) {
                found_obs = true;
                break;
            }
        }

        if( !found_obs ) { // not aimed at this tile and no obstacle here -- fly through
            return dmg;
        }
    }

    int target_part = random_entry( pl );

    // door motor mechanism is protected by closed doors
    if( part_flag( target_part, "DOOR_MOTOR" ) ) {
        // find the most strong openable that is not open
        int strongest_door_part = -1;
        int strongest_door_durability = INT_MIN;
        for( int part : pl ) {
            if( part_flag( part, "OPENABLE" ) && !parts[part].open ) {
                int door_durability = part_info( part ).durability;
                if (door_durability > strongest_door_durability) {
                   strongest_door_part = part;
                   strongest_door_durability = door_durability;
                }
            }
        }

        // if we found a closed door, target it instead of the door_motor
        if (strongest_door_part != -1) {
            target_part = strongest_door_part;
        }
    }

    int damage_dealt;

    int armor_part = part_with_feature( p, "ARMOR" );
    if( armor_part < 0 ) {
        // Not covered by armor -- damage part
        damage_dealt = damage_direct( target_part, dmg, type );
    } else {
        // Covered by armor -- hit both armor and part, but reduce damage by armor's reduction
        int protection = part_info( armor_part ).damage_reduction[ type ];
        // Parts on roof aren't protected
        bool overhead = part_flag( target_part, "ROOF" ) || part_info( target_part ).location == "on_roof";
        // Calling damage_direct may remove the damaged part
        // completely, therefore the other index (target_part) becomes
        // wrong if target_part > armor_part.
        // Damaging the part with the higher index first is save,
        // as removing a part only changes indices after the
        // removed part.
        if( armor_part < target_part ) {
            damage_direct( target_part, overhead ? dmg : dmg - protection, type );
            damage_dealt = damage_direct( armor_part, dmg, type );
        } else {
            damage_dealt = damage_direct( armor_part, dmg, type );
            damage_direct( target_part, overhead ? dmg : dmg - protection, type );
        }
    }

    return damage_dealt;
}

void vehicle::damage_all( int dmg1, int dmg2, damage_type type, const point &impact )
{
    if( dmg2 < dmg1 ) {
        std::swap( dmg1, dmg2 );
    }

    if( dmg1 < 1 ) {
        return;
    }

    for( size_t p = 0; p < parts.size(); p++ ) {
        int distance = 1 + square_dist( parts[p].mount.x, parts[p].mount.y, impact.x, impact.y );
        if( distance > 1 && part_info(p).location == part_location_structure &&
            !part_info(p).has_flag("PROTRUSION") ) {
            damage_direct( p, rng( dmg1, dmg2 ) / (distance * distance), type );
        }
    }
}

/**
 * Shifts all parts of the vehicle by the given amounts, and then shifts the
 * vehicle itself in the opposite direction. The end result is that the vehicle
 * appears to have not moved. Useful for re-zeroing a vehicle to ensure that a
 * (0, 0) part is always present.
 * @param delta How much to shift along each axis
 */
void vehicle::shift_parts( const point delta )
{
    // Don't invalidate the active item cache's location!
    active_items.subtract_locations( delta );
    for( auto &elem : parts ) {
        elem.mount -= delta;
    }

    decltype(labels) new_labels;
    for( auto &l : labels ) {
        new_labels.insert( label( l.x - delta.x, l.y - delta.y, l.text ) );
    }
    labels = new_labels;

    pivot_anchor[0] -= delta;
    refresh();

    //Need to also update the map after this
    g->m.reset_vehicle_cache( smz );

}

/**
 * Detect if the vehicle is currently missing a 0,0 part, and
 * adjust if necessary.
 * @return bool true if the shift was needed.
 */
bool vehicle::shift_if_needed() {
    if( !parts_at_relative(0, 0).empty() ) {
        // Shifting is not needed.
        return false;
    }
    //Find a frame, any frame, to shift to
    for ( size_t next_part = 0; next_part < parts.size(); ++next_part ) {
        if ( part_info(next_part).location == "structure"
                && !part_info(next_part).has_flag("PROTRUSION")
                && !parts[next_part].removed) {
            shift_parts( parts[next_part].mount );
            refresh();
            return true;
        }
    }
    // There are only parts with PROTRUSION left, choose one of them.
    for ( size_t next_part = 0; next_part < parts.size(); ++next_part ) {
        if ( !parts[next_part].removed ) {
            shift_parts( parts[next_part].mount );
            refresh();
            return true;
        }
    }
    return false;
}

int vehicle::break_off( int p, int dmg )
{
    /* Already-destroyed part - chance it could be torn off into pieces.
     * Chance increases with damage, and decreases with part max durability
     * (so lights, etc are easily removed; frames and plating not so much) */
    if( rng( 0, part_info(p).durability / 10 ) >= dmg ) {
        return dmg;
    }

    const auto pos = global_part_pos3( p );
    if( part_info(p).location == part_location_structure ) {
        // For structural parts, remove other parts first
        std::vector<int> parts_in_square = parts_at_relative( parts[p].mount.x, parts[p].mount.y );
        for( int index = parts_in_square.size() - 1; index >= 0; index-- ) {
            // Ignore the frame being destroyed
            if( parts_in_square[index] == p ) {
                continue;
            }

            if( parts[ parts_in_square[ index ] ].is_broken() ) {
                // Tearing off a broken part - break it up
                if( g->u.sees( pos ) ) {
                    add_msg(m_bad, _("The %s's %s breaks into pieces!"), name.c_str(),
                            parts[ parts_in_square[ index ] ].name().c_str() );
                }
                break_part_into_pieces(parts_in_square[index], pos.x, pos.y, true);
            } else {
                // Intact (but possibly damaged) part - remove it in one piece
                if( g->u.sees( pos ) ) {
                    add_msg(m_bad, _("The %1$s's %2$s is torn off!"), name.c_str(),
                            parts[ parts_in_square[ index ] ].name().c_str() );
                }
                item part_as_item = parts[parts_in_square[index]].properties_to_item();
                g->m.add_item_or_charges( pos, part_as_item );
            }
            remove_part( parts_in_square[index] );
        }
        /* After clearing the frame, remove it if normally legal to do
         * so (it's not holding the vehicle together). At a later date,
         * some more complicated system (such as actually making two
         * vehicles from the split parts) would be ideal. */
        if( can_unmount(p) ) {
            if( g->u.sees( pos ) ) {
                add_msg(m_bad, _("The %1$s's %2$s is destroyed!"),
                        name.c_str(), parts[ p ].name().c_str() );
            }
            break_part_into_pieces( p, pos.x, pos.y, true );
            remove_part(p);
        }
    } else {
        //Just break it off
        if( g->u.sees( pos ) ) {
            add_msg(m_bad, _("The %1$s's %2$s is destroyed!"),
                            name.c_str(), parts[ p ].name().c_str() );
        }

        break_part_into_pieces( p, pos.x, pos.y, true );
        remove_part( p );
    }

    return dmg;
}

bool vehicle::explode_fuel( int p, damage_type type )
{
    const itype_id &ft = part_info(p).fuel_type;
    struct fuel_explosion {
        // TODO: Move the values below to jsons
        int explosion_chance_hot ;
        int explosion_chance_cold;
        float explosion_factor;
        bool fiery_explosion;
        float fuel_size_factor;
    };

    static const std::map<itype_id, fuel_explosion> explosive_fuels = {{
        { fuel_type_gasoline,   { 2, 5, 1.0f, true, 0.1f } },
        { fuel_type_diesel,     { 20, 1000, 0.2f, false, 0.1f } }
    }};

    const auto iter = explosive_fuels.find( ft );
    if( iter == explosive_fuels.end() ) {
        // Not on the list means not explosive
        return false;
    }

    const fuel_explosion &data = iter->second;
    const int pow = 120 * (1 - exp(data.explosion_factor / -5000 * (parts[p].ammo_remaining() * data.fuel_size_factor)));
    //debugmsg( "damage check dmg=%d pow=%d amount=%d", dmg, pow, parts[p].amount );
    if( parts[ p ].is_broken() ) {
        leak_fuel( parts[ p ] );
    }

    int explosion_chance = type == DT_HEAT ? data.explosion_chance_hot : data.explosion_chance_cold;
    if( one_in( explosion_chance ) ) {
        g->u.add_memorial_log(pgettext("memorial_male","The fuel tank of the %s exploded!"),
            pgettext("memorial_female", "The fuel tank of the %s exploded!"),
            name.c_str());
        g->explosion( global_part_pos3( p ), pow, 0.7, data.fiery_explosion );
        mod_hp( parts[p], 0 - parts[ p ].hp(), DT_HEAT );
        parts[p].ammo_unset();
    }

    return true;
}

int vehicle::damage_direct( int p, int dmg, damage_type type )
{
    if( parts[p].is_broken() ) {
        return break_off( p, dmg );
    }

    int tsh = std::min( 20, part_info(p).durability / 10 );
    if( dmg < tsh && type != DT_TRUE ) {
        if( type == DT_HEAT && parts[p].is_tank() ) {
            explode_fuel( p, type );
        }

        return dmg;
    }

    dmg -= std::min<int>( dmg, part_info( p ).damage_reduction[ type ] );
    int dres = dmg - parts[p].hp();
    if( mod_hp( parts[ p ], 0 - dmg, type ) ) {
        insides_dirty = true;
        pivot_dirty = true;

        // destroyed parts lose any contained fuels, battery charges or ammo
        leak_fuel( parts [ p ] );

        for( const auto &e : parts[p].items ) {
            g->m.add_item_or_charges( global_part_pos3( p ), e );
        }
        parts[p].items.clear();

        invalidate_mass();
    }

    if( parts[p].is_tank() ) {
        explode_fuel( p, type );
    } else if( parts[ p ].is_broken() && part_flag(p, "UNMOUNT_ON_DAMAGE") ) {
        g->m.spawn_item( global_part_pos3( p ), part_info( p ).item, 1, 0, calendar::turn );
        remove_part( p );
    }

    return std::max( dres, 0 );
}

void vehicle::leak_fuel( vehicle_part &pt )
{
    // only liquid fuels from non-empty tanks can leak out onto map tiles
    if( !pt.is_tank() || pt.ammo_remaining() <= 0 ) {
        return;
    }

    // leak in random directions but prefer closest tiles and avoid walls or other obstacles
    auto tiles = closest_tripoints_first( 1, global_part_pos3( pt ) );
    tiles.erase( std::remove_if( tiles.begin(), tiles.end(), []( const tripoint& e ) {
        return !g->m.passable( e );
    } ), tiles.end() );

    // leak up to 1/3 of remaining fuel per iteration and continue until the part is empty
    auto *fuel = item::find_type( pt.ammo_current() );
    while( !tiles.empty() && pt.ammo_remaining() ) {
        int qty = pt.ammo_consume( rng( 0, std::max( pt.ammo_remaining() / 3, 1L ) ), global_part_pos3( pt ) );
        if( qty > 0 ) {
            g->m.add_item_or_charges( random_entry( tiles ), item( fuel, calendar::turn, qty ) );
        }
    }

    pt.ammo_unset();
}

std::map<itype_id, long> vehicle::fuels_left() const
{
    std::map<itype_id, long> result;
    for( const auto &p : parts ) {
        if( p.is_tank() && p.ammo_current() != "null" ) {
            result[ p.ammo_current() ] += p.ammo_remaining();
        }
    }
    return result;
}

bool vehicle::assign_seat( vehicle_part &pt, const npc& who )
{
    if( !pt.is_seat() || !pt.set_crew( who ) ) {
        return false;
    }

    // NPC's can only be assigned to one seat in the vehicle
    for( auto &e : parts ) {
        if( &e == &pt ) {
            continue; // skip this part
        }

        if( e.is_seat() ) {
            const npc *n = e.crew();
            if( n && n->getID() == who.getID() ) {
                e.unset_crew();
            }
        }
    }

    return true;
}

/**
 * Opens an openable part at the specified index. If it's a multipart, opens
 * all attached parts as well.
 * @param part_index The index in the parts list of the part to open.
 */
void vehicle::open(int part_index)
{
  if(!part_info(part_index).has_flag("OPENABLE")) {
    debugmsg("Attempted to open non-openable part %d (%s) on a %s!", part_index,
               parts[ part_index ].name().c_str(), name.c_str());
  } else {
    open_or_close(part_index, true);
  }
}

/**
 * Opens an openable part at the specified index. If it's a multipart, opens
 * all attached parts as well.
 * @param part_index The index in the parts list of the part to open.
 */
void vehicle::close(int part_index)
{
  if(!part_info(part_index).has_flag("OPENABLE")) {
    debugmsg("Attempted to close non-closeable part %d (%s) on a %s!", part_index,
               parts[ part_index ].name().c_str(), name.c_str());
  } else {
    open_or_close(part_index, false);
  }
}

bool vehicle::is_open(int part_index) const
{
  return parts[part_index].open;
}

void vehicle::open_all_at(int p)
{
    std::vector<int> parts_here = parts_at_relative(parts[p].mount.x, parts[p].mount.y);
    for( auto &elem : parts_here ) {
        if( part_flag( elem, VPFLAG_OPENABLE ) ) {
            // Note that this will open multi-square and non-multipart parts in the tile. This
            // means that adjacent open multi-square openables can still have closed stuff
            // on same tile after this function returns
            open( elem );
        }
    }
}

void vehicle::open_or_close(int const part_index, bool const opening)
{
    parts[part_index].open = opening ? 1 : 0;
    insides_dirty = true;
    g->m.set_transparency_cache_dirty( smz );

    if (!part_info(part_index).has_flag("MULTISQUARE")) {
        return;
    }

    /* Find all other closed parts with the same ID in adjacent squares.
     * This is a tighter restriction than just looking for other Multisquare
     * Openable parts, and stops trunks from opening side doors and the like. */
    for( size_t next_index = 0; next_index < parts.size(); ++next_index ) {
        if (parts[next_index].removed) {
            continue;
        }

        //Look for parts 1 square off in any cardinal direction
        const int dx = parts[next_index].mount.x - parts[part_index].mount.x;
        const int dy = parts[next_index].mount.y - parts[part_index].mount.y;
        const int delta = dx * dx + dy * dy;

        const bool is_near = (delta == 1);
        const bool is_id = part_info(next_index).get_id() == part_info(part_index).get_id();
        const bool do_next = !!parts[next_index].open ^ opening;

        if (is_near && is_id && do_next) {
            open_or_close(next_index, opening);
        }
    }
}

// A chance to stop skidding if moving in roughly the faced direction
void vehicle::possibly_recover_from_skid() {
    if( last_turn > 13 ) {
        // Turning on the initial skid is delayed, so move==face, initially. This filters out that case.
        return;
    }

    rl_vec2d mv = move_vec();
    rl_vec2d fv = face_vec();
    float dot = mv.dot_product(fv);
    // Threshold of recovery is Gaussianesque.

    if( fabs( dot ) * 100 > dice( 9,20 ) ){
        add_msg(_("The %s recovers from its skid."), name.c_str());
        skidding = false; // face_vec takes over.
        velocity *= dot; // Wheels absorb horizontal velocity.
        if(dot < -.8){
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
   float dot = mv.dot_product(fv);
   return velocity * dot;
}

rl_vec2d vehicle::velo_vec() const
{
    rl_vec2d ret;
    if(skidding)
       ret = move_vec();
    else
       ret = face_vec();
    ret = ret.normalized();
    ret = ret * velocity;
    return ret;
}

inline rl_vec2d degree_to_vec( double degrees )
{
    return rl_vec2d( cos( degrees * M_PI/180 ), sin( degrees * M_PI/180 ) );
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

float get_collision_factor(float const delta_v)
{
    if (std::abs(delta_v) <= 31) {
        return ( 1 - ( 0.9 * std::abs(delta_v) ) / 31 );
    } else {
        return 0.1;
    }
}

bool vehicle::is_foldable() const
{
    for (size_t i = 0; i < parts.size(); i++) {
        if (!part_flag(i, "FOLDABLE")) {
            return false;
        }
    }
    return true;
}

bool vehicle::restore(const std::string &data)
{
    std::istringstream veh_data(data);
    try {
        JsonIn json(veh_data);
        parts.clear();
        json.read(parts);
    } catch( const JsonError &e ) {
        debugmsg("Error restoring vehicle: %s", e.c_str());
        return false;
    }
    refresh();
    face.init(0);
    turn_dir = 0;
    turn(0);
    precalc_mounts(0, pivot_rotation[0], pivot_anchor[0]);
    precalc_mounts(1, pivot_rotation[1], pivot_anchor[1]);
    return true;
}

std::set<tripoint> &vehicle::get_points( const bool force_refresh )
{
    if( force_refresh || occupied_cache_time != calendar::turn ) {
        occupied_cache_time = calendar::turn;
        occupied_points.clear();
        tripoint pos = global_pos3();
        for( const auto &p : parts ) {
            const auto &pt = p.precalc[0];
            occupied_points.insert( tripoint( pos.x + pt.x, pos.y + pt.y, pos.z ) );
        }
    }

    return occupied_points;
}

inline int modulo(int v, int m) {
    // C++11: negative v and positive m result in negative v%m (or 0),
    // but this is supposed to be mathematical modulo: 0 <= v%m < m,
    const int r = v % m;
    // Adding m in that (and only that) case.
    return r >= 0 ? r : r + m;
}

bool is_sm_tile_outside( const tripoint &real_global_pos )
{
    const tripoint smp = ms_to_sm_copy( real_global_pos );
    const int px = modulo( real_global_pos.x, SEEX );
    const int py = modulo( real_global_pos.y, SEEY );
    auto sm = MAPBUFFER.lookup_submap( smp );
    if( sm == nullptr ) {
        debugmsg( "is_sm_tile_outside(): couldn't find submap %d,%d,%d", smp.x, smp.y, smp.z );
        return false;
    }

    if( px < 0 || px >= SEEX || py < 0 || py >= SEEY ) {
        debugmsg("err %d,%d", px, py);
        return false;
    }

    return !(sm->ter[px][py].obj().has_flag(TFLAG_INDOORS) ||
        sm->get_furn(px, py).obj().has_flag(TFLAG_INDOORS));
}

void vehicle::update_time( const time_point &update_to )
{
    if( smz < 0 ) {
        return;
    }

    const time_point update_from = last_update;
    if( update_to < update_from ) {
        // Special case going backwards in time - that happens
        last_update = update_to;
        return;
    }

    if( update_to >= update_from && update_to - update_from < 1_minutes ) {
        // We don't need to check every turn
        return;
    }
    last_update = update_to;

    // Weather stuff, only for z-levels >= 0
    // TODO: Have it wash cars from blood?
    if( funnels.empty() && solar_panels.empty() ) {
        return;
    }

    // Get one weather data set per vehicle, they don't differ much across vehicle area
    const tripoint veh_loc = real_global_pos3();
    auto accum_weather = sum_conditions( update_from, update_to, veh_loc );

    for( int idx : funnels ) {
        const auto &pt = parts[idx];

        // we need an unbroken funnel mounted on the exterior of the vehicle
        if( pt.is_broken() || !is_sm_tile_outside( veh_loc + pt.precalc[0] ) ) {
            continue;
        }

        // we need an empty tank (or one already containing water) below the funnel
        auto tank = std::find_if( parts.begin(), parts.end(), [&pt]( const vehicle_part &e ) {
            return pt.mount == e.mount && e.is_tank() && ( e.can_reload( "water" ) || e.can_reload( "water_clean" ) );
        } );

        if( tank == parts.end() ) {
            continue;
        }

        double area = pow( pt.info().size / units::legacy_volume_factor, 2 ) * M_PI;
        int qty = divide_roll_remainder( funnel_charges_per_turn( area, accum_weather.rain_amount ), 1.0 );
        double cost_to_purify = epower_to_power( ( qty + ( tank->can_reload( "water_clean" ) ? tank->ammo_remaining() : 0 ) )
                                  * item::find_type( "water_purifier" )->charges_to_use() );

        if( qty > 0 ) {
            if( has_part( global_part_pos3( pt ), "WATER_PURIFIER", true ) && ( fuel_left( "battery" ) > cost_to_purify  ) ) {
                tank->ammo_set( "water_clean", tank->ammo_remaining() + qty );
                discharge_battery( cost_to_purify );
            } else {
                tank->ammo_set( "water", tank->ammo_remaining() + qty );
            }
            invalidate_mass();
        }
    }

    if( !solar_panels.empty() ) {
        int epower = 0;
        for( int part : solar_panels ) {
            if( parts[ part ].is_broken() ) {
                continue;
            }

            const tripoint part_loc = veh_loc + parts[part].precalc[0];
            if( !is_sm_tile_outside( part_loc ) ) {
                continue;
            }

            epower += ( part_epower( part ) * accum_weather.sunlight ) / DAYLIGHT_LEVEL;
        }

        if( epower > 0 ) {
            add_msg( m_debug, "%s got %d epower from solar panels", name.c_str(), epower );
            charge_battery( epower_to_power( epower ) );
        }
    }
}

/*-----------------------------------------------------------------------------
 *                              VEHICLE_PART
 *-----------------------------------------------------------------------------*/
vehicle_part::vehicle_part()
    : mount( 0, 0 ), id( vpart_id::NULL_ID() ) {}

vehicle_part::vehicle_part( const vpart_id& vp, int const dx, int const dy, item&& obj )
    : mount( dx, dy ), id( vp ), base( std::move( obj ) )
{
        // Mark base item as being installed as a vehicle part
        base.item_tags.insert( "VEHICLE" );

    if( base.typeId() != vp->item ) {
        debugmsg( "incorrect vehicle part item, expected: %s, received: %s",
                  vp->item.c_str(), base.typeId().c_str() );
    }
}

vehicle_part::operator bool() const {
    return id != vpart_id::NULL_ID();
}

const item &vehicle_part::get_base() const
{
    return base;
}

void vehicle_part::set_base( const item &new_base )
{
    base = new_base;
}

item vehicle_part::properties_to_item() const
{
    item tmp = base;
    tmp.item_tags.erase( "VEHICLE" );

    // Cables get special handling: their target coordinates need to remain
    // stored, and if a cable actually drops, it should be half-connected.
    if( tmp.has_flag("CABLE_SPOOL") ) {
        tripoint local_pos = g->m.getlocal(target.first);
        if( !g->m.veh_at( local_pos ) ) {
            tmp.item_tags.insert("NO_DROP"); // That vehicle ain't there no more.
        }

        tmp.set_var( "source_x", target.first.x );
        tmp.set_var( "source_y", target.first.y );
        tmp.set_var( "source_z", target.first.z );
        tmp.set_var( "state", "pay_out_cable" );
        tmp.active = true;
    }

    return tmp;
}

std::string vehicle_part::name() const
{
    auto res = info().name();

    if( base.engine_displacement() > 0 ) {
        res.insert( 0, string_format( _( "%2.1fL " ), base.engine_displacement() / 100.0 ) );

    } else if( wheel_diameter() > 0 ) {
        res.insert( 0, string_format( _( "%d\" " ), wheel_diameter() ) );
    }

    if( base.is_faulty() ) {
        res += ( _( " (faulty)" ) );
    }

    if( base.has_var( "contained_name" ) ) {
        res += string_format( _( " holding %s" ), base.get_var( "contained_name" ) );
    }
    return res;
}

int vehicle_part::hp() const
{
    double dur = info().durability;
    return dur - ( dur * base.damage() / base.max_damage() );
}

float vehicle_part::damage() const
{
    return base.damage();
}

/** parts are considered broken at zero health */
bool vehicle_part::is_broken() const
{
    return base.damage() >= base.max_damage();
}

itype_id vehicle_part::ammo_current() const
{
    if( is_battery() ) {
        return "battery";
    }

    if( is_reactor() || is_turret() ) {
        return base.ammo_current();
    }

    if( is_tank() && !base.contents.empty() ) {
        return base.contents.front().typeId();
    }

    if( is_engine() ) {
        return info().fuel_type != "muscle" ? info().fuel_type : "null";
    }

    return "null";
}

long vehicle_part::ammo_capacity() const
{
    if( is_battery() || is_reactor() || is_turret() ) {
        return base.ammo_capacity();
    }

    if( base.is_watertight_container() ) {
        return base.get_container_capacity() / std::max( item::find_type( ammo_current() )->volume, units::from_milliliter( 1 ) );
    }

    return 0;
}

long vehicle_part::ammo_remaining() const
{
    if( is_battery() || is_reactor() || is_turret() ) {
        return base.ammo_remaining();
    }

    if( base.is_watertight_container() ) {
        return base.contents.empty() ? 0 : base.contents.back().charges;
    }

    return 0;
}

int vehicle_part::ammo_set( const itype_id &ammo, long qty )
{
    if( is_turret() ) {
        return base.ammo_set( ammo, qty ).ammo_remaining();
    }

    if( is_battery() || is_reactor() ) {
        base.ammo_set( ammo, qty >= 0 ? qty : ammo_capacity() );
        return base.ammo_remaining();
    }

    const itype *liquid = item::find_type( ammo );
    if( is_tank() && liquid->phase == LIQUID ) {
        base.contents.clear();
        auto stack = units::legacy_volume_factor / std::max( liquid->stack_size, 1 );
        long limit = units::from_milliliter( ammo_capacity() ) / stack;
        base.emplace_back( ammo, calendar::turn, qty >= 0 ? std::min( qty, limit ) : limit );
        return qty;
    }

    return -1;
}

void vehicle_part::ammo_unset() {
    if( is_battery() || is_reactor() || is_turret() ) {
        base.ammo_unset();

    } else if( is_tank() ) {
        base.contents.clear();
    }
}

long vehicle_part::ammo_consume( long qty, const tripoint& pos )
{
    if( is_battery() || is_reactor() ) {
        return base.ammo_consume( qty, pos );
    }

    int res = std::min( ammo_remaining(), qty );

    if( base.is_watertight_container() && !base.contents.empty() ) {
        item& liquid = base.contents.back();
        liquid.charges -= res;
        if( liquid.charges == 0 ) {
            base.contents.clear();
        }
    }

    return res;
}

float vehicle_part::consume_energy( const itype_id &ftype, float energy )
{
    if( base.contents.empty() || ( !is_battery() && !is_reactor() && !base.is_watertight_container() ) ) {
        return 0.0f;
    }

    item &fuel = base.contents.back();
    if( fuel.typeId() != ftype ) {
        return 0.0f;
    }

    assert( fuel.is_fuel() );
    float energy_per_unit = fuel.fuel_energy();
    long charges_to_use = static_cast<int>( std::ceil( energy / energy_per_unit ) );
    if( charges_to_use > fuel.charges ) {
        long had_charges = fuel.charges;
        base.contents.clear();
        return had_charges * energy_per_unit;
    }

    fuel.charges -= charges_to_use;
    return charges_to_use * energy_per_unit;
}

bool vehicle_part::can_reload( const itype_id &obj ) const
{
    // first check part is not destroyed and can contain ammo
    if( is_broken() || ammo_capacity() <= 0 ) {
        return false;
    }

    if( is_reactor() ) {
        return base.is_reloadable_with( obj );
    }

    if( is_tank() ) {
        if( !obj.empty() ) {
            // forbid filling tanks with non-liquids
            if( item::find_type( obj )->phase != LIQUID ) {
                return false;
            }
            // prevent mixing of different liquids
            if( ammo_current() != "null" && ammo_current() != obj ) {
                return false;
            }
        }
        // For tanks with set type, prevent filling with different types
        if( info().fuel_type != fuel_type_none && info().fuel_type != obj ) {
            return false;
        }
        return ammo_remaining() < ammo_capacity();
    }

    return false;
}

bool vehicle_part::fill_with( item &liquid, long qty )
{
    if( liquid.active || liquid.rotten() ) {
        // cannot refill using active liquids (those that rot) due to #18570
        return false;
    }

    if( !is_tank() || !can_reload( liquid.typeId() ) ) {
        return false;
    }

    base.fill_with( liquid, qty );
    return true;
}

const std::set<fault_id>& vehicle_part::faults() const
{
    return base.faults;
}

std::set<fault_id> vehicle_part::faults_potential() const
{
    return base.faults_potential();
}

bool vehicle_part::fault_set( const fault_id &f )
{
    if( !faults_potential().count( f ) ) {
        return false;
    }
    base.faults.insert( f );
    return true;
}

int vehicle_part::wheel_area() const
{
    return base.is_wheel() ? base.type->wheel->diameter * base.type->wheel->width : 0;
}

/** Get wheel diameter (inches) or return 0 if part is not wheel */
int vehicle_part::wheel_diameter() const
{
    return base.is_wheel() ? base.type->wheel->diameter : 0;
}

/** Get wheel width (inches) or return 0 if part is not wheel */
int vehicle_part::wheel_width() const
{
    return base.is_wheel() ? base.type->wheel->width : 0;
}

npc * vehicle_part::crew() const
{
    if( is_broken() || crew_id < 0 ) {
        return nullptr;
    }

    npc *const res = g->critter_by_id<npc>( crew_id );
    if( !res ) {
        return nullptr;
    }
    return res->is_friend() ? res : nullptr;
}

bool vehicle_part::set_crew( const npc &who )
{
    if( who.is_dead_state() || !who.is_friend() ) {
        return false;
    }
    if( is_broken() || ( !is_seat() && !is_turret() ) ) {
        return false;
    }
    crew_id = who.getID();
    return true;
}

void vehicle_part::unset_crew()
{
    crew_id = -1;
}

void vehicle_part::reset_target( const tripoint &pos )
{
    target.first = pos;
    target.second = pos;
}

bool vehicle_part::is_engine() const
{
    return info().has_flag( VPFLAG_ENGINE );
}

bool vehicle_part::is_light() const
{
    const auto &vp = info();
    return vp.has_flag( VPFLAG_CONE_LIGHT ) ||
           vp.has_flag( VPFLAG_CIRCLE_LIGHT ) ||
           vp.has_flag( VPFLAG_AISLE_LIGHT ) ||
           vp.has_flag( VPFLAG_DOME_LIGHT ) ||
           vp.has_flag( VPFLAG_ATOMIC_LIGHT );
}

bool vehicle_part::is_tank() const
{
    return base.is_watertight_container();
}

bool vehicle_part::is_battery() const
{
    return base.is_magazine() && base.ammo_type() == "battery";
}

bool vehicle_part::is_reactor() const
{
    return info().has_flag( "REACTOR" );
}

bool vehicle_part::is_turret() const
{
    return base.is_gun();
}

bool vehicle_part::is_seat() const
{
    return info().has_flag( "SEAT" );
}

const vpart_info &vehicle_part::info() const
{
    if( !info_cache ) {
        info_cache = &id.obj();
    }
    return *info_cache;
}

void vehicle::invalidate_mass()
{
    mass_dirty = true;
    mass_center_precalc_dirty = true;
    mass_center_no_precalc_dirty = true;
    // Anything that affects mass will also affect the pivot
    pivot_dirty = true;
}

void vehicle::refresh_mass() const
{
    calc_mass_center( true );
}

void vehicle::calc_mass_center( bool use_precalc ) const
{
    units::quantity<float, units::mass::unit_type> xf = 0;
    units::quantity<float, units::mass::unit_type> yf = 0;
    units::mass m_total = 0;
    for( size_t i = 0; i < parts.size(); i++ )
    {
        if( parts[i].removed ) {
            continue;
        }

        units::mass m_part = 0;
        const auto &pi = part_info( i );
        m_part += parts[i].base.weight();
        for( const auto &j : get_items( i ) ) {
            //m_part += j.type->weight;
            // Change back to the above if it runs too slowly
            m_part += j.weight();
        }

        if( pi.has_flag( VPFLAG_BOARDABLE ) && parts[i].has_flag( vehicle_part::passenger_flag ) ) {
            const player *p = get_passenger( i );
            // Sometimes flag is wrongly set, don't crash!
            m_part += p != nullptr ? p->get_weight() : units::mass( 0 );
        }

        if( use_precalc ) {
            xf += parts[i].precalc[0].x * m_part;
            yf += parts[i].precalc[0].y * m_part;
        } else {
            xf += parts[i].mount.x * m_part;
            yf += parts[i].mount.y * m_part;
        }

        m_total += m_part;
    }

    mass_cache = m_total;
    mass_dirty = false;

    const float x = xf / mass_cache;
    const float y = yf / mass_cache;
    if( use_precalc ) {
        mass_center_precalc.x = round( x );
        mass_center_precalc.y = round( y );
        mass_center_precalc_dirty = false;
    } else {
        mass_center_no_precalc.x = round( x );
        mass_center_no_precalc.y = round( y );
        mass_center_no_precalc_dirty = false;
    }
}

void vehicle::use_washing_machine( int p ) {
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
    /* captured animals take up all the cargo space */
    /*
    if( base.has_var( "contained_name" ) ) {
        part_info( part ).size = 0;
    } else {
        part_info( part ).size = base.get_container_capacity();
    }
    */
    parts[part].set_base( base );
}
