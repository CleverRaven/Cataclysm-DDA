#include "vehicle.h"
#include "game.h"
#include "item_factory.h"
#include "catajson.h"

// GENERAL GUIDELINES
// To determine mount position for parts (dx, dy), check this scheme:
//         orthogonal dir left: (Y-)
//                ^
//  back: X-   -------> forward dir: X+
//                v
//         orthogonal dir right (Y+)
//
// i.e, if you want to add a part to the back from the center of vehicle,
// use dx = -1, dy = 0;
// for the part 1 tile forward and two tiles left from the center of vehicle,
// use dx = 1, dy = -2.
//
// Internal parts should be added after external on the same mount point, i.e:
//  part {"x": 0, "y": 1, "part": "seat"},       // put a seat (it's external)
//  part {"x": 0, "y": 1, "part": "controls"},   // put controls for driver here
//  part {"x": 0, "y": 1, "seatbelt"}   // also, put a seatbelt here
// To determine, what parts can be external, and what can not, check
// vehicle_parts.json
// If you use wrong config, installation of part will fail

std::map<std::string, vpart_info> vehicle_part_types;

// Note on the 'symbol' flag in vehicle parts -
// the following symbols will be translated:
// y, u, n, b to NW, NE, SE, SW lines correspondingly
// h, j, c to horizontal, vertical, cross correspondingly
/**
 * Reads in a vehicle part from a JsonObject.
 */
void game::load_vehiclepart(JsonObject &jo)
{
    vpart_info next_part;

    next_part.id = jo.get_string("id");
    next_part.name = _(jo.get_string("name").c_str());
    next_part.sym = jo.get_string("symbol")[0];
    next_part.color = color_from_string(jo.get_string("color"));
    next_part.sym_broken = jo.get_string("broken_symbol")[0];
    next_part.color_broken = color_from_string(jo.get_string("broken_color"));
    next_part.dmg_mod = jo.has_member("damage_modifier") ? jo.get_int("damage_modifier") : 100;
    next_part.durability = jo.get_int("durability");
    //Handle the par1 union as best we can by accepting any ONE of its elements
    int element_count = (jo.has_member("par1") ? 1 : 0)
                      + (jo.has_member("power") ? 1 : 0)
                      + (jo.has_member("size") ? 1 : 0)
                      + (jo.has_member("wheel_width") ? 1 : 0)
                      + (jo.has_member("bonus") ? 1 : 0);
    if(element_count == 0) {
      //If not specified, assume 0
      next_part.par1 = 0;
    } else if(element_count == 1) {
      if(jo.has_member("par1")) {
        next_part.par1 = jo.get_int("par1");
      } else if(jo.has_member("power")) {
        next_part.par1 = jo.get_int("power");
      } else if(jo.has_member("size")) {
        next_part.par1 = jo.get_int("size");
      } else if(jo.has_member("wheel_width")) {
        next_part.par1 = jo.get_int("wheel_width");
      } else { //bonus
        next_part.par1 = jo.get_int("bonus");
      }
    } else {
      //Too many
      debugmsg("Error parsing vehicle part '%s': \
               Use AT MOST one of: par1, power, size, wheel_width, bonus",
               next_part.name.c_str());
      //Keep going to produce more messages if other parts are wrong
      next_part.par1 = 0;
    }
    next_part.fuel_type = jo.has_member("fuel_type") ? jo.get_string("fuel_type") : "NULL";
    next_part.item = jo.get_string("item");
    next_part.difficulty = jo.get_int("difficulty");
    next_part.location = jo.has_member("location") ? jo.get_string("location") : "";

    JsonArray jarr = jo.get_array("flags");
    while (jarr.has_more()){
        next_part.flags.insert(jarr.next_string());
    }

    //Plating shouldn't actually be shown; another part will be.
    //Calculate and cache z-ordering based off of location
    if(next_part.has_flag("ARMOR")) {
        next_part.z_order = -2;
    } else if(next_part.location == "on_roof") {
        next_part.z_order = 8;
    } else if(next_part.location == "center") {
        next_part.z_order = 7;
    } else if(next_part.location == "under") {
        //Have wheels show up over frames
        next_part.z_order = 6;
    } else if(next_part.location == "structure") {
        next_part.z_order = 5;
    } else if(next_part.location == "engine_block") {
        //Should be hidden by frames
        next_part.z_order = 4;
    } else if(next_part.location == "fuel_source") {
        //Should be hidden by frames
        next_part.z_order = 3;
    } else if(next_part.location == "roof") {
        //Shouldn't be displayed
        next_part.z_order = -1;
    } else {
        //Everything else
        next_part.z_order = 0;
    }

    vehicle_part_types[next_part.id] = next_part;
}

/**
 *Caches a vehicle definition from a JsonObject to be loaded after itypes is initialized.
 */
// loads JsonObject vehicle definition into a cached state so that it can be held until after itypes have been initialized
void game::load_vehicle(JsonObject &jo)
{
    vehicle_prototype *vproto = new vehicle_prototype;

    vproto->id = jo.get_string("id");
    vproto->name = jo.get_string("name");
    JsonArray parts = jo.get_array("parts");

    while (parts.has_more()){
        JsonObject part = parts.next_object();
        vproto->parts.push_back(std::pair<point, std::string>(point(part.get_int("x"), part.get_int("y")), part.get_string("part")));
    }
    vehprototypes.push(vproto);
}
/**
 *Works through cached vehicle definitions and creates vehicle objects from them.
 */
void game::finalize_vehicles()
{
    int part_x = 0, part_y = 0;
    std::string part_id = "";
    vehicle *next_vehicle;

    while (vehprototypes.size() > 0){
        vehicle_prototype *proto = vehprototypes.front();
        vehprototypes.pop();

        next_vehicle = new vehicle(this, proto->id.c_str());
        next_vehicle->name = _(proto->name.c_str());

        for (int i = 0; i < proto->parts.size(); ++i)
        {
            point p = proto->parts[i].first;
            part_x = p.x;
            part_y = p.y;

            part_id = proto->parts[i].second;

            if(next_vehicle->install_part(part_x, part_y, part_id) < 0) {
                debugmsg("init_vehicles: '%s' part '%s'(%d) can't be installed to %d,%d",
                        next_vehicle->name.c_str(), part_id.c_str(),
                        next_vehicle->parts.size(), part_x, part_y);
            }
        }

        vtypes[next_vehicle->type] = next_vehicle;
        delete proto;
    }
}
