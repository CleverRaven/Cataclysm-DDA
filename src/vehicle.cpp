#include "vehicle.h"
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
#include "ui.h"
#include "debug.h"
#include "sounds.h"
#include "translations.h"
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
#include "map_iterator.h"

#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <set>
#include <queue>
#include <math.h>
#include <array>

/*
 * Speed up all those if ( blarg == "structure" ) statements that are used everywhere;
 *   assemble "structure" once here instead of repeatedly later.
 */
static const itype_id fuel_type_gasoline("gasoline");
static const itype_id fuel_type_diesel("diesel");
static const itype_id fuel_type_battery("battery");
static const itype_id fuel_type_plutonium("plut_cell");
static const itype_id fuel_type_plasma("plasma");
static const itype_id fuel_type_water("water_clean");
static const itype_id fuel_type_muscle("muscle");
static const std::string part_location_structure("structure");

const skill_id skill_mechanics( "mechanics" );

const std::array<fuel_type, 7> &get_fuel_types()
{

    static const std::array<fuel_type, 7> fuel_types = {{
        fuel_type {fuel_type_gasoline,  c_ltred,   100, 1},
        fuel_type {fuel_type_diesel,    c_brown,   100, 1},
        fuel_type {fuel_type_battery,   c_yellow,  1,   1},
        fuel_type {fuel_type_plutonium, c_ltgreen, 1,   1000},
        fuel_type {fuel_type_plasma,    c_ltblue,  100, 100},
        fuel_type {fuel_type_water,     c_ltcyan,  1,   1},
        fuel_type {fuel_type_muscle,    c_white,   0,   1}
    }};

    return fuel_types;
}

int fuel_charges_to_amount_factor( const itype_id &ftype )
{
    for( auto & ft : get_fuel_types() ) {
        if( ft.id == ftype ) {
            return ft.charges_to_amount_factor;
        }
    }
    return 1;
}

enum vehicle_controls {
 toggle_cruise_control,
 toggle_lights,
 toggle_overhead_lights,
 toggle_dome_lights,
 toggle_aisle_lights,
 toggle_turrets,
 toggle_stereo,
 toggle_tracker,
 activate_horn,
 release_control,
 control_cancel,
 convert_vehicle,
 toggle_reactor,
 toggle_engine,
 toggle_fridge,
 toggle_recharger,
 cont_engines,
 try_disarm_alarm,
 trigger_alarm,
 toggle_doors,
 cont_turrets,
 manual_fire,
 toggle_camera,
 release_remote_control,
 toggle_chimes,
 toggle_plow,
 toggle_planter,
 toggle_reaper,
 toggle_scoop
};

class vehicle::turret_ammo_data {
public:
    /** Usable charges, may be 0 if there are none. */
    long charges = 0L;
    /**
     * Cached instance of the turret gun. It is always valid and set to the actual turret gun.
     * If @ref charges is > 0, it is guaranteed to have a proper curammo.
     */
    item gun;
private:
    /** The ammo type that should be consumed when the gun has fired. Note that this may be
     * different from the curammo of the @ref gun item (e.g. for charger guns that fire pseudo
     * ammo, but consume battery power). It must not be used when @ref source is @ref NONE. */
    const itype *ammo = nullptr;
    /** Whether the @ref ammo is to be taken from a tank (using @ref vehicle::drain) or from
     * an item in the vehicle part of the turret, or not at all (special UPS guns only). */
    enum {
        TANK,
        CARGO,
        NONE,
    } source = NONE;
    /** TODO: document me */
    int charge_mult = 1;
public:
    /**
     * We can not fire because the gun needs UPS charges, but there is not enough battery power
     * left in the vehicle. Will be false for all guns that don't need UPS charges.
     */
    bool is_missing_ups_charges = false;
    /** Init the struct based on a turret at the given vehicle part of the given vehicle. */
    turret_ammo_data( const vehicle &veh, int part );
    /** Consume charges according to @ref source and @ref ammo from the vehicle. */
    void consume( vehicle &veh, int part, long charges_consumed ) const;
};

// Map stack methods.
size_t vehicle_stack::size() const
{
    return mystack->size();
}

bool vehicle_stack::empty() const
{
    return mystack->empty();
}

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

std::list<item>::iterator vehicle_stack::begin()
{
    return mystack->begin();
}

std::list<item>::iterator vehicle_stack::end()
{
    return mystack->end();
}

std::list<item>::const_iterator vehicle_stack::begin() const
{
    return mystack->cbegin();
}

std::list<item>::const_iterator vehicle_stack::end() const
{
    return mystack->cend();
}

std::list<item>::reverse_iterator vehicle_stack::rbegin()
{
    return mystack->rbegin();
}

std::list<item>::reverse_iterator vehicle_stack::rend()
{
    return mystack->rend();
}

std::list<item>::const_reverse_iterator vehicle_stack::rbegin() const
{
    return mystack->crbegin();
}

std::list<item>::const_reverse_iterator vehicle_stack::rend() const
{
    return mystack->crend();
}

item &vehicle_stack::front()
{
    return mystack->front();
}

item &vehicle_stack::operator[]( size_t index )
{
    return *(std::next(mystack->begin(), index));
}

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
}

vehicle::~vehicle()
{
}

bool vehicle::player_in_control(player const& p) const
{
    int veh_part;

    if( g->m.veh_at( p.pos(), veh_part ) == this &&
        part_with_feature(veh_part, VPFLAG_CONTROLS, false) >= 0 && p.controlling_vehicle ) {
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

void vehicle::load (std::ifstream &stin)
{
    std::string type;
    getline(stin, type);
    this->type = vproto_id( type );

    std::stringstream derp;
    derp << type;
    JsonIn jsin(derp);
    try {
        deserialize(jsin);
    } catch( const JsonError &jsonerr ) {
        debugmsg("Bad vehicle json\n%s", jsonerr.c_str() );
    }
    refresh(); // part index lists are lost on save??
    shift_if_needed();
}

/** Checks all parts to see if frames are missing (as they might be when
 * loading from a game saved before the vehicle construction rules overhaul). */
void vehicle::add_missing_frames()
{
    static const vpart_str_id frame_id( "frame_vertical" );
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
            if(!found) {
                //No frame here! Install one.
                vehicle_part new_part;
                new_part.set_id( frame_part.id );
                new_part.mount.x = next_x;
                new_part.mount.y = next_y;
                new_part.hp = frame_part.durability;
                new_part.amount = 0;
                new_part.blood = 0;
                new_part.bigness = 0;
                parts.push_back (new_part);
            }
        }

        locations_checked.insert(mount_location);
    }
}

void vehicle::save (std::ofstream &stout)
{
    serialize(stout);
    stout << std::endl;
    return;
}

