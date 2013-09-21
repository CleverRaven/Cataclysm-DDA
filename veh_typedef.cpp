#include "vehicle.h"
#include "game.h"
#include "item_factory.h"
#include "catajson.h"

// GENERAL GUIDELINES
// When adding a new vehicle, you MUST REMEMBER to insert it in the vhtype_id enum
//  at the bottom of veh_type.h!
//
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

vpart_info vpart_list[num_vparts];

std::map<std::string, vpart_id> vpart_enums;

// Note on the 'symbol' flag in vehicle parts -
// the following symbols will be translated:
// y, u, n, b to NW, NE, SE, SW lines correspondingly
// h, j, c to horizontal, vertical, cross correspondingly
/**
 * Reads in the vehicle parts from a json file.
 */
void game::init_vehicle_parts()
{
  catajson vehicle_parts_json("data/raw/vehicle_parts.json", true);

  if (!json_good()) {
    throw (std::string)"data/raw/vehicle_parts.json wasn't found";
  }

  unsigned int index = 0;
  for (vehicle_parts_json.set_begin(); vehicle_parts_json.has_curr() && json_good(); vehicle_parts_json.next())
  {
    catajson next_json = vehicle_parts_json.curr();
    vpart_info next_part;

    next_part.id = next_json.get("id").as_string();
    next_part.name = _(next_json.get("name").as_string().c_str());
    next_part.sym = next_json.get("symbol").as_char();
    next_part.color = color_from_string(next_json.get("color").as_string());
    next_part.sym_broken = next_json.get("broken_symbol").as_char();
    next_part.color_broken = color_from_string(next_json.get("broken_color").as_string());
    next_part.dmg_mod = next_json.has("damage_modifier") ? next_json.get("damage_modifier").as_int() : 100;
    next_part.durability = next_json.get("durability").as_int();
    //Handle the par1 union as best we can by accepting any ONE of its elements
    int element_count = (next_json.has("par1") ? 1 : 0)
                      + (next_json.has("power") ? 1 : 0)
                      + (next_json.has("size") ? 1 : 0)
                      + (next_json.has("wheel_width") ? 1 : 0)
                      + (next_json.has("bonus") ? 1 : 0);
    if(element_count == 0) {
      //If not specified, assume 0
      next_part.par1 = 0;
    } else if(element_count == 1) {
      if(next_json.has("par1")) {
        next_part.par1 = next_json.get("par1").as_int();
      } else if(next_json.has("power")) {
        next_part.par1 = next_json.get("power").as_int();
      } else if(next_json.has("size")) {
        next_part.par1 = next_json.get("size").as_int();
      } else if(next_json.has("wheel_width")) {
        next_part.par1 = next_json.get("wheel_width").as_int();
      } else { //bonus
        next_part.par1 = next_json.get("bonus").as_int();
      }
    } else {
      //Too many
      debugmsg("Error parsing vehicle part '%s': \
               Use AT MOST one of: par1, power, size, wheel_width, bonus",
               next_part.name.c_str());
      //Keep going to produce more messages if other parts are wrong
      next_part.par1 = 0;
    }
    next_part.fuel_type = next_json.has("fuel_type") ? next_json.get("fuel_type").as_string() : "NULL";
    next_part.item = next_json.get("item").as_string();
    next_part.difficulty = next_json.get("difficulty").as_int();
    next_part.flags = next_json.get("flags").as_tags();

    vpart_list[index] = next_part;
    vpart_enums[next_part.id] = (vpart_id)index; // temporary for saves, until move to string vpart id
    index++;
  }

  if(!json_good()) {
    exit(1);
  }

}

void game::init_vehicles()
{
  catajson vehicles_json("data/raw/vehicles.json", true);

  if (!json_good()) {
    throw (std::string)"data/raw/vehicles.json wasn't found";
  }

  int part_x = 0;
  int part_y = 0;
  std::string part_id;
  vehicle *next_vehicle;
  for (vehicles_json.set_begin(); vehicles_json.has_curr() && json_good(); vehicles_json.next())
  {
    catajson next_json = vehicles_json.curr();

    next_vehicle = new vehicle(this, next_json.get("id").as_string().c_str());
    next_vehicle->name = _(next_json.get("name").as_string().c_str());
    catajson parts_list = next_json.get("parts");

    for(parts_list.set_begin(); parts_list.has_curr() && json_good(); parts_list.next()) {
      
      //See vehicle_parts.json for a list of part ids
      catajson next_part = parts_list.curr();
      part_x = next_part.get("x").as_int();
      part_y = next_part.get("y").as_int();
      part_id = next_part.get("part").as_string();
      if(next_vehicle->install_part(part_x, part_y, part_id) < 0) {
        debugmsg("init_vehicles: '%s' part '%s'(%d) can't be installed to %d,%d",
                    next_vehicle->name.c_str(), part_id.c_str(),
                    next_vehicle->parts.size(), part_x, part_y);
      }
      
    }

    vtypes[next_vehicle->type] = next_vehicle;

  }
}

