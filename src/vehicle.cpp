#include "vehicle.h"
#include "map.h"
#include "output.h"
#include "game.h"
#include "item.h"
#include "item_factory.h"
#include <sstream>
#include <stdlib.h>
#include "cursesdef.h"
#include "catacharset.h"

#include "debug.h"

const ammotype fuel_types[num_fuel_types] = { "gasoline", "battery", "plutonium", "plasma", "water" };
/*
 * Speed up all those if ( blarg == "structure" ) statements that are used everywhere;
 *   assemble "structure" once here instead of repeatedly later.
 */
static const std::string fuel_type_gasoline("gasoline");
static const std::string fuel_type_battery("battery");
static const std::string fuel_type_plutonium("plutonium");
static const std::string fuel_type_plasma("plasma");
static const std::string fuel_type_water("water");
static const std::string fuel_type_muscle("muscle");
static const std::string part_location_structure("structure");

enum vehicle_controls {
 toggle_cruise_control,
 toggle_lights,
 toggle_overhead_lights,
 toggle_turrets,
 toggle_tracker,
 activate_horn,
 release_control,
 control_cancel,
 convert_vehicle,
 toggle_reactor,
 toggle_engine,
 toggle_fridge,
 toggle_recharger
};

vehicle::vehicle(std::string type_id, int init_veh_fuel, int init_veh_status): type(type_id)
{
    generation = 0;
    posx = 0;
    posy = 0;
    velocity = 0;
    turn_dir = 0;
    turn_delta_per_tile = 0.0;
    of_turn_carry = 0;
    turret_mode = 0;
    lights_epower = 0;
    overhead_epower = 0;
    fridge_epower = 0;
    recharger_epower = 0;
    tracking_epower = 0;
    cruise_velocity = 0;
    player_thrust = 0;
    skidding = false;
    cruise_on = true;
    lights_on = false;
    tracking_on = false;
    overhead_lights_on = false;
    fridge_on = false;
    recharger_on = false;
    reactor_on = false;
    engine_on = false;

    refresh();
    //type can be null if the type_id parameter is omitted
    if(type != "null") {
      if(g->vtypes.count(type) > 0) {
        //If this template already exists, copy it
        *this = *(g->vtypes[type]);
        init_state(init_veh_fuel, init_veh_status);
      }
    }
}

vehicle::~vehicle()
{
}

bool vehicle::player_in_control (player *p)
{
    int veh_part;
    vehicle *veh = g->m.veh_at (p->posx, p->posy, veh_part);
    if (veh == NULL || veh != this)
        return false;
    return part_with_feature(veh_part, VPFLAG_CONTROLS, false) >= 0 && p->controlling_vehicle;
}

void vehicle::load (std::ifstream &stin)
{
    getline(stin, type);

    if ( type.size() > 1 && ( type[0] == '{' || type[1] == '{' ) ) {
        std::stringstream derp;
        derp << type;
        JsonIn jsin(derp);
        try {
            deserialize(jsin);
        } catch (std::string jsonerr) {
            debugmsg("Bad vehicle json\n%s", jsonerr.c_str() );
        }
    } else {
        load_legacy(stin);
    }
}

/** Checks all parts to see if frames are missing (as they might be when
 * loading from a game saved before the vehicle construction rules overhaul). */