void vehicle::init_state(int init_veh_fuel, int init_veh_status)
{
    bool destroySeats = false;
    bool destroyControls = false;
    bool destroyTank = false;
    bool destroyEngine = false;
    bool destroyTires = false;
    bool blood_covered = false;
    bool blood_inside = false;
    bool has_no_key = false;
    bool destroyAlarm = false;

    std::map<vpart_id, int> consistent_bignesses;

    // More realistically it should be -5 days old
    last_update_turn = 0;

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
        //most cars should have a desroyed alarm
        destroyAlarm = true;
    }

    //Provide some variety to non-mint vehicles
    if( veh_status != 0 ) {
        //Leave engine running in some vehicles, if the engine has not been destroyed
        if( veh_fuel_mult > 0 && all_parts_with_feature("ENGINE", true).size() > 0 &&
            one_in(8) && !destroyEngine && !has_no_key && has_engine_type_not(fuel_type_muscle, true) ) {
            engine_on = true;
        }

        //Turn on lights on some vehicles
        if( one_in(20) ) {
            lights_on = true;
        }

        // dome lights could be on if the person was hurrily looking for things (or being looted)
        if( one_in(16) ) {
            dome_lights_on = true;
        }

        // aisle lights being on would be common for the vehicles they are in (vans, RVs, 18-wheelers, etc)
        if( one_in(8) ) {
            aisle_lights_on = true;
        }

        //Turn flasher/overhead lights on separately (more likely since these are rarer)
        if( one_in(4) ) {
            overhead_lights_on = true;
        }

        if( one_in(10) ) {
            blood_covered = true;
        }

        if( one_in(8) ) {
            blood_inside = true;
        }

        //Fridge should always start out activated if present
        if( all_parts_with_feature("FRIDGE").size() > 0 ) {
            fridge_on = true;
        }
    }

    bool blood_inside_set = false;
    int blood_inside_x = 0;
    int blood_inside_y = 0;
    for( size_t p = 0; p < parts.size(); p++ ) {
        if( part_flag(p, "VARIABLE_SIZE") ) { // generate its bigness attribute.?
            const vpart_info &vpinfo = parts[p].info();
            if( consistent_bignesses.count( vpinfo.id ) < 1 ) {
                //generate an item for this type, & cache its bigness
                item tmp (vpinfo.item, 0);
                consistent_bignesses[vpinfo.id] = tmp.bigness;
            }
            parts[p].bigness = consistent_bignesses[vpinfo.id];
        }
        if( part_flag( p, "REACTOR" ) ) {
            // De-hardcoded reactors. Should always start active
            reactor_on = true;
        }

        if( part_flag(p, "FUEL_TANK") ) {   // set fuel status
            parts[p].amount = part_info(p).size * veh_fuel_mult / 100;
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
            parts[p].hp = part_info(p).durability;
        } else {
            //a bit of initial damage :)
            //clamp 4d8 to the range of [8,20]. 8=broken, 20=undamaged.
            int broken = 8;
            int unhurt = 20;
            int roll = dice( 4, 8 );
            if(roll < unhurt){
                if (roll <= broken) {
                    parts[p].hp = 0;
                    parts[p].amount = 0; //empty broken batteries and fuel tanks
                } else {
                    parts[p].hp = ((float)(roll - broken) /
                                   (unhurt - broken)) * part_info(p).durability;
                }
            } else {
                parts[p].hp= part_info(p).durability;
            }
            if ((destroySeats && (part_flag(p, "SEAT") || part_flag(p, "SEATBELT"))) ||
                (destroyControls && (part_flag(p, "CONTROLS") || part_flag(p, "SECURITY"))) ||
                (destroyEngine && part_flag(p, "ENGINE")) ||
                (destroyTires && part_flag(p, VPFLAG_WHEEL)) ||
                (destroyAlarm && part_flag(p, "SECURITY"))) {
                parts[p].hp = 0;
            }

            // Fuel tanks should be emptied as well
            if (destroyTank && (part_flag(p, "FUEL_TANK") || part_flag(p, "NEEDS_BATTERY_MOUNT"))){
                parts[p].hp = 0;
                parts[p].amount = 0;
            }

            //Solar panels have 25% of being destroyed
            if (part_flag(p, "SOLAR_PANEL") && one_in(4)) {
                parts[p].hp = 0;
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
                // coords relative to mount point; the center is always a seat
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
        if (part_flag(p, "SECURITY") && (has_no_key) && parts[p].hp > 0) {
            is_locked = true;
        }
    }
}
/**
 * Smashes up a vehicle that has already been placed; used for generating
 * very damaged vehicles. Additionally, any spot where two vehicles overlapped
 * (ie, any spot with multiple frames) will be completely destroyed, as that
 * was the collision point.
 */
void vehicle::smash() {
    for (size_t part_index = 0; part_index < parts.size(); part_index++) {
        //Skip any parts already mashed up or removed.
        if(parts[part_index].hp == 0 || parts[part_index].removed) {
            continue;
        }

        vehicle_part next_part = parts[part_index];
        std::vector<int> parts_in_square = parts_at_relative(next_part.mount.x, next_part.mount.y);
        int structures_found = 0;
        for (auto &square_part_index : parts_in_square) {
            if (part_info(square_part_index).location == part_location_structure) {
                structures_found++;
            }
        }

        if(structures_found > 1) {
            //Destroy everything in the square
            for (auto &square_part_index : parts_in_square) {
                parts[square_part_index].hp = 0;
            }
            continue;
        }

        //Everywhere else, drop by 10-120% of max HP (anything over 100 = broken)
        int damage = (int) (dice(1, 12) * 0.1 * part_info(part_index).durability);
        parts[part_index].hp -= damage;
        if (parts[part_index].hp <= 0) {
            parts[part_index].hp = 0;
            parts[part_index].amount = 0;
        }
    }
}

const std::string vehicle::disp_name()
{
    return string_format( _("the %s"), name.c_str() );
}

void vehicle::control_doors() {
    std::vector< int > door_motors = all_parts_with_feature( "DOOR_MOTOR", true );
    std::vector< int > doors_with_motors; // Indices of doors
    std::vector< tripoint > locations; // Locations used to display the doors
    doors_with_motors.reserve( door_motors.size() * 2); // it is possible to have one door to open and one to close for single motor
    locations.reserve( door_motors.size() * 2);
    if( door_motors.empty() ) {
        debugmsg( "vehicle::control_doors called but no door motors found" );
        return;
    }

    uimenu pmenu;
    pmenu.title = _("Select door to toggle");
    int doors[2]; // one door to open and one to close
    for( int p : door_motors ) {
        doors[0] = next_part_to_open(p);
        doors[1] = next_part_to_close(p);
        for (int door : doors) {
            if (door == -1)
                continue;

            int val = doors_with_motors.size();
            doors_with_motors.push_back(door);
            locations.push_back(tripoint(global_pos() + parts[p].precalc[0], smz));
            const char *actname = parts[door].open ? _("Close") : _("Open");
            pmenu.addentry(val, true, MENU_AUTOASSIGN, "%s %s", actname, part_info(door).name.c_str());
        }
    }

    pmenu.addentry( doors_with_motors.size(), true, 'q', _("Cancel") );
    pointmenu_cb callback( locations );
    pmenu.callback = &callback;
    pmenu.w_y = 0; // Move the menu so that we can see our vehicle
    pmenu.query();

    if( pmenu.ret >= 0 && pmenu.ret < (int)doors_with_motors.size() ) {
        int part = doors_with_motors[pmenu.ret];
        open_or_close( part, !(parts[part].open) );
    }
}

void vehicle::control_engines() {
    int e_toggle = 0;
    bool dirty = false;
    //count active engines
    int active_count = 0;
    for (size_t e = 0; e < engines.size(); ++e){
        if (is_part_on(engines[e])){
            active_count++;
        }
    }

    //show menu until user finishes
    while( e_toggle >= 0 && e_toggle < (int)engines.size() ) {
        e_toggle = select_engine();
        if( e_toggle >= 0 && e_toggle < (int)engines.size() &&
            (active_count > 1 || !is_part_on(engines[e_toggle]))) {
            active_count += (!is_part_on(engines[e_toggle])) ? 1 : -1;
            toggle_specific_engine(e_toggle, !is_part_on(engines[e_toggle]));
            dirty = true;
        }
    }

    if( !dirty ) { return; }

    // if current velocity greater than new configuration safe speed
    // drop down cruise velocity.
    int safe_vel = safe_velocity();
    if( velocity > safe_vel ) {
        cruise_velocity = safe_vel;
    }

    if( engine_on ) {
        add_msg( _("You turn off the %s's engines to change their configurations."), name.c_str() );
        engine_on = false;
    } else if( !g->u.controlling_vehicle ) {
        add_msg( _("You change the %s's engine configuration."), name.c_str() );
        return;
    }

    start_engines();
}

int vehicle::select_engine() {
    uimenu tmenu;
    std::string name;
    tmenu.text = _("Toggle which?");
    for( size_t e = 0; e < engines.size(); ++e ) {
        name = part_info(engines[e]).name;
        tmenu.addentry( e, true, -1, "[%s] %s",
                        ((parts[engines[e]].enabled) ? "x" : " ") , name.c_str() );
    }

    tmenu.addentry(-1, true, 'q', _("Finish"));
    tmenu.query();
    return tmenu.ret;
}

void vehicle::toggle_specific_engine(int e,bool on) {
    toggle_specific_part( engines[e], on );
}
void vehicle::toggle_specific_part(int p,bool on) {
    parts[p].enabled = on;
}
bool vehicle::is_engine_type_on(int e, const itype_id &ft) const
{
    return is_engine_on(e) && is_engine_type(e, ft);
}

bool vehicle::has_engine_type(const itype_id &ft, bool const enabled) const
{
    for( size_t e = 0; e < engines.size(); ++e ) {
        if( is_engine_type(e, ft) && (!enabled || is_engine_on(e)) ) {
            return true;
        }
    }
    return false;
}
bool vehicle::has_engine_type_not(const itype_id &ft, bool const enabled) const
{
    for( size_t e = 0; e < engines.size(); ++e ) {
        if( !is_engine_type(e, ft) && (!enabled || is_engine_on(e)) ) {
            return true;
        }
    }
    return false;
}

bool vehicle::is_engine_type(const int e, const itype_id  &ft) const {
    return part_info(engines[e]).fuel_type == ft;
}

bool vehicle::is_engine_on(int const e) const
{
    return (parts[engines[e]].hp > 0) && is_part_on(engines[e]);
}

bool vehicle::is_part_on(int const p) const
{
    return parts[p].enabled;
}

bool vehicle::is_active_engine_at(int const x, int const y) const
{
    for( size_t e = 0; e < engines.size(); ++e ) {
        if( is_engine_on(e) &&
            parts[engines[e]].mount.x == x &&
            parts[engines[e]].mount.y == y ) {
            return true;
        }
    }
    return false;
}

bool vehicle::is_alternator_on(int const a) const
{
    return (parts[alternators[a]].hp > 0) && is_active_engine_at(
        parts[alternators[a]].mount.x, parts[alternators[a]].mount.y );
}

bool vehicle::has_security_working() const
{
    bool found_security = false;
    for (size_t s = 0; s < speciality.size(); s++){
        if (part_flag(speciality[s], "SECURITY") && parts[speciality[s]].hp > 0){
            found_security = true;
            break;
        }
    }
    return found_security;
}

bool vehicle::interact_vehicle_locked()
{
    if (is_locked){
        const inventory &crafting_inv = g->u.crafting_inventory();
        add_msg(_("You don't find any keys in the %s."), name.c_str());
        if( crafting_inv.has_items_with_quality( "SCREW", 1, 1 ) ) {
            if (query_yn(_("You don't find any keys in the %s. Attempt to hotwire vehicle?"),
                            name.c_str())) {

                int mechanics_skill = g->u.skillLevel( skill_mechanics );
                int hotwire_time = 6000 / ((mechanics_skill > 0)? mechanics_skill : 1);
                //assign long activity
                g->u.assign_activity(ACT_HOTWIRE_CAR, hotwire_time, -1, INT_MIN, _("Hotwire"));
                g->u.activity.values.push_back(global_x());//[0]
                g->u.activity.values.push_back(global_y());//[1]
                g->u.activity.values.push_back(g->u.skillLevel( skill_mechanics ));//[2]
            } else {
                if( has_security_working() && query_yn(_("Trigger the %s's Alarm?"), name.c_str()) ) {
                    is_alarm_on = true;
                } else {
                    add_msg(_("You leave the controls alone."));
                }
            }
        } else {
            add_msg(_("You could use a screwdriver to hotwire it."));
        }
    }

    return !(is_locked);
}

void vehicle::smash_security_system(){

    //get security and controls location
    int s = -1;
    int c = -1;
    for (size_t d = 0; d < speciality.size(); d++){
        int p = speciality[d];
        if (part_flag(p, "SECURITY") && parts[p].hp > 0){
            s = p;
            c = part_with_feature(s, "CONTROLS");
            break;
        }
    }
    //controls and security must both be valid
    if (c >= 0 && s >= 0){
        int skill = g->u.skillLevel( skill_mechanics );
        int percent_controls = 70 / (1 + skill);
        int percent_alarm = (skill+3) * 10;
        int rand = rng(1,100);

        if (percent_controls > rand) {
            damage_direct (c, part_info(c).durability / 4);

            if (parts[c].removed || parts[c].hp <= 0){
                g->u.controlling_vehicle = false;
                is_alarm_on = false;
                add_msg(_("You destroy the controls..."));
            } else {
                add_msg(_("You damage the controls."));
            }
        }
        if (percent_alarm > rand) {
            damage_direct (s, part_info(s).durability / 5);
            //chance to disable alarm immediately
            if (percent_alarm / 4 > rand) {
                is_alarm_on = false;
            }
        }
        add_msg((is_alarm_on) ? _("The alarm keeps going.") : _("The alarm stops."));
    } else {
        debugmsg("No security system found on vehicle.");
    }
}

void vehicle::use_controls(const tripoint &pos)
{
    uimenu menu;
    menu.return_invalid = true;
    menu.text = _("Vehicle controls");

    int vpart;
    if (!interact_vehicle_locked()) {
        return;
    }

    bool has_basic_controls = false;
    bool has_electronic_controls = false;
    bool remotely_controlled = g->remoteveh() == this;

    if (g->m.veh_at(pos, vpart) == this) {
        if ( g->u.controlling_vehicle ) {
            // Always have this option
            // Let go without turning the engine off.
            menu.addentry( release_control, true, 'l', _("Let go of controls") );
        }
        std::vector<int> parts_for_check = parts_at_relative( parts[vpart].mount.x,
                                                              parts[vpart].mount.y);
        // iterate over all parts in the selected tile
        for (size_t p = 0; p < parts_for_check.size(); ++p) {
            if (part_flag(parts_for_check[p], "CONTROLS")) {
                has_basic_controls = true;
            }
            if (part_flag(parts_for_check[p], "CTRL_ELECTRONIC")) {
                has_electronic_controls = true;
            }
        }
    } else if( remotely_controlled ) {
        menu.addentry( release_remote_control, true, 'l', _("Stop controlling") );
        // iterate over all parts
        for( size_t p = 0; p < parts.size(); ++p ) {
            if (part_flag(p, "CTRL_ELECTRONIC")) {
                has_electronic_controls = true;
            }
        }
    }
    if( !has_basic_controls && !has_electronic_controls ) {
        add_msg(m_info, _("No controls there."));
        return;
    }
    bool has_lights = false;
    bool has_stereo = false;
    bool has_chimes = false;
    bool has_overhead_lights = false;
    bool has_horn = false;
    bool has_turrets = false;
    bool has_tracker = false;
    bool has_reactor = false;
    bool has_engine = false;
    bool has_mult_engine = false;
    bool has_fridge = false;
    bool has_recharger = false;
    bool can_trigger_alarm = false;
    bool has_door_motor = false;
    bool has_camera = false;
    bool has_camera_control = false;
    bool has_aisle_lights = false;
    bool has_dome_lights = false;
    bool has_plow = false;
    bool has_planter = false;
    bool has_scoop = false;
    bool has_reaper = false;

    for( size_t p = 0; p < parts.size(); p++ ) {
        if (part_flag(p, "CONE_LIGHT")) {
            has_lights = true;
        }
        if (part_flag(p, "CIRCLE_LIGHT")) {
            has_overhead_lights = true;
        }
        if (part_flag(p, "LIGHT")) {
            has_lights = true;
        }
        if (part_flag(p, "AISLE_LIGHT")) {
            has_aisle_lights = true;
        }
        if (part_flag(p, "DOME_LIGHT")) {
            has_dome_lights = true;
        }
        else if (part_flag(p, "TURRET")) {
            has_turrets = true;
        }
        else if (part_flag(p, "HORN")) {
            has_horn = true;
        }
        else if (part_flag(p, "TRACK")) {
            has_tracker = true;
        }
        else if (part_flag(p, "STEREO")) {
            has_stereo = true;
        }
        else if (part_flag(p, "CHIMES")) {
            has_chimes = true;
        }
        else if( part_flag( p, "REACTOR" ) ) {
            has_reactor = true;
        }
        else if (part_flag(p, "ENGINE")) {
            has_mult_engine = has_engine;
            has_engine = true;
        }
        else if (part_flag(p, "FRIDGE")) {
            has_fridge = true;
        }
        else if (part_flag(p, "RECHARGE")) {
            has_recharger = true;
        } else if (part_flag(p, "SECURITY") && !is_alarm_on && parts[p].hp > 0) {
            can_trigger_alarm = true;
        } else if (part_flag(p, "DOOR_MOTOR")) {
            has_door_motor = true;
        } else if( part_flag( p, "CAMERA" ) ) {
            if( part_flag( p, "CAMERA_CONTROL" ) ) {
                has_camera_control = true;
            } else {
                has_camera = true;
            }
        } else if( part_flag(p,"PLOW") ) {
            has_plow = true;
        } else if( part_flag(p,"PLANTER") ) {
            has_planter = true;
        } else if( part_flag(p,"SCOOP") ) {
            has_scoop = true;
        } else if( part_flag(p,"REAPER") ) {
            has_reaper = true;
        }
    }

    // Toggle engine on/off, stop driving if we are driving.
    if( has_engine ) {
        if( g->u.controlling_vehicle || ( remotely_controlled && engine_on ) ) {
            menu.addentry( toggle_engine, true, 'e', _("Stop driving") );
        } else if( has_engine_type_not(fuel_type_muscle, true ) ) {
            menu.addentry( toggle_engine, true, 'e', engine_on ?
                           _("Turn off the engine") : _("Turn on the engine") );
        }
        if( has_electronic_controls  ) {
            menu.addentry( toggle_cruise_control, true, 'c', cruise_on ?
                           _("Disable cruise control") : _("Enable cruise control") );
        }
    }

    if( is_alarm_on && velocity == 0 && !remotely_controlled ) {
        menu.addentry( try_disarm_alarm, true, 'z', _("Try to disarm alarm.") );
    }

    // Lights if they are there - Note you can turn them on even when damaged, they just don't work
    if ( has_electronic_controls && has_lights ) {
        menu.addentry( toggle_lights, true, 'h', lights_on ?
                       _("Turn off headlights") : _("Turn on headlights") );
    }

    if ( has_electronic_controls && has_stereo ) {
        menu.addentry( toggle_stereo, true, 'm', stereo_on ?
                       _("Turn off stereo") : _("Turn on stereo") );
    }

    if (has_electronic_controls && has_chimes) {
        menu.addentry( toggle_chimes, true, 'i', chimes_on ?
                       _("Turn off chimes") : _("Turn on chimes") );
    }

   if ( has_electronic_controls && has_overhead_lights ) {
        menu.addentry( toggle_overhead_lights, true, 'v', overhead_lights_on ?
                       _("Turn off overhead lights") : _("Turn on overhead lights") );
   }


    if ( has_electronic_controls && has_dome_lights && fuel_left( fuel_type_battery, true ) ) {
        menu.addentry( toggle_dome_lights, true, 'D', dome_lights_on ?
                       _("Turn off dome lights") : _("Turn on dome lights") );
    }

    if ( has_electronic_controls && has_aisle_lights ) {
        menu.addentry( toggle_aisle_lights, true, 'A', aisle_lights_on ?
                       _("Turn off aisle lights") : _("Turn on aisle lights") );
    }

    //Honk the horn!
    if (has_horn) {
        menu.addentry( activate_horn, true, 'o', _("Honk horn") );
    }

    // Turrets: off or burst mode
    if ( has_electronic_controls && has_turrets ) {
        menu.addentry( toggle_turrets, true, 't', turret_mode == turret_mode_off ?
                       _("Enable turrets") : _("Disable turrets") );
    }

    // Turn the fridge on/off
    if ( has_electronic_controls && has_fridge ) {
        menu.addentry( toggle_fridge, true, 'f', fridge_on ? _("Turn off fridge") : _("Turn on fridge") );
    }

    // Turn the recharging station on/off
    if ( has_electronic_controls && has_recharger ) {
        menu.addentry( toggle_recharger, true, 'r', recharger_on ? _("Turn off recharger") : _("Turn on recharger") );
    }

    // Tracking on the overmap
    if (has_tracker) {
        menu.addentry( toggle_tracker, true, 'g', tracking_on ?
                       _("Disable tracking device") : _("Enable tracking device") );
    }

    const bool can_be_folded = is_foldable();
    const bool is_convertible = (tags.count("convertible") > 0);
    if( ( can_be_folded || is_convertible ) && !remotely_controlled ) {
        menu.addentry( convert_vehicle, true, 'f', string_format( _( "Fold %s" ), name.c_str() ) );
    }

    // Turn the reactor on/off
    if( has_reactor ) {
        menu.addentry( toggle_reactor, true, 'k', reactor_on ? _("Turn off reactor") : _("Turn on reactor") );
    }
    // Toggle doors remotely
    if ( has_electronic_controls && has_door_motor ) {
        menu.addentry( toggle_doors, true, 'd', _("Toggle door") );
    }
    // control an engine
    if (has_mult_engine) {
        menu.addentry( cont_engines, true, 'y', _("Control individual engines") );
    }
    // start alarm
    if ( has_electronic_controls && can_trigger_alarm ) {
        menu.addentry( trigger_alarm, true, 'p', _("Trigger alarm") );
    }
    // cycle individual turret modes
    if( has_turrets ) {
        menu.addentry( cont_turrets, true, 'x', _("Configure individual turrets") );
        menu.addentry( manual_fire, true, 'w', _("Aim turrets manually") );
    }
    // toggle cameras
    if( has_electronic_controls && (camera_on || ( has_camera && has_camera_control )) ) {
        menu.addentry( toggle_camera, true, 'M', camera_on ?
                       _("Turn off camera system") : _("Turn on camera system") );
    }
    if( has_electronic_controls && has_plow ){
        menu.addentry( toggle_plow, true, MENU_AUTOASSIGN, plow_on ?
                       _("Turn plow off") : _("Turn plow on") );
    }
    if( has_electronic_controls && has_planter ){
        menu.addentry( toggle_planter, true, 'P', planter_on ?
                       _("Turn planter off") : _("Turn planter on") );
    }
    if( has_electronic_controls && has_scoop ) {
        menu.addentry( toggle_scoop, true, 'S', scoop_on ?
                       _("Turn off scoop system") : _("Turn on scoop system") );
    }
    if( has_electronic_controls && has_reaper ){
        menu.addentry( toggle_reaper, true, 'H', reaper_on ?  _("Turn off reaper") : _("Turn on reaper") );
    }
    menu.addentry( control_cancel, true, ' ', _("Do nothing") );

    menu.query();

    switch( static_cast<vehicle_controls>( menu.ret ) ) {
    case trigger_alarm:
        is_alarm_on = true;
        add_msg(_("You trigger the alarm"));
        break;
    case cont_engines:
        control_engines();
        break;
    case try_disarm_alarm:
        smash_security_system();
        break;
    case toggle_cruise_control:
        cruise_on = !cruise_on;
        add_msg((cruise_on) ? _("Cruise control turned on") : _("Cruise control turned off"));
        break;
    case toggle_aisle_lights:
        if(aisle_lights_on || fuel_left(fuel_type_battery, true)) {
            aisle_lights_on = !aisle_lights_on;
            add_msg((aisle_lights_on) ? _("Aisle lights turned on") : _("Aisle lights turned off"));
        } else {
            add_msg(_("The aisle lights won't come on!"));
        }
        break;
    case toggle_dome_lights:
        if(dome_lights_on || fuel_left(fuel_type_battery, true)) {
            dome_lights_on = !dome_lights_on;
            add_msg((dome_lights_on) ? _("Dome lights turned on") : _("Dome lights turned off"));
        } else {
            add_msg(_("The dome lights won't come on!"));
        }
        break;
    case toggle_lights:
        if(lights_on || fuel_left(fuel_type_battery, true) ) {
            lights_on = !lights_on;
            add_msg((lights_on) ? _("Headlights turned on") : _("Headlights turned off"));
        } else {
            add_msg(_("The headlights won't come on!"));
        }
        break;
    case toggle_stereo:
        if( ( stereo_on || fuel_left( fuel_type_battery, true ) ) ) {
            stereo_on = !stereo_on;
            add_msg( stereo_on ? _("Turned on music") : _("Turned off music") );
        } else {
            add_msg(_("The stereo won't come on!"));
        }
        break;
    case toggle_chimes:
        if( ( chimes_on || fuel_left( fuel_type_battery, true ) ) ) {
            chimes_on = !chimes_on;
            add_msg( chimes_on ? _("Turned on chimes") : _("Turned off chimes") );
        } else {
            add_msg(_("The chimes won't come on!"));
        }
        break;
    case toggle_overhead_lights:
        if( overhead_lights_on ) {
            overhead_lights_on = false;
            add_msg(_("Overhead lights turned off"));
        } else if( fuel_left(fuel_type_battery, true) ) {
            overhead_lights_on = true;
            add_msg(_("Overhead lights turned on"));
        } else {
            add_msg(_("The lights won't come on!"));
        }
        break;
    case activate_horn:
        honk_horn();
        break;
    case toggle_turrets:
        cycle_global_turret_mode();
        break;
    case toggle_fridge:
        if( fridge_on ) {
            fridge_on = false;
            add_msg(_("Fridge turned off"));
        } else if (fuel_left(fuel_type_battery, true)) {
            fridge_on = true;
            add_msg(_("Fridge turned on"));
        } else {
            add_msg(_("The fridge won't turn on!"));
        }
        break;
    case toggle_recharger:
        if( recharger_on ) {
            recharger_on = false;
            add_msg(_("Recharger turned off"));
        } else if (fuel_left(fuel_type_battery, true)) {
            recharger_on = true;
            add_msg(_("Recharger turned on"));
        } else {
            add_msg(_("The recharger won't turn on!"));
        }
        break;
    case toggle_reactor:
    {
        bool can_toggle = reactor_on;
        if( !can_toggle ) {
            for( int p : reactors ) {
                if( parts[p].hp > 0 && parts[p].amount > 0 ) {
                    can_toggle = true;
                    break;
                }
            }
        }

        if( can_toggle ) {
            reactor_on = !reactor_on;
            add_msg((reactor_on) ? _("Reactor turned on") :
                       _("Reactor turned off"));
        }
        else {
            add_msg(_("The reactor won't turn on!"));
        }
    }
        break;
    case toggle_engine:
        if( g->u.controlling_vehicle || ( remotely_controlled && engine_on ) ) {
            //if we are controlling the vehicle, stop it.
            if (engine_on && has_engine_type_not(fuel_type_muscle, true)){
                add_msg(_("You turn the engine off and let go of the controls."));
            } else {
                add_msg(_("You let go of the controls."));
            }
            engine_on = false;
            g->u.controlling_vehicle = false;
            g->setremoteveh( nullptr );
        } else if (engine_on) {
            if (has_engine_type_not(fuel_type_muscle, true))
                add_msg(_("You turn the engine off."));
            engine_on = false;
        } else {
            start_engines();
        }
        break;
    case release_control:
        g->u.controlling_vehicle = false;
        add_msg(_("You let go of the controls."));
        break;
    case release_remote_control:
        g->u.controlling_vehicle = false;
        g->setremoteveh( nullptr );
        add_msg(_("You stop controlling the vehicle."));
        break;
    case convert_vehicle:
        if( fold_up() ) {
            return; // `this` has been deleted!
        }
        break;
    case toggle_tracker:
        if (tracking_on)
        {
            overmap_buffer.remove_vehicle( this );
            tracking_on = false;
            add_msg(_("tracking device disabled"));
        } else if (fuel_left(fuel_type_battery, true))
        {
            overmap_buffer.add_vehicle( this );
            tracking_on = true;
            add_msg(_("tracking device enabled"));
        } else {
            add_msg(_("tracking device won't turn on"));
        }
        break;
    case toggle_doors:
        control_doors();
        break;
    case cont_turrets:
        control_turrets();
        break;
    case manual_fire:
        aim_turrets();
        break;
    case toggle_camera:
        if( camera_on ) {
            camera_on = false;
            add_msg( _("Camera system disabled") );
        } else if( fuel_left(fuel_type_battery, true) ) {
            camera_on = true;
            add_msg( _("Camera system enabled") );
        } else {
            add_msg( _("Camera system won't turn on") );
        }
        break;
    case toggle_plow:
        add_msg( plow_on ? _("Plow system stopped"): _("Plow system started"));
        plow_on = !plow_on;
        break;
    case toggle_planter:
        add_msg(planter_on ? _("Planter system stopped"): _("Planter system started"));
        planter_on = !planter_on;
        break;
    case control_cancel:
        break;
    case toggle_reaper:
        add_msg(reaper_on?_("Reaper turned off"):_("Reaper turned on"));
        reaper_on = !reaper_on;
        break;
    case toggle_scoop:
        scoop_on = !scoop_on;
        break;
    }
    refresh();
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
    item bicycle;
    if (can_be_folded) {
        bicycle.make( "generic_folded_vehicle" );
    } else {
        bicycle.make( "folding_bicycle" );
    }

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
    // some of them (like bigness), some are expect to be
    // vehicle specific and therefor constant (like id, mount).
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
        bicycle.set_var( "weight", total_mass() * 1000 );
        bicycle.set_var( "volume", total_folded_volume() );
        bicycle.set_var( "name", string_format(_("folded %s"), name.c_str()) );
        bicycle.set_var( "vehicle_name", name );
        // TODO: a better description?
        bicycle.set_var( "description", string_format(_("A folded %s."), name.c_str()) );
    }

    g->m.add_item_or_charges( g->u.pos(), bicycle );
    g->m.destroy_vehicle(this);

    // TODO: take longer to fold bigger vehicles
    // TODO: make this interruptable
    g->u.moves -= 500;
    return true;
}

