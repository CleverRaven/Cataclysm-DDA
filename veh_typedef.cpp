#include "game.h"
#include "vehicle.h"
#include "item_factory.h"
#include "catajson.h"

#include "debug.h"
#include <exception>

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

std::vector<std::string> legacy_vpart_id; // potential FIXME: provide -static- hardcoded enum if vpart order shifts

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

    vehicle_part_types[next_part.id] = next_part;
    legacy_vpart_id.push_back(next_part.id);
  }

  if(!json_good()) {
    exit(1);
  }

}

void game::load_vehicle_part(JsonObject &jo)
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
    // TODO: Condense this into a method as part of JsonArray's functionality!
    JsonArray ja = jo.get_array("flags");
    std::set<std::string> flagset;
    if (ja.size() > 0)
    {
        while (ja.has_more())
        {
            flagset.insert(ja.next_string());
        }
    }
    // /End Condense Note
    next_part.flags = flagset;//jo.get("flags").as_tags(); // needs changing

    vehicle_part_types[next_part.id] = next_part;
    legacy_vpart_id.push_back(next_part.id);
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

void game::load_vehicle(JsonObject &jo)
{
    vehicle *next_vehicle;
    std::string part_id = "";
    int part_x = 0, part_y = 0;
DebugLog() << "Parsing Vehicle!\n";
DebugLog() << "Current game:" << "" << "\n";
    next_vehicle = new vehicle(this, jo.get_string("id").c_str());
DebugLog() << "--Vehicle Initialized\n";
    next_vehicle->name = _(jo.get_string("name").c_str());
DebugLog() << "--Name Obtained\n";
    JsonArray parts_list = jo.get_array("parts");
DebugLog() << "--ID: "<<next_vehicle->type << "\n--NAME: "<<next_vehicle->name << "\n--#Parts: "<<parts_list.size()<<"\n";
    if (parts_list.size() > 0)
    {
DebugLog() << "--Working through Parts!\n";
        while (parts_list.has_more())
        {
DebugLog() << "---::Parsing Part!\n";
            JsonObject next_part = parts_list.next_object();
DebugLog() << "---::Part Obtained\n";
            part_x = next_part.get_int("x");
            part_y = next_part.get_int("y");
            part_id = next_part.get_string("part");
DebugLog() << "---::Part info <"<<part_x<<", "<<part_y<<", \""<<part_id<<"\">\n";
            next_vehicle->part_cache.push(std::make_pair<point, std::string>(point(part_x, part_y), part_id));
            /*
            if (next_vehicle->install_part(part_x, part_y, part_id) < 0)
            {
                debugmsg("load_vehicle: '%s' part '%s'(%d) can't be installed to %d,%d",
                        next_vehicle->name.c_str(), part_id.c_str(),
                        next_vehicle->parts.size(), part_x, part_y);
            }
            */
        }
    }
DebugLog() << "--Adding Vehicle to vtypes! Veh is Real? "<<(next_vehicle != NULL ? "YES":"NO") << "\n";
DebugLog() << "--Game is still real? "<<(this != NULL ? "YES":"NO") << "\n";
    //vtypes[next_vehicle->type] = next_vehicle;
}