void vehicle::add_missing_frames()
{
    //No need to check the same (x, y) spot more than once
    std::set< std::pair<int, int> > locations_checked;
    for (int i = 0; i < parts.size(); i++) {
        int next_x = parts[i].mount_dx;
        int next_y = parts[i].mount_dy;
        std::pair<int, int> mount_location = std::make_pair(next_x, next_y);

        if(locations_checked.count(mount_location) == 0) {
            std::vector<int> parts_here = parts_at_relative(next_x, next_y, false);
            bool found = false;
            for(std::vector<int>::iterator here = parts_here.begin();
                    here != parts_here.end(); here++) {
                if(part_info(*here).location == part_location_structure) {
                    found = true;
                    break;
                }
            }
            if(!found) {
                //No frame here! Install one.
                vehicle_part new_part;
                new_part.setid("frame_vertical");
                new_part.mount_dx = next_x;
                new_part.mount_dy = next_y;
                new_part.hp = vehicle_part_types["frame_vertical"].durability;
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

    std::map<std::string, int> consistent_bignesses;

    // veh_fuel_multiplier is percentage of fuel
    // 0 is empty, 100 is full tank, -1 is random 1% to 7%
    int veh_fuel_mult = init_veh_fuel;
    if (init_veh_fuel == - 1) {
        veh_fuel_mult = rng (1,7);
    }
    if (init_veh_fuel > 100) {
        veh_fuel_mult = 100;
    }

    // im assuming vehicles only spawn in active maps
    levx = g->levx;
    levy = g->levy;

    // veh_status is initial vehicle damage
    // -1 = light damage (DEFAULT)
    //  0 = undamaged
    //  1 = disabled, destroyed tires OR engine
    int veh_status = -1;
    if (init_veh_status == 0) {
        veh_status = 0;
    }
    if (init_veh_status == 1) {
     veh_status = 1;
     if (one_in(5)) {           //  seats are destroyed 20%
      destroySeats = true;
     } else if (one_in(5)) {    // controls are destroyed 16%
      destroyControls = true;
     } else if (one_in(5)) {    // battery, minireactor or gasoline tank are destroyed 13%
      destroyTank = true;
     } else if (one_in(5)) {   // engine are destroyed 10%
      destroyEngine = true;
     } else {                   // tires are destroyed 41%
      destroyTires = true;
     }
    }

    //Provide some variety to non-mint vehicles
    if(veh_status != 0) {

        //Leave engine running in some vehicles, if the engine has not been destroyed
        if(veh_fuel_mult > 0
                && all_parts_with_feature("ENGINE", true).size() > 0
                && one_in(8)
                && !destroyEngine) {
            engine_on = true;
        }

        //Turn on lights on some vehicles
        if(one_in(20)) {
            lights_on = true;
        }

        //Turn flasher/overhead lights on separately (more likely since these are rarer)
        if(one_in(4)) {
            overhead_lights_on = true;
        }

        if(one_in(10)) {
            blood_covered = true;
        }

        if(one_in(8)) {
            blood_inside = true;
        }

        //Fridge should always start out activated if present
        if(all_parts_with_feature("FRIDGE").size() > 0) {
            fridge_on = true;
        }
    }

    // Reactor should always start out activated if present
    std::vector<int> fuel_tanks = all_parts_with_feature(VPFLAG_FUEL_TANK);
    for(int p = 0; p < fuel_tanks.size(); p++) {
        if(part_info(fuel_tanks[p]).fuel_type == fuel_type_plutonium) {
            reactor_on = true;
            break;
        }
    }

    bool blood_inside_set = false;
    int blood_inside_x = 0;
    int blood_inside_y = 0;
    for (int p = 0; p < parts.size(); p++)
    {
        if (part_flag(p, "VARIABLE_SIZE")){ // generate its bigness attribute.?
            if(consistent_bignesses.count(parts[p].id) < 1){
                //generate an item for this type, & cache its bigness
                item tmp (itypes[part_info(p).item], 0);
                consistent_bignesses[parts[p].id] = tmp.bigness;
            }
            parts[p].bigness = consistent_bignesses[parts[p].id];
        }
        if (part_flag(p, "FUEL_TANK")) {   // set fuel status
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
            parts[p].hp= part_info(p).durability;
        } else {
         //a bit of initial damage :)
         //clamp 4d8 to the range of [8,20]. 8=broken, 20=undamaged.
         int broken = 8, unhurt = 20;
         int roll = dice(4,8);
         if(roll < unhurt){
            if (roll <= broken)
               parts[p].hp= 0;
            else
               parts[p].hp= ((float)(roll-broken) / (unhurt-broken)) * part_info(p).durability;
            }
         else // new.
         {
            parts[p].hp= part_info(p).durability;
         }

         if (destroySeats) { // vehicle is disabled because no seats
          if (part_flag(p, "SEAT")) {
           parts[p].hp= 0;
          }
          if (part_flag(p, "SEATBELT")) {
           parts[p].hp= 0;
          }
         }
         if (destroyControls) { // vehicle is disabled because no controls
          if (part_flag(p, "CONTROLS")) {
           parts[p].hp= 0;
          }
         }
         if (destroyTank) { // vehicle is disabled because no battery, minireactor or gasoline tank
          if (part_flag(p, "FUEL_TANK")) {
           parts[p].hp= 0;
           parts[p].amount = part_info(p).size * 0;
          }
         }
         if (destroyEngine) { // vehicle is disabled because engine is dead
          if (part_flag(p, "ENGINE")) {
           parts[p].hp= 0;
          }
         }
         if (destroyTires) { // vehicle is disabled because flat tires
          if (part_flag(p, "WHEEL")) {
             parts[p].hp= 0;
          }
         }
         if (part_flag(p, "SOLAR_PANEL") && one_in(4)) {//Solar panels have a 1 in four chance of being broken.
            parts[p].hp= 0;
         }

         /* Bloodsplatter the front-end parts. Assume anything with x > 0 is
          * the "front" of the vehicle (since the driver's seat is at (0, 0).
          * We'll be generous with the blood, since some may disappear before
          * the player gets a chance to see the vehicle. */
         if(blood_covered && parts[p].mount_dx > 0) {
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
                int distSq = pow((blood_inside_x - parts[p].mount_dx), 2) + \
                             pow((blood_inside_y - parts[p].mount_dy), 2);
                if (distSq <= 1) {
                    parts[p].blood = rng(200, 400) - distSq*100;
                }
            } else if (part_flag(p, "SEAT")) {
                // Set the center of the bloody mess inside
                blood_inside_x = parts[p].mount_dx;
                blood_inside_y = parts[p].mount_dy;
                blood_inside_set = true;
            }
         }

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
    for (int part_index = 0; part_index < parts.size(); part_index++) {
        //Skip any parts already mashed up
        if(parts[part_index].hp == 0) {
            continue;
        }

        vehicle_part next_part = parts[part_index];
        std::vector<int> parts_in_square = parts_at_relative(next_part.mount_dx, next_part.mount_dy);
        int structures_found = 0;
        for (int square_part_index = 0; square_part_index < parts_in_square.size(); square_part_index++) {
            if (part_info(parts_in_square[square_part_index]).location == part_location_structure) {
                structures_found++;
            }
        }

        if(structures_found > 1) {
            //Destroy everything in the square
            for (int square_part_index = 0; square_part_index < parts_in_square.size(); square_part_index++) {
                parts[parts_in_square[square_part_index]].hp = 0;
            }
            continue;
        }

        //Everywhere else, drop by 10-120% of max HP (anything over 100 = broken)
        int damage = (int) (dice(1, 12) * 0.1 * part_info(part_index).durability);
        parts[part_index].hp -= damage;
        if (parts[part_index].hp < 0) {
            parts[part_index].hp = 0;
        }
    }
}

void vehicle::use_controls()
{
    std::vector<vehicle_controls> options_choice;
    std::vector<uimenu_entry> options_message;
    int vpart;
    // Always have this option


    // Let go without turning the engine off.
    if (g->u.controlling_vehicle &&
        g->m.veh_at(g->u.posx, g->u.posy, vpart) == this) {
        options_choice.push_back(release_control);
        options_message.push_back(uimenu_entry(_("Let go of controls"), 'l'));
    }


    bool has_lights = false;
    bool has_overhead_lights = false;
    bool has_horn = false;
    bool has_turrets = false;
    bool has_tracker = false;
    bool has_reactor = false;
    bool has_engine = false;
    bool has_fridge = false;
    bool has_recharger = false;
    for (int p = 0; p < parts.size(); p++) {
        if (part_flag(p, "CONE_LIGHT")) {
            has_lights = true;
        }
        if (part_flag(p, "CIRCLE_LIGHT")) {
            has_overhead_lights = true;
        }
        if (part_flag(p, "LIGHT")) {
            has_lights = true;
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
        else if (part_flag(p, VPFLAG_FUEL_TANK) &&
                 part_info(p).fuel_type == fuel_type_plutonium) {
            has_reactor = true;
        }
        else if (part_flag(p, "ENGINE")) {
            has_engine = true;
        }
        else if (part_flag(p, "FRIDGE")) {
            has_fridge = true;
        }
        else if (part_flag(p, "RECHARGE")) {
            has_recharger = true;
        }
    }

    // Toggle engine on/off, stop driving if we are driving.
    if (!has_pedals && has_engine) {
        options_choice.push_back(toggle_engine);
        if (g->u.controlling_vehicle) {
            options_message.push_back(uimenu_entry(_("Stop driving."), 's'));
        } else {
            options_message.push_back(uimenu_entry((engine_on) ? _("Turn off the engine") :
                                                   _("Turn on the engine"), 'e'));
        }
    }

    options_choice.push_back(toggle_cruise_control);
    options_message.push_back(uimenu_entry((cruise_on) ? _("Disable cruise control") :
                                           _("Enable cruise control"), 'c'));


    // Lights if they are there - Note you can turn them on even when damaged, they just don't work
    if (has_lights) {
        options_choice.push_back(toggle_lights);
        options_message.push_back(uimenu_entry((lights_on) ? _("Turn off headlights") :
                                               _("Turn on headlights"), 'h'));
    }

   if (has_overhead_lights) {
       options_choice.push_back(toggle_overhead_lights);
       options_message.push_back(uimenu_entry(overhead_lights_on ? _("Turn off overhead lights") :
                                              _("Turn on overhead lights"), 'v'));
   }

    //Honk the horn!
    if (has_horn) {
        options_choice.push_back(activate_horn);
        options_message.push_back(uimenu_entry(_("Honk horn"), 'o'));
    }

    // Turrets: off or burst mode
    if (has_turrets) {
        options_choice.push_back(toggle_turrets);
        options_message.push_back(uimenu_entry((0 == turret_mode) ? _("Switch turrets to burst mode") :
                                               _("Disable turrets"), 't'));
    }

    // Turn the fridge on/off
    if (has_fridge) {
        options_choice.push_back(toggle_fridge);
        options_message.push_back(uimenu_entry(fridge_on ? _("Turn off fridge") :
                                               _("Turn on fridge"), 'f'));
    }

    // Turn the recharging station on/off
    if (has_recharger) {
        options_choice.push_back(toggle_recharger);
        options_message.push_back(uimenu_entry(recharger_on ? _("Turn off recharger") :
                                               _("Turn on recharger"), 'r'));
    }

    // Tracking on the overmap
    if (has_tracker) {
        options_choice.push_back(toggle_tracker);
        options_message.push_back(uimenu_entry((tracking_on) ? _("Disable tracking device") :
                                                _("Enable tracking device"), 'g'));

    }

    if( tags.count("convertible") ) {
        options_choice.push_back(convert_vehicle);
        options_message.push_back(uimenu_entry(_("Fold bicycle"), 'f'));
    }

    // Turn the reactor on/off
    if (has_reactor) {
        options_choice.push_back(toggle_reactor);
        options_message.push_back(uimenu_entry(reactor_on ? _("Turn off reactor") :
                                               _("Turn on reactor"), 'm'));
    }

    options_choice.push_back(control_cancel);
    options_message.push_back(uimenu_entry(_("Do nothing"), ' '));

    uimenu selectmenu;
    selectmenu.return_invalid = true;
    selectmenu.text = _("Vehicle controls");
    selectmenu.entries = options_message;
    selectmenu.query();
    int select = selectmenu.ret;

    if (select < 0) {
        return;
    }

    switch(options_choice[select]) {
    case toggle_cruise_control:
        cruise_on = !cruise_on;
        g->add_msg((cruise_on) ? _("Cruise control turned on") : _("Cruise control turned off"));
        break;
    case toggle_lights:
        if(lights_on || fuel_left(fuel_type_battery) ) {
            lights_on = !lights_on;
            g->add_msg((lights_on) ? _("Headlights turned on") : _("Headlights turned off"));
        } else {
            g->add_msg(_("The headlights won't come on!"));
        }
        break;
    case toggle_overhead_lights:
        if( !overhead_lights_on || fuel_left(fuel_type_battery) ) {
            overhead_lights_on = !overhead_lights_on;
            g->add_msg((overhead_lights_on) ? _("Overhead lights turned on") :
                       _("Overhead lights turned off"));
        } else {
            g->add_msg(_("The lights won't come on!"));
        }
        break;
    case activate_horn:
        g->add_msg(_("You honk the horn!"));
        honk_horn();
        break;
    case toggle_turrets:
        cycle_turret_mode();
        break;
    case toggle_fridge:
        if( !fridge_on || fuel_left(fuel_type_battery) ) {
            fridge_on = !fridge_on;
            g->add_msg((fridge_on) ? _("Fridge turned on") :
                       _("Fridge turned off"));
        } else {
            g->add_msg(_("The fridge won't turn on!"));
        }
        break;
    case toggle_recharger:
        if( !recharger_on || fuel_left(fuel_type_battery) ) {
            recharger_on = !recharger_on;
            g->add_msg((recharger_on) ? _("Recharger turned on") :
                       _("Recharger turned off"));
        } else {
            g->add_msg(_("The recharger won't turn on!"));
        }
        break;
    case toggle_reactor:
        if(!reactor_on || fuel_left(fuel_type_plutonium)) {
            reactor_on = !reactor_on;
            g->add_msg((reactor_on) ? _("Reactor turned on") :
                       _("Reactor turned off"));
        }
        else {
            g->add_msg(_("The reactor won't turn on!"));
        }
        break;
    case toggle_engine:
        if (g->u.controlling_vehicle) {
            if (engine_on) {
                engine_on = false;
                g->add_msg(_("You turn the engine off and let go of the controls."));
            } else {
                g->add_msg(_("You let go of the controls."));
            }
            g->u.controlling_vehicle = false;
            break;
        } else if (engine_on) {
            engine_on = false;
            g->add_msg(_("You turn the engine off."));
        } else {
          int pwr, max_pwr;
          pwr = engine_power( &max_pwr );
          if (pwr < 1) {
              if (max_pwr < 1) {
                  g->add_msg (_("The %s doesn't have an working engine!"), name.c_str());
              } else if(has_pedals) {
                  g->add_msg (_("The %s's pedals are out of reach!"), name.c_str());
              } else {
                  g->add_msg (_("The %s's engine emits a sneezing sound."), name.c_str());
              }
          }
          else {
              start_engine();
          }
        }
        break;
    case release_control:
        g->u.controlling_vehicle = false;
        g->add_msg(_("You let go of the controls."));
        break;
    case convert_vehicle:
    {
        if(g->u.controlling_vehicle) {
            g->add_msg(_("As the pitiless metal bars close on your nether regions, you reconsider trying to fold the bicycle while riding it."));
            break;
        }
        g->add_msg(_("You painstakingly pack the bicycle into a portable configuration."));
        // create a folding bicycle item
        item bicycle;
        bicycle.make( itypes["folding_bicycle"] );

        // Drop stuff in containers on ground
        for (int p = 0; p < parts.size(); p++) {
            if( part_flag( p, "CARGO" ) ) {
                for( std::vector<item>::iterator it = parts[p].items.begin();
                     it != parts[p].items.end(); ++it ) {
                    g->m.add_item_or_charges( g->u.posx, g->u.posy, *it );
                }
                parts[p].items.clear();
            }
        }

        // Store data of all parts, iuse::unfold_bicyle only loads
        // some of them (like bigness), some are expect to be
        // vehicle specific and therefor constant (like id, mount_dx).
        // Writing everything here is easier to manage, as only
        // iuse::unfold_bicyle has to adopt to changes.
        try {
            std::ostringstream veh_data;
            JsonOut json(veh_data);
            json.write(parts);
            bicycle.item_vars["folding_bicycle_parts"] = veh_data.str();
        } catch(std::string e) {
            debugmsg("Error storing vehicle: %s", e.c_str());
        }

        g->m.add_item_or_charges(g->u.posx, g->u.posy, bicycle);
        // Remove vehicle
        unboard_all();
        g->m.destroy_vehicle(this);

        g->u.moves -= 500;
        break;
    }
    case toggle_tracker:
        if (tracking_on)
        {
            g->cur_om->remove_vehicle(om_id);
            tracking_on = false;
            g->add_msg(_("tracking device disabled"));
        } else if (fuel_left(fuel_type_battery))
        {
            om_id = g->cur_om->add_vehicle(this);
            tracking_on = true;
            g->add_msg(_("tracking device enabled"));
        } else {
            g->add_msg(_("tracking device won't turn on"));
        }
        break;
    case control_cancel:
        break;
    }
}

void vehicle::start_engine()
{
    bool muscle_powered = false;
    // TODO: Make chance of success based on engine condition.
    for(int p = 0; p < engines.size(); p++) {
        if(parts[engines[p]].hp > 0) {
            if(part_info(engines[p]).fuel_type == fuel_type_gasoline) {
                int engine_power = part_power(engines[p]);
                if(engine_power < 50) {
                    // Small engines can be pull-started
                    engine_on = true;
                } else {
                    // Starter motor battery draw proportional to engine power
                    if(!discharge_battery(engine_power / 10)) {
                        engine_on = true;
                    }
                }
            } else if (part_info(engines[p]).fuel_type == fuel_type_muscle) {
                muscle_powered = true;
            } else {
                // Electric & plasma engines
                engine_on = true;
            }
        }
    }

    if(engine_on == true) {
        g->add_msg(_("The %s's engine starts up."), name.c_str());
    } else if (!muscle_powered) {
        g->add_msg (_("The %s's engine fails to start."), name.c_str());
    }
}

void vehicle::honk_horn ()
{
    for (int p = 0; p < parts.size(); p++) {
        if (part_flag( p, "HORN" )) {
            //Get global position of horn
            const int horn_x = global_x() + parts[p].precalc_dx[0];
            const int horn_y = global_y() + parts[p].precalc_dy[0];
            //Determine sound
            vpart_info &horn_type = part_info(p);
            if (horn_type.bonus >= 40) {
                g->sound( horn_x, horn_y, horn_type.bonus, _("HOOOOORNK!") );
            } else if (horn_type.bonus >= 20) {
                g->sound( horn_x, horn_y, horn_type.bonus, _("BEEEP!") );
            } else {
                g->sound( horn_x, horn_y, horn_type.bonus, _("honk.") );
            }
        }
    }
}

vpart_info& vehicle::part_info (int index)
{
    if (index >= 0 && index < parts.size()) {
        return vehicle_part_int_types[parts[index].iid];
        // slow autovivication // vehicle_part_types[parts[index].id];
    } else {
        return vehicle_part_int_types[0];//"null"];
    }
}

// engines & alternators all have power.
// engines provide, whilst alternators consume.
int vehicle::part_power( int index, bool at_full_hp ) {
    if (!part_flag(index, VPFLAG_ENGINE) &&
        !part_flag(index, VPFLAG_ALTERNATOR)) {
       return 0; //not an engine.
    }
    int p;
    if (part_flag (index, VPFLAG_VARIABLE_SIZE)) { // example: 2.42-L V-twin engine
       p = parts[index].bigness;
    } else { // example: foot crank
       p = part_info(index).power;
    }
    if (p < 0)
        return p; // Consumers always draw full power, even if broken
    if (at_full_hp)
        return p; // Assume full hp
    return (p * parts[index].hp / part_info(index).durability);
}

// alternators, solar panels, reactors, and accessories all have epower.
// alternators, solar panels, and reactors provide, whilst accessories consume.
int vehicle::part_epower( int index ) {
    int e = part_info(index).epower;
    if (e < 0)
        return e; // Consumers always draw full power, even if broken
    return (e * parts[index].hp / part_info(index).durability);
}

int vehicle::epower_to_power (int epower) {
    // Convert epower units (watts) to power units
    // Used primarily for calculating battery charge/discharge
    // TODO: convert batteries to use energy units based on watts (watt-ticks?)
    const int conversion_factor = 373; // 373 epower == 373 watts == 1 power == 0.5 HP
    int power = epower / conversion_factor;
    // epower remainder results in chance at additional charge/discharge
    if (x_in_y(abs(epower % conversion_factor), conversion_factor)) {
        power += epower >= 0 ? 1 : -1;
    }
    return power;
}

int vehicle::power_to_epower (int power) {
    // Convert power units to epower units (watts)
    // Used primarily for calculating battery charge/discharge
    // TODO: convert batteries to use energy units based on watts (watt-ticks?)
    // Some car values: http://en.wikipedia.org/wiki/Power-to-weight_ratio#Vehicles:tabnext
    const int conversion_factor = 373; // 373 epower == 373 watts == 1 power == 0.5 HP
    return power * conversion_factor;
}

bool vehicle::has_structural_part(int dx, int dy)
{
    std::vector<int> parts_here = parts_at_relative(dx, dy, false);
    for( std::vector<int>::iterator it = parts_here.begin(); it != parts_here.end(); ++it ) {
        if(part_info(*it).location == part_location_structure && !part_info(*it).has_flag("PROTRUSION")) {
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
bool vehicle::can_mount (int dx, int dy, std::string id)
{
    //The part has to actually exist.
    if(vehicle_part_types.count(id) == 0) {
        return false;
    }

    //It also has to be a real part, not the null part
    const vpart_info part = vehicle_part_types[id];
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
    for (std::vector<int>::const_iterator part_it = parts_in_square.begin();
         part_it != parts_in_square.end(); ++part_it) {
        const vpart_info other_part = vehicle_part_types[parts[*part_it].id];

        //Parts with no location can stack with each other (but not themselves)
        if(part.id == other_part.id ||
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
    if(parts.size() > 0) {
        if(!has_structural_part(dx, dy) &&
                !has_structural_part(dx+1, dy) &&
                !has_structural_part(dx, dy+1) &&
                !has_structural_part(dx-1, dy) &&
                !has_structural_part(dx, dy-1)) {
            return false;
        }
    }

    // Alternators must be installed on a gas engine
    if(vehicle_part_types[id].has_flag(VPFLAG_ALTERNATOR)) {
        bool anchor_found = false;
        for (std::vector<int>::const_iterator it = parts_in_square.begin();
             it != parts_in_square.end(); ++it) {
            if(part_info(*it).has_flag(VPFLAG_ENGINE) &&
               part_info(*it).fuel_type == fuel_type_gasoline) {
                anchor_found = true;
            }
        }
        if(!anchor_found) {
            return false;
        }
    }

    //Seatbelts must be installed on a seat
    if(vehicle_part_types[id].has_flag("SEATBELT")) {
        bool anchor_found = false;
        for (std::vector<int>::const_iterator it = parts_in_square.begin();
             it != parts_in_square.end(); ++it) {
            if(part_info(*it).has_flag("BELTABLE")) {
                anchor_found = true;
            }
        }
        if(!anchor_found) {
            return false;
        }
    }

    //Internal must be installed into a cargo area.
    if(vehicle_part_types[id].has_flag("INTERNAL")) {
        bool anchor_found = false;
        for (std::vector<int>::const_iterator it = parts_in_square.begin();
             it != parts_in_square.end(); ++it) {
            if(part_info(*it).has_flag("CARGO")) {
                anchor_found = true;
            }
        }
        if(!anchor_found) {
            return false;
        }
    }

    // curtains must be installed on (reinforced)windshields
    // TODO: do this automatically using "location":"on_mountpoint"
    if (vehicle_part_types[id].has_flag("CURTAIN")) {
        bool anchor_found = false;
        for (std::vector<int>::const_iterator it = parts_in_square.begin();
             it != parts_in_square.end(); it++) {
            if (part_info(*it).has_flag("WINDOW")) {
                anchor_found = true;
            }
        }
        if (!anchor_found) {
            return false;
        }
    }

    //Anything not explicitly denied is permitted
    return true;
}

bool vehicle::can_unmount (int p)
{
    if(p < 0 || p > parts.size()) {
        return false;
    }

    int dx = parts[p].mount_dx;
    int dy = parts[p].mount_dy;

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
    if(part_flag(p, "WINDOW") && part_with_feature(p, "CURTAIN") >= 0) {
        return false;
    }

    //Structural parts have extra requirements
    if(part_info(p).location == part_location_structure) {

        /* To remove a structural part, there can be only structural parts left
         * in that square (might be more than one in the case of wreckage) */
        for(int part_index = 0; part_index < parts_in_square.size(); part_index++) {
            if(part_info(parts_in_square[part_index]).location != part_location_structure) {
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
                if(parts_over_there.size() > 0) {
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
                for(int next_part = 1; next_part < connected_parts.size(); next_part++) {
                    if(!is_connected(connected_parts[0], connected_parts[next_part], parts[p])) {
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
bool vehicle::is_connected(vehicle_part &to, vehicle_part &from, vehicle_part &excluded)
{
    int target_x = to.mount_dx;
    int target_y = to.mount_dy;

    int excluded_x = excluded.mount_dx;
    int excluded_y = excluded.mount_dy;

    //Breadth-first-search components
    std::list<vehicle_part> discovered;
    vehicle_part current_part;
    std::list<vehicle_part> searched;

    //We begin with just the start point
    discovered.push_back(from);

    while(!discovered.empty()) {
        current_part = discovered.front();
        discovered.pop_front();
        int current_x = current_part.mount_dx;
        int current_y = current_part.mount_dy;

        for(int i = 0; i < 4; i++) {
            int next_x = current_x + (i < 2 ? (i == 0 ? -1 : 1) : 0);
            int next_y = current_y + (i < 2 ? 0 : (i == 2 ? -1 : 1));

            if(next_x == target_x && next_y == target_y) {
                //Success!
                return true;
            } else if(next_x == excluded_x && next_y == excluded_y) {
                //There might be a path, but we're not allowed to go that way
                continue;
            }

            std::vector<int> parts_there = parts_at_relative(next_x, next_y);

            if(!parts_there.empty()) {
                vehicle_part next_part = parts[parts_there[0]];
                //Only add the part if we haven't been here before
                bool found = false;
                for(std::list<vehicle_part>::iterator it = discovered.begin();
                        it != discovered.end(); it++) {
                    if(it->mount_dx == next_x && it->mount_dy == next_y) {
                        found = true;
                        break;
                    }
                }
                if(!found) {
                    for(std::list<vehicle_part>::iterator it = searched.begin();
                        it != searched.end(); it++) {
                        if(it->mount_dx == next_x && it->mount_dy == next_y) {
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

bool vehicle::change_headlight_direction(int p)
{
    // Need map-relative coordinates to compare to output of look_around.
    int gx = global_x() + parts[p].precalc_dx[0];
    int gy = global_y() + parts[p].precalc_dy[0];
    // Stash offset and set it to the location of the part so look_around will start there.
    int px = g->u.view_offset_x;
    int py = g->u.view_offset_y;
    g->u.view_offset_x = gx - g->u.posx;
    g->u.view_offset_y = gy - g->u.posy;
    popup(_("Choose a facing direction for the new headlight."));
    point headlight_target = g->look_around();
    // Restore previous view offsets.
    g->u.view_offset_x = px;
    g->u.view_offset_y = py;

    int delta_x = headlight_target.x - gx;
    int delta_y = headlight_target.y - gy;

    const double PI = 3.14159265358979f;
    int dir = int(atan2(static_cast<float>(delta_y), static_cast<float>(delta_x)) * 180.0 / PI);
    dir -= face.dir();
    while(dir < 0)
        dir += 360;
    while(dir > 360)
        dir -= 360;

    parts[p].direction = dir;
    return true; // TODO: Allow user to cancel, will then return false
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
int vehicle::install_part (int dx, int dy, std::string id, int hp, bool force)
{
    if (!force && !can_mount (dx, dy, id)) {
        return -1;  // no money -- no ski!
    }
    vehicle_part new_part;
    new_part.setid(id);
    new_part.mount_dx = dx;
    new_part.mount_dy = dy;
    new_part.hp = hp < 0 ? vehicle_part_types[id].durability : hp;
    new_part.amount = 0;
    new_part.blood = 0;
    item tmp(itypes[vehicle_part_types[id].item], 0);
    new_part.bigness = tmp.bigness;
    parts.push_back (new_part);

    refresh();
    return parts.size() - 1;
}

// share damage & bigness betwixt veh_parts & items.
void vehicle::get_part_properties_from_item(int partnum, item& i)
{
    //transfer bigness if relevant.
    itype_id  pitmid = part_info(partnum).item;
    itype* itemtype = itypes[pitmid];
    if(itemtype->is_var_veh_part())
       parts[partnum].bigness = i.bigness;

    // item damage is 0,1,2,3, or 4. part hp is 1..durability.
    // assuming it rusts. other item materials disentigrate at different rates...
    int health = 5 - i.damage;
    health *= part_info(partnum).durability; //[0,dur]
    health /= 5;
    parts[partnum].hp = health;
}

void vehicle::give_part_properties_to_item(int partnum, item& i)
{
    //transfer bigness if relevant.
    itype_id  pitmid = part_info(partnum).item;
    itype* itemtype = itypes[pitmid];
    if(itemtype->is_var_veh_part())
       i.bigness = parts[partnum].bigness;

    // remove charges if part is made of a tool
    if(itemtype->is_tool())
        i.charges = 0;

    // translate part damage to item damage.
    // max damage is 4, min damage 0.
    // this is very lossy.
    int dam;
    float hpofdur = (float)parts[partnum].hp / part_info(partnum).durability;
    dam = (1 - hpofdur) * 5;
    if (dam > 4) dam = 4;
    if (dam < 0) dam = 0;
    i.damage = dam;
}

void vehicle::remove_part (int p)
{
    if (p >= parts.size()) {
        debugmsg("Tried to remove part %d but only %d parts!", p, parts.size());
        return;
    }
    if (part_flag(p, "TRACK")) {
        // disable tracking if there are no other trackers installed.
        if (tracking_on)
        {
            bool has_tracker = false;
            for (int i = 0; i != parts.size(); i++){
                if (i != p && part_flag(i, "TRACK")){
                    has_tracker = true;
                    break;
                }
            }
            if (!has_tracker){ // disable tracking
                g->cur_om->remove_vehicle(om_id);
                tracking_on = false;
            }
        }
    }

    // if a windshield is removed (usually destroyed) also remove curtains
    // attached to it.
    if(part_flag(p, "WINDOW")) {
        int curtain = part_with_feature(p, "CURTAIN", false);
        if (curtain >= 0) {
            int x = parts[curtain].precalc_dx[0], y = parts[curtain].precalc_dy[0];
            item it = item_from_part(curtain);
            g->m.add_item_or_charges(global_x() + x, global_y() + y, it, 2);
            remove_part(curtain);
        }
    }

    //Ditto for seatbelts
    if(part_flag(p, "SEAT")) {
        int seatbelt = part_with_feature(p, "SEATBELT", false);
        if (seatbelt >= 0) {
            int x = parts[seatbelt].precalc_dx[0], y = parts[seatbelt].precalc_dy[0];
            item it = item_from_part(seatbelt);
            g->m.add_item_or_charges(global_x() + x, global_y() + y, it, 2);
            remove_part(seatbelt);
        }
    }

    //If we remove the (0, 0) frame, we need to shift things around
    if(part_info(p).location == "structure" && parts[p].mount_dx == 0 && parts[p].mount_dy == 0) {
        //Find a frame, any frame, to shift to
        for(int next_part = 0; next_part < parts.size(); next_part++) {
            if(next_part != p
                    && part_info(next_part).location == "structure"
                    && !part_info(next_part).has_flag("PROTRUSION")) {
                shift_parts(parts[next_part].mount_dx, parts[next_part].mount_dy);
                break;
            }
        }
    }

    const int dx = global_x() + parts[p].precalc_dx[0];
    const int dy = global_y() + parts[p].precalc_dy[0];
    for (int i = 0; i < parts[p].items.size(); i++) {
        g->m.add_item_or_charges(dx + rng(-3, +3), dy + rng(-3, +3), parts[p].items[i]);
    }
    parts.erase(parts.begin() + p);
    refresh();

    if(parts.size() == 0) {
        g->m.destroy_vehicle(this);
    } else {
        g->m.update_vehicle_cache(this, false);
    }
}

/**
 * Breaks the specified part into the pieces defined by its breaks_into entry.
 * @param p The index of the part to break.
 * @param x The map x-coordinate to place pieces at (give or take).
 * @param y The map y-coordinate to place pieces at (give or take).
 * @param scatter If true, pieces are scattered near the target square.
 */
void vehicle::break_part_into_pieces(int p, int x, int y, bool scatter) {
    std::vector<break_entry> break_info = part_info(p).breaks_into;
    for(int index = 0; index < break_info.size(); index++) {
        int quantity = rng(break_info[index].min, break_info[index].max);
        for(int num = 0; num < quantity; num++) {
            const int actual_x = scatter ? x + rng(-SCATTER_DISTANCE, SCATTER_DISTANCE) : x;
            const int actual_y = scatter ? y + rng(-SCATTER_DISTANCE, SCATTER_DISTANCE) : y;
            item piece(itypes[break_info[index].item_id], g->turn);
            g->m.add_item_or_charges(actual_x, actual_y, piece);
        }
    }
}

item vehicle::item_from_part( int part )
{
    itype_id itm = part_info(part).item;
    int bigness = parts[part].bigness;
    itype* parttype = itypes[itm];
    item tmp(parttype, g->turn);

    //transfer damage, etc.
    give_part_properties_to_item(part, tmp);
    if( parttype->is_var_veh_part() ) {
        tmp.bigness = bigness;
    }
    return tmp;
}

const std::vector<int> vehicle::parts_at_relative (const int dx, const int dy, bool use_cache)
{
    if ( use_cache == false ) {
        std::vector<int> res;
        for (int i = 0; i < parts.size(); i++) {
            if (parts[i].mount_dx == dx && parts[i].mount_dy == dy) {
                res.push_back (i);
            }
        }
        return res;
    } else {
        if ( relative_parts.find( point(dx, dy) ) != relative_parts.end() ) {
            return relative_parts[ point(dx, dy) ];
        } else {
            std::vector<int> res;
            return res;
        }
    }
}

int vehicle::part_with_feature (int part, const vpart_bitflags &flag, bool unbroken) {
    if (part_flag(part, flag)) {
        return part;
    }
    std::map<point, std::vector<int> >::const_iterator it = relative_parts.find( point( parts[part].mount_dx, parts[part].mount_dy ) );
    if ( it != relative_parts.end() ) {
        const std::vector<int> & parts_here = it->second;
        for (int i = 0; i < parts_here.size(); i++) {
            if (part_flag(parts_here[i], flag) && (!unbroken || parts[parts_here[i]].hp > 0)) {
                return parts_here[i];
            }
        }
    }
    return -1;
}

int vehicle::part_with_feature (int part, const std::string &flag, bool unbroken)
{
    std::vector<int> parts_here = parts_at_relative(parts[part].mount_dx, parts[part].mount_dy);
    for( std::vector<int>::iterator part_it = parts_here.begin();
         part_it != parts_here.end(); ++part_it ) {
        if (part_flag(*part_it, flag) && (!unbroken || parts[*part_it].hp > 0)) {
            return *part_it;
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
std::vector<int> vehicle::all_parts_with_feature(const std::string& feature, bool unbroken)
{
    std::vector<int> parts_found;
    for(int part_index = 0; part_index < parts.size(); part_index++) {
        if(part_info(part_index).has_flag(feature) &&
                (!unbroken || parts[part_index].hp > 0)) {
            parts_found.push_back(part_index);
        }
    }
    return parts_found;
}

std::vector<int> vehicle::all_parts_with_feature(const vpart_bitflags & feature, bool unbroken)
{
    std::vector<int> parts_found;
    for(int part_index = 0; part_index < parts.size(); part_index++) {
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
std::vector<int> vehicle::all_parts_at_location(const std::string& location)
{
    std::vector<int> parts_found;
    for(int part_index = 0; part_index < parts.size(); part_index++) {
        if(part_info(part_index).location == location) {
            parts_found.push_back(part_index);
        }
    }
    return parts_found;
}

bool vehicle::part_flag( int part, const std::string &flag ) {
    if (part < 0 || part >= parts.size()) {
        return false;
    } else {
        return part_info(part).has_flag(flag);
    }
}

bool vehicle::part_flag( int part, const vpart_bitflags &flag) {
   if (part < 0 || part >= parts.size()) {
        return false;
    } else {
        return part_info(part).has_flag(flag);
    }
}

int vehicle::part_at(int dx, int dy)
{
    for (int p = 0; p < parts.size(); p++) {
        if (parts[p].precalc_dx[0] == dx && parts[p].precalc_dy[0] == dy) {
            return p;
        }
    }
    return -1;
}

int vehicle::global_part_at(int x, int y)
{
 int dx = x - global_x();
 int dy = y - global_y();
 return part_at(dx,dy);
}

/**
 * Given a vehicle part which is inside of this vehicle, returns the index of
 * that part. This exists solely because activities relating to vehicle editing
 * require the index of the vehicle part to be passed around.
 * @param part The part to find.
 * @return The part index, -1 if it is not part of this vehicle.
 */
int vehicle::index_of_part(vehicle_part *part)
{
  if(part != NULL) {
    for(int index = 0; index < parts.size(); index++) {
      vehicle_part next_part = parts[index];
      if(part->id == next_part.id &&
              part->mount_dx == next_part.mount_dx &&
              part->mount_dy == next_part.mount_dy &&
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
int vehicle::part_displayed_at(int local_x, int local_y)
{
    std::vector<int> parts_in_square = parts_at_relative(local_x, local_y);

    if(parts_in_square.empty()) {
        return -1;
    }

    int top_part = 0;
    for(int index = 1; index < parts_in_square.size(); index++) {
        if(part_info(parts_in_square[top_part]).z_order <
                part_info(parts_in_square[index]).z_order) {
            top_part = index;
        }
    }

    return parts_in_square[top_part];
}

char vehicle::part_sym( int p ) {
    if (p < 0 || p >= parts.size()) {
        return ' ';
    }

    int displayed_part = part_displayed_at(parts[p].mount_dx, parts[p].mount_dy);

    if (part_flag (displayed_part, VPFLAG_OPENABLE) && parts[displayed_part].open) {
        return '\''; // open door
    } else {
        return parts[displayed_part].hp <= 0 ?
            part_info(displayed_part).sym_broken : part_info(displayed_part).sym;
    }
}

// similar to part_sym(int p) but for use when drawing SDL tiles. Called only by cata_tiles during draw_vpart
// vector returns at least 1 element, max of 2 elements. If 2 elements the second denotes if it is open or damaged
std::string vehicle::part_id_string(int p, char &part_mod)
{
    part_mod = 0;
    std::string idinfo;
    if (p < 0 || p >= parts.size()){
        return "";
    }

    int displayed_part = part_displayed_at(parts[p].mount_dx, parts[p].mount_dy);
    idinfo = parts[displayed_part].id;

    if (part_flag (displayed_part, VPFLAG_OPENABLE) && parts[displayed_part].open) {
        part_mod = 1; // open
    } else if (parts[displayed_part].hp <= 0){
        part_mod = 2; // broken
    }

    return idinfo;
}

nc_color vehicle::part_color (int p)
{
    if (p < 0 || p >= parts.size()) {
        return c_black;
    }

    nc_color col;

    int parm = -1;

    //If armoring is present and the option is set, it colors the visible part
    if (OPTIONS["VEHICLE_ARMOR_COLOR"] == true)
      parm = part_with_feature(p, VPFLAG_ARMOR, false);

    if (parm >= 0) {
        col = part_info(parm).color;
    } else {

        int displayed_part = part_displayed_at(parts[p].mount_dx, parts[p].mount_dy);

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

    // curtains turn windshields gray
    int curtains = part_with_feature(p, VPFLAG_CURTAIN, false);
    if (curtains >= 0) {
        if (part_with_feature(p, VPFLAG_WINDOW, true) >= 0 && !parts[curtains].open)
            col = part_info(curtains).color;
    }

    //Invert colors for cargo parts with stuff in them
    int cargo_part = part_with_feature(p, VPFLAG_CARGO);
    if(cargo_part >= 0 && !parts[cargo_part].items.empty()) {
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
int vehicle::print_part_desc(WINDOW *win, int y1, int width, int p, int hl /*= -1*/)
{
    if (p < 0 || p >= parts.size()) {
        return y1;
    }
    std::vector<int> pl = this->parts_at_relative(parts[p].mount_dx, parts[p].mount_dy);
    int y = y1;
    for (int i = 0; i < pl.size(); i++)
    {
        int dur = part_info (pl[i]).durability;
        int per_cond = parts[pl[i]].hp * 100 / (dur < 1? 1 : dur);
        nc_color col_cond;
        if (parts[pl[i]].hp >= dur) {
            col_cond = c_green;
        } else if (per_cond >= 80) {
            col_cond = c_ltgreen;
        } else if (per_cond >= 50) {
            col_cond = c_yellow;
        } else if (per_cond >= 20) {
            col_cond = c_ltred;
        } else if (parts[pl[i]].hp > 0) {
            col_cond = c_red;
        } else { //Broken
            col_cond = c_dkgray;
        }

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

        bool armor = part_flag(pl[i], "ARMOR");
        std::string left_sym, right_sym;
        if(armor) {
            left_sym = "("; right_sym = ")";
        } else if(part_info(pl[i]).location == part_location_structure) {
            left_sym = "["; right_sym = "]";
        } else {
            left_sym = "-"; right_sym = "-";
        }

        mvwprintz(win, y, 1, int(i) == hl? hilite(c_ltgray) : c_ltgray, left_sym.c_str());
        mvwprintz(win, y, 2, int(i) == hl? hilite(col_cond) : col_cond, partname.c_str());
        mvwprintz(win, y, 2 + utf8_width(partname.c_str()), int(i) == hl? hilite(c_ltgray) : c_ltgray, right_sym.c_str());
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

    return y;
}

void vehicle::print_fuel_indicator (void *w, int y, int x, bool fullsize, bool verbose, bool desc)
{
    WINDOW *win = (WINDOW *) w;
    const nc_color fcs[num_fuel_types] = { c_ltred, c_yellow, c_ltgreen, c_ltblue, c_ltcyan };
    const char fsyms[5] = { 'E', '\\', '|', '/', 'F' };
    nc_color col_indf1 = c_ltgray;
    int yofs = 0;
    for (int i = 0; i < num_fuel_types; i++) {
        int cap = fuel_capacity(fuel_types[i]);
        if (cap > 0 && ( basic_consumption(fuel_types[i]) > 0 || fullsize ) ) {
            mvwprintz(win, y + yofs, x, col_indf1, "E...F");
            int amnt = cap > 0? fuel_left(fuel_types[i]) * 99 / cap : 0;
            int indf = (amnt / 20) % 5;
            mvwprintz(win, y + yofs, x + indf, fcs[i], "%c", fsyms[indf]);
            if (verbose) {
                if (g->debugmon) {
                    mvwprintz(win, y + yofs, x + 6, fcs[i], "%d/%d", fuel_left(fuel_types[i]), cap);
                } else {
                    mvwprintz(win, y + yofs, x + 6, fcs[i], "%d", (fuel_left(fuel_types[i]) * 100) / cap);
                    wprintz(win, c_ltgray, "%c", 045);
                }
            }
            if (desc) {
                wprintz(win, c_ltgray, " - %s", ammo_name(fuel_types[i]).c_str() );
            }
            if (fullsize) {
                yofs++;
            }
        }
    }
}

void vehicle::coord_translate (int reldx, int reldy, int &dx, int &dy)
{
    tileray tdir (face.dir());
    tdir.advance (reldx);
    dx = tdir.dx() + tdir.ortho_dx(reldy);
    dy = tdir.dy() + tdir.ortho_dy(reldy);
}

void vehicle::coord_translate (int dir, int reldx, int reldy, int &dx, int &dy)
{
    tileray tdir (dir);
    tdir.advance (reldx);
    dx = tdir.dx() + tdir.ortho_dx(reldy);
    dy = tdir.dy() + tdir.ortho_dy(reldy);
}

void vehicle::precalc_mounts (int idir, int dir)
{
    if (idir < 0 || idir > 1)
        idir = 0;
    for (int p = 0; p < parts.size(); p++)
    {
        int dx, dy;
        coord_translate (dir, parts[p].mount_dx, parts[p].mount_dy, dx, dy);
        parts[p].precalc_dx[idir] = dx;
        parts[p].precalc_dy[idir] = dy;
    }
}

std::vector<int> vehicle::boarded_parts()
{
    std::vector<int> res;
    for (int p = 0; p < parts.size(); p++) {
        if (part_flag (p, VPFLAG_BOARDABLE) &&
                parts[p].has_flag(vehicle_part::passenger_flag)) {
            res.push_back (p);
        }
    }
    return res;
}

int vehicle::free_seat()
{
    for (int p = 0; p < parts.size(); p++) {
        if (part_flag (p, VPFLAG_BOARDABLE) &&
               !parts[p].has_flag(vehicle_part::passenger_flag)) {
            return p;
        }
    }
    return -1;
}

player *vehicle::get_passenger (int p)
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

int vehicle::global_x ()
{
    return smx * SEEX + posx;
}

int vehicle::global_y ()
{
    return smy * SEEY + posy;
}

int vehicle::omap_x() {
    return levx + (global_x() / SEEX);
}

int vehicle::omap_y() {
    return levy + (global_y() / SEEY);
}

void vehicle::update_map_x(int x) {
    levx = x;
    if (tracking_on)
        g->cur_om->vehicles[om_id].x = omap_x()/2;
}

void vehicle::update_map_y(int y) {
    levy = y;
    if (tracking_on)
        g->cur_om->vehicles[om_id].y = omap_y()/2;
}

int vehicle::fuel_left (const ammotype & ftype)
{
    int fl = 0;
    for(int p = 0; p < fuel.size(); p++) {
        if(ftype == part_info(fuel[p]).fuel_type) {
            fl += parts[fuel[p]].amount;
        }
    }
    return fl;
}

int vehicle::fuel_capacity (const ammotype & ftype)
{
    int cap = 0;
    for(int p = 0; p < fuel.size(); p++) {
        if(ftype == part_info(fuel[p]).fuel_type) {
            cap += part_info(fuel[p]).size;
        }
    }
    return cap;
}

int vehicle::refill (const ammotype & ftype, int amount)
{
    for (int p = 0; p < parts.size(); p++)
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

int vehicle::drain (const ammotype & ftype, int amount) {
  int drained = 0;

  for (int p = 0; p < fuel.size(); p++) {
    vehicle_part &tank=parts[fuel[p]];
    if (part_info(fuel[p]).fuel_type == ftype && tank.amount > 0) {
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

int vehicle::basic_consumption (const ammotype & ftype)
{
    int fcon = 0;
    for(int p = 0; p < engines.size(); p++) {
        if(ftype == part_info(engines[p]).fuel_type && parts[engines[p]].hp > 0) {
            if(part_info(engines[p]).fuel_type == fuel_type_battery) {
                // electric engine - use epower instead
                fcon += abs(epower_to_power(part_epower(engines[p])));
            }
            else {
                fcon += part_power(engines[p]);
            }
        }
    }
    return fcon;
}


int vehicle::solar_epower()
{
    int epower = 0;
    for(int p = 0; p < solar_panels.size(); p++) {
        if(parts[solar_panels[p]].hp > 0) {
            int part_x = global_x() + parts[solar_panels[p]].precalc_dx[0];
            int part_y = global_y() + parts[solar_panels[p]].precalc_dy[0];
            // Can't use g->in_sunlight() because it factors in vehicle roofs.
            if( !g->m.has_flag_ter_or_furn( TFLAG_INDOORS, part_x, part_y ) ) {
                epower += (part_epower(solar_panels[p]) * g->natural_light_level()) / DAYLIGHT_LEVEL;
            }
        }
    }
    return epower;
}

/*
int vehicle::max_velocity( bool fueled )
{
    int pwr = fueled ? engine_power_cur : engine_power_max;
    return pwr * 10;
}

int vehicle::safe_velocity (bool fueled)
{
    int pwrs = 0;
    int cnt = 0;
    for (int i = 0; i < engines.size(); i++) {
        int p = engines[i];
        if ( parts[p].hp > 0 &&
                (fuel_left(part_info(p).fuel_type) || !fueled ||
                 part_info(p).fuel_type == fuel_type_muscle)) {
            int m2c = 100;

            if (part_info(p).fuel_type == fuel_type_gasoline)     m2c = 60;
            else if (part_info(p).fuel_type == fuel_type_plasma)  m2c = 75;
            else if (part_info(p).fuel_type == fuel_type_battery) m2c = 90;
            else if (part_info(p).fuel_type == fuel_type_muscle)  m2c = 45;

            pwrs += part_power(p) * m2c / 100;
            cnt++;
        }
    }
    if (cnt > 0)
        pwrs = pwrs * 4 / (3 + cnt);
    return (int) (pwrs * k_dynamics() * k_mass()) * 80;
}
*/

void vehicle::spew_smoke( double joules, int part ) {
    if (joules < 50000 && rng(0, 50000) > joules )
        return;
    point p( parts[part].mount_dx, parts[part].mount_dy );
    int smoke = joules / 50 + 1;
    int dir = 1;
    if ( velocity < 0 )
        dir = -1;
    // Move back from engine/muffler til we find an open space
    while (relative_parts.find(p) != relative_parts.end()) p.x-=dir;
    int rdx, rdy;
    coord_translate (p.x, p.y, rdx, rdy);
    g->m.add_field(global_x() + rdx, global_y() + rdy, fd_smoke, smoke);
}

/**
 * Generate noise or smoke from a vehicle with engines turned on
 * engine_output = 0.01 ->  background engine noise while not generating thrust
 * engine_output = 0.5  ->  engine noise at half throttle
 * engine_output = 1.0  ->  max engine noise
 * Also updates exhaust_dx and exhaust_dy if get_smoke is true.
 */
void vehicle::noise_and_smoke( double load, double time ) {
    float noise = 0.0f;
    float mufflesmoke = 0.0f;
    float muffle = 1.0f;
    int exhaust_part = -1;
    for (size_t p = 0; p < parts.size(); p++) {
        if (part_flag(p, "MUFFLER") && parts[p].hp > 0 && part_info(p).bonus < muffle) {
            muffle = float(part_info(p).bonus)/100;
            exhaust_part = int(p);
        }
    }

    for (size_t e = 0; e < engines.size(); e++) {
        int p = engines[e];
        if (parts[p].hp > 0 &&
                (fuel_left (part_info(p).fuel_type) ||
                 part_info(p).fuel_type == fuel_type_muscle) ) {
            float pwr = 10.0f; // Default noise if nothing else found, shouldn't happen
            float max_pwr = float(part_power(p))/100;
            float cur_pwr = load * max_pwr;
            bool is_gas = part_info(p).fuel_type == fuel_type_gasoline;
            bool is_plasma = part_info(p).fuel_type == fuel_type_plasma;
            bool is_battery = part_info(p).fuel_type == fuel_type_battery;
            bool is_muscle = part_info(p).fuel_type == fuel_type_muscle;

            if( is_gas && parts[p].hp > 0 ) { // Otherwise no smoke from this engine
                double j = power_to_epower(part_power(p, true)) * load * time * muffle;
                if (exhaust_part == -1) {
                    spew_smoke( j, p );
                } else {
                    mufflesmoke += j;
                }
            }
            if      (is_gas)      pwr = (cur_pwr*20 + max_pwr*2 + 10)*muffle;
            else if (is_plasma)   pwr = (cur_pwr*10 + 10)*muffle;
            else if (is_battery)  pwr = cur_pwr*3;
            else if (is_muscle)   pwr = cur_pwr*5;
            noise = std::max(noise, pwr); // Only the loudest engine counts.
        }
    }
    if (exhaust_part != -1)
        spew_smoke( mufflesmoke, exhaust_part );
    // Even a car with engines off will make noise rolling over the terrain
    noise = std::max(noise, float(fabs(velocity/2.236/100)));

    std::string soundmessage = "";
    if (!has_pedals) {
      if (noise > 250)
        soundmessage = "BRUMBRUMBRUMBRUM!!!";
      else if (noise > 140)
        soundmessage = "BRRROARRR!!";
      else if (noise > 80)
        soundmessage = "ROARRR!";
      else if (noise > 60)
        soundmessage = "roarrr!";
      else if (noise > 30)
        soundmessage = "vroom!";
      else if (noise > 15)
        soundmessage = "whirrr!";
      else
        soundmessage = "hummm!";
    }
    if (rng(0, 30) > noise)
        g->sound(global_x(), global_y(), noise, soundmessage.c_str());
}

bool vehicle::valid_wheel_config ()
{
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    int count = 0;
    std::vector<int> wheel_indices = all_parts_with_feature(VPFLAG_WHEEL);
    if(wheel_indices.size() == 0) {
        //No wheels!
        return false;
    } else if(wheel_indices.size() == 1) {
        //Has to be a stable wheel
        if(part_info(wheel_indices[0]).has_flag("STABLE")) {
            //Valid only if the vehicle is 1 square in size (1 structural part)
            return (all_parts_at_location(part_location_structure).size() == 1);
        } else {
            return false;
        }
    }
    for (int w = 0; w < wheel_indices.size(); w++) {
        int p = wheel_indices[w];
        if (!count) {
            x1 = x2 = parts[p].mount_dx;
            y1 = y2 = parts[p].mount_dy;
        }
        if (parts[p].mount_dx < x1) {
            x1 = parts[p].mount_dx;
        }
        if (parts[p].mount_dx > x2) {
            x2 = parts[p].mount_dx;
        }
        if (parts[p].mount_dy < y1) {
            y1 = parts[p].mount_dy;
        }
        if (parts[p].mount_dy > y2) {
            y2 = parts[p].mount_dy;
        }
        count++;
    }
    float xo = 0, yo = 0;
    float wo = 0, w2;
    for (int p = 0; p < parts.size(); p++)
    { // lets find vehicle's center of masses
        w2 = itypes[part_info(p).item]->weight;
        if (w2 < 1)
            continue;
        xo = xo * wo / (wo + w2) + parts[p].mount_dx * w2 / (wo + w2);
        yo = yo * wo / (wo + w2) + parts[p].mount_dy * w2 / (wo + w2);
        wo += w2;
    }
//    g->add_msg("cm x=%.3f y=%.3f m=%d  x1=%d y1=%d x2=%d y2=%d", xo, yo, (int) wo, x1, y1, x2, y2);
    if ((int)xo < x1 || (int)xo > x2 || (int)yo < y1 || (int)yo > y2) {
        return false; // center of masses not inside support of wheels (roughly)
    }
    return true;
}

/**
 * Power for batteries are in W, but usage for rest is in 0.5*HP, so coeff is 373
 * This does not seem to match up for consumption, as old coeff was 100
 * Keeping coeff of 100, but should probably be adjusted later
 * http://en.wikipedia.org/wiki/Energy_density -> 46MJ/kg, 36MJ/l for gas
 * Gas tanks are 6000 and 30000, assuming that's in milliliters, so 36000J/ml
 * Battery sizes are 1k, 12k, 30k, 50k, 100k
 */
void vehicle::consume_fuel( double load )
{
    ammotype ftypes[3] = { fuel_type_gasoline, fuel_type_battery, fuel_type_plasma };
    int ftype_coeff[3] = {              36000,           3600000,            36000 };
    double base_con[3];
    for( int ft = 0; ft < 3; ft++ )
        base_con[ft] = basic_consumption(ftypes[ft]) / ftype_coeff[ft];
    for (int ft = 0; ft < 3; ft++)
    {
        if( !base_con[ft] )
            continue; // no consumption for engines of that type
        double amnt_precise = load * base_con[ft];
        int amnt = int(amnt_precise);
        // consumption remainder results in chance at additional fuel consumption
        if (x_in_y(int(amnt_precise*1000) % 1000, 1000)) {
            amnt += 1;
        }
        for (int p = 0; p < fuel.size(); p++) {
            if(part_info(fuel[p]).fuel_type == ftypes[ft]) {
                if (parts[fuel[p]].amount >= amnt) {
                    // enough fuel located in this part
                    parts[fuel[p]].amount -= amnt;
                    break;
                }
                else {
                    amnt -= parts[fuel[p]].amount;
                    parts[fuel[p]].amount = 0;
                }
            }
        }
    }
}

void vehicle::charge_battery (int amount)
{
    for(int f=0;f<fuel.size() && amount > 0;f++)
    {
        if(part_info(fuel[f]).fuel_type == fuel_type_battery)
        {
            int empty = part_info(fuel[f]).size - parts[fuel[f]].amount;
            if(empty < amount)
            {
                amount -= empty;
                parts[fuel[f]].amount = part_info(fuel[f]).size;
            }
            else
            {
                parts[fuel[f]].amount += amount;
                amount = 0;
            }
        }
    }
}

int vehicle::discharge_battery (int amount)
{
    int avail_charge;

    for(int f = 0; f < fuel.size() && amount > 0; f++) {
        if(part_info(fuel[f]).fuel_type == fuel_type_battery) {
            avail_charge = parts[fuel[f]].amount;
            if(avail_charge < amount) {
                amount -= avail_charge;
                parts[fuel[f]].amount = 0;
            }
            else {
                parts[fuel[f]].amount -= amount;
                amount = 0;
            }
        }
    }

    return amount; // non-zero if discharge amount exceeds available battery charge
}

void vehicle::thrust(int percent) {
  if (cruise_on) {
      cruise_velocity += percent*10; // Steps of 10 mph
  } else {
      player_thrust = percent; // Store the value and calculate accel later
  }
}

bool vehicle::do_environmental_effects() {
    bool needed = false;
    // check for smoking parts
    for (int p = 0; p < parts.size(); p++)
    {
        int part_x = global_x() + parts[p].precalc_dx[0];
        int part_y = global_y() + parts[p].precalc_dy[0];

        /* Only lower blood level if:
         * - The part is outside.
         * - The weather is any effect that would cause the player to be wet. */
        if (parts[p].blood > 0 &&
                g->m.is_outside(part_x, part_y) && g->levz >= 0 &&
                g->weather >= WEATHER_DRIZZLE && g->weather <= WEATHER_ACID_RAIN) {
            needed = true;
            parts[p].blood--;
        }
        if (part_flag(p, VPFLAG_ENGINE) && parts[p].hp <= 0 && parts[p].amount > 0) {
            needed = true;
            parts[p].amount--;
            for (int ix = -1; ix <= 1; ix++) {
                for (int iy = -1; iy <= 1; iy++) {
                    if (!rng(0, 2)) {
                        g->m.add_field(part_x + ix, part_y + iy, fd_smoke, rng(2, 4));
                    }
                }
            }
        }
    }
    return needed;
}

double vehicle::generate_all_power()
{
    // Producers of epower
    int max_power, gas_epower, alt_power, eng_power;
    int epower = solar_epower();
    if (engine_on)
        eng_power = engine_power(&max_power, &gas_epower, &alt_power);
    if (engine_on) // Could have died in line above
        epower += gas_epower;

    // These consume epower (negative values). Values cached in refresh()
    if(lights_on) epower += lights_epower;
    if(overhead_lights_on) epower += overhead_epower;
    if(tracking_on) epower += tracking_epower;
    if(fridge_on) epower += fridge_epower;
    if(recharger_on) epower += recharger_epower;

    int battery_missing = power_to_epower(fuel_capacity(fuel_type_battery) - fuel_left(fuel_type_battery));

    if(reactor_on && battery_missing - epower > 0) {
        // Still not enough surplus epower to fully charge battery
        // Produce additional epower from any reactors
        int reactor_output = 0;
        int reactors_fuel_epower = 0;
        bool reactor_gave_power = false;
        for(int p = 0; p < reactors.size(); p++) {
            if (parts[reactors[p]].hp > 0 && battery_missing - epower > 0) {
                reactor_output = part_info(reactors[p]).epower;
                reactors_fuel_epower = power_to_epower(parts[reactors[p]].amount);
                // check if enough fuel for full reactor output
                reactor_output = std::min(reactors_fuel_epower, reactor_output);
                reactor_output = std::min(battery_missing - epower, reactor_output);
                // calculate battery-equivalent fuel consumption
                int battery_consumed = epower_to_power(reactor_output);
                // 1 plutonium == 100 battery - divide consumption by 100
                int plutonium_consumed = battery_consumed / 100;
                // battery remainder results in chance at additional plutonium consumption
                if (x_in_y(battery_consumed % 100, 100))
                    plutonium_consumed += 1;
                parts[reactors[p]].amount -= plutonium_consumed;
                epower += reactor_output;
                reactor_gave_power = true;
            }
        }
        if (!reactor_gave_power) {
            // all reactors out of fuel or destroyed
            reactor_on = false;
            if(player_in_control(&g->u) || g->u_see(global_x(), global_y())) {
                g->add_msg(_("The %s's reactor dies!"), name.c_str());
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

    if (battery_deficit) {
        lights_on = false;
        tracking_on = false;
        overhead_lights_on = false;
        fridge_on = false;
        recharger_on = false;
        if(player_in_control(&g->u) || g->u_see(global_x(), global_y())) {
            g->add_msg("The %s's battery dies!",name.c_str());
        }
        if (gas_epower < 0) { // Did gas engines try to draw epower?
            // Not enough epower to run gas engine ignition system
            engine_on = false;
            if(player_in_control(&g->u) || g->u_see(global_x(), global_y())) {
                g->add_msg("The %s's engine dies!",name.c_str());
            }
        }
    }

    // Return idle engine load, used if engines are not asked to generate thrust this turn
    double load = 0;
    if (engine_on) {
        load = alt_power / power_to_epower(max_power);
        if (load < 0.01)
            load = 0.01; // Assume min 1%
    }
    return load;
}

int vehicle::engine_power( int *r_power_max, int *r_epower, int *r_alt_power )
{
    int power_cur = 0; 
    int power_max = 0;
    int epower = 0;
    bool debug = false;
    for (int e = 0; e < engines.size(); e++) {
        int p = engines[e];
        if (parts[p].hp > 0) {
            power_max += part_power(p, true);
            if (debug) g->add_msg("power_max = %d", power_max);
            if (fuel_left(part_info(p).fuel_type)) {
                power_cur += part_power(p);
                if (debug) g->add_msg("power_cur = %d", power_cur);
                epower += part_epower(p);
            } else if (part_info(p).fuel_type == fuel_type_muscle && player_in_control(&g->u)) {
                // Extra logic for pedals so that they only work if player is powering them
                int part_under_player;
                if (g->m.veh_at(g->u.posx, g->u.posy, part_under_player)) {
                    if (p == part_with_feature(int(part_under_player), VPFLAG_ENGINE))
                        power_cur += part_power(p);
                }
            }
        }
    }

    // Penalty for having multiple engines installed
    power_cur = power_cur * 4 / (3 + engines.size());
    power_max = power_max * 4 / (3 + engines.size());

    // Alternators produce epower and reduce available power
    int alt_power = 0;
    for(int p = 0; p < alternators.size(); p++) {
        alt_power += part_power(alternators[p]);
        epower += part_epower(alternators[p]);
    }
    power_cur += alt_power;

    if (r_power_max) *r_power_max = power_max;
    if (r_epower) *r_epower = epower;
    if (r_alt_power) *r_alt_power = alt_power;

    if (engine_on) {
        if (power_cur <= 0) {
            if (g->u_see(global_x(), global_y())) {
                if (alt_power < 0 && power_cur - alt_power > 0) {
                    g->add_msg(_("The %s's alternators choke the engine!"), name.c_str());
                } else {
                    g->add_msg(_("The %s's engine dies!"), name.c_str());
                }
            }
            engine_on = false;
        }
    }
    return power_cur;
}

void vehicle::do_turn_actions()
{
    if (velocity || player_thrust || cruise_velocity) {
        of_turn = 1 + of_turn_carry;
        of_turn_carry = 0;
        if( of_turn <= 0 )
            of_turn = 0.1f; // Always allow 1 move per turn
    } else {
        of_turn = 0;
    }

    if (has_environmental_effects)
        has_environmental_effects = do_environmental_effects();

    if (turret_mode) { // handle turrets
        bool turret_active = false;
        for (int p = 0; p < parts.size(); p++) {
            if (fire_turret(p))
                turret_active = true;
        }
        if (!turret_active) {
            turret_mode = 0;
            g->add_msg(_("The %s's turrets run out of ammo and shut down."), name.c_str());
        }
    }

    double load = generate_all_power();
    if (skidding && player_in_control(&g->u))
        possibly_recover_from_skid();
    if (!engine_on)
        return;

    // Vehicle still wants to turn a bit after last turn, let it
    if (!skidding && abs(turn_dir) < 12 && abs(turn_dir) > 2) {
        g->add_msg("T] Adding leftover turn %d to facing %d.", turn_dir, face.dir());
        face.init(face.dir() + turn_dir);
        move = face;
        turn_dir = 0;
    }
    // Snap face to 15 degrees increments to improve driving experience
    if (!skidding && face.dir() % 15 != 0) {
        g->add_msg("T] Adjusting facing %d to cardinal direction.", face.dir());
        face.init((face.dir()+8)/15*15);
        if (move.dir() != face.dir() )
            move = face;
    }
    double tiles_per_turn = std::max(1.0, TURN_TIME_S * velocity / 223.6 / TILE_SIZE_M);
    turn_delta_per_tile = turn_dir / tiles_per_turn;

    if (of_turn == 0) {
        // Consume fuel etc here, since we're ending the vehicles turn now
        // Otherwise handle this while generating thrust in pass_through_current_maptile()
        consume_fuel( load * 6.0 );
        noise_and_smoke( load, 6.0 );
    }
    if( rng(0, 500) == 0 )
        total_mass(); // Update cached_mass occasionally since we're spending fuel
}

void vehicle::turn( int deg )
{
    if (deg == 0 || velocity == 0)
        return;
    if (velocity < 0)
        deg = -deg;
    turn_dir += deg;
    double tiles_per_turn = std::min(1.0, TURN_TIME_S * velocity / 223.6 / TILE_SIZE_M);
    turn_delta_per_tile += turn_dir / tiles_per_turn;
}

// Can be called while already stopped
void vehicle::stop ()
{
    g->add_msg("STOP! v=%d", velocity);
    velocity = 0;
    skidding = false;
    move = face;
    turn_dir = 0;
    turn_delta_per_tile = 0.0;
    of_turn_carry = 0;
    of_turn = 0;
}

// vel_dec is a positive number that will be subtracted
// if going forwards and subtracted otherwise.
// Will call stop() if no velocity left.
bool vehicle::slow_down( int vel_dec )
{
    g->add_msg("slow v=%d by %d", velocity, vel_dec);
    if (!velocity)
        return true;
    // 20 is slow enough for bicycles to go in reverse.
    if (vel_dec + 20 >= abs(velocity)) {
        stop();
    } else {
        velocity -= (velocity>0 ? vel_dec : -vel_dec);
    }
    return (velocity == 0);
}

double vehicle::calculate_forces( vehicle_forces *vf, double v, double fdir ) {
    bool debug = 0;
    if (debug>=1) g->add_msg("calc_forces %d", int(v*1000));
    // http://en.wikipedia.org/wiki/Density#Air
    double air_density = 1.204; // in kg/m^3  TODO: Vary it based on altitude/temperature
    // http://en.wikipedia.org/wiki/Gravitation
    double gravity = 9.80665;
    // Slow down vehicle due to air resistance.
    // Don't need to consider grip here, since the force acts on car chassis and not tires
    // http://en.wikipedia.org/wiki/Drag_%28physics%29 -> Fd = 1/2 p v^2 Cd A
    // kg*m/s^2 = kg/m^3 * m/s * m/s * m^2 
    vf->drag_newton = air_density * v * v * drag_coeff;
    if (debug>=4) g->add_msg("drag_N = %d", int(vf->drag_newton));

    // http://en.wikipedia.org/wiki/Downforce
    // kg*m/s^2 = kg/m^3 * m/s * m/s * m^2
    vf->downforce_newton = 0.5 * air_density * v * v * downforce;
    if (debug>=5) g->add_msg("downF_N = %d", int(vf->downforce_newton));
    // Add gravity to downforce
    // kg*m/s^2 = 9.81 m/s^2(earth gravity) * kg
    vf->downforce_newton += (cached_mass * gravity);

    vf->wheel_newton_max = vf->tire_friction_coeff * vf->downforce_newton;
    if (debug>=5) g->add_msg("max_wheel_N = %d", int(vf->wheel_newton_max));
    // Allow more slowdown than wheels can handle, could be terrain or car frame slowing down
    vf->ground_res_newton = vf->ground_res_coeff * vf->downforce_newton;
    if (debug>=4) g->add_msg("groundres_N = %d", int(vf->ground_res_newton));

    // Find max force tires can handle. Don't make it go negative from rres
    vf->wheel_newton_available = std::max( vf->wheel_newton_max - vf->ground_res_newton, 0.0 );
    if (debug>=5) g->add_msg("avail_wheel_N = %d", int(vf->wheel_newton_available));

    if( velocity < 0 )
        vf->engine_newton_max *= 0.6; // Less power available in reverse;
    vf->is_thrusting = ( velocity == 0 ||
                ( velocity > 0 ^ fdir < 0 ) ); // XOR operator
    if( vf->is_thrusting ) {
        vf->engine_newton = std::min(vf->engine_newton_max * fdir, vf->wheel_newton_available);
        vf->user_applied_newton = vf->engine_newton;
    } else {
        vf->engine_newton = 0.0;
        vf->user_applied_newton = vf->wheel_newton_available * fdir;
    }
    if( !vf->valid_wheel_config )
        vf->user_applied_newton = 0.0;
    if (debug>=3) g->add_msg("applied_N = %d", int(vf->user_applied_newton));

    // Subtract drag/ground res if driving forwards, add them if driving backwards
    vf->newton_total = vf->user_applied_newton + (vf->ground_res_newton + vf->drag_newton) * (velocity > 0 ? -1 : 1);
    if (debug>=2) g->add_msg("total_N = %d", int(vf->newton_total));

    return vf->newton_total;
}

double vehicle::calculate_time_to_move_one_tile( vehicle_forces *vf, double fdir ) {
    int debug = 0;
    if (debug>=1) g->add_msg("calc_time_ %d", int(fdir*1000));
    double v = velocity/2.236/100; // Convert to m/s
    vf->velocity_start = v;
    vf->distance_traveled = TILE_SIZE_M;
    double t = TURN_TIME_S;

    int nr_calcs = 0;
    do {
        vf->time_taken = t;
        // TODO: engine_newton_max should depend on speed (and lots of other stuff)
        vf->engine_newton_max = std::max(0.0, vf->engine_watt_max * t / TILE_SIZE_M);
        if( debug>=5 ) g->add_msg("eng_N = %d", int(vf->engine_newton_max));
        // Get forces at initial speed (start of tile)
        double newton_start = calculate_forces( vf, v, fdir );
        // note, newton, kinetic, and velocity can all be negative
        // joules = kg*m^2/s^2 = kg * m/s * m/s;
        vf->kinetic_start = 0.5 * cached_mass * v * v * ( v < 0 ? -1 : 1 );
        // Calculate speed at end if drag etc at start of tile were kept constant
        double kin_end = vf->kinetic_start + newton_start * TILE_SIZE_M;
        double v_end = sqrt( 2 * fabs(kin_end) / cached_mass );
        if( kin_end < 0 )
            v_end *= -1;
        // Now we can calculate drag etc using v_end (end of tile)
        double newton_end = calculate_forces( vf, v_end, fdir );
        // Get the average force working on the vehicle through the tile
        vf->newton_average = ( newton_start + newton_end ) / 2;
        // Calculate the correct kinetic value, using average force from tile start til tile end
        vf->kinetic_end = vf->kinetic_start + vf->newton_average * TILE_SIZE_M;
        // And there we have an estimate of the speed
        vf->velocity_end = sqrt( 2 * fabs(vf->kinetic_end) / cached_mass );
        if (vf->kinetic_end < 0.0)
            vf->velocity_end *= -1;
        if ( vf->velocity_start != 0.0 &&
                !(vf->velocity_start > 0.0 ^ vf->velocity_end < 0.0) ) { // XAND
            // Don't go from forward to reverse
            vf->kinetic_end = 0.0;
            vf->velocity_end = 0.0;
        }
        // Update some other values as well now that we know speed etc
        vf->velocity_average = (v + vf->velocity_end) / 2;
        vf->engine_watt_average = vf->newton_average * vf->velocity_average;
        vf->distance_traveled = fabs(vf->velocity_average * t); // No negatives
        // More closely match t to actual time taken, for use in next iteration
        t = t * TILE_SIZE_M / vf->distance_traveled;
        if( fabs(vf->velocity_end) < 0.1 )
            vf->velocity_end = 0.0;
        if( vf->distance_traveled < 0.001 || t >= TURN_TIME_S || t <= 0.001 || vf->velocity_end == 0.0 ) {
            vf->time_taken = TURN_TIME_S;
            break; // For gameplay purposes you can always travel 1 tile in a turn
        }
    } while( fabs( vf->distance_traveled - TILE_SIZE_M ) > 0.0001 && nr_calcs++ < 999);
    if( nr_calcs >= 1000 )
        debugmsg( "Nr of calcs in calculate_time_ more than 1000!" );
    if( vf->time_taken > TURN_TIME_S )
        vf->time_taken = TURN_TIME_S;
    return vf->time_taken;
}

double vehicle::calculate_movement( vehicle_forces *vf, double engine_output, double target_speed, int tile_move_cost )
{
    int debug = 0;
    // tile_move_cost is from 2 (road, dirt) -> 10
    // TODO: Remove tile_move_cost as a parameter and calculate individually for each wheel 
    if (debug>=1) g->add_msg("calc_forces eo=%d ts=%d", int(engine_output*100), int(target_speed));
    int unused1;
    vf->eng_pwr_cur = engine_power(&vf->eng_pwr_max, &unused1, &vf->eng_alt_pwr);
    // TODO: Add gearboxes, vary engine power based on speed/gear
    vf->engine_watt_max = engine_on ? double(power_to_epower(vf->eng_pwr_cur)) : 0.0;
    vf->valid_wheel_config = valid_wheel_config(); // Cache this

    // http://hyperphysics.phy-astr.gsu.edu/hbase/mechanics/frictire.html
    // http://mfes.com/friction.html
    // http://atlasf1.autosport.com/99/san/preview/soerensen.html
    // http://hpwizard.com/tire-friction-coefficient.html
    // http://books.google.no/books?id=5pIpV1LY5HMC&pg=PA358&source=gbs_toc_r&cad=4
    // http://www.datsuns.com/Tech/tech_tires-2.htm
    vf->tire_friction_coeff = std::max(0.1, 0.9 - tile_move_cost/10.0);
    if (debug>=5) g->add_msg("tire_f_coeff*k=%d", int(vf->tire_friction_coeff*1000));
    // TODO: Less available brake/accel force when wheels are damaged?

    // TODO: Add deformation friction, make rolling friction scale with v instead of downforce
    // Slow down vehcile due to ground friction
    // http://en.wikipedia.org/wiki/Rolling_resistance
    // http://www.engineeringtoolbox.com/rolling-friction-resistance-d_1303.html
    // TODO: Calculate rolling res properly
    vf->rolling_res_coeff = wheels_area() < 30 ? 0.0035 : 0.01; // Assume bicycle tires if < 30
    // Riding on the car frame gives lower rres than tires, which are made for max friction
    if (!vf->valid_wheel_config) {
        vf->rolling_res_coeff = 0.4;
    } else if (skidding) {
        vf->rolling_res_coeff = 0.7;
    }
    // TODO: Differentiate between grass/road etc 
    if (debug>=5) g->add_msg("rres_coeff = %d", int(vf->rolling_res_coeff*1000));
    // In difficult terrain you roll a lot worse, plus, debris
    // Just add resistance from rolling res/debris hitting car body here
    vf->ground_res_coeff = vf->rolling_res_coeff + 0.01 * pow(tile_move_cost, 3) / 8 - 0.01;
    if (debug>=5) g->add_msg("gres_coeff = %d", int(vf->ground_res_coeff*1000));


    char buf[100];
    if( engine_output > 1.5 ) { // Special acceleration mode
        vf->engine_watt_max = double(power_to_epower(vf->eng_pwr_cur));
        sprintf( buf, "engine_watt_max = %f", vf->engine_watt_max );
//        debugmsg(buf);
        if( engine_output > 2.5 )
            vf->engine_watt_max = double(power_to_epower(vf->eng_pwr_max));
        sprintf( buf, "engine_watt_max = %f", vf->engine_watt_max );
//        debugmsg(buf);
        if (vf->engine_watt_max < 1)
            return 0.0;
        double total_time = 0.0, tile_time;
        int nr_calcs = 0;
        int old_velocity = velocity;
        velocity = 0;
        do {
            tile_time = calculate_time_to_move_one_tile( vf, 1.0 );
            velocity = int( vf->velocity_end * 223.6 + 0.5 );
            if( velocity == 0 ) {
                velocity = old_velocity;
                return 0.0;
            }
            total_time += tile_time;
            sprintf(buf, "Took %f time for one tile, speed is now %f.", tile_time, vf->velocity_end);
//            debugmsg(buf);
        } while ( nr_calcs++ < 999 && vf->velocity_end < target_speed );
        // Subtract time used for velocity above target_speed
        total_time += -tile_time + tile_time *
                      (target_speed - vf->velocity_start) /
                      (vf->velocity_end - vf->velocity_start);
        velocity = old_velocity;
        return ( nr_calcs < 999 ? total_time : 0.0 );
    }
    if( engine_output >= -1.0 ) { // Normal thrusting mode, no cruise control
        return calculate_time_to_move_one_tile( vf, engine_output );
    }
    double eng_est = -1.0, eng_step = 2.0;
    int nr_calcs = 0;
    bool last_vel_too_high = false;
    do { // -2.0 engine_output, cruise control mode
        calculate_time_to_move_one_tile( vf, eng_est );
        sprintf(buf, "Power %f got speed %f, want %f. step = %f", eng_est, vf->velocity_end, target_speed, eng_step);
//        debugmsg(buf);
        if( vf->velocity_end > target_speed ) {
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
    } while( nr_calcs++ < 999 && fabs(vf->velocity_end - target_speed) > 0.01 );
//    debugmsg( "Exiting function calc_move" );
    if( nr_calcs >= 999 )
        debugmsg("More than 1000 speed calculations trying to cruise control!");
    return vf->time_taken;
}

/**
 * Called to process vehicle actions that occur while it's moving through the tile it's currently in.
 * If it return false, it was unable to move and did not perform any actions.
 * We can afford to do some heavy calcs here, since this routine is only called on moving vehicles
 */
bool vehicle::drive_one_tile()
{
    int debug = 0;
    const bool pl_ctrl = player_in_control(&g->u);
    const int gx = global_x();
    const int gy = global_y();
    if (!g->m.inbounds(gx, gy)) {
        if (g->debugmon) { debugmsg ("stopping out-of-map vehicle. (x,y)=(%d,%d)", gx, gy); }
        stop();
        of_turn = 0;
        return false;
    }

    // Handle wheels. Sink in water, mess up ground, set off traps.
    int curgen, num_wheels, submerged_wheels;
    do {
        std::vector<int> wheel_indices = all_parts_with_feature(VPFLAG_WHEEL, false);
        num_wheels = wheel_indices.size();
        submerged_wheels = 0;
        curgen = generation;
        for (int w = 0; w < num_wheels; w++) {
            const int p = wheel_indices[w];
            const int px = gx + parts[p].precalc_dx[0];
            const int py = gy + parts[p].precalc_dy[0];
            // deep water
            if (g->m.ter_at(px, py).has_flag(TFLAG_DEEP_WATER))
                submerged_wheels++;
    
            if (one_in(2)) {
                if (g->m.displace_water(gx + parts[p].precalc_dx[0],
                                        gy + parts[p].precalc_dy[0]) && pl_ctrl)
                    g->add_msg(_("You hear a splash!"));
            }

            // now we're gonna handle traps we're standing on
            handle_trap( gx + parts[p].precalc_dx[0], gy + parts[p].precalc_dy[0], p );
            if (curgen != generation)
                break; // Parts have changed
        }
    } while (curgen != generation); // Redo if parts changed

    // submerged wheels threshold is 2/3.
    if (num_wheels && (float)submerged_wheels / num_wheels > .666) {
        g->add_msg(_("Your %s sank."), name.c_str());
        // destroy vehicle (sank to nowhere)
        g->m.destroy_vehicle(this);
        return false;
    }

    if (!valid_wheel_config()) {
        // if not enough wheels, mess up the ground a bit.
        for (int p = 0; p < parts.size(); p++) {
            const int px = gx + parts[p].precalc_dx[0];
            const int py = gy + parts[p].precalc_dy[0];
            const ter_id &pter = g->m.ter(px, py);
            if (pter == t_dirt || pter == t_grass) {
                g->m.ter_set(px, py, t_dirtmound);
            }
        }
    }

    // Eventually send vehicle skidding if no-one is controlling it
    if (!boarded_parts().size() && one_in (10)) {
        start_skid(0);
    }

    if (pl_ctrl) {
        int ds = g->u.skillLevel("driving");
        if (skidding) {
            if (one_in(4)) { // Might turn uncontrollably while skidding
                turn(rng(-2, 2) * 15);
            }
            g->u.practice( g->turn, "driving", 1 );
        } else if (pl_ctrl && dice(3, ds*20+25)+ds*20 < velocity/100) {
            g->add_msg(_("You fumble with the %s's controls."), name.c_str());
            int r;
            // TODO: Add somthing that changes velocity for r == 0
            do { r= dice(3, 3) - 6; } while( r == 0 );
            turn(r * 15);
        }
        if( velocity > one_in(ds*ds*20000+40000) ) {
            g->u.practice( g->turn, "driving", 1 );
        }
    } else {
        // cruise control TODO: enable for NPC?
        cruise_on = false;
    }

    // TODO: Maybe add engine strain again
    vehicle_forces vf;
    // TODO: Remove ter cost from these two
    if (skidding) {
        calculate_movement( &vf, 0.0, 0.0, g->m.move_cost_ter_furn(gx, gy));
    } else if (cruise_on) {
        calculate_movement( &vf, -2.0, cruise_velocity / 223.6, g->m.move_cost_ter_furn(gx, gy));
    } else {
        calculate_movement( &vf, player_thrust / 223.6, 0.0, g->m.move_cost_ter_furn(gx, gy));
    }
    velocity = int(vf.velocity_end * 223.6 + 0.5); // m/s to 100*mph

    // Less fuel use/noise if not running at full engine output
    double load = (fabs(vf.engine_watt_average) + power_to_epower(vf.eng_alt_pwr)) /
                           power_to_epower(vf.eng_pwr_max);
    // Assume engine is always running at min 1%
    if( load < 0.01 )
        load = 0.01;

    if( has_pedals && g->u.has_bionic("bio_torsionratchet") ) {
        // Charge one power for every 60 turns pedaling all out
        if( g->turn.get_turn() % 60 == 0 && load * vf.time_taken * 100 > rng(0, 100) )
            g->u.charge_power(1);
    }

    consume_fuel( load * vf.time_taken ); // Consume fuel
    noise_and_smoke( load, vf.time_taken ); // Make smoke and noise

    if( !vf.valid_wheel_config && ( cruise_on || player_thrust ) ) {
        if( pl_ctrl ) {
            if (velocity) {
                g->add_msg(_("The %s tries to change speed, but doesn't have enough wheels!"), name.c_str());
            } else {
                g->add_msg (_("The %s doesn't have enough wheels to move!"), name.c_str());
            }
        }
    }

    if (debug>=1) g->add_msg("New velocity = %d", int(vf.velocity_end));
    velocity = int(vf.velocity_end * 223.6 + 0.5);

    if (!skidding && fabs(turn_delta_per_tile) > 0.1) {
        int ds = g->u.skillLevel("driving");
        // c_n = kg * v^2 / r = kg * v^2 * tdpt / ( 180 * tilesize )
        double centrifugal_newton = cached_mass * velocity / 223.6 * velocity / 223.6 * turn_delta_per_tile / 180 / TILE_SIZE_M;
        double grip = ( vf.wheel_newton_max / 4 + vf.wheel_newton_available ) * TILE_SIZE_M;
        if (centrifugal_newton > grip) {
            if( dice(3, ds*5+5) > int(centrifugal_newton / grip * 100)) {
                g->add_msg(_("The %s's wheels lose their grip momentarily, but you quickly adjust the steering to compensate."), name.c_str());
                turn_delta_per_tile = 0;
            } else {
                start_skid( 0 );
            }
        } else {
            int td = int(turn_delta_per_tile + rng_float(0.0, 1.0) );
            if (abs(td) >= abs(turn_dir))
                td = turn_dir; // We've turned as much as we wanted to, so stop here
            turn_dir -= td;
            face.init( face.dir() + td );
            if (debug>=3) g->add_msg("M] Turned %d to %d.", int(td), face.dir());
            if ( abs(face.dir() - move.dir()) > 10 )
                move = face;
        }
    } else if (skidding && fabs(turn_delta_per_tile) > 0.1) {
        // We're skidding, absorb turning energy. Estimate moments of inertia 1000-8000 from:
        // http://www.nhtsa.gov/Research/Vehicle+Research+&+Testing+%28VRTC%29/ci.Crash+Avoidance+Data.print
        double moi = cached_mass * cached_mass / 600; // inaccurate, kg*m^2
        double grip = ( vf.wheel_newton_max / 40 + vf.wheel_newton_available ) * TILE_SIZE_M; // J
        double angular_momentum = turn_dir / 360.0 / TURN_TIME_S; // rad/s
        // kg*m^2/s^2 = kg*m^2 * rad/s * rad/s
        double kin_rot = 0.5 * moi * angular_momentum * angular_momentum;
        if (grip > kin_rot) {
            kin_rot = 0.0;
        } else {
            kin_rot -= grip;
        }
        angular_momentum = sqrt( 2 * kin_rot / moi );
        if (debug>=1) g->add_msg("Skidding! Pre-skid: f=%d m=%d t=%d td=%d", face.dir(), move.dir(), turn_dir, int(turn_delta_per_tile));
        turn_dir = int ( angular_momentum * 360 * TURN_TIME_S ) * ( turn_dir < 0 ? -1 : 1 );
        turn_delta_per_tile = turn_dir / (TURN_TIME_S * velocity / 223.6 / TILE_SIZE_M);
    }
    if (debug>=3) g->add_msg("f=%d m=%d t=%d td=%d", face.dir(), move.dir(), turn_dir, int(turn_delta_per_tile));

    return true;
}


// Helper sort function for vehicle::collision
bool _compare_veh_collisions_mass (const veh_collision& first, const veh_collision& second) {
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
bool vehicle::collision (int dx, int dy, bool just_detect)
{
    std::list<veh_collision> veh_veh_colls;
    std::list<veh_collision> veh_misc_colls;
    std::vector<int> structural_indices = all_parts_at_location(part_location_structure);
    for (int i = 0; i < structural_indices.size(); i++)
    {
        const int p = structural_indices[i];
        // coords of where part will go due to movement (dx/dy)
        // and turning (precalc_dx/dy [1])
        const int x = parts[p].precalc_dx[1];
        const int y = parts[p].precalc_dy[1];
        const int dsx = global_x() + dx + parts[p].precalc_dx[1];
        const int dsy = global_y() + dy + parts[p].precalc_dy[1];
        veh_collision coll = get_point_collision(dsx, dsy);  // TODO: Unsafe, fix
        coll.precalc_x = x;
        coll.precalc_y = y;
        if( coll.type != veh_coll_nothing && just_detect ) {
            return true;
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
    
    return false;
}

veh_collision vehicle::get_point_collision (int x, int y)
{
    veh_collision ret;
    ret.x = x;
    ret.y = y;
    ret.target_name = g->m.name(x, y).c_str();
    int target_part; // Not used here
    vehicle *oveh = g->m.veh_at(x, y, target_part);

    // Is it a vehicle collision?
    if (oveh && (oveh->posx != posx || oveh->posy != posy))
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
//        ret.target_name = z->name().c_str();
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
void vehicle::process_collision (veh_collision coll)
{
    std::vector<int> structural_indices = all_parts_at_location(part_location_structure);
    for (int i = 0; i < structural_indices.size(); i++)
    {
        const int p = structural_indices[i];
        if (coll.precalc_x == parts[p].precalc_dx[1] && coll.precalc_y == parts[p].precalc_dy[1]) {
            while (parts[p].hp > 0 && apply_damage_from_collision_to_point(p, coll));
        }
    }
}

bool vehicle::apply_damage_from_collision_to_point(int part, veh_collision coll)
{
    int mondex = g->mon_at(coll.x, coll.y);
    int npcind = g->npc_at(coll.x, coll.y);
    bool u_here = coll.x == g->u.posx && coll.y == g->u.posy && !g->u.in_vehicle;
    monster *z = mondex >= 0 ? &g->zombie(mondex) : NULL;
    player *ph = (npcind >= 0 ? g->active_npc[npcind] : (u_here? &g->u : 0));
                        
    float e = coll.elasticity;
    int x = coll.x, y = coll.y;
    bool pl_ctrl = player_in_control (&g->u);

    // Damage armor before damaging any other parts
    int parm = part_with_feature (part, VPFLAG_ARMOR);
    if (parm < 0)
        parm = part; // No armor found, use frame instead
    
    int dmg_mod = part_info(part).dmg_mod;
    float mass = total_mass();
    int degree = rng (70, 100);

    // Calculate damage resulting from d_E
    material_type* vpart_item_mat1 = material_type::find_material(itypes[part_info(parm).item]->m1);
    material_type* vpart_item_mat2 = material_type::find_material(itypes[part_info(parm).item]->m2);
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
    const float prev_velocity = velocity / 100;
    int turns_stunned = 0;

    do {
        // Impulse of object
        const float vel1 = velocity / 100;

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
        // damage for vehicle-part - only if not a hallucination
        if (!z || !z->is_hallucination()) {
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
                int z_armor = part_flag(part, "SHARP")? z->type->armor_cut : z->type->armor_bash;
                if (z_armor < 0) {
                    z_armor = 0;
                }
                dam -= z_armor;
            }
            if (dam < 0) { dam = 0; }

            // No blood from hallucinations
            if (z && !z->is_hallucination()) {
                if (part_flag(part, "SHARP")) {
                    parts[part].blood += (20 + dam) * 5;
                } else if (dam > rng (10, 30)) {
                    parts[part].blood += (10 + dam / 2) * 5;
                }
            }

            turns_stunned = rng (0, dam) > 10? rng (1, 2) + (dam > 40? rng (1, 2) : 0) : 0;
            if (part_flag(part, "SHARP")) {
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
                    g->fling_player_or_monster (0, z, move.dir() + angle, vel2_a);
                }
                if (z->hp < 1 || z->is_hallucination()) {
                    g->kill_mon (mondex, pl_ctrl);
                }
            } else {
                ph->hitall (dam, 40);
                if (vel2_a > rng (10, 20)) {
                    g->fling_player_or_monster (ph, 0, move.dir() + angle, vel2_a);
                }
            }
        }

        if (vel1_a*100 != velocity) {
            velocity = vel1_a*100;
        } else {
            velocity = 0;
        }

    } while (!smashed && velocity != 0);

    // Apply special effects from collision.
    if (!(coll.type == veh_coll_body)) {
        if (pl_ctrl) {
            if (snd.length() > 0) {
                g->add_msg (_("Your %s's %s rams into a %s with a %s"), name.c_str(),
                            part_info(part).name.c_str(), coll.target_name.c_str(), snd.c_str());
            } else {
                g->add_msg (_("Your %s's %s rams into a %s."), name.c_str(),
                            part_info(part).name.c_str(), coll.target_name.c_str());
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
                        name.c_str(), part_info(part).name.c_str(), dname.c_str(),
                        turns_stunned > 0 && z? _(" and stuns it") : "");
        }

        if (part_flag(part, "SHARP")) {
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
        if (turn_roll < abs(prev_velocity - (float)(velocity / 100)) * 2 ) {
            //delta_v = vel1 - vel1_a
            //delta_v = 50 mph -> 100% probability of skidding
            //delta_v = 25 mph -> 50% probability of skidding
            start_skid(one_in (2)? turn_amount : -turn_amount);
        }
    }
    damage (parm, part_dmg, 1);
    point collision_point(parts[part].mount_dx, parts[part].mount_dy);
    // Shock damage
    damage_all(part_dmg / 2, part_dmg, 1, collision_point);
    return false;
}

void vehicle::handle_trap (int x, int y, int part)
{
    int pwh = part_with_feature (part, VPFLAG_WHEEL);
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
    if (msg.size() > 0 && g->u_see(x, y)) {
        g->add_msg (msg.c_str(), name.c_str(), part_info(part).name.c_str(), g->traps[t]->name.c_str());
    }
    if (noise > 0) {
        g->sound(x, y, noise, snd);
    }
    if (wreckit && chance >= rng (1, 100)) {
        damage (part, 500);
    }
    if (expl > 0) {
        g->explosion(x, y, expl, shrap, false);
    }
}

// total volume of all the things
int vehicle::stored_volume(int part) {
   if (!part_flag(part, "CARGO")) {
        return 0;
   }
   int cur_volume = 0;
   for (int i = 0; i < parts[part].items.size(); i++) {
       cur_volume += parts[part].items[i].volume();
   }
   return cur_volume;
}

int vehicle::max_volume(int part) {
    if (part_flag(part, "CARGO")) {
        return vehicle_part_types[parts[part].id].size;
    }
    return 0;
}

// free space
int vehicle::free_volume(int part) {
   const int maxvolume = this->max_volume(part);
   return ( maxvolume - stored_volume(part) );
}

// returns true if full, modified by arguments:
// (none):                            size >= max || volume >= max
// (addvolume >= 0):                  size+1 > max || volume + addvolume > max
// (addvolume >= 0, addnumber >= 0):  size + addnumber > max || volume + addvolume > max
bool vehicle::is_full(const int part, const int addvolume, const int addnumber) {
   const int maxitems = MAX_ITEM_IN_VEHICLE_STORAGE;
   const int maxvolume = this->max_volume(part);

   if ( addvolume == -1 ) {
       if ( parts[part].items.size() < maxitems ) return true;
       int cur_volume=stored_volume(part);
       return (cur_volume >= maxvolume ? true : false );
   } else {
       if ( parts[part].items.size() + ( addnumber == -1 ? 1 : addnumber ) > maxitems ) return true;
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

    if (!part_flag(part, "CARGO")) {
        return false;
    }

    if (parts[part].items.size() >= max_storage)
        return false;
    it_ammo *ammo = dynamic_cast<it_ammo*> (itm.type);
    if (part_flag(part, "TURRET")) {
        if (!ammo || (ammo->type != part_info(part).fuel_type ||
                 ammo->type == fuel_type_gasoline ||
                 ammo->type == fuel_type_plasma)) {
            return false;
        }
    }
    int cur_volume = 0;
    int add_volume = itm.volume();
    bool tryaddcharges=(itm.charges  != -1 && (itm.is_food() || itm.is_ammo()));
    // iterate anyway since we need a volume total
      for (int i = 0; i < parts[part].items.size(); i++) {
        cur_volume += parts[part].items[i].volume();
        if( tryaddcharges && parts[part].items[i].type->id == itm.type->id ) {
          parts[part].items[i].charges+=itm.charges;
          return true;
        }
      }

    if ( cur_volume + add_volume > maxvolume ) {
      return false;
    }
    parts[part].items.push_back (itm);
    return true;
}

void vehicle::remove_item (int part, int itemdex)
{
    if (itemdex < 0 || itemdex >= parts[part].items.size())
        return;
    parts[part].items.erase (parts[part].items.begin() + itemdex);
}

void vehicle::place_spawn_items()
{
    for(std::vector<vehicle_item_spawn>::iterator next_spawn = item_spawns.begin();
            next_spawn != item_spawns.end(); next_spawn++) {
        if(rng(1, 100) <= next_spawn->chance) {
            //Find the cargo part in that square
            int part = part_at(next_spawn->x, next_spawn->y);
            part = part_with_feature(part, "CARGO", false);
            if(part < 0) {
                debugmsg("No CARGO parts at (%d, %d) of %s!",
                        next_spawn->x, next_spawn->y, name.c_str());
            } else {
                bool partbroken = ( parts[part].hp < 1 );
                int idmg = 0;
                for(std::vector<std::string>::iterator next_id = next_spawn->item_ids.begin();
                        next_id != next_spawn->item_ids.end(); next_id++) {
                    if ( partbroken ) {
                        int idmg = rng(1, 10);
                        if ( idmg > 5 ) {
                            continue;
                        }
                    }
                    item new_item = item_controller->create(*next_id, g->turn);
                    new_item = new_item.in_its_container(&(itypes));
                    if ( idmg > 0 ) {
                        new_item.damage = (signed char)idmg;
                    }
                    add_item(part, new_item);
                }
                for(std::vector<std::string>::iterator next_group_id = next_spawn->item_groups.begin();
                        next_group_id != next_spawn->item_groups.end(); next_group_id++) {
                    if ( partbroken ) {
                        int idmg = rng(1, 10);
                        if ( idmg > 5 ) {
                            continue;
                        }
                    }
                    Item_tag group_tag = item_controller->id_from(*next_group_id);
                    item new_item = item_controller->create(group_tag, g->turn);
                    new_item = new_item.in_its_container(&(itypes));
                    if ( idmg > 0 ) {
                        new_item.damage = (signed char)idmg;
                    }
                    add_item(part, new_item);
                }
            }
        }
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
    relative_parts.clear();
    lights_epower = 0;
    overhead_epower = 0;
    tracking_epower = 0;
    fridge_epower = 0;
    recharger_epower = 0;
    has_pedals = false;

    // Main loop over all vehicle parts.
    for (int p = 0; p < parts.size(); p++) {
        const vpart_info& vpi = part_info(p);
        if (vpi.has_flag(VPFLAG_LIGHT) || vpi.has_flag(VPFLAG_CONE_LIGHT)) {
            lights.push_back(p);
            lights_epower += vpi.epower;
        }
        if (vpi.has_flag(VPFLAG_CIRCLE_LIGHT)) overhead_epower += vpi.epower;
        if (vpi.has_flag(VPFLAG_TRACK)) tracking_epower += vpi.epower;
        if (vpi.has_flag(VPFLAG_FRIDGE)) fridge_epower += vpi.epower;
        if (vpi.has_flag(VPFLAG_RECHARGE)) recharger_epower += vpi.epower;
        if (vpi.has_flag(VPFLAG_ALTERNATOR)) alternators.push_back(p);
        if (vpi.has_flag(VPFLAG_FUEL_TANK)) fuel.push_back(p);
        if (vpi.has_flag(VPFLAG_ENGINE)) engines.push_back(p);
        if (vpi.has_flag(VPFLAG_FUEL_TANK) && vpi.fuel_type == fuel_type_plutonium)
            reactors.push_back(p);
        if (vpi.has_flag(VPFLAG_SOLAR_PANEL)) solar_panels.push_back(p);
        if (part_flag(p, "PEDALS")) has_pedals = true;
        // Build map of point -> all parts in that point
        point pt(parts[p].mount_dx, parts[p].mount_dy);
        if (relative_parts.find(pt) == relative_parts.end())
            relative_parts[pt].clear();
        relative_parts[pt].push_back(p);
    }

    total_mass(); // Mass in kg, takes some effort to find. Puts it in cached_mass
    calculate_air_resistance(); // Updates air_resistance and downforce

    precalc_mounts(0, face.dir());
    insides_dirty = true;
    has_environmental_effects = true; // Doesn't hurt (much) to check an extra time for these
    generation++; // So that it's possible for other functions to abort if parts change
}

void vehicle::refresh_insides()
{
    insides_dirty = false;
    for (int p = 0; p < parts.size(); p++) {
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
            std::vector<int> parts_n3ar = parts_at_relative (parts[p].mount_dx + ndx,
                                                                parts[p].mount_dy + ndy);
            bool cover = false; // if we aren't covered from sides, the roof at p won't save us
            for (int j = 0; j < parts_n3ar.size(); j++) {
                int pn = parts_n3ar[j];
                if (part_flag(pn, "ROOF") && parts[pn].hp > 0) { // another roof -- cover
                    cover = true;
                    break;
                }
                else
                if (part_flag(pn, "OBSTACLE") && parts[pn].hp > 0) {
                    // found an obstacle, like board or windshield or door
                    if (parts[pn].inside || (part_flag(pn, "OPENABLE") && parts[pn].open)) {
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

bool vehicle::is_inside (int p)
{
    if (p < 0 || p >= parts.size()) return false;
    if (insides_dirty) refresh_insides ();
    return parts[p].inside;
}

void vehicle::unboard_all ()
{
    std::vector<int> bp = boarded_parts ();
    for (int i = 0; i < bp.size(); i++) {
        g->m.unboard_vehicle (global_x() + parts[bp[i]].precalc_dx[0],
                              global_y() + parts[bp[i]].precalc_dy[0]);
    }
}

int vehicle::damage (int p, int dmg, int type, bool aimed)
{
    if (dmg < 1) {
        return dmg;
    }

    std::vector<int> pl = parts_at_relative(parts[p].mount_dx, parts[p].mount_dy);
    if (!aimed)
    {
        bool found_obs = false;
        for (int i = 0; i < pl.size(); i++)
            if (part_flag (pl[i], "OBSTACLE") &&
                (!part_flag (pl[i], "OPENABLE") || !parts[pl[i]].open))
            {
                found_obs = true;
                break;
            }
        if (!found_obs) // not aimed at this tile and no obstacle here -- fly through
            return dmg;
    }
    int parm = part_with_feature (p, "ARMOR");
    int pdm = pl[rng (0, pl.size()-1)];
    int dres;
    if (parm < 0)
        // not covered by armor -- damage part
        dres = damage_direct (pdm, dmg, type);
    else
    {
        // covered by armor -- damage armor first
        // half damage for internal part(over parts not covered)
        bool overhead = part_flag(pdm, "ROOF") ||
                        part_info(pdm).location == "on_roof";
        // Calling damage_direct may remove the damaged part
        // completely, therefor the other indes (pdm) becames
        // wrong if pdm > parm.
        // Damaging the part with the higher index first is save,
        // as removing a part only changes indizes after the
        // removed part.
        if (int(parm) < pdm) {
            damage_direct (pdm, overhead ? dmg : dmg / 2, type);
            dres = damage_direct (parm, dmg, type);
        } else {
            dres = damage_direct (parm, dmg, type);
            damage_direct (pdm, overhead ? dmg : dmg / 2, type);
        }
    }
    return dres;
}

void vehicle::damage_all (int dmg1, int dmg2, int type, const point &impact)
{
    if (dmg2 < dmg1) { std::swap(dmg1, dmg2); }
    if (dmg1 < 1) { return; }
    std::map<int, int> structure_to_damage;
    for (int p = 0; p < parts.size(); p++) {
        int distance = 1 + square_dist( parts[p].mount_dx, parts[p].mount_dy, impact.x, impact.y );
        if( distance > 1 && part_info(p).location == part_location_structure ) {
            structure_to_damage[p] = rng( dmg1, dmg2 ) / (distance * distance);
//            damage_direct (p, rng( dmg1, dmg2 ) / (distance * distance), type);
        }
    }
    for (std::map<int, int>::iterator i = structure_to_damage.begin(); i != structure_to_damage.end(); i++) {
      damage_direct(i->first, i->second, type);
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
void vehicle::shift_parts(const int dx, const int dy)
{
    for(int p = 0; p < parts.size(); p++) {
        parts[p].mount_dx -= dx;
        parts[p].mount_dy -= dy;
    }

    //Don't use the cache as it hasn't been updated yet
    std::vector<int> origin_parts = parts_at_relative(0, 0, false);

    posx += parts[origin_parts[0]].precalc_dx[0];
    posy += parts[origin_parts[0]].precalc_dy[0];

    refresh();

    //Need to also update the map after this
    g->m.reset_vehicle_cache();

}

int vehicle::damage_direct (int p, int dmg, int type)
{
    if (parts[p].hp <= 0) {
        /* Already-destroyed part - chance it could be torn off into pieces.
         * Chance increases with damage, and decreases with part max durability
         * (so lights, etc are easily removed; frames and plating not so much) */
        if(rng(0, part_info(p).durability / 10) < dmg) {
            int x_pos = global_x() + parts[p].precalc_dx[0];
            int y_pos = global_y() + parts[p].precalc_dy[0];
            if(part_info(p).location == part_location_structure) {
                //For structural parts, remove other parts first
                std::vector<int> parts_in_square = parts_at_relative(parts[p].mount_dx, parts[p].mount_dy);
                for(int index = parts_in_square.size(); index > 0; index--) {
                    //Ignore the frame being destroyed
                    if(parts_in_square[index] != p) {
                        if(g->u_see(x_pos, y_pos)) {
                            g->add_msg(_("The %s's %s is torn off!"), name.c_str(),
                                    part_info(parts_in_square[index]).name.c_str());
                        }
                        item part_as_item = item_from_part(parts_in_square[index]);
                        g->m.add_item_or_charges(x_pos, y_pos, part_as_item, true);
                        remove_part(parts_in_square[index]);
                    }
                }
                /* After clearing the frame, remove it if normally legal to do
                 * so (it's not holding the vehicle together). At a later date,
                 * some more complicated system (such as actually making two
                 * vehicles from the split parts) would be ideal. */
                if(can_unmount(p)) {
                    if(g->u_see(x_pos, y_pos)) {
                        g->add_msg(_("The %s's %s is destroyed!"),
                                name.c_str(), part_info(p).name.c_str());
                    }
                    break_part_into_pieces(p, x_pos, y_pos, true);
                    remove_part(p);
                }
            } else {
                //Just break it off
                if(g->u_see(x_pos, y_pos)) {
                    g->add_msg(_("The %s's %s is destroyed!"),
                                    name.c_str(), part_info(p).name.c_str());
                }
                break_part_into_pieces(p, x_pos, y_pos, true);
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
    if (dmg >= tsh || type != 1)
    {
        dres -= parts[p].hp;
        int last_hp = parts[p].hp;
        parts[p].hp -= dmg;
        if (parts[p].hp < 0)
            parts[p].hp = 0;
        if (!parts[p].hp && last_hp > 0)
            insides_dirty = true;
        if (part_flag(p, "FUEL_TANK"))
        {
            ammotype ft = part_info(p).fuel_type;
            if (ft == fuel_type_gasoline || ft == fuel_type_plasma)
            {
                int pow = parts[p].amount / 40;
    //            debugmsg ("damage check dmg=%d pow=%d", dmg, pow);
                if (parts[p].hp <= 0)
                    leak_fuel (p);
                if (type == 2 ||
                    (one_in (ft == fuel_type_gasoline ? 2 : 4) && pow > 5 && rng (75, 150) < dmg))
                {
                    g->u.add_memorial_log(pgettext("memorial_male","The fuel tank of the %s exploded!"),
                        pgettext("memorial_female", "The fuel tank of the %s exploded!"),
                        name.c_str());
                    g->explosion (global_x() + parts[p].precalc_dx[0], global_y() + parts[p].precalc_dy[0],
                                pow, 0, ft == fuel_type_gasoline);
                    parts[p].hp = 0;
                }
            }
        }
        else
        if (parts[p].hp <= 0 && part_flag(p, "UNMOUNT_ON_DAMAGE"))
        {
            g->m.spawn_item(global_x() + parts[p].precalc_dx[0],
                           global_y() + parts[p].precalc_dy[0],
                           part_info(p).item, 1, 0, g->turn);
            remove_part (p);
        }
    }
    if (dres < 0)
        dres = 0;
    return dres;
}

void vehicle::leak_fuel (int p)
{
    if (!part_flag(p, "FUEL_TANK"))
        return;
    ammotype ft = part_info(p).fuel_type;
    if (ft == fuel_type_gasoline)
    {
        int x = global_x();
        int y = global_y();
        for (int i = x - 2; i <= x + 2; i++)
            for (int j = y - 2; j <= y + 2; j++)
                if (g->m.move_cost(i, j) > 0 && one_in(2))
                {
                    if (parts[p].amount < 100)
                    {
                        parts[p].amount = 0;
                        return;
                    }
                    g->m.spawn_item(i, j, fuel_type_gasoline);
                    g->m.spawn_item(i, j, fuel_type_gasoline);
                    parts[p].amount -= 100;
                }
    }
    parts[p].amount = 0;
}

void vehicle::cycle_turret_mode()
{
    if (++turret_mode > 1) {
        turret_mode = 0;
    }
    g->add_msg((0 == turret_mode) ? _("Turrets: Disabled") : _("Turrets: Burst mode"));
}

bool vehicle::fire_turret (int p, bool burst)
{
    if (!part_flag (p, "TURRET"))
        return false;
    it_gun *gun = dynamic_cast<it_gun*> (itypes[part_info(p).item]);
    if (!gun) {
        return false;
    }
    long charges = burst? gun->burst : 1;
    std::string whoosh = "";
    if (!charges)
        charges = 1;
    ammotype amt = part_info (p).fuel_type;
    if (amt == fuel_type_gasoline || amt == fuel_type_plasma || amt == fuel_type_battery)
    {
        if (amt == fuel_type_gasoline) {
            charges = 20; // hacky
        } else if (amt == fuel_type_battery) {
            if (one_in(100)) {
                //~ the sound of a charge-rifle firing a massive ball of plasma
                whoosh = _("whoosh!");
                charges = rng(5,8); // kaboom
            } else {
                charges = rng(1,4);
            }
        }
        int fleft = fuel_left (amt);
        if (fleft < 1) {
            return false;
        }
        it_ammo *ammo = dynamic_cast<it_ammo*>(itypes[amt]);
        if (!ammo) {
            return false;
        }
        if (fire_turret_internal (p, *gun, *ammo, charges, whoosh)) {
            // consume fuel
            if (amt == fuel_type_plasma) {
                charges *= 10; // hacky, too
            } else if (amt == fuel_type_battery) {
                charges *= charges * 5;
            }
            for (int p = 0; p < parts.size(); p++) {
                if (part_flag(p, "FUEL_TANK") &&
                        part_info(p).fuel_type == amt &&
                        parts[p].amount > 0) {
                    parts[p].amount -= charges;
                    if (parts[p].amount < 0) {
                        parts[p].amount = 0;
                    }
                }
            }
        }
    } else {
        if (parts[p].items.size() > 0) {
            it_ammo *ammo = dynamic_cast<it_ammo*> (parts[p].items[0].type);
            if (!ammo || ammo->type != amt || parts[p].items[0].charges < 1) {
                return false;
            }
            if (charges > parts[p].items[0].charges) {
                charges = parts[p].items[0].charges;
            }
            const int power = fuel_left(fuel_type_battery);
            if (gun->item_tags.count( "USE_UPS" )) {
                if (power < 5) { return false; }
            } else if (gun->item_tags.count( "USE_UPS_20") ) {
                if (power < 20) { return false; }
            } else if (gun->item_tags.count( "USE_UPS_40") ) {
                if (power < 40) { return false; }
            }
            if (fire_turret_internal (p, *gun, *ammo, charges)) {
                // consume ammo
                if (charges >= parts[p].items[0].charges) {
                    parts[p].items.erase (parts[p].items.begin());
                } else {
                    parts[p].items[0].charges -= charges;
                }
            }
        }
    }
    return true;
}

bool vehicle::fire_turret_internal (int p, it_gun &gun, it_ammo &ammo, long charges, const std::string &extra_sound)
{
    int x = global_x() + parts[p].precalc_dx[0];
    int y = global_y() + parts[p].precalc_dy[0];
    // code copied form mattack::smg, mattack::flamethrower
    int range = ammo.type == fuel_type_gasoline ? 5 : 12;

    // Check for available power for turrets that use it.
    npc tmp;
    tmp.set_fake( true );
    tmp.name = rmp_format(_("<veh_player>The %s"), part_info(p).name.c_str());
    tmp.skillLevel(gun.skill_used).level(8);
    tmp.skillLevel("gun").level(4);
    tmp.recoil = abs(velocity) / 100 / 4;
    tmp.posx = x;
    tmp.posy = y;
    tmp.str_cur = 16;
    tmp.dex_cur = 8;
    tmp.per_cur = 12;
    tmp.weapon = item(&gun, 0);
    it_ammo curam = ammo;
    tmp.weapon.curammo = &curam;
    tmp.weapon.charges = charges;

    const bool u_see = g->u_see(x, y);

    int fire_t, boo_hoo;
    Creature *target = tmp.auto_find_hostile_target(range, boo_hoo, fire_t);
    if (target == NULL) {
        if (u_see) {
            if (boo_hoo > 1) {
                g->add_msg(_("%s points in your direction and emits %d annoyed sounding beeps."),
                tmp.name.c_str(), boo_hoo);
            } else if (boo_hoo > 0) {
                g->add_msg(_("%s points in your direction and emits an IFF warning beep."),
                tmp.name.c_str());
            }
        }
        return false;
    }

    // make a noise, if extra noise is to be made
    if (extra_sound != "") {
        g->sound(x, y, 20, extra_sound);
    }
    // notify player if player can see the shot
    if( g->u_see(x, y) ) {
        g->add_msg(_("The %s fires its %s!"), name.c_str(), part_info(p).name.c_str());
    }
    // Spawn a fake UPS to power any turreted weapons that need electricity.
    item tmp_ups( itypes["UPS_on"], 0 );
    // Drain a ton of power
    tmp_ups.charges = drain( fuel_type_battery, 1000 );
    item &ups_ref = tmp.i_add(tmp_ups);
    tmp.fire_gun(target->xpos(), target->ypos(), true);
    // Return whatever is left.
    refill( fuel_type_battery, ups_ref.charges );

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
               vehicle_part_types[parts[part_index].id].name.c_str(), name.c_str());
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
               vehicle_part_types[parts[part_index].id].name.c_str(), name.c_str());
  } else {
    open_or_close(part_index, false);
  }
}

void vehicle::open_or_close(int part_index, bool opening)
{
  parts[part_index].open = opening ? 1 : 0;
  insides_dirty = true;

  if(part_info(part_index).has_flag("MULTISQUARE")) {
    /* Find all other closed parts with the same ID in adjacent squares.
     * This is a tighter restriction than just looking for other Multisquare
     * Openable parts, and stops trunks from opening side doors and the like. */
    for(int next_index = 0; next_index < parts.size(); next_index++) {
      //Look for parts 1 square off in any cardinal direction
      int xdiff = parts[next_index].mount_dx - parts[part_index].mount_dx;
      int ydiff = parts[next_index].mount_dy - parts[part_index].mount_dy;
      if((xdiff * xdiff + ydiff * ydiff == 1) && // (x^2 + y^2) == 1
              (part_info(next_index).id == part_info(part_index).id) &&
              (parts[next_index].open == opening ? 0 : 1)) {
        open_or_close(next_index, opening);
      }
    }
  }
}

void vehicle::start_skid (int turn_deg)
{
    if (turn_deg != 0)
        turn(turn_deg);
    if (skidding) // Already skidding, don't print message
        return;
    skidding = true;
    if (player_in_control(&g->u))
        g->add_msg (_("You lose control of %s."), name.c_str());
}

// a chance to stop skidding if moving in roughly the faced direction
void vehicle::possibly_recover_from_skid () {
   if (!skidding)
       return;
   rl_vec2d mv = move_vec();
   rl_vec2d fv = face_vec();
   float dot = mv.dot_product(fv);
   //threshold of recovery is gaussianesque.

   if (100000 * fabs(dot) * fabs(dot) * fabs(dot) / turn_delta_per_tile / velocity > dice(9,20)) {
      g->add_msg(_("The %s recovers from its skid."), name.c_str());
      skidding = false; //face_vec takes over.
      velocity *= dot; //wheels absorb horizontal velocity.
      if (dot < -.3) {
         //pointed backwards, velo-wise.
         velocity *= -1; //move backwards.
      }
      move = face;
   }
}

// if not skidding, move_vec == face_vec, mv <dot> fv == 1, velocity*1 is returned.
float vehicle::forward_velocity(){
   rl_vec2d mv = move_vec();
   rl_vec2d fv = face_vec();
   float dot = mv.dot_product(fv);
   return velocity * dot;
}

rl_vec2d vehicle::velo_vec(){
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
rl_vec2d vehicle::move_vec(){
    float mx,my;
    mx = cos (move.dir() * M_PI/180);
    my = sin (move.dir() * M_PI/180);
    rl_vec2d ret(mx,my);
    return ret;
}

// normalized.
rl_vec2d vehicle::face_vec(){
    float fx,fy;
    fx = cos (face.dir() * M_PI/180);
    fy = sin (face.dir() * M_PI/180);
    rl_vec2d ret(fx,fy);
    return ret;
}

float get_collision_factor(float delta_v)
{
    if (abs(delta_v) <= 31) {
        return ( 1 - ( 0.9 * abs(delta_v) ) / 31 );
    } else {
        return 0.1;
    }
}

int vehicle::total_mass()
{
    int m = 0;
    for (int i = 0; i < parts.size(); i++)
    {
        m += itypes[part_info(i).item]->weight;
        for (int j = 0; j < parts[i].items.size(); j++) {
            m += parts[i].items[j].type->weight;
        }
        if (part_flag(i,VPFLAG_BOARDABLE) && parts[i].has_flag(vehicle_part::passenger_flag)) {
            m += 81500; // TODO: get real weight
        }
    }
    m = std::max(1, m/1000); // Convert to kg, avoid zero values
    cached_mass = m;
    return m;
}

void vehicle::center_of_mass(int &x, int &y)
{
    float xf = 0, yf = 0;
    int m_total = total_mass();
    for (int i = 0; i < parts.size(); i++)
    {
        int m_part = 0;
        m_part += itypes[part_info(i).item]->weight;
        for (int j = 0; j < parts[i].items.size(); j++) {
            m_part += parts[i].items[j].type->weight;
        }
        if (part_flag(i,VPFLAG_BOARDABLE) && parts[i].has_flag(vehicle_part::passenger_flag)) {
            m_part += 81500; // TODO: get real weight
        }
        xf += parts[i].precalc_dx[0] * m_part / 1000;
        yf += parts[i].precalc_dy[0] * m_part / 1000;
    }
    xf /= m_total;
    yf /= m_total;
    x = int(xf + 0.5); //round to nearest
    y = int(yf + 0.5);
}

float vehicle::wheels_area (int *cnt)
{
    int count = 0;
    int total_area = 0;
    std::vector<int> wheel_indices = all_parts_with_feature(VPFLAG_WHEEL);
    for (int i = 0; i < wheel_indices.size(); i++)
    {
        int p = wheel_indices[i];
        int width = part_info(p).wheel_width;
        int bigness = parts[p].bigness;
        // 9 inches, for reference, is about normal for cars.
        total_area += ((float)width/9) * bigness;
        count++;
    }
    if (cnt) {
        *cnt = count;
    }
    return total_area;
}

void vehicle::calculate_air_resistance()
{
    // Calculate air resistance
    int t_form_drag = 0;
    int t_skin_friction = 0;
    int t_downforce = 0;
    const int max_obst = 13;
    int obst[max_obst];
    for (int o = 0; o < max_obst; o++)
        obst[o] = 0;
    std::vector<int> structure_indices = all_parts_at_location(part_location_structure);
    t_skin_friction = structure_indices.size(); // Assume 1 skin friction per square
    for (int i = 0; i < structure_indices.size(); i++)
    {
        int p = structure_indices[i];
        int frame_size = part_with_feature(p, VPFLAG_OBSTACLE) ? 10 : 5;
        int pos = parts[p].mount_dy + max_obst / 2;
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
    int last_obst = 0;
    for (int o = 0; o < max_obst; o++) {
        t_form_drag += abs(obst[o]-last_obst); // Surface drag
        if (obst[o] > last_obst)
            t_downforce += 5; // Generate some downforce
        last_obst = obst[o];
    }
    t_form_drag += last_obst; // Last tile -> empty space
    // Some values with these calc. TODO: Add downforce settings to car parts, calc whole body
    // Car type        t_form_drag   t_downforce    t_skin_friction
    // Racecar body             10             5                  3
    // Sports car               20            10                 20
    // Flatbed truck            20            10                 30
    // apc                      20             5                 59
    // Aim for roughly these Cd * A values:
    // http://en.wikipedia.org/wiki/Automobile_drag_coefficient
    // http://www.rapid-racer.com/aerodynamics.php
    drag_coeff = (float(t_form_drag * t_skin_friction) + (t_skin_friction + 10) * (t_skin_friction + 10) + 500) / 3500; // Use i_skin_friction as area
    downforce = float(t_downforce) / 10;
}