double vehicle::engine_cold_factor( const int e )
{
    if( !is_engine_type( e, fuel_type_diesel ) ) { return 0.0; }

    return 1.0 - (std::max( 0, std::min( 40, g->get_temperature() ) ) / 40.0);
}

int vehicle::engine_start_time( const int e )
{
    if( !is_engine_on( e ) || is_engine_type( e, fuel_type_muscle ) ||
        !fuel_left( part_info( engines[e] ).fuel_type ) ) { return 0; }

    const double dmg = 1.0 - ((double)parts[engines[e]].hp / part_info( engines[e] ).durability);
    const double cold_factor = engine_cold_factor( e );

    return 100 + (part_power( engines[e], true ) / 16) + (100 * dmg) + (40 * cold_factor);
}

bool vehicle::start_engine( const int e )
{
    if( !is_engine_on( e ) ) { return false; }

    const vpart_info &einfo = part_info( engines[e] );

    if( !fuel_left( einfo.fuel_type ) ) {
        if( einfo.fuel_type == fuel_type_muscle ) {
            add_msg( _("The %s's mechanism is out of reach!"), name.c_str() );
        } else {
            add_msg( _("Looks like the %1$s is out of %2$s."), einfo.name.c_str(),
                item::nname( einfo.fuel_type ).c_str() );
        }
        return false;
    }

    const double dmg = 1.0 - ((double)parts[engines[e]].hp / einfo.durability);
    const int engine_power = part_power( engines[e], true );
    const double cold_factor = engine_cold_factor( e );

    if( einfo.fuel_type != fuel_type_muscle ) {
        if( einfo.fuel_type == fuel_type_gasoline && dmg > 0.75 && one_in( 20 ) ) {
            backfire( e );
        } else {
            const tripoint pos = global_pos3() + parts[engines[e]].precalc[0];
            sounds::ambient_sound( pos, engine_start_time( e ) / 10, "" );
        }
    }

    if( einfo.fuel_type == fuel_type_gasoline || einfo.fuel_type == fuel_type_diesel ) {
        // Small engines can be started without a battery (pull start or kick start)
        if( engine_power >= 50 ) {
            const int penalty = ((engine_power * dmg) / 2) + ((engine_power * cold_factor) / 5);
            if( discharge_battery( (engine_power + penalty) / 10, true ) != 0 ) {
                add_msg( _("The %s makes a rapid clicking sound."), einfo.name.c_str() );
                return false;
            }
        }

        // Damaged engines have a chance of failing to start
        if( x_in_y( dmg * 100, 120 - (20 * cold_factor) ) ) {
            if( one_in( 2 ) ) {
                add_msg( _("The %s makes a deep clunking sound."), einfo.name.c_str() );
            } else {
                add_msg( _("The %s makes a terrible clanking sound."), einfo.name.c_str() );
            }
            return false;
        }
    }

    return true;
}

void vehicle::start_engines( const bool take_control )
{
    int has_engine = false;
    int start_time = 0;

    for( size_t e = 0; e < engines.size(); ++e ) {
        has_engine = has_engine || is_engine_on( e );
        start_time = std::max( start_time, engine_start_time( e ) );
    }

    if( !has_engine ) {
        add_msg( m_info, _("The %s doesn't have an engine!"), name.c_str() );
        return;
    }

    if( take_control && !g->u.controlling_vehicle ) {
        g->u.controlling_vehicle = true;
        add_msg( _("You take control of the %s."), name.c_str() );
    }

    g->u.assign_activity( ACT_START_ENGINES, start_time );
    g->u.activity.placement = global_pos3() - g->u.pos();
    g->u.activity.values.push_back( take_control );
}

