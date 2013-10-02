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
std::vector<vehicle_prototype> vtype_cache;

// Note on the 'symbol' flag in vehicle parts -
// the following symbols will be translated:
// y, u, n, b to NW, NE, SE, SW lines correspondingly
// h, j, c to horizontal, vertical, cross correspondingly
/**
 * Reads in the vehicle parts from a json file.
 */

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
}

void game::cache_vehicles(JsonObject &jo)
{
    std::string part_id = "";
    int part_x = 0, part_y = 0;

    vehicle_prototype vproto;
    vproto.id = jo.get_string("id");
    vproto.name = jo.get_string("name");

    JsonArray parts_list = jo.get_array("parts");

    if (parts_list.size() > 0)
    {
        while (parts_list.has_more())
        {
            JsonObject next_part = parts_list.next_object();
            part_x = next_part.get_int("x");
            part_y = next_part.get_int("y");
            part_id = next_part.get_string("part");

            vproto.parts.push_back(std::make_pair<point, std::string>(point(part_x, part_y), part_id));
        }
    }


    vtype_cache.push_back(vproto); // load into memory, keep them here until time to finalize them.
}

void game::load_vehicle(vehicle_prototype &prototype)
{
    vehicle *next_vehicle;
    std::string part_id = "";
    int part_x = 0, part_y = 0;

    next_vehicle = new vehicle(this, prototype.id);
    next_vehicle->name = _(prototype.name.c_str());
    if (prototype.parts.size() > 0)
    {
        for (int i = 0; i < prototype.parts.size(); ++i)
        {
            part_x = prototype.parts[i].first.x;
            part_y = prototype.parts[i].first.y;
            part_id = prototype.parts[i].second;

            if (next_vehicle->install_part(part_x, part_y, part_id) < 0)
            {
                debugmsg("load_vehicle: '%s' part '%s'(%d) can't be installed to %d,%d",
                        next_vehicle->name.c_str(), part_id.c_str(),
                        next_vehicle->parts.size(), part_x, part_y);
            }
        }
    }
    vtypes[next_vehicle->type] = next_vehicle;
}
