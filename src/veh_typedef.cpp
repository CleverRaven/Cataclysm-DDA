#include "vehicle.h"
#include "game.h"
#include "item_factory.h"
#include "json.h"

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
std::vector<vpart_info> vehicle_part_int_types; // rapid lookup, for part_info etc

std::map<std::string, vpart_bitflags> vpart_bitflag_map; // for data/json loading

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
    if(jo.has_member("power")) {
        next_part.power = jo.get_int("power");
    } else { //defaults to 0
        next_part.power = 0;
    }
    //Handle the par1 union as best we can by accepting any ONE of its elements
    int element_count = (jo.has_member("par1") ? 1 : 0)
                      + (jo.has_member("size") ? 1 : 0)
                      + (jo.has_member("wheel_width") ? 1 : 0)
                      + (jo.has_member("bonus") ? 1 : 0);

    if(element_count == 0) {
      //If not specified, assume 0
      next_part.par1 = 0;
    } else if(element_count == 1) {
      if(jo.has_member("par1")) {
        next_part.par1 = jo.get_int("par1");
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

    next_part.bitflags = 0;
    JsonArray jarr = jo.get_array("flags");
    std::string nstring="";
    while (jarr.has_more()){
        nstring=jarr.next_string();
        next_part.flags.insert(nstring);
        if ( vpart_bitflag_map.find(nstring) != vpart_bitflag_map.end() ) {
            next_part.bitflags |= mfb( vpart_bitflag_map.find(nstring)->second );
        }
    }

    JsonArray breaks_into = jo.get_array("breaks_into");
    while(breaks_into.has_more()) {
        JsonObject next_entry = breaks_into.next_object();
        break_entry next_break_entry;
        next_break_entry.item_id = next_entry.get_string("item");
        next_break_entry.min = next_entry.get_int("min");
        next_break_entry.max = next_entry.get_int("max");
        //Sanity check
        if(next_break_entry.max < next_break_entry.min) {
            debugmsg("For vehicle part %s: breaks_into item '%s' has min (%d) > max (%d)!",
                             next_part.name.c_str(), next_break_entry.item_id.c_str(),
                             next_break_entry.min, next_break_entry.max);
        }
        next_part.breaks_into.push_back(next_break_entry);
    }

    //Calculate and cache z-ordering based off of location
    if(next_part.location == "on_roof") {
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
    } else if(next_part.location == "armor") {
        //Shouldn't be displayed (the color is used, but not the symbol)
        next_part.z_order = -2;
    } else {
        //Everything else
        next_part.z_order = 0;
    }

    next_part.loadid = vehicle_part_int_types.size();

    vehicle_part_types[next_part.id] = next_part;
    vehicle_part_int_types.push_back(next_part);
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

    std::map<point, bool> cargo_spots;

    JsonArray parts = jo.get_array("parts");
    point pxy;
    std::string pid;
    while (parts.has_more()){
        JsonObject part = parts.next_object();
        pxy = point(part.get_int("x"), part.get_int("y"));
        pid = part.get_string("part");
        vproto->parts.push_back(std::pair<point, std::string>(pxy, pid));
        if ( vehicle_part_types[pid].has_flag("CARGO") ) {
            cargo_spots[pxy] = true;
        }
    }

    JsonArray items = jo.get_array("items");
    while(items.has_more()) {
        JsonObject spawn_info = items.next_object();
        vehicle_item_spawn next_spawn;
        next_spawn.x = spawn_info.get_int("x");
        next_spawn.y = spawn_info.get_int("y");
        next_spawn.chance = spawn_info.get_int("chance");
        if(next_spawn.chance <= 0 || next_spawn.chance > 100) {
            debugmsg("Invalid spawn chance in %s (%d, %d): %d%%",
                vproto->name.c_str(), next_spawn.x, next_spawn.y, next_spawn.chance);
        } else if ( cargo_spots.find( point(next_spawn.x, next_spawn.y) ) == cargo_spots.end() ) {
            debugmsg("Invalid spawn location (no CARGO vpart) in %s (%d, %d): %d%%",
                vproto->name.c_str(), next_spawn.x, next_spawn.y, next_spawn.chance);
        }
        if(spawn_info.has_array("items")) {
            //Array of items that all spawn together (ie jack+tire)
            JsonArray item_group = spawn_info.get_array("items");
            while(item_group.has_more()) {
                next_spawn.item_ids.push_back(item_group.next_string());
            }
        } else if(spawn_info.has_string("items")) {
            //Treat single item as array
            next_spawn.item_ids.push_back(spawn_info.get_string("items"));
        }
        if(spawn_info.has_array("item_groups")) {
            //Pick from a group of items, just like map::place_items
            JsonArray item_group_names = spawn_info.get_array("item_groups");
            while(item_group_names.has_more()) {
                next_spawn.item_groups.push_back(item_group_names.next_string());
            }
        } else if(spawn_info.has_string("item_groups")) {
            next_spawn.item_groups.push_back(spawn_info.get_string("item_groups"));
        }
        vproto->item_spawns.push_back(next_spawn);
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

        for (int i = 0; i < proto->item_spawns.size(); i++) {
            next_vehicle->item_spawns.push_back(proto->item_spawns[i]);
        }

        vtypes[next_vehicle->type] = next_vehicle;
        delete proto;
    }
}

void init_vpart_bitflag_map() {
    vpart_bitflag_map["ARMOR"]=VPFLAG_ARMOR;               // (!!!) map::draw
    vpart_bitflag_map["TRANSPARENT"]=VPFLAG_TRANSPARENT;   // (!!!) map::draw
    vpart_bitflag_map["EVENTURN"]=VPFLAG_EVENTURN;         // (!!!) lightmap
    vpart_bitflag_map["ODDTURN"]=VPFLAG_ODDTURN;           // ""
    vpart_bitflag_map["CONE_LIGHT"]=VPFLAG_CONE_LIGHT;     // ""
    vpart_bitflag_map["CIRCLE_LIGHT"]=VPFLAG_CIRCLE_LIGHT; // ""
    vpart_bitflag_map["BOARDABLE"]=VPFLAG_BOARDABLE;
    vpart_bitflag_map["AISLE"]=VPFLAG_AISLE;               // (!!!) map::move_cost
    vpart_bitflag_map["CONTROLS"]=VPFLAG_CONTROLS;
    vpart_bitflag_map["OBSTACLE"]=VPFLAG_OBSTACLE;         // (!!!) map::move_cost
    vpart_bitflag_map["OPAQUE"]=VPFLAG_OPAQUE;             // (!!!) map::trans
    vpart_bitflag_map["OPENABLE"]=VPFLAG_OPENABLE;
    vpart_bitflag_map["SEATBELT"]=VPFLAG_SEATBELT;         // crashes
    vpart_bitflag_map["WHEEL"]=VPFLAG_WHEEL;
    vpart_bitflag_map["ALTERNATOR"]=VPFLAG_ALTERNATOR;
    vpart_bitflag_map["ENGINE"]=VPFLAG_ENGINE;
    vpart_bitflag_map["FRIDGE"]=    VPFLAG_FRIDGE;
    vpart_bitflag_map["FUEL_TANK"]= VPFLAG_FUEL_TANK;
    vpart_bitflag_map["LIGHT"]=     VPFLAG_LIGHT;
    vpart_bitflag_map["WINDOW"]=     VPFLAG_WINDOW;
    vpart_bitflag_map["CURTIAN"]=     VPFLAG_CURTIAN;
    vpart_bitflag_map["CARGO"]=     VPFLAG_CARGO;   
    vpart_bitflag_map["SOLAR_PANEL"]=     VPFLAG_SOLAR_PANEL;   
/*    vpart_bitflag_map["SWIMMABLE"] = VPFLAG_SWIMMABLE; */ // only relevent for cars in water

}