void vehicle::backfire( const int e )
{
    const int power = part_power( engines[e], true );
    const tripoint pos = global_pos3() + parts[engines[e]].precalc[0];
    sounds::ambient_sound( pos, 40 + (power / 30), "BANG!" );
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
        if( ( horn_type.id != vpart_str_id( "horn_bicycle" ) ) && no_power ) {
            continue;
        }
        if( ! honked ) {
            add_msg( _("You honk the horn!") );
            honked = true;
        }
        //Get global position of horn
        const auto horn_pos = global_pos3() + parts[p].precalc[0];
        //Determine sound
        if( horn_type.bonus >= 40 ) {
            sounds::sound( horn_pos, horn_type.bonus, _("HOOOOORNK!") );
        } else if( horn_type.bonus >= 20 ) {
            sounds::sound( horn_pos, horn_type.bonus, _("BEEEP!") );
        } else {
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

    const bool odd_turn = (calendar::turn % 2 == 0);
    for( size_t p = 0; p < parts.size(); ++p ) {
        if( !part_flag( p, "BEEPER" ) ) {
            continue;
        }
        if( ( odd_turn && part_flag( p, VPFLAG_EVENTURN ) ) ||
            ( !odd_turn && part_flag( p, VPFLAG_ODDTURN ) ) ) {
            continue;
        }

        const vpart_info &beeper_type = part_info( p );
        //Get global position of backup beeper
        const tripoint beeper_pos = global_pos3() + parts[p].precalc[0];
        //Determine sound
        sounds::sound( beeper_pos, beeper_type.bonus, _("beep!") );
    }
}

void vehicle::play_music()
{
    for( size_t p = 0; p < parts.size(); ++p ) {
        if ( ! part_flag( p, "STEREO" ) )
            continue;
        // epower is negative for consumers
        if( drain( fuel_type_battery, -part_epower( p ) ) == 0 ) {
            stereo_on = false;
            return;
        }
        const auto radio_pos = tripoint( global_pos() + parts[p].precalc[0], smz );
        iuse::play_music( &g->u, radio_pos, 15, 50 );
    }
}

void vehicle::play_chimes()
{
    if (one_in(3)) {
        for( size_t p = 0; p < parts.size(); ++p ) {
            if ( ! part_flag( p, "CHIMES" ) )
                continue;
            // epower is negative for consumers
            if( drain( fuel_type_battery, -part_epower( p ) ) == 0 ) {
                chimes_on = false;
                return;
            }
            const auto chimes_pos = tripoint( global_pos() + parts[p].precalc[0], smz );
            sounds::sound(chimes_pos, 40, _("a simple melody blaring from the loudspeakers.") );
        }
    }
}

const vpart_info& vehicle::part_info (int index, bool include_removed) const
{
    if (index < (int)parts.size()) {
        if (!parts[index].removed || include_removed) {
            return parts[index].info();
        }
    }
    return vpart_str_id::NULL_ID.obj();
}

// engines & alternators all have power.
// engines provide, whilst alternators consume.
int vehicle::part_power(int const index, bool const at_full_hp) const
{
    if( !part_flag(index, VPFLAG_ENGINE) &&
        !part_flag(index, VPFLAG_ALTERNATOR) ) {
       return 0; // not an engine.
    }
    int pwr;
    if( part_flag (index, VPFLAG_VARIABLE_SIZE) ) { // example: 2.42-L V-twin engine
       pwr = parts[index].bigness;
    } else { // example: foot crank
       pwr = part_info(index).power;
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
        return pwr * (0.25 + (0.75 * ((double)parts[index].hp / part_info(index).durability)));
    } else {
        return pwr * parts[index].hp / part_info(index).durability;
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
    return e * parts[index].hp / part_info(index).durability;
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
 * Returns whether or not the vehicle part with the given id can be mounted in
 * the specified square.
 * @param dx The local x-coordinate to mount in.
 * @param dy The local y-coordinate to mount in.
 * @param id The id of the part to install.
 * @return true if the part can be mounted, false if not.
 */
bool vehicle::can_mount(int const dx, int const dy, const vpart_str_id &id) const
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
        if( part.loadid == other_part.loadid ||
                (!part.location.empty() && part.location == other_part.location)) {
            return false;
        }
        // Until we have an interface for handling multiple components with CARGO space,
        // exclude them from being mounted in the same tile.
        if( part.has_flag( "CARGO" ) && other_part.has_flag( "CARGO" ) ) {
            return false;
        }

    }

    //All parts after the first must be installed on or next to an existing part
    if(!parts.empty()) {
        if(!has_structural_part(dx, dy) &&
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
    if (part.has_flag("CURTAIN")) {
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

    std::vector<int> parts_in_square = parts_at_relative(dx, dy, false);

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

    //Structural parts have extra requirements
    if(part_info(p).location == part_location_structure) {

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
 * @param excluded The part that is being removed and, therefore, should not
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
                vehicle_part next_part = parts[parts_there[0]];
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
 * @param hp The starting HP of the part. If negative, default to max HP.
 * @param force true if the part should be installed even if not legal,
 *              false if illegal part installation should fail.
 * @return false if the part could not be installed, true otherwise.
 */
int vehicle::install_part (int dx, int dy, const vpart_str_id &id, int hp, bool force)
{
    if (!force && !can_mount (dx, dy, id)) {
        return -1;  // no money -- no ski!
    }
    item tmp(id.obj().item, 0);
    vehicle_part new_part(id, dx, dy, &tmp);
    if (hp >= 0) {
        new_part.hp = hp;
    }
    return install_part(dx, dy, new_part);
}

int vehicle::install_part (int dx, int dy, const vpart_str_id &id, const item &used_item)
{
    if (!can_mount (dx, dy, id)) {
        return -1;  // no money -- no ski!
    }
    vehicle_part new_part(id, dx, dy, &used_item);
    return install_part(dx, dy, new_part);
}

int vehicle::install_part( int dx, int dy, const vehicle_part &new_part )
{
    parts.push_back( new_part );
    parts.back().mount.x = dx;
    parts.back().mount.y = dy;
    refresh();
    return parts.size() - 1;
}

/**
 * Mark a part as removed from the vehicle.
 * @return bool true if the vehicle's 0,0 point shifted.
 */
bool vehicle::remove_part (int p)
{
    if (p >= (int)parts.size()) {
        debugmsg("Tried to remove part %d but only %d parts!", p, parts.size());
        return false;
    }
    if (parts[p].removed) {
        /* This happens only when we had to remove part, because it was depending on
         * other part (using recursive remove_part() call) - currently curtain
         * depending on presence of window and seatbelt depending on presence of seat.
         */
        return false;
    }
    if (part_flag(p, "TRACK")) {
        // disable tracking if there are no other trackers installed.
        if (tracking_on)
        {
            bool has_tracker = false;
            for (int i = 0; i != (int)parts.size(); i++){
                if (i != p && part_flag(i, "TRACK")){
                    has_tracker = true;
                    break;
                }
            }
            if (!has_tracker){ // disable tracking
                overmap_buffer.remove_vehicle( this );
                tracking_on = false;
            }
        }
    }

    if (part_flag(p, "ATOMIC_LIGHT")) {
        // disable atomic lights if this was the last one
        has_atomic_lights = false;
        for (int i = 0; i != (int)parts.size(); i++){
            if (i != p && part_flag(i, "ATOMIC_LIGHT")){
                has_atomic_lights = true;
                break;
            }
        }
    }

    int x = parts[p].precalc[0].x;
    int y = parts[p].precalc[0].y;
    tripoint part_loc( global_x() + x, global_y() + y, smz );

    // if a windshield is removed (usually destroyed) also remove curtains
    // attached to it.
    if(part_flag(p, "WINDOW")) {
        int curtain = part_with_feature(p, "CURTAIN", false);
        if (curtain >= 0) {
            item it = parts[curtain].properties_to_item();
            g->m.add_item_or_charges( part_loc, it, 2 );
            remove_part(curtain);
        }
    }

    //Ditto for seatbelts
    if(part_flag(p, "SEAT")) {
        int seatbelt = part_with_feature(p, "SEATBELT", false);
        if (seatbelt >= 0) {
            item it = parts[seatbelt].properties_to_item();
            g->m.add_item_or_charges( part_loc, it, 2 );
            remove_part(seatbelt);
        }
    }

    // Unboard any entities standing on removed boardable parts
    if(part_flag(p, "BOARDABLE")) {
        std::vector<int> bp = boarded_parts();
        for( auto &elem : bp ) {
            if( elem == p ) {
                g->m.unboard_vehicle( part_loc );
            }
        }
    }

    // Update current engine configuration if needed
    if(part_flag(p, "ENGINE") && engines.size() > 1){
        bool any_engine_on = false;

        for(auto &e : engines) {
            if(e != p && is_part_on(e)) {
                any_engine_on = true;
                break;
            }
        }

        if(!any_engine_on) {
            engine_on = false;
            for(auto &e : engines) {
                toggle_specific_part(e, true);
            }
        }
    }

    parts[p].removed = true;
    removed_part_count++;

    // If the player is currently working on the removed part, stop them as it's futile now.
    const player_activity &act = g->u.activity;
    if( act.type == ACT_VEHICLE && act.moves_left > 0 && act.values.size() > 6 ) {
        if( g->m.veh_at( tripoint( act.values[0], act.values[1], g->u.posz() ) ) == this ) {
            if( act.values[6] >= p ) {
                g->u.cancel_activity();
                add_msg( m_info, _( "The vehicle part you were working on has gone!" ) );
            }
        }
    }

    const auto iter = labels.find( label( parts[p].mount.x, parts[p].mount.y ) );
    if( iter != labels.end() ) {
        if( parts_at_relative( parts[p].mount.x, parts[p].mount.y, false ).empty() ) {
            labels.erase( iter );
        }
    }

    for( auto &i : get_items(p) ) {
        // Note: this can spawn items on the other side of the wall!
        tripoint dest( part_loc.x + rng( -3, 3 ), part_loc.y + rng( -3, 3 ), smz );
        g->m.add_item_or_charges( dest, i );
    }
    g->m.dirty_vehicle_list.insert(this);
    refresh();
    return shift_if_needed();
}

void vehicle::part_removal_cleanup() {
    bool changed = false;
    for (std::vector<vehicle_part>::iterator it = parts.begin(); it != parts.end(); /* noop */) {
        if ((*it).removed) {
            auto items = get_items( std::distance( parts.begin(), it ) );
            while( !items.empty() ) {
                items.erase( items.begin() );
            }
            it = parts.erase(it);
            changed = true;
        }
        else {
            ++it;
        }
    }
    removed_part_count = 0;
    if (changed || parts.empty()) {
        refresh();
        if(parts.empty()) {
            g->m.destroy_vehicle(this);
            return;
        } else {
            g->m.update_vehicle_cache( this, smz );
        }
    }
    shift_if_needed();
    refresh(); // Rebuild cached indices
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
        // to build the component (smash a veh box that took 10 lumps of steel,
        // find 12 steel lumps scattered after atom-smashing it with a tree trunk)
            const int actual_x = scatter ? x + rng(-SCATTER_DISTANCE, SCATTER_DISTANCE) : x;
            const int actual_y = scatter ? y + rng(-SCATTER_DISTANCE, SCATTER_DISTANCE) : y;
            tripoint dest( actual_x, actual_y, smz );
            g->m.add_item_or_charges( dest, piece );
    }
}

std::vector<int> vehicle::parts_at_relative (const int dx, const int dy, bool const use_cache) const
{
    if ( use_cache == false ) {
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

int vehicle::part_with_feature (int part, vpart_bitflags const flag, bool unbroken) const
{
    if (part_flag(part, flag)) {
        return part;
    }
    const auto it = relative_parts.find( parts[part].mount );
    if ( it != relative_parts.end() ) {
        const std::vector<int> & parts_here = it->second;
        for (auto &i : parts_here) {
            if (part_flag(i, flag) && (!unbroken || parts[i].hp > 0)) {
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
        if( part_flag( elem, flag ) && ( !unbroken || parts[elem].hp > 0 ) ) {
            return elem;
        }
    }
    return -1;
}

/**
 * Returns the label at the coordinates given (mount coordinates)
 */
std::string const& vehicle::get_label(int const x, int const y) const
{
    auto const it = labels.find(label(x, y));
    if (it == labels.end()) {
        static std::string const fallback;
        return fallback;
    }

    return it->text;
}

/**
 * Sets the label at the coordinates given (mount coordinates)
 */
void vehicle::set_label(int x, int y, std::string text)
{
    auto const it = labels.find(label(x, y));
    if (it == labels.end()) {
        labels.insert(label(x, y, std::move(text)));
    } else {
        // labels should really be a map
        labels.insert(labels.erase(it), label(x, y, std::move(text)));
    }
}

int vehicle::next_part_to_close(int p, bool outside)
{
    std::vector<int> parts_here = parts_at_relative(parts[p].mount.x, parts[p].mount.y);

    // We want reverse, since we close the outermost thing first (curtains), and then the innermost thing (door)
    for(std::vector<int>::reverse_iterator part_it = parts_here.rbegin();
        part_it != parts_here.rend();
        ++part_it)
    {

        if(part_flag(*part_it, VPFLAG_OPENABLE)
           && parts[*part_it].hp > 0  // 0 HP parts can't be opened or closed
           && parts[*part_it].open == 1
           && (!outside || !part_flag(*part_it, "OPENCLOSE_INSIDE")) )
        {
            return *part_it;
        }
    }
    return -1;
}

int vehicle::next_part_to_open(int p, bool outside)
{
    std::vector<int> parts_here = parts_at_relative(parts[p].mount.x, parts[p].mount.y);

    // We want forwards, since we open the innermost thing first (curtains), and then the innermost thing (door)
    for( auto &elem : parts_here ) {
        if( part_flag( elem, VPFLAG_OPENABLE ) && parts[elem].hp > 0 && parts[elem].open == 0 &&
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
                (!unbroken || parts[part_index].hp > 0)) {
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
                (!unbroken || parts[part_index].hp > 0)) {
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
      if(part->get_id() == next_part.get_id() &&
              part->mount == next_part.mount &&
              part->hp == next_part.hp) {
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
        return parts[displayed_part].hp <= 0 ?
            part_info(displayed_part).sym_broken : part_info(displayed_part).sym;
    }
}

// similar to part_sym(int p) but for use when drawing SDL tiles. Called only by cata_tiles during draw_vpart
// vector returns at least 1 element, max of 2 elements. If 2 elements the second denotes if it is open or damaged
const vpart_str_id &vehicle::part_id_string(int const p, char &part_mod) const
{
    part_mod = 0;
    if( p < 0 || p >= (int)parts.size() || parts[p].removed ) {
        return NULL_ID;
    }

    int displayed_part = part_displayed_at(parts[p].mount.x, parts[p].mount.y);
    const vpart_str_id &idinfo = parts[displayed_part].get_id();

    if (part_flag (displayed_part, VPFLAG_OPENABLE) && parts[displayed_part].open) {
        part_mod = 1; // open
    } else if (parts[displayed_part].hp <= 0){
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
    if( OPTIONS["VEHICLE_ARMOR_COLOR"] ) {
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
            col = c_ltred;
        } else if (parts[displayed_part].hp <= 0) {
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
 * @param w The window to draw in.
 * @param y1 The y-coordinate to start drawing at.
 * @param width The width of the window.
 * @param p The index of the part being examined.
 * @param hl The index of the part to highlight (if any).
 */
int vehicle::print_part_desc(WINDOW *win, int y1, int width, int p, int hl /*= -1*/) const
{
    if (p < 0 || p >= (int)parts.size()) {
        return y1;
    }
    std::vector<int> pl = this->parts_at_relative(parts[p].mount.x, parts[p].mount.y);
    int y = y1;
    for (size_t i = 0; i < pl.size(); i++)
    {
        int dur = part_info (pl[i]).durability;
        int per_cond = parts[pl[i]].hp * 100 / (dur < 1? 1 : dur);
        nc_color col_cond = getDurabilityColor(per_cond);

        std::string partname;
        // part bigness, if that's relevant.
        if (part_flag(pl[i], "VARIABLE_SIZE") && part_flag(pl[i], "ENGINE")) {
            //~ 2.8-Liter engine
            partname = string_format(_("%4.2f-Liter %s"),
                                     (float)(parts[pl[i]].bigness) / 100,
                                     part_info(pl[i]).name.c_str());
        } else if (part_flag(pl[i], "VARIABLE_SIZE") && part_flag(pl[i], "WHEEL")) {
            //~ 14" wheel
            partname = string_format(_("%d\" %s"),
                                     parts[pl[i]].bigness,
                                     part_info(pl[i]).name.c_str());
        } else {
            partname = part_info(pl[i]).name;
        }
        if( part_flag( pl[i], "CARGO" ) ) {
            //~ used/total volume of a cargo vehicle part
            partname += string_format(_(" (vol: %d/%d)"), stored_volume( pl[i] ), max_volume( pl[i] ) );
        }

        bool armor = part_flag(pl[i], "ARMOR");
        std::string left_sym, right_sym;
        if(armor) {
            left_sym = "("; right_sym = ")";
        } else if(part_info(pl[i]).location == part_location_structure) {
            left_sym = "["; right_sym = "]";
        } else {
            left_sym = "-"; right_sym = "-";
        }

        mvwprintz(win, y, 1, (int)i == hl? hilite(c_ltgray) : c_ltgray, "%s", left_sym.c_str());
        mvwprintz(win, y, 2, (int)i == hl? hilite(col_cond) : col_cond, "%s", partname.c_str());
        mvwprintz(win, y, 2 + utf8_width(partname), (int)i == hl? hilite(c_ltgray) : c_ltgray, "%s", right_sym.c_str());
//         mvwprintz(win, y, 3 + utf8_width(part_info(pl[i]).name), c_ltred, "%d", parts[pl[i]].blood);

        if (i == 0 && is_inside(pl[i])) {
            //~ indicates that a vehicle part is inside
            mvwprintz(win, y, width-2-utf8_width(_("In")), c_ltgray, _("In"));
        } else if (i == 0) {
            //~ indicates that a vehicle part is outside
            mvwprintz(win, y, width-2-utf8_width(_("Out")), c_ltgray, _("Out"));
        }
        y++;
    }

    // print the label for this location
    const std::string label = get_label(parts[p].mount.x, parts[p].mount.y);
    if (label != "") {
        mvwprintz(win, y + 1, 1, c_ltred, _("Label: %s"), label.c_str());
    }

    return y;
}

/**
 * Return whether a fuel_type should be printed
 * @param fuel_type ID of the fuel to be checked
 * @param fullsize true if it's expected to print multiple rows
 * @return true if the fuel will/should display
 */
bool vehicle::should_print_fuel_indicator (itype_id fuel_type, bool fullsize) const
{
    return fuel_capacity( fuel_type ) > 0 && ( basic_consumption( fuel_type ) > 0 || fullsize );
}

/**
 * Returns an array of fuel types that can be printed
 * @param fullsize true if it's expected to print multiple rows
 * @return An array of printable fuel type ids
 */
std::vector< itype_id > vehicle::get_printable_fuel_types (bool fullsize) const
{
    std::vector< itype_id > fuels;
    for( auto &ft : get_fuel_types() ) {
        if(should_print_fuel_indicator(ft.id, fullsize)) {
            fuels.push_back(ft.id);
        }
    }

    for( int p : fuel ) {
        const itype_id ft = part_info( p ).fuel_type;
        if( std::find( fuels.begin(), fuels.end(), ft ) == fuels.end() && should_print_fuel_indicator(ft, fullsize)) {
            fuels.push_back(ft);
        }
    }

    return fuels;
}

/**
 * Returns the color of a fuel type
 * @param fuel_type The ID of the fuel type
 * @return the color corresponding to the fuel type
 */
nc_color vehicle::get_fuel_color ( const itype_id &fuel_type ) const
{
    for( auto &f : get_fuel_types() ) {
        if( f.id == fuel_type ) {
            return f.color;
        }
    }
    return item::find_type( fuel_type )->color;
}

/**
 * Prints all of the fuel indicators of the vehical
 * @param w Pointer to the window to draw in.
 * @param y Y location to draw at.
 * @param x X location to draw at.
 * @param start_index Starting index in array of fuel gauges to start reading from
 * @param fullsize true if it's expected to print multiple rows
 * @param verbose true if there should be anything after the gauge (either the %, or number)
 * @param desc true if the name of the fuel should be at the end
 * @param isHorizontal true if the menu is not vertical
 */
void vehicle::print_fuel_indicators (void *w, int y, int x, int start_index, bool fullsize, bool verbose, bool desc, bool isHorizontal) const
{
    WINDOW *win = (WINDOW *) w;
    int yofs = 0;
    std::vector< itype_id > fuels = get_printable_fuel_types(fullsize);
    int max_gauge = ((isHorizontal) ? 12 : 5) + start_index;
    int max_size = std::min((int)fuels.size(), max_gauge);

    for( int i = start_index; i < max_size; i++ ) {
        const itype_id &f = fuels[i];
        print_fuel_indicator(w, y + yofs, x, f, verbose, desc);
        if (fullsize) {
            yofs++;
        }
    }

    // check if the current index is less than the max size minus 12 or 5, to indicate that there's more
    if((start_index < (int)fuels.size() -  ((isHorizontal) ? 12 : 5)) && fullsize) {
        mvwprintz(win, y + yofs, x, c_ltgreen, ">");
        wprintz(win, c_ltgray, " for more");
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
void vehicle::print_fuel_indicator (void *w, int y, int x, itype_id fuel_type, bool verbose, bool desc) const
{
    const char fsyms[5] = { 'E', '\\', '|', '/', 'F' };
    WINDOW *win = (WINDOW *) w;
    nc_color col_indf1 = c_ltgray;
    int cap = fuel_capacity( fuel_type );
    int f_left = fuel_left( fuel_type );
    nc_color f_color = get_fuel_color(fuel_type);
    mvwprintz(win, y, x, col_indf1, "E...F");
    int amnt = cap > 0 ? f_left * 99 / cap : 0;
    int indf = (amnt / 20) % 5;
    mvwprintz( win, y, x + indf, f_color, "%c", fsyms[indf] );
    if (verbose) {
        if( debug_mode ) {
            mvwprintz( win, y, x + 6, f_color, "%d/%d", f_left, cap );
        } else {
            mvwprintz( win, y, x + 6, f_color, "%d", (f_left * 100) / cap );
            wprintz( win, c_ltgray, "%c", 045 );
        }
    }
    if (desc) {
        wprintz(win, c_ltgray, " - %s", item::nname( fuel_type ).c_str() );
    }
}

void vehicle::coord_translate (const point &p, point &q) const
{
    coord_translate(pivot_rotation[0], pivot_anchor[0], p, q);
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
     const int player_id = parts[p].passenger_id;
     if( player_id == g->u.getID()) {
      return &g->u;
     }
     int npcdex = g->npc_by_id (player_id);
     if (npcdex >= 0) {
      return g->active_npc[npcdex];
     }
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

int vehicle::total_mass() const
{
    int m = 0;
    for (size_t i = 0; i < parts.size(); i++)
    {
        if (parts[i].removed) {
          continue;
        }
        m += item::find_type( part_info(i).item )->weight;
        for( auto &j : get_items(i) ) {
            m += j.type->weight;
        }
        if (part_flag(i,VPFLAG_BOARDABLE) && parts[i].has_flag(vehicle_part::passenger_flag)) {
            m += 81500; // TODO: get real weight
        }
    }
    return m/1000;
}

int vehicle::total_folded_volume() const
{
    int m = 0;
    for( size_t i = 0; i < parts.size(); i++ ) {
        if( parts[i].removed ) {
            continue;
        }
        m += part_info(i).folded_volume;
    }
    return m;
}

void vehicle::center_of_mass(int &x, int &y, bool use_precalc) const
{
    float xf = 0, yf = 0;
    int m_total = total_mass();
    for (size_t i = 0; i < parts.size(); i++)
    {
        if (parts[i].removed) {
          continue;
        }
        int m_part = 0;
        m_part += item::find_type( part_info(i).item )->weight;
        for( auto &j : get_items(i) ) {
            m_part += j.type->weight;
        }
        if (part_flag(i,VPFLAG_BOARDABLE) && parts[i].has_flag(vehicle_part::passenger_flag)) {
            m_part += 81500; // TODO: get real weight
        }
        if (use_precalc) {
            xf += parts[i].precalc[0].x * m_part / 1000;
            yf += parts[i].precalc[0].y * m_part / 1000;
        } else {
            xf += parts[i].mount.x * m_part / 1000;
            yf += parts[i].mount.y * m_part / 1000;
        }
    }
    xf /= m_total;
    yf /= m_total;
    x = round(xf);
    y = round(yf);
}

point vehicle::pivot_point() const
{
    // TODO: a part type that let you override this
    point p;
    center_of_mass(p.x, p.y, false);
    return p;
}

point vehicle::pivot_displacement() const
{
    // precalc_mounts always produces a result that puts the pivot point at (0,0).
    // If the pivot point changes, this artificially moves the vehicle, as the position
    // of the old pivot point will appear to move from (posx+0, posy+0) to some other point
    // (posx+dx,posy+dy) even if there is no change in vehicle position or rotation.
    // This method finds that movement so it can be cancelled out when actually moving
    // the vehicle.

    // rotate the old pivot point around the new pivot point with the old rotation angle
    point dp;
    coord_translate(pivot_rotation[0], pivot_anchor[1], pivot_anchor[0], dp);
    return dp;
}

int vehicle::fuel_left (const itype_id & ftype, bool recurse) const
{
    int fl = 0;
    for(auto &p : fuel) {
        if(ftype == part_info(p).fuel_type) {
            fl += parts[p].amount;
        }
    }

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
        int part_under_player;
        vehicle *veh = g->m.veh_at( g->u.pos(), part_under_player );
        bool player_controlling = player_in_control(g->u);

        //if the engine in the player tile is a muscle engine, and player is controlling vehicle
        if (veh == this && player_controlling && part_under_player >= 0) {
            int p = part_with_feature(part_under_player, VPFLAG_ENGINE);
            if (p >= 0 && part_info(p).fuel_type == fuel_type_muscle) {
                fl += 10;
            }
        }
    }

    return fl;
}

int vehicle::fuel_capacity (const itype_id &ftype) const
{
    int cap = 0;
    for(auto &p : fuel) {
        if(ftype == part_info(p).fuel_type) {
            cap += part_info(p).size;
        }
    }
    return cap;
}

int vehicle::refill (const itype_id & ftype, int amount)
{
    for (size_t p = 0; p < parts.size(); p++)
    {
        if (part_flag(p, VPFLAG_FUEL_TANK) &&
            part_info(p).fuel_type == ftype &&
            parts[p].amount < part_info(p).size &&
            parts[p].hp > 0)
        {
            int need = part_info(p).size - parts[p].amount;
            if (amount < need)
            {
                parts[p].amount += amount;
                return 0;
            }
            else
            {
                parts[p].amount += need;
                amount -= need;
            }
        }
    }
    return amount;
}

int vehicle::drain (const itype_id & ftype, int amount) {
    if(ftype == "battery") {
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

    for (auto &p : fuel) {
        vehicle_part &tank = parts[p];
        if (part_info(p).fuel_type == ftype && tank.amount > 0) {
            if (tank.amount > (amount - drained)) {
                tank.amount -= (amount - drained);
                drained = amount;
                break;
            } else {
                drained += tank.amount;
                tank.amount = 0;
            }
        }
    }

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
    if ((engine_on && has_engine_type_not(fuel_type_muscle, true)) || skidding) {
        return (int) (safe_velocity (fueled) * k_mass() / (1 + strain ()) / 10);
    } else if ((has_engine_type(fuel_type_muscle, true))){
        //limit vehicle weight for muscle engines
        int mass = total_mass();
        ///\EFFECT_STR caps vehicle weight for muscle engines
        int move_mass = std::max((g->u).str_cur * 25, 150);
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
        if( part_flag(p, VPFLAG_ENGINE) && parts[p].hp <= 0 && parts[p].amount > 0 ) {
            needed = true;
            parts[p].amount--;
            tripoint tmp = part_pos;
            int &tx = tmp.x;
            int &ty = tmp.y;
            for( tx = part_pos.x - 1; tx <= part_pos.x + 1; tx++ ) {
                for( ty = part_pos.y - 1; ty <= part_pos.y + 1; ty++ ) {
                    if( !rng(0, 2) ) {
                        g->m.add_field( tmp, fd_smoke, rng(2, 4), 0 );
                    }
                }
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
            } else if(is_engine_type(e, fuel_type_plasma)) {
                m2c = 75;
            } else if(is_engine_type(e, fuel_type_battery)) {
                m2c = 90;
            } else if(is_engine_type(e, fuel_type_muscle)) {
                m2c = 45;
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

void vehicle::spew_smoke( double joules, int part )
{
    if( rng(1, 100000) > joules ) {
        return;
    }
    point p = parts[part].mount;
    int smoke = int(std::max(joules / 10000 , 1.0));
    // Move back from engine/muffler til we find an open space
    while( relative_parts.find(p) != relative_parts.end() ) {
        p.x += ( velocity < 0 ? 1 : -1 );
    }
    point q;
    coord_translate( p, q );
    tripoint dest( global_x() + q.x, global_y() + q.y, smz );
    g->m.add_field( dest, fd_smoke, smoke, 0 );
}

/**
 * Generate noise or smoke from a vehicle with engines turned on
 * load = how hard the engines are working, from 0.0 til 1.0
 * time = how many seconds to generated smoke for
 */
void vehicle::noise_and_smoke( double load, double time )
{
    const int sound_levels[] = { 0, 15, 30, 60, 100, 140, 180, INT_MAX };
    const char *sound_msgs[] = { "", _("hummm!"), _("whirrr!"), _("vroom!"), _("roarrr!"), _("ROARRR!"),
                                 _("BRRROARRR!!"), _("BRUMBRUMBRUMBRUM!!!") };
    double noise = 0.0;
    double mufflesmoke = 0.0;
    double muffle = 1.0, m;
    int exhaust_part = -1;
    for( size_t p = 0; p < parts.size(); p++ ) {
        if( part_flag(p, "MUFFLER") ) {
            m = 1.0 - (1.0 - part_info(p).bonus / 100.0) * parts[p].hp / part_info(p).durability;
            if( m < muffle ) {
                muffle = m;
                exhaust_part = int(p);
            }
        }
    }

    for( size_t e = 0; e < engines.size(); e++ ) {
        int p = engines[e];
        if( is_engine_on(e) &&
                (is_engine_type(e, fuel_type_muscle) || fuel_left (part_info(p).fuel_type)) ) {
            double pwr = 10.0; // Default noise if nothing else found, shouldn't happen
            double max_pwr = double(power_to_epower(part_power(p, true)))/40000;
            double cur_pwr = load * max_pwr;

            if( is_engine_type(e, fuel_type_gasoline) || is_engine_type(e, fuel_type_diesel)) {
                const double dmg = 1.0 - ((double)parts[p].hp / part_info( p ).durability);
                if( is_engine_type( e, fuel_type_gasoline ) && dmg > 0.75 &&
                        one_in( 200 - (150 * dmg) ) ) {
                    backfire( e );
                }
                double j = power_to_epower(part_power(p, true)) * load * time * muffle;
                if( (exhaust_part == -1) && engine_on ) {
                    spew_smoke( j, p );
                } else {
                    mufflesmoke += j;
                }
                pwr = (cur_pwr*15 + max_pwr*3 + 5) * muffle;
            } else if(is_engine_type(e, fuel_type_plasma)) {
                pwr = (cur_pwr*9 + 1) * muffle;
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
        spew_smoke( mufflesmoke, exhaust_part );
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

float vehicle::wheels_area (int *const cnt) const
{
    int count = 0;
    int total_area = 0;
    const auto &wheel_indices = wheelcache;
    for( auto &wheel_indice : wheel_indices ) {
        int p = wheel_indice;
        int width = part_info(p).wheel_width;
        int bigness = parts[p].bigness;
        // 9 inches, for reference, is about normal for cars.
        total_area += ((float)width / 9) * bigness;
        count++;
    }
    if (cnt) {
        *cnt = count;
    }

    if (all_parts_with_feature("FLOATS").size() > 0) {
        return 13;
    }

    return total_area;
}

float vehicle::k_friction() const
{
    // calculate safe speed reduction due to wheel friction
    float fr0 = 1000.0;
    float kf = ( fr0 / (fr0 + wheels_area()) );
    return kf;
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
    float wa = wheels_area();
    if (wa <= 0)
       return 0;

    float ma0 = 50.0;
    // calculate safe speed reduction due to mass
    float km = ma0 / (ma0 + (total_mass()) / (8 * (float) wa));

    return km;
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

bool vehicle::sufficient_wheel_config() const
{
    std::vector<int> floats = all_parts_with_feature(VPFLAG_FLOATS);
    if( !floats.empty() ) {
        return floats.size() > 2;
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

bool vehicle::balanced_wheel_config () const
{
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    int count = 0;
    std::vector<int> wheel_indices = all_parts_with_feature(VPFLAG_WHEEL);
    // find the bounding box of the wheels
    // TODO: find convex hull instead
    for (auto &w : wheel_indices) {
        if (!count) {
            x1 = x2 = parts[w].precalc[0].x;
            y1 = y2 = parts[w].precalc[0].y;
        }
        if (parts[w].precalc[0].x < x1) {
            x1 = parts[w].precalc[0].x;
        }
        if (parts[w].precalc[0].x > x2) {
            x2 = parts[w].precalc[0].x;
        }
        if (parts[w].precalc[0].y < y1) {
            y1 = parts[w].precalc[0].y;
        }
        if (parts[w].precalc[0].y > y2) {
            y2 = parts[w].precalc[0].y;
        }
        count++;
    }
    // find the center of mass of the vehicle
    int xo, yo;
    center_of_mass(xo, yo);
//    add_msg("cm x=%d y=%d x1=%d y1=%d x2=%d y2=%d", xo, yo, x1, y1, x2, y2);
    if (xo < x1 || xo > x2 || yo < y1 || yo > y2) {
        return false; // center of mass not inside support of wheels (roughly)
    }
    return true;
}

bool vehicle::valid_wheel_config () const
{
    return sufficient_wheel_config() && balanced_wheel_config();
}

/**
 * Power for batteries are in W, but usage for rest is in 0.5*HP, so coeff is 373
 * This does not seem to match up for consumption, as old coeff is 100
 * Keeping coeff of 100, but should probably be adjusted later
 * http://en.wikipedia.org/wiki/Energy_density -> 46MJ/kg, 36MJ/l for gas
 * Gas tanks are 6000 and 30000, assuming that's in milliliters, so 36000J/ml
 * Battery sizes are 1k, 12k, 30k, 50k, and 100k. present day = 53kWh(200MJ) for 450kg
 * Efficiency tank to wheel is roughly 15% for gas, 85% for electric
 */
void vehicle::consume_fuel( double load = 1.0 )
{
    float st = strain();
    for( auto &ft : get_fuel_types() ) {
        // if no engines use this fuel, skip
        int amnt_fuel_use = basic_consumption( ft.id );
        if (amnt_fuel_use == 0) continue;

        //get exact amount of fuel needed
        double amnt_precise = double(amnt_fuel_use) / ft.coeff;

        amnt_precise *= load * (1.0 + st * st * 100);
        int amnt = int(amnt_precise);
        // consumption remainder results in chance at additional fuel consumption
        if( x_in_y(int(amnt_precise*1000) % 1000, 1000) ) {
            amnt += 1;
        }
        for( auto &elem : fuel ) {
            if( part_info( elem ).fuel_type == ft.id ) {
                if( parts[elem].amount >= amnt ) {
                    // enough fuel located in this part
                    parts[elem].amount -= amnt;
                    break;
                } else {
                    amnt -= parts[elem].amount;
                    parts[elem].amount = 0;
                }
            }
        }
    }
    //do this with chance proportional to current load
    // But only if the player is actually there!
    if( load > 0 && one_in( (int) (1 / load) ) &&
        fuel_left( fuel_type_muscle ) > 0 ) {
        //charge bionics when using muscle engine
        if (g->u.has_bionic("bio_torsionratchet")) {
            g->u.charge_power(1);
        }
        //cost proportional to strain
        int mod = 1 + 4 * st;
        if (one_in(10)) {
            g->u.fatigue += mod;
            g->u.mod_hunger(mod);
            g->u.thirst += mod;
        }
        g->u.mod_stat( "stamina", -mod * 20);
    }
}

void vehicle::power_parts()
{
    int epower = 0;

    // Consumers of epower
    if( lights_on ) epower += lights_epower;
    if( overhead_lights_on ) epower += overhead_epower;
    if( tracking_on ) epower += tracking_epower;
    if( fridge_on ) epower += fridge_epower;
    if( recharger_on ) epower += recharger_epower;
    if( is_alarm_on ) epower += alarm_epower;
    if( camera_on ) epower += camera_epower;
    if( dome_lights_on ) epower += dome_lights_epower;
    if( aisle_lights_on ) epower += aisle_lights_epower;
    if( scoop_on ) epower += scoop_epower;
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
    }

    // Producers of epower
    if(engine_on) {
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
    if( reactor_on && epower_capacity_left - epower > 0 ) {
        // Still not enough surplus epower to fully charge battery
        // Produce additional epower from any reactors
        bool reactor_working = false;
        for( auto &elem : reactors ) {
            if( parts[elem].hp > 0 && parts[elem].amount > 0 ) {
                // Efficiency: one unit of fuel is this many units of battery
                // Note: One battery is roughtly 373 units of epower
                const int efficiency = part_info( elem ).power;
                const int avail_fuel = parts[elem].amount * efficiency;

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

                parts[elem].amount -= fuel_consumed;
                reactor_working = true;

                epower += reactors_output;
            }
        }

        if( !reactor_working ) {
            // All reactors out of fuel or destroyed
            reactor_on = false;
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
        is_alarm_on = false;
        lights_on = false;
        if( tracking_on ) {
            overmap_buffer.remove_vehicle( this );
            tracking_on = false;
        }
        overhead_lights_on = false;
        fridge_on = false;
        stereo_on = false;
        chimes_on = false;
        recharger_on = false;
        camera_on = false;
        dome_lights_on = false;
        aisle_lights_on = false;
        scoop_on = false;
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
    vehicle* veh = g->m.veh_at( veh_local );

    if( veh != nullptr ) {
        return veh;
    }

    // Nope. Load up its submap...
    point veh_in_sm = point( where.x, where.y );
    point veh_sm = overmapbuffer::ms_to_sm_remain( veh_in_sm );

    auto sm = MAPBUFFER.lookup_submap( veh_sm.x, veh_sm.y, where.z );
    if( sm == nullptr ) {
        return nullptr;
    }

    // ...find the right vehicle inside it...
    for( auto &elem : sm->vehicles ) {
        vehicle *found_veh = elem;
        point veh_location( found_veh->posx, found_veh->posy );

        if( veh_in_sm == veh_location ) {
            veh = found_veh;
            break;
        }
    }

    // ...and hand it over.
    return veh;
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
    for(auto &f : fuel) {
        if(part_info(f).fuel_type == fuel_type_battery && parts[f].hp > 0) {
            int empty = part_info(f).size - parts[f].amount;
            if(empty < amount) {
                amount -= empty;
                parts[f].amount = part_info(f).size;
                if (amount <= 0) {
                    break;
                }
            } else {
                parts[f].amount += amount;
                amount = 0;
                break;
            }
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
    int avail_charge;

    for(auto &f : fuel) {
        if(part_info(f).fuel_type == fuel_type_battery && parts[f].hp > 0) {
            avail_charge = parts[f].amount;
            if(avail_charge < amount) {
                amount -= avail_charge;
                parts[f].amount = 0;
                if (amount <= 0) {
                    break;
                }
            }
            else {
                parts[f].amount -= amount;
                amount = 0;
                break;
            }
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

    if (stereo_on) {
        play_music();
    }

    if (chimes_on) {
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
    if(scoop_on){
        operate_scoop();
    }
    if( planter_on ){
        operate_planter();
    }
    if( plow_on ){
        operate_plow();
    }
    if( reaper_on ){
        operate_reaper();
    }
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

void vehicle::operate_reaper(){
    const tripoint &veh_start = global_pos3();
    for( const int reaper_id : all_parts_with_feature( "REAPER" ) ){
        const tripoint reaper_pos = veh_start + parts[reaper_id].precalc[0];
        const int plant_produced =  rng( 1, parts[reaper_id].info().bonus );
        const int seed_produced = rng(1, 3);
        if( g->m.furn(reaper_pos) != f_plant_harvest ){
            continue;
        }
        const itype &type = *g->m.i_at(reaper_pos).front().type;
        if( type.id == "fungal_seeds" || type.id == "marloss_seed" ) {
            // Otherworldly plants, the earth-made reaper can not handle those.
            continue;
        }
        g->m.furn_set( reaper_pos, f_null );
        g->m.i_clear( reaper_pos );
        for( auto &i : iexamine::get_harvest_items( type, plant_produced, seed_produced, false ) ) {
            g->m.add_item_or_charges( reaper_pos, i );
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
                    i->bday = calendar::turn;
                    g->m.add_item( loc, *i );
                    v.erase( i );
                } else {
                    item tmp = *i;
                    tmp.charges = 1;
                    tmp.bday = calendar::turn;
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
        int max_pickup_size = parts[scoop].info().size / 10;
        const char *sound_msgs[] = {_("Whirrrr"), _("Ker-chunk"), _("Swish"), _("Cugugugugug")};
        sounds::sound( global_pos3() + parts[scoop].precalc[0], rng( 20, 35 ),
                       sound_msgs[rng( 0, 3 )] );
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
                if( it.volume() < max_pickup_size ) {
                    that_item_there = g->m.item_from( position, itemdex );
                    break;
                }
                itemdex++;
            }
            if( !that_item_there ) {
                continue;
            }
            if( one_in( chance_to_damage_item ) && that_item_there->damage < 4 ) {
                //The scoop will not destroy the item, but it may damage it a bit.
                that_item_there->damage++;
                //The scoop gets a lot louder when breaking an item.
                sounds::sound( position, rng(10, (long)that_item_there->volume() * 2 + 10),
                               _("BEEEThump") );
            }
            const int battery_deficit = discharge_battery( that_item_there->weight() *
                                                           scoop_epower / rng( 8, 15 ) );
            if( battery_deficit == 0 && add_item( scoop, *that_item_there ) ) {
                g->m.i_rem( position, itemdex );
            } else {
                //otherwise move on to the next scoop.
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
            const char *sound_msgs[] = { _("WHOOP WHOOP"), _("NEEeu NEEeu NEEeu"), _("BLEEEEEEP"), _("WREEP")};
            sounds::sound( global_pos3(), (int) rng(45,80), sound_msgs[rng(0,3)] );
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
    //for each of your badly damaged tanks (lower than 50% health), leak a small amount of liquid
    // damaged batteries self-discharge without leaking
    for( auto &elem : fuel ) {
        vehicle_part &part = parts[elem];
        vpart_info pinfo = part_info( elem );
        if( pinfo.fuel_type != fuel_type_gasoline && pinfo.fuel_type != fuel_type_diesel &&
            pinfo.fuel_type != fuel_type_battery && pinfo.fuel_type != fuel_type_water ) {
            // Not a liquid fuel or battery
            continue;
        }
        float damage_ratio = ( float )part.hp / ( float )pinfo.durability;
        if( part.amount > 0 && damage_ratio < 0.5f ) {
            int leak_amount = ( 0.5 - damage_ratio ) * ( 0.5 - damage_ratio ) * part.amount / 10;
            point q;
            if( leak_amount < 1 ) {
                leak_amount = 1;
            }
            // Don't leak batteries from a damaged battery
            if( pinfo.fuel_type != fuel_type_battery ) {
                coord_translate( part.mount, q );
                // m.spawn_item() will spawn water in bottles, so instead we create
                //   the leak manually and directly call m.add_item_or_charges().
                item leak( pinfo.fuel_type, calendar::turn );
                leak.charges = leak_amount;
                tripoint dest( global_x() + q.x, global_y() + q.y, smz );
                g->m.add_item_or_charges( dest, leak );
            }
            part.amount -= leak_amount;
        }
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

    if (stereo_on == true) {
        play_music();
    }

    if (chimes_on == true) {
        play_chimes();
    }

    // No need to change velocity
    if( !thd ) {
        return;
    }

    bool pl_ctrl = player_in_control( g->u );

    // No need to change velocity if there are no wheels
    if( !valid_wheel_config() && velocity == 0 ) {
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

    int accel = acceleration();
    int max_vel = max_velocity();
    //get braking power
    int brake = 30 * k_mass();
    int brk = abs(velocity) * brake / 100;
    if (brk < accel) {
        brk = accel;
    }
    if (brk < 10 * 100) {
        brk = 10 * 100;
    }
    //pos or neg if acc or brake
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
        if (total_power () <= 0 || !engine_on) {
            if (pl_ctrl) {
                if( total_power( false ) <= 0 ) {
                    add_msg( m_info, _("The %s doesn't have an engine!"), name.c_str() );
                } else if( has_engine_type( fuel_type_muscle, true ) ) {
                    add_msg( m_info, _("The %s's mechanism is out of reach!"), name.c_str() );
                } else if( !engine_on ) {
                    add_msg( _("The %s's engine isn't on!"), name.c_str() );
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

void vehicle::turn (int deg)
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
        // Coords of where part will go due to movement (dx/dy/dz)
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
        } else if( !just_detect && sgn( velocity_after ) != sign_before ) {
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

    int target_part = -1;
    vehicle *oveh = g->m.veh_at( p, target_part );
    // Disable veh/critter collisions when bashing floor
    // TODO: More elegant code
    const bool is_veh_collision = !bash_floor && oveh != nullptr && oveh != this;
    const bool is_body_collision = !bash_floor && critter != nullptr;

    veh_collision ret;
    ret.type = veh_coll_nothing;
    ret.part = part;

    // Vehicle collisions are a special case. just return the collision.
    // The map takes care of the dynamic stuff.
    if( is_veh_collision ) {
       ret.type = veh_coll_veh;
       //"imp" is too simplistic for veh-veh collisions
       ret.target = oveh;
       ret.target_part = target_part;
       ret.target_name = oveh->disp_name();
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
    } else if( g->m.move_cost_ter_furn( p ) == 0 ||
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
    const float mass = total_mass();
    int degree = rng( 70, 100 );

    //Calculate damage resulting from d_E
    const itype *type = item::find_type( part_info( ret.part ).item );
    std::vector<std::string> vpart_item_mats = type->materials;
    float vpart_dens = 0;
    for( auto mat_id : vpart_item_mats ) {
        vpart_dens += material_type::find_material(mat_id)->density();
    }
    vpart_dens /= vpart_item_mats.size(); // average

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
    int turns_stunned = 0;

    const int prev_velocity = coll_velocity;
    const int vel_sign = sgn( coll_velocity );
    // Velocity of the object we're hitting
    // Assuming it starts at 0, but we'll probably hit it many times
    // in one collision, so accumulate the velocity gain from each hit.
    float vel2 = 0;
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
                } else if( g->m.move_cost_ter_furn( p ) == 0 ) {
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

            turns_stunned = ( rng( 0, dam ) > 10 ) + ( rng( 0, dam ) > 40 );
            if( turns_stunned > 0 ) {
                critter->add_effect( "stunned", turns_stunned );
            }

            const int angle = (100 - degree) * 2 * ( one_in( 2 ) ? 1 : -1 );
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
                    // Also handle the weird case when we don't have enough force
                    // but still have to push (in such case compare momentum)
                    const float push_force = fabs(vel2_a) > 10.0f ? fabs(vel2_a) : 10.1f;
                    const int angle_sum = vel2_a > 0 ?
                        move.dir() + angle : -(move.dir() + angle);
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

        coll_velocity = vel1_a * 100;
        // Stop processing when sign inverts, not when we reach 0
    } while( !smashed && sgn( coll_velocity ) == vel_sign );

    // Apply special effects from collision.
    if( critter != nullptr ) {
        if( pl_ctrl ) {
            if( turns_stunned > 0 ) {
                //~ 1$s - vehicle name, 2$s - part name, 3$s - NPC or monster
                add_msg (m_warning, _("Your %1$s's %2$s rams into %3$s and stuns it!"),
                         name.c_str(), part_info(ret.part).name.c_str(), ret.target_name.c_str());
            } else {
                //~ 1$s - vehicle name, 2$s - part name, 3$s - NPC or monster
                add_msg (m_warning, _("Your %1$s's %2$s rams into %3$s!"),
                         name.c_str(), part_info(ret.part).name.c_str(), ret.target_name.c_str());
            }
        }

        if( part_flag( ret.part, "SHARP" ) ) {
            g->m.adjust_field_strength( p, fd_blood, 1 );
        } else {
            sounds::sound( p, 20, snd );
        }
    } else {
        if( pl_ctrl ) {
            if( snd.length() > 0 ) {
                //~ 1$s - vehicle name, 2$s - part name, 3$s - collision object name, 4$s - sound message
                add_msg (m_warning, _("Your %1$s's %2$s rams into a %3$s with a %4$s"),
                         name.c_str(), part_info(ret.part).name.c_str(), ret.target_name.c_str(), snd.c_str());
            } else {
                //~ 1$s - vehicle name, 2$s - part name, 3$s - collision object name
                add_msg (m_warning, _("Your %1$s's %2$s rams into a %3$s."),
                         name.c_str(), part_info(ret.part).name.c_str(), ret.target_name.c_str());
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
    // todo; make trapfuncv?

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
            g->m.add_trap(p, tr_shotgun_1);
        } else {
            g->m.remove_trap(p);
            g->m.spawn_item(p, "shotgun_sawn");
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
    } else {
        return;
    }
    if( g->u.sees(p) ) {
        if( g->u.knows_trap( p ) ) {
            add_msg(m_bad, _("The %1$s's %s runs over %2$s."), name.c_str(),
                    part_info(part).name.c_str(), tr.name.c_str() );
        } else {
            add_msg(m_bad, _("The %1$s's %2$s runs over something."), name.c_str(),
                    part_info(part).name.c_str() );
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
        g->explosion( p, expl, 0.5f, shrap, false );
    }
}

// total volume of all the things
int vehicle::stored_volume(int const part) const
{
    if (!part_flag(part, "CARGO")) {
        return 0;
    }
    int cur_volume = 0;
    for( auto &i : get_items(part) ) {
       cur_volume += i.volume();
    }
    return cur_volume;
}

int vehicle::max_volume(int const part) const
{
    if (part_flag(part, "CARGO")) {
        return parts[part].info().size;
    }
    return 0;
}

// free space
int vehicle::free_volume(int const part) const
{
   const int maxvolume = this->max_volume(part);
   return ( maxvolume - stored_volume(part) );
}

// returns true if full, modified by arguments:
// (none):                            size >= max || volume >= max
// (addvolume >= 0):                  size+1 > max || volume + addvolume > max
// (addvolume >= 0, addnumber >= 0):  size + addnumber > max || volume + addvolume > max
bool vehicle::is_full(const int part, const int addvolume, const int addnumber) const
{
   const int maxitems = MAX_ITEM_IN_VEHICLE_STORAGE;
   const int maxvolume = this->max_volume(part);

   if ( addvolume == -1 ) {
       if( (int)get_items(part).size() < maxitems ) return true;
       int cur_volume=stored_volume(part);
       return (cur_volume >= maxvolume ? true : false );
   } else {
       if ( (int)get_items(part).size() + ( addnumber == -1 ? 1 : addnumber ) > maxitems ) return true;
       int cur_volume=stored_volume(part);
       return ( cur_volume + addvolume > maxvolume ? true : false );
   }

}

bool vehicle::add_item (int part, item itm)
{
    const int max_storage = MAX_ITEM_IN_VEHICLE_STORAGE; // (game.h)
    const int maxvolume = this->max_volume(part);         // (game.h => vehicle::max_volume(part) ) in theory this could differ per vpart ( seat vs trunk )

    // const int max_weight = ?! // TODO: weight limit, calc per vpart & vehicle stats, not a hard user limit.
    // add creaking sounds and damage to overloaded vpart, outright break it past a certian point, or when hitting bumps etc

    if( !part_flag(part, "CARGO") || parts[part].hp <= 0 ) {
        return false;
    }

    if( (int)parts[part].items.size() >= max_storage ) {
        return false;
    }
    if (part_flag(part, "TURRET")) {
        const itype *gun_type = item::find_type( part_info( part ).item ); // TODO: maybe cache this?
        const ammotype atype = itm.ammo_type();
        if( !itm.is_ammo() || !gun_type->gun || atype != gun_type->gun->ammo ) {
            return false;
        }
    }
    int cur_volume = 0;
    int add_volume = itm.volume();
    bool tryaddcharges=(itm.charges  != -1 && itm.count_by_charges());
    // iterate anyway since we need a volume total
    for (auto &i : parts[part].items) {
        cur_volume += i.volume();
        if( tryaddcharges && i.merge_charges( itm ) ) {
            return true;
        }
    }

    if ( cur_volume + add_volume > maxvolume ) {
        return false;
    }
    return add_item_at( part, parts[part].items.end(), itm );
}

bool vehicle::add_item_at(int part, std::list<item>::iterator index, item itm)
{
    const auto new_pos = parts[part].items.insert( index, itm );
    if( itm.needs_processing() ) {
        active_items.add( new_pos, parts[part].mount );
    }

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
    for( auto &spawn : type.obj().item_spawns ) {
        const vehicle_item_spawn *next_spawn = &spawn;
        if(rng(1, 100) <= next_spawn->chance) {
            //Find the cargo part in that square
            int part = part_with_feature_at_relative(next_spawn->pos, "CARGO", false);
            if(part < 0) {
                debugmsg("No CARGO parts at (%d, %d) of %s!",
                        next_spawn->pos.x, next_spawn->pos.y, name.c_str());
            } else {
                bool partbroken = ( parts[part].hp < 1 );
                int idmg = 0;
                for( auto &elem : next_spawn->item_ids ) {
                    if ( partbroken ) {
                        int idmg = rng(1, 10);
                        if ( idmg > 5 ) {
                            continue;
                        }
                    }
                    item new_item( elem, calendar::turn );
                    new_item = new_item.in_its_container();
                    if ( idmg > 0 ) {
                        new_item.damage = (signed char)idmg;
                    }
                    add_item(part, new_item);
                }
                for( auto &elem : next_spawn->item_groups ) {
                    if ( partbroken ) {
                        int idmg = rng(1, 10);
                        if ( idmg > 5 ) {
                            continue;
                        }
                    }
                    item new_item = item_group::item_from( elem, 0 );
                    if( new_item.is_null() ) {
                        continue;
                    }
                    if ( idmg > 0 ) {
                        new_item.damage = (signed char)idmg;
                    }
                    add_item(part, new_item);
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

    if( turret_mode != turret_mode_off ) { // handle turrets
        for( size_t p = 0; p < parts.size(); p++ ) {
            if( !part_flag( p, "TURRET" ) ) {
                continue;
            }
            bool success = fire_turret( p, false );
            if( !success && parts[p].target.first != parts[p].target.second ) {
                add_msg( m_bad, _("%s failed to fire! It isn't loaded and/or powered."), part_info( p ).name.c_str() );
            }
            // Clear manual target
            parts[p].target.second = parts[p].target.first;
        }

        if( turret_mode == turret_mode_manual ) {
            // Manual mode is automatically set when aiming from off, but not from auto
            // It should only be set for one turn
            turret_mode = turret_mode_off;
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
    lights.clear();
    alternators.clear();
    fuel.clear();
    engines.clear();
    reactors.clear();
    solar_panels.clear();
    funnels.clear();
    relative_parts.clear();
    loose_parts.clear();
    wheelcache.clear();
    speciality.clear();
    floating.clear();
    lights_epower = 0;
    overhead_epower = 0;
    tracking_epower = 0;
    fridge_epower = 0;
    recharger_epower = 0;
    dome_lights_epower = 0;
    aisle_lights_epower = 0;
    alternator_load = 0;
    camera_epower = 0;
    has_atomic_lights = false;
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
        if( vpi.has_flag(VPFLAG_LIGHT) || vpi.has_flag(VPFLAG_CONE_LIGHT) ) {
            lights.push_back( p );
            lights_epower += vpi.epower;
        }
        if( vpi.has_flag(VPFLAG_CIRCLE_LIGHT) ) {
            overhead_epower += vpi.epower;
        }
        if( vpi.has_flag(VPFLAG_DOME_LIGHT) ) {
            lights.push_back( p);
            dome_lights_epower += vpi.epower;
        }
        if( vpi.has_flag(VPFLAG_AISLE_LIGHT) ) {
            lights.push_back( p);
            aisle_lights_epower += vpi.epower;
        }
        if( vpi.has_flag(VPFLAG_TRACK) ) {
            tracking_epower += vpi.epower;
        }
        if( vpi.has_flag(VPFLAG_FRIDGE) ) {
            fridge_epower += vpi.epower;
        }
        if( vpi.has_flag(VPFLAG_RECHARGE) ) {
            recharger_epower += vpi.epower;
        }
        if( vpi.has_flag(VPFLAG_ALTERNATOR) ) {
            alternators.push_back( p );
        }
        if( vpi.has_flag("SCOOP") ) {
            scoop_epower += vpi.epower;
        }
        if( vpi.has_flag(VPFLAG_FUEL_TANK) ) {
            fuel.push_back( p );
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
        if (vpi.has_flag("SECURITY")){
            speciality.push_back(p);
        }
        if( vpi.has_flag( "CAMERA" ) ) {
            camera_epower += vpi.epower;
        }
        if( vpi.has_flag( "ATOMIC_LIGHT" ) ) {
            has_atomic_lights = true;
        }
        if( vpi.has_flag( VPFLAG_FLOATS ) ) {
            floating.push_back( p );
        }
        if( plow_on && vpi.has_flag( "PLOW" ) ){
            extra_drag += vpi.power;
        }
        if( planter_on && vpi.has_flag( "PLANTER" ) ){
            extra_drag += vpi.power;
        }
        if( reaper_on && vpi.has_flag( "REAPER" ) ){
            extra_drag += vpi.power;
        }
        // Build map of point -> all parts in that point
        const point pt = parts[p].mount;
        // This will keep the parts at point pt sorted
        vii = std::lower_bound( relative_parts[pt].begin(), relative_parts[pt].end(), p, svpv );
        relative_parts[pt].insert( vii, p );
    }

    // NB: using the _old_ pivot point, don't recalc here, we only do that when moving!
    precalc_mounts( 0, pivot_rotation[0], pivot_anchor[0] );
    check_environmental_effects = true;
    insides_dirty = true;
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
        if ( !(part_with_feature(p, "ROOF") >= 0 && parts[p].hp > 0) ) {
            parts[p].inside = false;
            continue;
        }

        parts[p].inside = true; // inside if not otherwise
        for (int i = 0; i < 4; i++) { // let's check four neighbour parts
            int ndx = i < 2? (i == 0? -1 : 1) : 0;
            int ndy = i < 2? 0 : (i == 2? - 1: 1);
            std::vector<int> parts_n3ar = parts_at_relative (parts[p].mount.x + ndx,
                                                             parts[p].mount.y + ndy);
            bool cover = false; // if we aren't covered from sides, the roof at p won't save us
            for (auto &j : parts_n3ar) {
                if (part_flag(j, "ROOF") && parts[j].hp > 0) { // another roof -- cover
                    cover = true;
                    break;
                }
                else
                if (part_flag(j, "OBSTACLE") && parts[j].hp > 0) {
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

bool vehicle::is_inside(int const p) const
{
    if (p < 0 || p >= (int)parts.size()) {
        return false;
    }
    if (insides_dirty) {
        // TODO: this is a bit of a hack as refresh_insides has side effects
        // this should be called elsewhere and not in a function that intends to just query
        const_cast<vehicle*>(this)->refresh_insides();
    }
    return parts[p].inside;
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
    int parm = part_with_feature( p, "ARMOR" );
    int pdm = random_entry( pl );
    int dres;
    if( parm < 0 ) {
        // not covered by armor -- damage part
        dres = damage_direct( pdm, dmg, type );
    } else {
        // covered by armor -- damage armor first
        // half damage for internal part(over parts not covered)
        bool overhead = part_flag( pdm, "ROOF" ) || part_info( pdm ).location == "on_roof";
        // Calling damage_direct may remove the damaged part
        // completely, therefor the other indes (pdm) becames
        // wrong if pdm > parm.
        // Damaging the part with the higher index first is save,
        // as removing a part only changes indizes after the
        // removed part.
        if( parm < pdm ) {
            damage_direct( pdm, overhead ? dmg : dmg / 2, type );
            dres = damage_direct( parm, dmg, type );
        } else {
            dres = damage_direct( parm, dmg, type );
            damage_direct( pdm, overhead ? dmg : dmg / 2, type );
        }
    }

    return dres;
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
 * @param dx How much to shift on the x-axis.
 * @param dy How much to shift on the y-axis.
 */
void vehicle::shift_parts( const point delta )
{
    for( auto &elem : parts ) {
        elem.mount -= delta;
    }

    decltype(labels) new_labels;
    for( auto &l : labels ) {
        new_labels.insert( label( l.x - delta.x, l.y - delta.y, l.text ) );
    }
    labels = new_labels;

    //Don't use the cache as it hasn't been updated yet
    std::vector<int> origin_parts = parts_at_relative(0, 0, false);

    posx += parts[origin_parts[0]].precalc[0].x;
    posy += parts[origin_parts[0]].precalc[0].y;

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

int vehicle::damage_direct( int p, int dmg, damage_type type )
{
    if (parts[p].hp <= 0) {
        /* Already-destroyed part - chance it could be torn off into pieces.
         * Chance increases with damage, and decreases with part max durability
         * (so lights, etc are easily removed; frames and plating not so much) */
        if(rng(0, part_info(p).durability / 10) < dmg) {
            const auto pos = global_pos() + parts[p].precalc[0];
            if(part_info(p).location == part_location_structure) {
                //For structural parts, remove other parts first
                std::vector<int> parts_in_square = parts_at_relative(parts[p].mount.x, parts[p].mount.y);
                for(int index = parts_in_square.size() - 1; index >= 0; index--) {
                    //Ignore the frame being destroyed
                    if(parts_in_square[index] != p) {
                        if(parts[parts_in_square[index]].hp == 0) {
                            //Tearing off a broken part - break it up
                            if(g->u.sees( pos )) {
                                add_msg(m_bad, _("The %s's %s breaks into pieces!"), name.c_str(),
                                        part_info(parts_in_square[index]).name.c_str());
                            }
                            break_part_into_pieces(parts_in_square[index], pos.x, pos.y, true);
                        } else {
                            //Intact (but possibly damaged) part - remove it in one piece
                            if(g->u.sees( pos )) {
                                add_msg(m_bad, _("The %1$s's %2$s is torn off!"), name.c_str(),
                                        part_info(parts_in_square[index]).name.c_str());
                            }
                            item part_as_item = parts[parts_in_square[index]].properties_to_item();
                            tripoint dest( pos, smz );
                            g->m.add_item_or_charges( dest, part_as_item, true );
                            remove_part(parts_in_square[index]);
                        }
                    }
                }
                /* After clearing the frame, remove it if normally legal to do
                 * so (it's not holding the vehicle together). At a later date,
                 * some more complicated system (such as actually making two
                 * vehicles from the split parts) would be ideal. */
                if(can_unmount(p)) {
                    if(g->u.sees( pos )) {
                        add_msg(m_bad, _("The %1$s's %2$s is destroyed!"),
                                name.c_str(), part_info(p).name.c_str());
                    }
                    break_part_into_pieces(p, pos.x, pos.y, true);
                    remove_part(p);
                }
            } else {
                //Just break it off
                if(g->u.sees( pos )) {
                    add_msg(m_bad, _("The %1$s's %2$s is destroyed!"),
                                    name.c_str(), part_info(p).name.c_str());
                }
                break_part_into_pieces(p, pos.x, pos.y, true);
                remove_part(p);
            }
        }
        return dmg;
    }

    int tsh = part_info(p).durability / 10;
    if (tsh > 20) {
        tsh = 20;
    }
    int dres = dmg;
    if( dmg >= tsh || type == DT_HEAT || type == DT_TRUE )
    {
        dres -= parts[p].hp;
        int last_hp = parts[p].hp;
        parts[p].hp -= dmg;
        if( parts[p].hp < 0 ) {
            parts[p].hp = 0;
        }

        if( parts[p].hp == 0 && last_hp > 0) {
            insides_dirty = true;
        }

        if( part_flag( p, "FUEL_TANK" ) ) {
            const itype_id &ft = part_info(p).fuel_type;
            if( ft == fuel_type_gasoline || ft == fuel_type_diesel || ft == fuel_type_plasma ) {
                // TODO: Move the values below to jsons
                // Defaults
                int explosion_chance = 0;
                float explosion_factor = 0;
                bool fiery_explosion = false;
                float fuel_size_factor = 0;

                // Gasoline
                if (ft == fuel_type_gasoline) {
                    if (type == DT_HEAT) {
                        explosion_chance = 2;
                    } else {
                        explosion_chance = 5;
                    }
                    fiery_explosion = true; // Produces lasting flames
                    fuel_size_factor = .1; // Smaller units than normal
                    explosion_factor = 1;

                // Diesel
                } else if (ft == fuel_type_diesel) {
                    if (type == DT_HEAT) {
                        explosion_chance = 20; // Still somewhat vulnerable to heat damage
                    } else {
                        explosion_chance = 1000; // Elsewise very unlikely to explode
                    }
                    fiery_explosion = false; // Doesn't produce lasting flames
                    fuel_size_factor = .1; // Smaller units than normal
                    explosion_factor = .2; // Only partial explosions

                // Hydrogen
                } else if (ft == fuel_type_plasma) {
                    // Very likely to explode; real life tanks are armored to stop this.
                    if (type == DT_HEAT) {
                        explosion_chance = 1;
                    } else {
                        explosion_chance = 2;
                    }
                    fiery_explosion = false; // WOOF!!; but no lasting flames
                    fuel_size_factor = 1;
                    explosion_factor = 1.4; // Higher energy density, but dampened by the explosion type
                }
                const int pow = 120 * (1 - exp(explosion_factor / -5000 * (parts[p].amount * fuel_size_factor)));
                //debugmsg( "damage check dmg=%d pow=%d amount=%d", dmg, pow, parts[p].amount );
                if(parts[p].hp <= 0) {
                    leak_fuel( p );
                }
                if (one_in(explosion_chance)) {
                    g->u.add_memorial_log(pgettext("memorial_male","The fuel tank of the %s exploded!"),
                        pgettext("memorial_female", "The fuel tank of the %s exploded!"),
                        name.c_str());
                    g->explosion( tripoint( global_x() + parts[p].precalc[0].x,
                                            global_y() + parts[p].precalc[0].y,
                                            smz ),
                                  pow, 0.7, 0, fiery_explosion );
                    parts[p].hp = 0;
                    parts[p].amount = 0;
                }
            }
        } else if (parts[p].hp <= 0 && part_flag(p, "UNMOUNT_ON_DAMAGE")) {
            tripoint dest( global_x() + parts[p].precalc[0].x,
                           global_y() + parts[p].precalc[0].y,
                           smz );
            g->m.spawn_item( dest, part_info(p).item, 1, 0, calendar::turn );
            remove_part( p );
        }
    }
    if (dres < 0)
        dres = 0;
    return dres;
}

void vehicle::leak_fuel (int p)
{
    if (!part_flag(p, "FUEL_TANK")) {
        return;
    }

    const itype_id &ft = part_info(p).fuel_type;
    item leak( ft, calendar::turn );

    if (ft == fuel_type_gasoline || ft == fuel_type_diesel) {
        tripoint minp = global_pos3();
        tripoint maxp = minp;
        minp.x -= 2;
        minp.y -= 2;
        maxp.x += 2;
        maxp.y += 2;

        for ( const tripoint &pt : g->m.points_in_rectangle(minp, maxp) ){
            if (g->m.move_cost(pt) > 0 && one_in(2)) {
                int leak_amount = rng(79, 121);

                if (parts[p].amount < leak_amount) {
                    parts[p].amount = 0;
                    return;
                }

                leak.charges = leak_amount;
                g->m.add_item_or_charges( pt, leak );
                parts[p].amount -= leak_amount;
            }
        }
    }
    parts[p].amount = 0;
}

std::string aim_type( const vehicle_part &part )
{
    const auto &target = part.target;
    if( target.first == target.second ) {
        return part.mode > 0 ? _("Auto") : _("No target");
    }

    if( !g->u.sees( target.first ) ) {
        return _("Unseen");
    }

    const Creature *critter = g->critter_at( target.first );
    if( critter != nullptr && g->u.sees( *critter ) ) {
        return critter->disp_name();
    } else if( g->m.has_furn( target.first ) ) {
        return g->m.furn_at( target.first ).name;
    } else {
        return g->m.tername( target.first );
    }
}

int vehicle::get_turret_range( int p, bool manual )
{
    if( !part_flag( p, "TURRET" ) ) {
        return -1;
    }

    if( ( !manual && part_flag( p, "MANUAL" ) ) ||
        ( manual && part_flag( p, "NO_MANUAL" ) ) ) {
        return -1;
    }

    if( !manual ) {
        return part_info( p ).range;
    }

    const auto turret_data = turret_has_ammo( p );
    if( turret_data.charges <= 0L ) {
        // Couldn't find ammo
        return -1;
    }

    const item &gun = turret_data.gun;
    const itype *am_itype = gun.get_curammo();
    const auto &ammo_data = *am_itype->ammo;
    const auto &gun_data = *gun.type->gun;
    return gun_data.range + ammo_data.range;
}

turret_fire_ability vehicle::turret_can_shoot( const int p, const tripoint &pos )
{
    if( part_flag( p, "MANUAL" ) ) {
        return turret_wont_aim;
    }

    if( parts[p].mode == 0 ) {
        return turret_is_off;
    }

    if( turret_has_ammo( p ).charges < 1 ) {
        return turret_no_ammo;
    }

    const int turrange = get_turret_range( p, true );
    tripoint tpos( global_pos3() + parts[p].precalc[0] );
    if( rl_dist( tpos, pos ) > turrange ) {
        return turret_out_of_range;
    }

    return turret_all_ok;
}

std::map< int, turret_fire_ability > vehicle::turrets_can_shoot( const tripoint &pos )
{
    std::vector< int > turrets = all_parts_with_feature( "TURRET", true );
    std::map< int, turret_fire_ability > ret;
    for( const int turret_index : turrets ) {
        ret[turret_index] = turret_can_shoot( turret_index, pos );
    }

    return ret;
}

bool vehicle::aim_turrets()
{
    std::vector< int > turrets = all_parts_with_feature( "TURRET", true );
    // We only want turrets that can aim by themselves and are on
    auto cant_shoot = [&]( const int tur ) {
        return part_flag( tur, "MANUAL" ) || parts[tur].mode == 0;
    };
    turrets.erase( std::remove_if( turrets.begin(), turrets.end(), cant_shoot ), turrets.end() );
    if( turrets.empty() ) {
        add_msg( m_warning, _("Can't aim turrets: all turrets are offline") );
        return false;
    }

    // Find the radius of a circle (centered at u) that encompasses points turrets can aim at
    // Each turret range circle is represented by 4 points
    // Maybe extend to 8 (diagonals) for better accuracy with circular?
    std::vector< tripoint > bounds;
    bounds.reserve( 4 * turrets.size() );
    for( const int turret_index : turrets ) {
        const int turrange = get_turret_range( turret_index, true );
        if( turrange <= 0 ) {
            continue;
        }

        tripoint tpos = global_pos3() + parts[turret_index].precalc[0];
        bounds.push_back( { tpos.x + turrange, tpos.y, tpos.z } );
        bounds.push_back( { tpos.x - turrange, tpos.y, tpos.z } );
        bounds.push_back( { tpos.x, tpos.y + turrange, tpos.z } );
        bounds.push_back( { tpos.x, tpos.y - turrange, tpos.z } );
        // Reset turrets
        parts[turret_index].target.first = tpos;
        parts[turret_index].target.second = tpos;
    }

    const tripoint &upos = g->u.pos3();
    int range = 0;
    for( auto &bnd : bounds ) {
        int dist = rl_dist( upos, bnd );
        range = std::max( range, dist );
    }

    if( range <= 0 ) {
        return false;
    }

    // Fake gun item to aim
    item pointer( "vehicle_pointer", 0 );
    pointer.set_curammo( "pointer_fake_ammo" );
    auto tmpammo = pointer.get_curammo()->ammo.get();
    tmpammo->range = range;

    target_mode tmode = TARGET_MODE_TURRET; // We can't aim here yet
    tripoint player_pos = g->u.pos3();
    auto trajectory = g->pl_target_ui( player_pos, range, &pointer, tmode );
    if( trajectory.empty() ) {
        add_msg( m_info, _("Clearing targets") );
        return false;
    }

    const tripoint &targ = trajectory.back();
    for( int turret_index : turrets ) {
        int turrange = get_turret_range( turret_index, true );
        tripoint tpos( global_pos() + parts[turret_index].precalc[0], g->u.posz() );
        if( turrange < rl_dist( tpos, targ ) ) {
            continue;
        }

        parts[turret_index].target.second = targ;
    }

    if( turret_mode == turret_mode_off ) {
        turret_mode = turret_mode_manual;
    }

    // Take at least a single whole turn to aim
    ///\EFFECT_INT speeds up aiming of vehicle turrets
    g->u.moves = std::min( 0, g->u.moves - 100 + (5 * g->u.int_cur) );
    g->draw_ter();
    return true;
}

void vehicle::control_turrets() {
    std::vector< int > all_turrets = all_parts_with_feature( "TURRET", true );
    std::vector< int > turrets;
    std::vector< tripoint > locations;

    uimenu pmenu;
    for( int p : all_turrets ) {
        if( !part_flag( p, "MANUAL" ) ) {
            turrets.push_back( p );
            locations.push_back( tripoint( global_pos() + parts[p].precalc[0], smz ) );
        }
    }

    pointmenu_cb callback( locations );

    int selected = 0;

    while( true ) {
        pmenu.title = _("Cycle turret mode");
        pmenu.callback = &callback;
        // Regen menu entries
        for( int i = 0; i < (int)turrets.size(); i++ ) {
            int p = turrets[i];
            const item gun( part_info( p ).item, 0 );
            bool charge = gun.is_charger_gun();
            bool burst = gun.burst_size() > 1;
            char sym;
            if( parts[p].mode == 0 ) {
                sym = ' ';
            } else if( parts[p].mode > 9 && burst ) {
                sym = 'B'; // Burst
            } else if( parts[p].mode > 9 && charge ) {
                sym = 'X'; // Big charge
            } else if( parts[p].mode > 0 && parts[p].mode < 10 ) {
                sym = '0' + parts[p].mode;
            } else if( parts[p].mode < -1 ) {
                sym = 'M'; // Manual burst
            } else if( parts[p].mode == -1 ) {
                sym = 'm'; // Manual
            } else {
                sym = 'x';
            }
            pmenu.addentry( i, true, MENU_AUTOASSIGN, "[%c] %s", sym, part_info( p ).name.c_str() );
        }

        pmenu.addentry( turrets.size(), true, 'q', _("Finish") );
        pmenu.w_y = 0; // Move the menu so that we can see our vehicle

        pmenu.selected = selected;
        pmenu.fselected = selected;
        pmenu.query();
        if( pmenu.ret < 0 || pmenu.ret >= (int)turrets.size() ) {
            break;
        }

        selected = pmenu.ret;
        cycle_turret_mode( turrets[selected], false );

        pmenu.reset();
    }

    if( turret_mode == turret_mode_off ) {
        add_msg( m_warning, _("Turrets have been configured, but the vehicle turret system is off.") );
    }
}

void vehicle::cycle_turret_mode( int p, bool only_manual_modes )
{
    if( !part_flag( p, "TURRET" ) ) {
        debugmsg( "vehicle::cycle_turret_mode tried to pick a non-turret part" );
        return;
    }

    const item gun( part_info( p ).item, 0 );
    if( gun.type->gun == nullptr ) {
        debugmsg( "vehicle::cycle_turret_mode tried to pick a non-turret part" );
        return;
    }

    vehicle_part &tr = parts[p];
    const bool auto_only = gun.has_flag("BURST_ONLY");
    const bool burst_or_charge = gun.burst_size() > 1 || gun.is_charger_gun();
    const bool was_auto = tr.mode > 0;
    if( tr.mode < -1 && !auto_only ) {
        tr.mode = -1;
    } else if( tr.mode < 0 && !only_manual_modes ) {
        tr.mode = 0;
    } else if( tr.mode <= 0 && !only_manual_modes && !auto_only ) {
        tr.mode = 1;
    } else if( tr.mode <= 1 && !only_manual_modes ) {
        tr.mode = burst_or_charge ? 1000 : -1;
    } else {
        tr.mode = burst_or_charge ? -1000 : -1;
    }

    if( only_manual_modes ) {
        const char *name = part_info( p ).name.c_str();
        if( tr.mode < -1 && gun.is_charger_gun() ) {
            add_msg( m_info, _("Setting turret %s to full power mode."), name );
        } else if( tr.mode == -1 && gun.is_charger_gun() ) {
            add_msg( m_info, _("Setting turret %s to low power mode."), name );
        } else if( tr.mode < -1 ) {
            add_msg( m_info, _("Setting turret %s to burst mode."), name );
        } else if( tr.mode == -1 ) {
            add_msg( m_info, _("Setting turret %s to single-fire mode."), name );
        } else if( tr.mode == 0 ) {
            add_msg( m_info, _("Disabling turret %s."), name );
        } else {
            add_msg( m_info, _("Setting turret %s to buggy mode."), name );
        }

        if( was_auto ) {
            add_msg( m_warning, _("Disabling automatic target acquisition on %s"), name );
        }
    }
}

void vehicle::cycle_global_turret_mode()
{
    std::vector< int > turrets = all_parts_with_feature( "TURRET", true );
    if( turret_mode != turret_mode_off ) {
        turret_mode = turret_mode_off;
        add_msg( _("Turrets: Disabled") );
        // Clear aim points
        for( int p : turrets ) {
            parts[p].target.second = parts[p].target.first;
        }

        return;
    }

    turret_mode = turret_mode_autotarget;
    add_msg( _("Turrets: Enabled") );
    for( int p : turrets ) {
        if( parts[p].mode > 0 ) {
            return; // No need to warn
        }
    }

    add_msg( m_warning, _("Turret system is on, but all turrets are configured not to shoot.") );
}

std::map<itype_id, long> vehicle::fuels_left() const
{
    std::map<itype_id, long> result;
    for( const auto &part : parts ) {
        const vpart_info &vpinfo = part.info();
        if( !vpinfo.has_flag( VPFLAG_FUEL_TANK ) || part.amount <= 0 ) {
            continue;
        }
        result[vpinfo.fuel_type] += part.amount;
    }
    return result;
}

vehicle::turret_ammo_data vehicle::turret_has_ammo( int const p ) const
{
    return turret_ammo_data( *this, p );
}

vehicle::turret_ammo_data::turret_ammo_data( const vehicle &veh, int const part )
: gun( veh.part_info( part ).item, 0 )
{
    // Start out with an infinite amount and lower it based on the available amount
    long ammo_for = std::numeric_limits<long>::max();
    if( !gun.is_gun() ) {
        return;
    }
    const auto &gun_data = *gun.type->gun;
    const long power = veh.fuel_left( fuel_type_battery );
    if( gun_data.ups_charges > 0 ) {
        ammo_for = std::min( ammo_for, power / gun_data.ups_charges );
        if( ammo_for <= 0L ) {
            is_missing_ups_charges = true;
            // No point in going any further, the gun can not fire at all.
            return;
        }
    }

    // NO_AMMO guns don't have specific ammo type that could be consumed after firing, (therefor
    // source NONE). They should (theoretically) require UPS charges, but that is checked above
    // and already included in the value of ammo_for.
    // UPS charges will be consumed directly in manual_fire_turret/automatic_fire_turret,
    // gun needs to have a valid curamm, this is a required precondition.
    if( gun.has_flag( "NO_AMMO" ) ) {
        source = NONE;
        ammo = nullptr;
        gun.set_curammo( "generic_no_ammo" );
        charges = ammo_for;
        return;
    }

    // In case of charger guns, we return max charge rather than max burst.
    // Note that charger guns have special ammo type, but they consume regular "battery" ammo from
    // the vehicle tank, therefor ammo != gun.curammo, but both have to be valid.
    if ( gun.is_charger_gun() ) {
        charge_mult *= 5;
        source = TANK;
        ammo = item::find_type( fuel_type_battery );
        gun.set_curammo( "charge_shot" );
        charges = std::min( ammo_for, power / charge_mult );
        return;
    }

    // Find fuel (inside a tank) that is valid ammo for the turret, several fuel types might match:
    // consider a flamethrower on a car with a gasoline tank *and* a napalm tank.
    const ammotype &amt = gun_data.ammo;
    for( auto &fuel_and_amount : veh.fuels_left() ) {
        const itype *type = item::find_type( fuel_and_amount.first );
        if( !type->ammo || type->ammo->type != amt ) {
            continue;
        }
        // TODO (or not?): select which ammo to use. For now, we use the first one (usually there is only one anyway).
        // However, note that the ordering of the map may change on each game as it compares *pointers*
        const itype_id &ammo_id = fuel_and_amount.first;
        const long liquid_fuel = fuel_and_amount.second;

        if( ammo_id == fuel_type_plasma ) {
            charge_mult *= 10; // 1 unit of hydrogen adds 10 units to hydro tank
        }

        gun.set_curammo( ammo_id );
        if( !gun.has_curammo() ) {
            debugmsg( "turret %s tried to use %s (which isn't an ammo type) as ammo for %s",
                      veh.part_info( part ).id.c_str(), ammo_id.c_str(), gun.typeId().c_str() );
            return; // charges is still 0, so the caller won't use gun.curammo
        }
        ammo = gun.get_curammo();
        source = TANK;
        charges = std::min( ammo_for, liquid_fuel / charge_mult );
        return;
    }

    auto items = veh.get_items( part );
    if( items.empty() ) {
        return;
    }

    const itype *am_type = items.front().type;
    if( !am_type->ammo || am_type->ammo->type != amt || items.front().charges < 1 ) {
        return;
    }

    charges = std::min( ammo_for, items.front().charges );
    gun.set_curammo( items.front() );
    ammo = gun.get_curammo();
    source = CARGO;
}

bool vehicle::fire_turret( int p, bool manual )
{
    if( !part_flag ( p, "TURRET" ) ) {
        return false;
    }

    auto &target = parts[p].target;
    // Don't let manual-only turrets aim
    if( !manual && part_flag( p, "MANUAL" ) ) {
        return false;
    }

    // Don't let non-manual turrets get aimed manually
    if( manual && part_flag( p, "NO_MANUAL" ) ) {
        if( manual ) {
            add_msg( m_bad, _("This turret can't be aimed manually") );
        }

        return false;
    }

    if( parts[p].mode == 0 ) {
        if( !manual ) {
            return false;
        }

        cycle_turret_mode( p, true );
    }

    // turret_mode_manual means the turrets are globally off, but some are aimed
    // target.first == target.second means the turret isn't aimed
    if( !manual && ( turret_mode == turret_mode_manual || parts[p].mode < 0 ) &&
        target.first == target.second ) {
        return false;
    }

    const auto turret_data = turret_has_ammo( p );
    const item &gun = turret_data.gun;
    if( !gun.is_gun() ) {
        return false;
    }

    if( turret_data.is_missing_ups_charges ) {
        if( manual ) {
            add_msg( m_bad, _("This turret is not powered") );
        }
        return false;
    }
    long charges = std::max( 1, turret_data.gun.burst_size() );
    if( gun.is_charger_gun() ) {
        if( one_in(100) ) {
            charges = rng( 5, 8 ); // kaboom
        } else {
            charges = rng( 1, 4 );
        }

        if( charges > abs( parts[p].mode ) ) {
            charges = abs( parts[p].mode ); // Currently only limiting, not increasing
        }
    }
    charges = std::min( charges, turret_data.charges );
    if( charges <= 0 ) {
        if( manual ) {
            add_msg( m_bad, _("This turret doesn't have enough ammo") );
        }

        return false;
    }
    const itype *am_type = turret_data.gun.get_curammo();
    long charges_left = charges;
    // TODO sometime: change that g->u to a parameter, so that NPCs can shoot too
    const bool success = manual ?
    // TODO: unify those two functions.
        manual_fire_turret( p, g->u, *gun.type, *am_type, charges_left ) :
        automatic_fire_turret( p, *gun.type, *am_type, charges_left );
    if( success ) {
        turret_data.consume( *this, p, charges - charges_left );
    }

    // If manual, we need to know if the shot was actually executed
    return !manual || success;
}

void vehicle::turret_ammo_data::consume( vehicle &veh, int const part, long const charges_consumed ) const
{
    switch( source ) {
        case TANK:
            veh.drain( ammo->id, charges_consumed * charge_mult );
            break;
        case CARGO:
            {
                auto items = veh.get_items( part );
                item &ammo_item = items.front();
                ammo_item.charges -= charges_consumed * charge_mult;
                if( ammo_item.charges <= 0 ) {
                    items.erase( items.begin() );
                }
            }
            break;
        case NONE:
            // no conventional ammo is consumed, UPS-power may have been consumed in
            // automatic_fire_turret or manual_fire_turret
            break;
    }
}

bool vehicle::automatic_fire_turret( int p, const itype &gun, const itype &ammo, long &charges )
{
    tripoint pos = global_pos3();
    pos.x += parts[p].precalc[0].x;
    pos.y += parts[p].precalc[0].y;
    int range = part_info( p ).range;

    npc tmp;
    tmp.set_fake( true );
    tmp.add_effect( "on_roof", 1 );
    tmp.name = rmp_format(_("<veh_player>The %s"), part_info(p).name.c_str());
    tmp.skillLevel(gun.gun->skill_used).level(8);
    tmp.skillLevel( skill_id( "gun" ) ).level(4);
    tmp.recoil = abs(velocity) / 100 / 4;
    tmp.setpos( pos );
    tmp.str_cur = 16;
    tmp.dex_cur = 8;
    tmp.per_cur = 12;
    // Assume vehicle turrets are defending the player.
    tmp.attitude = NPCATT_DEFEND;
    tmp.weapon = item(gun.id, 0);
    tmp.weapon.set_curammo( ammo.id );
    tmp.weapon.charges = charges;
    tmp.weapon.update_charger_gun_ammo();

    int area = std::max( aoe_size( tmp.weapon.get_curammo()->ammo->ammo_effects ),
                         aoe_size( tmp.weapon.type->gun->ammo_effects ) );
    if( area > 0 ) {
        area += area == 1 ? 1 : 2; // Pad a bit for less friendly fire
    }

    tripoint targ = pos;
    auto &target = parts[p].target;
    if( target.first == target.second && !part_flag( p, "MANUAL" ) ) {
        // Manual target not set, find one automatically
        const bool u_see = g->u.sees( pos );
        int boo_hoo;
        Creature *auto_target = tmp.auto_find_hostile_target( range, boo_hoo, area );
        if( auto_target == nullptr ) {
            if( u_see && boo_hoo ) {
                add_msg( m_warning, ngettext( "%s points in your direction and emits an IFF warning beep.",
                                              "%s points in your direction and emits %d annoyed sounding beeps.",
                                              boo_hoo),
                            tmp.name.c_str(), boo_hoo);
            }
            return false;
        }

        targ = auto_target->pos3();
    } else if( target.first != target.second ) {
        // Target set manually
        // Make sure we didn't move between aiming and firing (it's a bug if we did)
        if( targ != target.first ) {
            target.second = target.first;
            return false;
        }

        targ = target.second;
        // Remove the target
        target.second = target.first;
    } else {
        // Shouldn't happen
        target.first = target.second;
        return false;
    }

    // Move the charger gun "whoosh" here - no need to pass it from above
    if( tmp.weapon.is_charger_gun() && charges > 20 ) {
        sounds::sound( targ, 20, _("whoosh!") );
    }
    // notify player if player can see the shot
    if( g->u.sees( pos ) ) {
        add_msg(_("The %1$s fires its %2$s!"), name.c_str(), part_info(p).name.c_str());
    }
    // Spawn a fake UPS to power any turreted weapons that need electricity.
    item tmp_ups( "fake_UPS", 0 );
    // Drain a ton of power
    tmp_ups.charges = drain( fuel_type_battery, 1000 );
    tmp.worn.insert( tmp.worn.end(), tmp_ups );
    tmp.fire_gun( targ, (long)abs( parts[p].mode ) );
    // Return whatever is left.
    refill( fuel_type_battery, tmp.worn.back().charges );
    charges = tmp.weapon.charges; // Return real ammo, in case of burst ending early

    return true;
}

bool vehicle::manual_fire_turret( int p, player &shooter, const itype &guntype,
                                  const itype &ammotype, long &charges )
{
    tripoint pos = global_pos3() + tripoint( parts[p].precalc[0], 0 );

    // Place the shooter at the turret
    const tripoint &oldpos = shooter.pos();
    shooter.setpos( pos );

    // Create a fake gun
    // TODO: Damage the gun based on part hp
    item gun( guntype.id, 0 );
    gun.set_curammo( ammotype.id );
    gun.charges = charges;

    // Give shooter fake weapon
    item old_weapon = shooter.weapon;
    shooter.weapon = gun;

    // Spawn a fake UPS to power any turreted weapons that need electricity.
    item tmp_ups( "fake_UPS", 0 );
    // Drain a ton of power
    tmp_ups.charges = drain( fuel_type_battery, 1000 );
    // Fire_gun expects that the fake UPS is a last worn item
    shooter.worn.insert( shooter.worn.end(), tmp_ups );

    const int range = shooter.weapon.gun_range( &shooter );
    auto mons = shooter.get_visible_creatures( range );
    constexpr target_mode tmode = TARGET_MODE_TURRET_MANUAL; // No aiming yet!
    tripoint shooter_pos = shooter.pos3();
    auto trajectory = g->pl_target_ui( shooter_pos, range, &shooter.weapon, tmode );
    shooter.recoil = abs(velocity) / 100 / 4;
    if( !trajectory.empty() ) {
        // Need to redraw before shooting
        g->draw_ter();
        const tripoint &targ = trajectory.back();
        // Put our shooter on the roof of the vehicle
        shooter.add_effect( "on_roof", 1 );
        shooter.fire_gun( targ, (long)abs( parts[p].mode ) );
        // And now back - we don't want to get any weird behavior
        shooter.remove_effect( "on_roof" );
    }

    // Done shooting, clean up
    item &ups = shooter.worn.back();
    if( ups.type->id == "fake_UPS" ) {
        refill( fuel_type_battery, ups.charges );
        shooter.worn.pop_back();
    } else {
        // This shouldn't happen, not even if armor below the UPS is destroyed
        // ...but let's handle anyway
        auto upses = shooter.remove_items_with( [] ( item &it ) {
            return it.type->id == "fake_UPS";
        } );
        for( auto &up : upses ) {
            refill( fuel_type_battery, up.charges );
        }
    }

    charges = shooter.weapon.charges;

    // Place the shooter back where we took them from
    shooter.setpos( oldpos );
    // Give back old weapon
    shooter.weapon = old_weapon;

    // Deactivate automatic aiming
    if( parts[p].mode > 0 ) {
        parts[p].mode = -parts[p].mode;
        add_msg( m_warning, _("Deactivating automatic target acquisition for this turret" ));
    }

    return !trajectory.empty();
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
               parts[part_index].info().name.c_str(), name.c_str());
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
               parts[part_index].info().name.c_str(), name.c_str());
  } else {
    open_or_close(part_index, false);
  }
}

void vehicle::open_all_at(int p)
{
    std::vector<int> parts_here = parts_at_relative(parts[p].mount.x, parts[p].mount.y);
    for( auto &elem : parts_here ) {
        if( part_flag( elem, VPFLAG_OPENABLE ) ) {
            // Note that this will open mutlisquare and non-multipart parts in the tile. This
            // means that adjacent open multisquare openables can still have closed stuff
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
        const bool is_id = part_info(next_index).loadid == part_info(part_index).loadid;
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
    // Threshold of recovery is gaussianesque.

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

// normalized.
rl_vec2d vehicle::move_vec() const
{
    float mx,my;
    mx = cos (move.dir() * M_PI/180);
    my = sin (move.dir() * M_PI/180);
    rl_vec2d ret(mx,my);
    return ret;
}

// normalized.
rl_vec2d vehicle::face_vec() const
{
    float fx,fy;
    fx = cos (face.dir() * M_PI/180);
    fy = sin (face.dir() * M_PI/180);
    rl_vec2d ret(fx,fy);
    return ret;
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

int vehicle::obstacle_at_part( int p ) const
{
    if( part_flag( p, VPFLAG_OBSTACLE ) && parts[p].hp > 0 ) {
        return p;
    }

    int part = part_with_feature( p, VPFLAG_OBSTACLE, true );
    if( part < 0 ) {
        return -1; // No obstacle here
    }

    if( part_flag( part, VPFLAG_OPENABLE ) && parts[part].open ) {
        return -1; // Open door here
    }

    return part;
}

std::set<tripoint> &vehicle::get_points( const bool force_refresh )
{
    if( force_refresh || occupied_cache_turn != calendar::turn ) {
        occupied_cache_turn = calendar::turn;
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
    const tripoint smp = overmapbuffer::ms_to_sm_copy( real_global_pos );
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

void vehicle::update_time( const calendar &update_to )
{
    if( smz < 0 ) {
        return;
    }

    const auto update_from = last_update_turn;
    if( update_to - update_from < MINUTES(1) ) {
        // We don't need to check every turn
        return;
    }
    last_update_turn = update_to;

    // Weather stuff, only for z-levels >= 0
    // TODO: Have it wash cars from blood?
    if( funnels.empty() && solar_panels.empty() ) {
        return;
    }

    // Get one weather data set per veh, they don't differ much across veh area
    const tripoint veh_loc = real_global_pos3();
    auto accum_weather = sum_conditions( update_from, update_to, veh_loc );
    if( !funnels.empty() ) {
        double rain_amount = 0.0;
        // TODO: double acid_amount = 0.0;
        for( int part : funnels ) {
            if( parts[part].hp <= 0 ) {
                continue;
            }

            const tripoint part_loc = veh_loc + parts[part].precalc[0];
            if( !is_sm_tile_outside( part_loc ) ) {
                continue;
            }

            const int part_size = part_info( part ).size;
            const double funnel_area_mm = M_PI * part_size * part_size;
            rain_amount += funnel_charges_per_turn( funnel_area_mm, accum_weather.rain_amount );
        }

        const int rain_val = divide_roll_remainder( rain_amount, 1.0 );
        if( rain_val > 0 ) {
            refill( "water", rain_val );
            add_msg( m_debug, "%s got %d water from funnels", name.c_str(), rain_val );
        }
    }

    if( !solar_panels.empty() ) {
        int epower = 0;
        for( int part : solar_panels ) {
            if( parts[part].hp <= 0 ) {
                continue;
            }

            const tripoint part_loc = veh_loc + parts[part].precalc[0];
            if( !is_sm_tile_outside( part_loc ) ) {
                continue;
            }

            epower += ( part_epower( part ) * accum_weather.sunlight ) / DAYLIGHT_LEVEL;
        }

        if( epower > 0 ) {
            add_msg( m_debug, "%s got %d epower from solars", name.c_str(), epower );
            charge_battery( epower_to_power( epower ) );
        }
    }
}

/*-----------------------------------------------------------------------------
 *                              VEHICLE_PART
 *-----------------------------------------------------------------------------*/
vehicle_part::vehicle_part( int const dx, int const dy )
: id( NULL_ID )
, mount( dx, dy )
, precalc( { { point( -1, -1 ), point( -1, -1 ) } } )
, amount( 0 )
{
}

vehicle_part::vehicle_part( const vpart_str_id &sid, int const dx, int const dy,
                            const item *const it )
: vehicle_part( dx, dy )
{
    set_id( sid );
    if( it != nullptr ) {
        properties_from_item( *it );
    }
}

void vehicle_part::set_id( const vpart_str_id & str )
{
    id = str;
}

const vpart_str_id &vehicle_part::get_id() const
{
    return id.id();
}

void vehicle_part::properties_from_item( const item &used_item )
{
    const vpart_info &vpinfo = info();
    if( used_item.is_var_veh_part() ) {
        bigness = used_item.bigness;
    }
    // item damage is 0,1,2,3, or 4. part hp is 1..durability.
    // assuming it rusts. other item materials disintegrate at different rates...
    int health = 5 - used_item.damage;
    health *= vpinfo.durability; //[0,dur]
    health /= 5;
    hp = std::max( 1, health );
    // Transfer fuel from item to tank
    const itype_id &desired_liquid = vpinfo.fuel_type;
    const int fuel_per_charge = fuel_charges_to_amount_factor( desired_liquid );
    if( used_item.charges > 0 && desired_liquid == fuel_type_battery ) {
        amount = std::min<int>( used_item.charges * fuel_per_charge, vpinfo.size );
    } else if( !used_item.contents.empty() ) {
        const item &liquid = used_item.contents[0];
        if( liquid.type->id == desired_liquid ) {
            amount = std::min<int>( liquid.charges * fuel_per_charge, vpinfo.size );
        }
    }
}

item vehicle_part::properties_to_item() const
{
    const vpart_info &vpinfo = info();
    item tmp( vpinfo.item, calendar::turn );
    if( tmp.is_var_veh_part() ) {
        tmp.bigness = bigness;
    }
    // tools go unloaded to prevent user from exploiting this to
    // refill their (otherwise not refillable) tools
    if( tmp.is_tool() || tmp.is_gun() ) {
        tmp.charges = 0;
    }
    // Cables get special handling: their target coordinates need to remain
    // stored, and if a cable actually drops, it should be half-connected.
    if( tmp.has_flag("CABLE_SPOOL") ) {
        tripoint local_pos = g->m.getlocal(target.first);
        if(g->m.veh_at( local_pos ) == nullptr) {
            tmp.item_tags.insert("NO_DROP"); // That vehicle ain't there no more.
        }

        tmp.set_var( "source_x", target.first.x );
        tmp.set_var( "source_y", target.first.y );
        tmp.set_var( "source_z", target.first.z );
        tmp.set_var( "state", "pay_out_cable" );
        tmp.active = true;
    }
    // translate part damage to item damage.
    // max damage is 4, min damage 0.
    // this is very lossy.
    float hpofdur = ( float )hp / vpinfo.durability;
    tmp.damage = std::min( 4, std::max<int>( 0, ( 1 - hpofdur ) * 5 ) );
    // Transfer fuel back to tank, but not to gun or it'll crash
    if( !tmp.is_gun() && !vpinfo.fuel_type.empty() && vpinfo.fuel_type != "null" && amount > 0 ) {
        const itype_id &desired_liquid = vpinfo.fuel_type;
        const int fuel_per_charge = fuel_charges_to_amount_factor( desired_liquid );
        if( desired_liquid == fuel_type_battery ) {
            tmp.charges = amount / fuel_per_charge;
        } else {
            item liquid( desired_liquid, calendar::turn );
            liquid.charges = amount / fuel_per_charge;
            if( liquid.charges > 0 ) {
                tmp.put_in( liquid );
            }
        }
    }
    return tmp;
}

const vpart_info &vehicle_part::info() const
{
    return id.obj();
}
