#include "vehicle.h"
#include "game.h"
#include "item_factory.h"
#include "catajson.h"

// GENERAL GUIDELINES
// When adding a new vehicle, you MUST REMEMBER to insert it in the vhtype_id enum
//  at the bottom of veh_type.h!
// also, before using PART, you MUST call VEHICLE
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
//  PART (0, 1, vp_seat);       // put a seat (it's external)
//  PART (0, 1, vp_controls);   // put controls for driver here
//  PART (0, 1, vp_seatbelt);   // also, put a seatbelt here
// To determine, what parts can be external, and what can not, check
//  vpart_id enum in veh_type.h file
// If you use wrong config, installation of part will fail

vpart_info vpart_list[num_vparts];

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

    next_part.name = next_json.get("name").as_string();
    next_part.sym = next_json.get("symbol").as_char();
    next_part.color = color_from_string(next_json.get("color").as_string());
    next_part.sym_broken = next_json.get("broken_symbol").as_char();
    next_part.color_broken = color_from_string(next_json.get("broken_color").as_string());
    next_part.dmg_mod = next_json.has("damage_modifier") ? next_json.get("damage_modifier").as_int() : 100;
    next_part.durability = next_json.get("durability").as_int();
    //Handle the par1 union as best we can by accepting any ONE of its elements
    int element_count = next_json.has("par1") ? 1 : 0
                      + next_json.has("power") ? 1 : 0
                      + next_json.has("size") ? 1 : 0
                      + next_json.has("wheel_width") ? 1 : 0
                      + next_json.has("bonus") ? 1 : 0;
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

    index++;
  }

  if(!json_good()) {
    exit(1);
  }

}

void game::init_vehicles()
{
    vehicle *veh;
    int index = 0;
    int pi;
    vtypes.push_back(new vehicle(this, (vhtype_id)index++)); // veh_null
    vtypes.push_back(new vehicle(this, (vhtype_id)index++)); // veh_custom

#define VEHICLE(nm) { veh = new vehicle(this, (vhtype_id)index++); veh->name = nm; vtypes.push_back(veh); }
#define PART(mdx, mdy, id) { pi = veh->install_part(mdx, mdy, id); \
    if (pi < 0) debugmsg("init_vehicles: '%s' part '%s'(%d) can't be installed to %d,%d", veh->name.c_str(), vpart_list[id].name.c_str(), veh->parts.size(), mdx, mdy); }

    //        name
    VEHICLE (_("Bicycle"));
    //    o
    //    #
    //    o

    //   dx, dy,    part_id
    PART (0, 0,     vp_frame_v2);
    PART (0, 0,     vp_saddle);
    PART (0, 0,     vp_controls);
    PART (0, 0,     vp_engine_foot_crank);
    PART (1, 0,     vp_wheel_bicycle);
    PART (-1, 0,    vp_wheel_bicycle);
    PART (-1, 0,    vp_cargo_box);

    //        name
    VEHICLE (_("Motorcycle Chassis"));
    //    o
    //    ^
    //    #
    //    o

    //   dx, dy,    part_id
    PART (0, 0,     vp_frame_v2);
    PART (0, 0,     vp_saddle);
    PART (1, 0,     vp_frame_handle);
    PART (1, 0,     vp_fuel_tank_gas);
    PART (-1, 0,    vp_wheel_motorbike);
    //        name
    VEHICLE (_("Motorcycle"));
    //    o
    //    ^
    //    #
    //    o

    //   dx, dy,    part_id
    PART (0, 0,     vp_frame_v2);
    PART (0, 0,     vp_saddle);
    PART (0, 0,     vp_controls);
    PART (0, 0,     vp_engine_gas_v2);
    PART (1, 0,     vp_frame_handle);
    PART (1, 0,     vp_head_light);
    PART (1, 0,     vp_fuel_tank_gas);
    PART (2, 0,     vp_wheel_motorbike);
    PART (-1, 0,    vp_wheel_motorbike);
    PART (-1, 0,    vp_cargo_box);

    //        name
    VEHICLE (_("Quad Bike"));
    //   0^0
    //    #
    //   0H0

    //   dx, dy,    part_id
    PART (0, 0,     vp_frame_v2);
    PART (0, 0,     vp_saddle);
    PART (0, 0,     vp_controls);
    PART (1, 0,     vp_frame_cover);
    PART (1, 0,     vp_engine_gas_v2);
    PART (1, 0,     vp_head_light);
    PART (1, 0,     vp_fuel_tank_gas);
    PART (1, 0,     vp_steel_plate);
    PART (-1,0,     vp_frame_h);
    PART (-1,0,     vp_cargo_trunk);
    PART (-1,0,     vp_steel_plate);
    PART (1, -1,    vp_wheel_motorbike);
    PART (1,  1,    vp_wheel_motorbike);
    PART (-1,-1,    vp_wheel_motorbike);
    PART (-1, 1,    vp_wheel_motorbike);

        //        name
    VEHICLE (_("Quad Bike Chassis"));
    //   0^0
    //    #
    //   0H0

    //   dx, dy,    part_id
    PART (0, 0,     vp_frame_v2);
    PART (0, 0,     vp_saddle);
    PART (1, 0,     vp_frame_cover);
    PART (-1,0,     vp_frame_h);
    PART (1, -1,    vp_wheel_motorbike);
    PART (-1,-1,    vp_wheel_motorbike);
    PART (-1, 1,    vp_wheel_motorbike);

    //        name
    VEHICLE (_("Car"));
    //   o--o
    //   |""|
    //   +##+
    //   +##+
    //   |HH|
    //   o++o

    //   dx, dy,    part_id
    PART (0, 0,     vp_frame_v2);
    PART (0, 0,     vp_seat);
    PART (0, 0,     vp_seatbelt);
    PART (0, 0,     vp_controls);
    PART (0, 0,     vp_roof);
    PART (0, 1,     vp_frame_v2);
    PART (0, 1,     vp_seat);
    PART (0, 1,     vp_seatbelt);
    PART (0, 1,     vp_roof);
    PART (0, -1,    vp_door);
    PART (0, 2,     vp_door);
    PART (-1, 0,     vp_frame_v2);
    PART (-1, 0,     vp_seat);
    PART (-1, 0,     vp_seatbelt);
    PART (-1, 0,     vp_roof);
    PART (-1, 1,     vp_frame_v2);
    PART (-1, 1,     vp_seat);
    PART (-1, 1,     vp_seatbelt);
    PART (-1, 1,     vp_roof);
    PART (-1, -1,    vp_door);
    PART (-1, 2,     vp_door);
    PART (1, 0,     vp_frame_h);
    PART (1, 0,     vp_window);
    PART (1, 0,     vp_head_light);
    PART (1, 1,     vp_frame_h);
    PART (1, 1,     vp_window);
    PART (1, 1,     vp_head_light);
    PART (1, -1,    vp_frame_v);
    PART (1, 2,     vp_frame_v);
    PART (2, 0,     vp_frame_h);
    PART (2, 0,     vp_engine_gas_v6);
    PART (2, 1,     vp_frame_h);
    PART (2, -1,    vp_wheel);
    PART (2, 2,     vp_wheel);
    PART (-2, 0,     vp_frame_v);
    PART (-2, 0,     vp_cargo_trunk);
    PART (-2, 0,     vp_muffler);
    PART (-2, 0,     vp_roof);
    PART (-2, 1,     vp_frame_v);
    PART (-2, 1,     vp_cargo_trunk);
    PART (-2, 1,     vp_roof);
    PART (-2, -1,    vp_board_v);
    PART (-2, -1,    vp_fuel_tank_gas);
    PART (-2, 2,     vp_board_v);
    PART (-3, -1,    vp_wheel);
    PART (-3, 0,     vp_door);
    PART (-3, 1,     vp_door);
    PART (-3, 2,     vp_wheel);

    //        name
    VEHICLE (_("Car Chassis"));
    //   o--o
    //   |""|
    //   +##+
    //   +##+
    //   |HH|
    //   o++o

    //   dx, dy,    part_id
    PART (0, 0,     vp_frame_v2);
    PART (0, 0,     vp_seat);
    PART (0, 1,     vp_frame_v2);
    PART (-1, 0,    vp_frame_v2);
    PART (-1, 1,    vp_frame_v2);
    PART (1, 0,     vp_frame_h);
    PART (1, 1,     vp_frame_h);
    PART (1, -1,    vp_frame_v);
    PART (1, 2,     vp_frame_v);
    PART (2, 0,     vp_frame_h);
    PART (2, 1,     vp_frame_h);
    PART (2, -1,    vp_wheel);
    PART (2, 2,     vp_wheel);
    PART (-2, 0,     vp_frame_v2);
    PART (-2, 1,     vp_frame_v2);
    PART (-2, -1,    vp_board_v);
    PART (-2, -1,    vp_fuel_tank_gas);
    PART (-2, 2,     vp_board_v);
    PART (-3, -1,    vp_wheel);
    PART (-3, 2,     vp_wheel);
    //        name
    VEHICLE (_("Flatbed Truck"));
    // 0-^-0
    // |"""|
    // +###+
    // |"""|
    // |HHH|
    // 0HHH0

    PART (0, 0,     vp_frame_v);
    PART (0, 0,     vp_cargo_box);
    PART (0, 0,     vp_roof);
//    PART (0, 0,     vp_seatbelt);
    PART (0, -1,    vp_frame_v2);
    PART (0, -1,    vp_seat);
    PART (0, -1,    vp_seatbelt);
    PART (0, -1,     vp_roof);
    PART (0, 1,     vp_frame_v2);
    PART (0, 1,     vp_seat);
    PART (0, 1,     vp_seatbelt);
    PART (0, 1,     vp_roof);
    PART (0, -2,    vp_door);
    PART (0, 2,     vp_door);
    PART (0, -1,     vp_controls);

    PART (1, 0,     vp_frame_h);
    PART (1, 0,     vp_window);
    PART (1, -1,    vp_frame_h);
    PART (1, -1,    vp_window);
    PART (1, -1,    vp_head_light);
    PART (1, 1,     vp_frame_h);
    PART (1, 1,     vp_window);
    PART (1, 1,    vp_head_light);
    PART (1, -2,    vp_frame_v);
    PART (1, 2,     vp_frame_v);

    PART (2, -1,    vp_frame_h);
    PART (2, 0,     vp_frame_cover);
    PART (2, 0,     vp_engine_gas_v6);
    PART (2, 1,     vp_frame_h);
    PART (2, -2,    vp_wheel_wide);
    PART (2,  2,    vp_wheel_wide);

    PART (-1, -1,   vp_frame_h);
    PART (-1, -1,   vp_window);
    PART (-1, 0,    vp_frame_h);
    PART (-1, 0,    vp_window);
    PART (-1, 1,    vp_frame_h);
    PART (-1, 1,    vp_window);
    PART (-1, -2,   vp_board_b);
    PART (-1, -2,   vp_fuel_tank_gas);
    PART (-1, 2,    vp_board_n);
    PART (-1, 2,    vp_fuel_tank_gas);

    PART (-2, -1,   vp_frame_v);
    PART (-2, -1,   vp_cargo_trunk);
    PART (-2, 0,    vp_frame_v);
    PART (-2, 0,    vp_cargo_trunk);
    PART (-2, 1,    vp_frame_v);
    PART (-2, 1,    vp_cargo_trunk);
    PART (-2, -2,   vp_board_v);
    PART (-2, 2,    vp_board_v);

    PART (-3, -1,   vp_frame_h);
    PART (-3, -1,   vp_cargo_trunk);
    PART (-3, 0,    vp_frame_h);
    PART (-3, 0,    vp_cargo_trunk);
    PART (-3, 1,    vp_frame_h);
    PART (-3, 1,    vp_cargo_trunk);
    PART (-3, -2,   vp_wheel_wide);
    PART (-3, 2,    vp_wheel_wide);

	VEHICLE (_("Semi Truck"));
	// semitrucksleeper
    // |=^^=|
    // O-HH-O
    // |""""|
    // +#oo#+
    // |--+-|
    // |#oo#|
    // |----|
    //  H||H
    // OO++OO
    // OO++OO
	// Based loosely on a Peterbilt Semi. 6L engine and 4 fuel tanks. 2 seater. Sleeper cab has zero visibility when opaque door is closed.

    // dx, dy, part_id
	PART (0, 0, vp_frame_v2);
	PART (0, 0, vp_cargo_box);
	PART (0, 0, vp_roof);
	PART (0, 1, vp_frame_v2);
	PART (0, 1, vp_bed);
	PART (0, 1, vp_roof);
	PART (0, -1, vp_frame_v2);
	PART (0, -1, vp_cargo_box);
	PART (0, -1, vp_roof);
	PART (0, 2, vp_board_v);
	PART (0, 2, vp_fuel_tank_gas);
	PART (0, -2, vp_frame_v2);
	PART (0, -2, vp_bed);
	PART (0, -2, vp_roof);
	PART (0, -3, vp_board_v);
	PART (0, -3, vp_fuel_tank_gas);

	PART (1, 0, vp_door_i);
	PART (1, -1, vp_board_h);
	PART (1, 1, vp_board_h);
	PART (1, -2, vp_board_h);
	PART (1, 2, vp_board_v);
	PART (1, 2, vp_fuel_tank_gas);
	PART (1, -3, vp_board_v);
	PART (1, -3, vp_fuel_tank_gas);

	PART (-1, 0, vp_board_h);
	PART (-1, 1, vp_board_h);
	PART (-1, -1, vp_board_h);
	PART (-1, -2, vp_board_h);
	PART (-1, 2, vp_board_n);
	PART (-1, -3, vp_board_b);

	PART (2, -1, vp_frame_h);
	PART (2, -1, vp_cargo_box);
	PART (2, -1, vp_roof);
	PART (2, 1, vp_frame_v2);
	PART (2, 1, vp_seat);
	PART (2, 1, vp_seatbelt);
	PART (2, 1, vp_roof);
	PART (2, -2, vp_frame_v2);
	PART (2, -2, vp_seat);
	PART (2, -2, vp_seatbelt);
	PART (2, -2, vp_roof);
	PART (2, 0, vp_frame_h);
	PART (2, 0, vp_cargo_box);
	PART (2, 0, vp_roof);
	PART (2, 2, vp_door);
	PART (2, -3, vp_door);
	PART (2, -2, vp_controls);

	PART (-2, 0, vp_frame_v);
	PART (-2, -1, vp_frame_v);
	PART (-2, 1, vp_frame_v2);
	PART (-2, 1, vp_cargo_trunk);
	PART (-2, -2, vp_frame_v2);
	PART (-2, -2, vp_cargo_trunk);


	PART (3, 0, vp_frame_h);
	PART (3, 0, vp_window);
	PART (3, -1, vp_frame_h);
	PART (3, -1, vp_window);
	PART (3, 1, vp_frame_h);
	PART (3, 1, vp_window);
	PART (3, -2, vp_frame_h);
	PART (3, -2, vp_window);
	PART (3, 2, vp_board_v);
	PART (3, -3, vp_board_v);

	PART (-3, 0, vp_frame_c);
	PART (-3, -1, vp_frame_c);
	PART (-3, 1, vp_wheel_wide);
	PART (-3, -2, vp_wheel_wide);
	PART (-3, 2, vp_wheel_wide);
	PART (-3, -3, vp_wheel_wide);

	PART (4, 0, vp_frame_v2);
	PART (4, -1, vp_frame_v2);
	PART (4, -1, vp_engine_gas_v8);
	PART (4, 1, vp_frame_h);
	PART (4, 1, vp_head_light);
	PART (4, -2, vp_frame_h);
	PART (4, -2, vp_head_light);
	PART (4, 2, vp_wheel_wide);
	PART (4, -3, vp_wheel_wide);

	PART (-4, 0, vp_frame_c);
	PART (-4, -1, vp_frame_c);
	PART (-4, 1, vp_wheel_wide);
	PART (-4, -2, vp_wheel_wide);
	PART (-4, 2, vp_wheel_wide);
	PART (-4, -3, vp_wheel_wide);

	PART (5, 0, vp_frame_cover);
	PART (5, -1, vp_frame_cover);
	PART (5, 1, vp_frame_h2);
	PART (5, -2, vp_frame_h2);
	PART (5, 2, vp_frame_u);
	PART (5, -3, vp_frame_y);

	VEHICLE (_("Truck Trailer"));
	// trucktrailer
    // |----|
    // |-++-|
    // |-++-|
    // |----|
    // |-HH-|
    // |----|
    // OO++OO
    // OO++OO
	// |----|
    // |-++-|
	// Pelletier trailer. Awaiting hitching of vehicles to each other...

	// dx, dy, part_id
	PART (0, 0, vp_frame_v2);
	PART (0, -1, vp_frame_v2);
	PART (0, 1, vp_frame_h);
	PART (0, -2, vp_frame_h);
	PART (0, 2, vp_board_v);
	PART (0, -3, vp_board_v);

	PART (1, 0, vp_frame_h);
	PART (1, -1, vp_frame_h);
	PART (1, 1, vp_frame_h);
	PART (1, -2, vp_frame_h);
	PART (1, 2, vp_board_v);
	PART (1, -3, vp_board_v);

	PART (-1, 0, vp_frame_c);
	PART (-1, -1, vp_frame_c);
	PART (-1, 1, vp_wheel_wide);
	PART (-1, -2, vp_wheel_wide);
	PART (-1, 2, vp_wheel_wide);
	PART (-1, -3, vp_wheel_wide);

	PART (2, 0, vp_frame_h);
	PART (2, -1, vp_frame_h);
	PART (2, 1, vp_frame_h);
	PART (2, -2, vp_frame_h);
	PART (2, 2, vp_board_v);
	PART (2, -3, vp_board_v);

	PART (-2, 0, vp_frame_c);
	PART (-2, -1, vp_frame_c);
	PART (-2, 1, vp_wheel_wide);
	PART (-2, -2, vp_wheel_wide);
	PART (-2, 2, vp_wheel_wide);
	PART (-2, -3, vp_wheel_wide);

	PART (3, 0, vp_frame_h);
	PART (3, -1, vp_frame_h);
	PART (3, 1, vp_frame_h);
	PART (3, -2, vp_frame_h);
	PART (3, 2, vp_board_v);
	PART (3, -3, vp_board_v);

	PART (-3, 0, vp_frame_h);
	PART (-3, -1, vp_frame_h);
	PART (-3, 1, vp_frame_h);
	PART (-3, -2, vp_frame_h);
	PART (-3, 2, vp_board_v);
	PART (-3, -3, vp_board_v);

	PART (4, 0, vp_frame_c);
	PART (4, -1, vp_frame_c);
	PART (4, 1, vp_frame_h);
	PART (4, -2, vp_frame_h);
	PART (4, 2, vp_board_v);
	PART (4, -3, vp_board_v);

	PART (-4, 0, vp_door_o);
	PART (-4, -1, vp_door_o);
	PART (-4, 1, vp_board_h);
	PART (-4, -2, vp_board_h);
	PART (-4, 2, vp_board_n);
	PART (-4, -3, vp_board_b);

	PART (5, 0, vp_board_h);
	PART (5, -1, vp_board_h);
	PART (5, 1, vp_board_h);
	PART (5, -2, vp_board_h);
	PART (5, 2, vp_board_u);
	PART (5, -3, vp_board_y);

        VEHICLE (_("Wagon"));
    // HHH
    // HHH
    // HHH

        PART (0, 0, vp_frame_v2);
        PART (0, 1, vp_frame_v2);
        PART (0, -1, vp_frame_v2);
        PART (1, 0, vp_frame_v2);
        PART (1, 1, vp_frame_v2);
        PART (1, -1, vp_frame_v2);
	PART (-1, 0, vp_frame_v2);
        PART (-1, 1, vp_frame_v2);
        PART (-1, -1, vp_frame_v2);

	VEHICLE (_("Beetle"));
	// vwbug
    // oHHo
    // |--|
    // +HH+
    // o\/o
	//Volkswagen Bug. Removed back seats entirely to make it feel smaller. Engine in back and cargo/fuel in front.

	// dx, dy, part_id
	PART (0, 0, vp_frame_v2);
	PART (0, 0, vp_seat);
	PART (0, 0, vp_seatbelt);
	PART (0, 0, vp_roof);
	PART (0, 0, vp_controls);
	PART (0, 1, vp_frame_v2);
	PART (0, 1, vp_seat);
	PART (0, 1, vp_seatbelt);
	PART (0, 1, vp_roof);
	PART (0, -1, vp_door);
	PART (0, 2, vp_door);

	PART (1, 0, vp_frame_h);
	PART (1, 0, vp_window);
        PART (1, 0, vp_head_light);
	PART (1, 1, vp_frame_h);
	PART (1, 1, vp_window);
        PART (1, 1, vp_head_light);
	PART (1, -1, vp_board_v);
	PART (1, 2, vp_board_v);

	PART (-1, 0, vp_frame_u);
	PART (-1, 0, vp_engine_gas_i4);
	PART (-1, 1, vp_board_y);
	PART (-1, -1, vp_wheel);
	PART (-1, 2, vp_wheel);

	PART (2, 0, vp_frame_v2);
	PART (2, 0, vp_cargo_trunk);
	PART (2, 1, vp_frame_v2);
	PART (2, 1, vp_fuel_tank_gas);
	PART (2, -1, vp_wheel);
	PART (2, 2, vp_wheel);

	VEHICLE (_("Bubble Car"));
    //  |-|
    // |o#o|
    // |###|
    // |oHo|
    //  +-+

	// dx, dy, part_id
	PART (0, 0, vp_frame_v2);
	PART (0, 0, vp_seat);
	PART (0, 0, vp_engine_motor);
	PART (0, 0, vp_fuel_tank_plut);
	PART (0, 0, vp_seatbelt);
	PART (0, 0, vp_roof);
	PART (0, 1, vp_frame_v2);
	PART (0, 1, vp_seat);
	PART (0, 1, vp_seatbelt);
	PART (0, 1, vp_roof);
	PART (0, -1, vp_frame_v2);
	PART (0, -1, vp_seat);
	PART (0, -1, vp_seatbelt);
	PART (0, -1, vp_roof);
	PART (0, 2, vp_frame_v);
	PART (0, 2, vp_window);
	PART (0, -2, vp_frame_v);
	PART (0, -2, vp_window);

	PART (1, 0, vp_frame_h);
	PART (1, 0, vp_seat);
	PART (1, 0, vp_seatbelt);
	PART (1, 0, vp_roof);
	PART (1, 0, vp_controls);
        PART (0, 0, vp_head_light);
	PART (1, 1, vp_wheel);
	PART (1, 1, vp_window);
	PART (1, -1, vp_wheel);
	PART (1, -1, vp_window);
	PART (1, 2, vp_frame_u);
	PART (1, 2, vp_window);
	PART (1, -2, vp_frame_y);
	PART (1, -2, vp_window);

	PART (-1, 0, vp_frame_h);
	PART (-1, 0, vp_cargo_trunk);
	PART (-1, 1, vp_wheel);
	PART (-1, 1, vp_window);
	PART (-1, -1, vp_wheel);
	PART (-1, -1, vp_window);
	PART (-1, 2, vp_door);
	PART (-1, -2, vp_door);

	PART (2, 0, vp_frame_h);
	PART (2, 0, vp_window);
	PART (2, 1, vp_frame_u);
	PART (2, 1, vp_window);
	PART (2, -1, vp_frame_y);
	PART (2, -1, vp_window);

	PART (-2, 0, vp_frame_h);
	PART (-2, 0, vp_window);
	PART (-2, 1, vp_frame_n);
	PART (-2, 1, vp_window);
	PART (-2, -1, vp_frame_b);
	PART (-2, -1, vp_window);

	VEHICLE (_("Golf Cart"));
	// Yamaha golf cart
    // oo
    // --
    // oo
	// Just an electric golf cart.

    // dx, dy, part_id
	PART (0, 0, vp_frame_h);
	PART (0, 0, vp_seat);
	PART (0, 0, vp_roof);
	PART (0, 0, vp_engine_motor);
	PART (0, 0, vp_controls);
	PART (0, 1, vp_frame_h);
	PART (0, 1, vp_seat);
	PART (0, 1, vp_roof);
	PART (0, 1, vp_fuel_tank_batt);

	PART (1, 0, vp_wheel_small);
	PART (1, 1, vp_wheel_small);

	PART (-1, 0, vp_wheel_small);
	PART (-1, 1, vp_wheel_small);

	VEHICLE (_("Scooter"));
	// Vespa scooter
    // o
    // ^
    // o
	// Just an underpowered gas scooter.

    // dx, dy, part_id
	PART (0, 0, vp_frame_handle);
	PART (0, 0, vp_head_light);
	PART (0, 0, vp_saddle);
	PART (0, 0, vp_engine_gas_1cyl);
	PART (0, 0, vp_fuel_tank_gas);
	PART (0, 0, vp_controls);

	PART (1, 0, vp_wheel_small);

	PART (-1, 0, vp_wheel_small);

	VEHICLE (_("Military Cargo Truck"));
	// Army M35A2 2.5 ton cargo truck
    // |^^^|
    // O-H-O
    // |"""|
    // +###+
    // |"""|
    // |#-#|
    // OO-OO
    // OO-OO
    // |#-#|
	// 3 seater. 6L engine default.

    // dx, dy, part_id
	PART (0, 0, vp_frame_v2);
	PART (0, 0, vp_window);
	PART (0, -1, vp_frame_h);
	PART (0, -1, vp_window);
	PART (0, 1, vp_frame_h);
	PART (0, 1, vp_window);
	PART (0, -2, vp_board_v);
	PART (0, 2, vp_board_v);

	PART (1, 0, vp_frame_v2);
	PART (1, 0, vp_seat);
	PART (1, 0, vp_fuel_tank_gas);
// 	PART (1, 0, vp_fuel_tank_hydrogen);
	PART (1, 0, vp_seatbelt);
	PART (1, 0, vp_roof);
	PART (1, -1, vp_frame_v2);
	PART (1, -1, vp_seat);
	PART (1, -1, vp_fuel_tank_gas);
//	PART (1, -1, vp_fuel_tank_hydrogen);
	PART (1, -1, vp_seatbelt);
	PART (1, -1, vp_roof);
	PART (1, -1, vp_controls);
	PART (1, 1, vp_frame_v2);
	PART (1, 1, vp_seat);
	PART (1, 1, vp_fuel_tank_gas);
//	PART (1, 1, vp_fuel_tank_hydrogen);
	PART (1, 1, vp_seatbelt);
	PART (1, 1, vp_roof);
	PART (1, -2, vp_door);
	PART (1, 2, vp_door);

	PART (-1, 0, vp_frame_h);
	PART (-1, -1, vp_frame_v2);
	PART (-1, -1, vp_seat);
	PART (-1, 1, vp_frame_v2);
	PART (-1, 1, vp_seat);
	PART (-1, -2, vp_frame_v);
	PART (-1, 2, vp_frame_v);

	PART (2, 0, vp_frame_h);
	PART (2, 0, vp_window);
	PART (2, -1, vp_frame_h);
	PART (2, -1, vp_window);
	PART (2, 1, vp_frame_h);
	PART (2, 1, vp_window);
	PART (2, -2, vp_frame_v);
	PART (2, 2, vp_frame_v);

	PART (-2, 0, vp_frame_h);
	PART (-2, -1, vp_wheel_wide);
	PART (-2, -1, vp_seat);
	PART (-2, -1, vp_steel_plate);
	PART (-2, 1, vp_wheel_wide);
	PART (-2, 1, vp_seat);
	PART (-2, 1, vp_steel_plate);
	PART (-2, -2, vp_wheel_wide);
	PART (-2, -2, vp_steel_plate);
	PART (-2, 2, vp_wheel_wide);
	PART (-2, 2, vp_steel_plate);

	PART (3, 0, vp_frame_v2);
	PART (3, -1, vp_frame_h);
	PART (3, -1, vp_head_light);
	PART (3, 1, vp_frame_h);
	PART (3, 1, vp_head_light);
	PART (3, 0, vp_engine_gas_v8);
	PART (3, 0, vp_steel_plate);
//	switch for hydrogen fuel or use both and change (3,0) to (3,1) and (3,-1)
//	PART (3, 0, vp_engine_plasma);
	PART (3, -2, vp_wheel_wide);
	PART (3, -2, vp_steel_plate);
	PART (3, 2, vp_wheel_wide);
	PART (3, 2, vp_steel_plate);

	PART (-3, 0, vp_frame_h);
	PART (-3, -1, vp_wheel_wide);
	PART (-3, -1, vp_seat);
	PART (-3, -1, vp_steel_plate);
	PART (-3, 1, vp_wheel_wide);
	PART (-3, 1, vp_seat);
	PART (-3, 1, vp_steel_plate);
	PART (-3, -2, vp_wheel_wide);
	PART (-3, -2, vp_steel_plate);
	PART (-3, 2, vp_wheel_wide);
	PART (-3, 2, vp_steel_plate);

	PART (4, 0, vp_frame_h2);
	PART (4, 0, vp_steel_plate);
	PART (4, -1, vp_frame_h2);
	PART (4, -1, vp_steel_plate);
	PART (4, 1, vp_frame_h2);
	PART (4, 1, vp_steel_plate);
	PART (4, -2, vp_frame_y);
	PART (4, -2, vp_steel_plate);
	PART (4, 2, vp_frame_u);
	PART (4, 2, vp_steel_plate);

	PART (-4, 0, vp_frame_h);
	PART (-4, -1, vp_frame_v2);
	PART (-4, -1, vp_seat);
	PART (-4, 1, vp_frame_v2);
	PART (-4, 1, vp_seat);
	PART (-4, -2, vp_frame_v);
	PART (-4, 2, vp_frame_v);

	VEHICLE (_("Schoolbus"));
	// Schoolbus
	// O=^=O
	// """""
	// "#..+
	// "#.#"
	// "#.#"
	// "#.#"
	// "#.#"
	// "#.#"
	// O#.#O
	// "#.#"
	// ""+""

    // dx, dy, part_id
	PART ( 0, 0, vp_frame_v2);
  PART ( 0, 0, vp_aisle_v2);
	PART ( 0, 0, vp_roof);
	PART ( 0, 1, vp_frame_v2);
  PART ( 0, 1, vp_aisle_h2);
	PART ( 0, 1, vp_roof);
	PART ( 0, 2, vp_door);
	PART ( 0, -1, vp_frame_v2);
	PART ( 0, -1, vp_seat);
	PART ( 0, -1, vp_controls);
	PART ( 0, -1, vp_roof);
	PART ( 0, -2, vp_frame_v);
	PART ( 0, -2, vp_window);

	PART ( 1, -2, vp_frame_h);
	PART ( 1, -2, vp_window);
	PART ( 1, -1, vp_frame_h);
	PART ( 1, -1, vp_window);
	PART ( 1, 0, vp_frame_h);
	PART ( 1, 0, vp_window);
	PART ( 1, 1, vp_frame_h);
	PART ( 1, 1, vp_window);
	PART ( 1, 2, vp_frame_h);
	PART ( 1, 2, vp_window);

	PART ( 2, -2, vp_wheel_wide);
	PART ( 2, -1, vp_frame_h2);
	PART ( 2, -1, vp_head_light);
	PART ( 2, 0, vp_frame_cover);
	PART ( 2, 0, vp_engine_gas_v8);
	PART ( 2, 1, vp_frame_h2);
	PART ( 2, 1, vp_head_light);
	PART ( 2, 2, vp_wheel_wide);

	PART ( -1, -2, vp_frame_v);
	PART ( -1, -2, vp_window);
	PART ( -1, -1, vp_frame_h2);
	PART ( -1, -1, vp_seat);
	PART ( -1, -1, vp_roof);
	PART ( -1, 0, vp_frame_v2);
  PART ( -1, 0, vp_aisle_v2);
	PART ( -1, 0, vp_roof);
	PART ( -1, 1, vp_frame_h2);
	PART ( -1, 1, vp_seat);
	PART ( -1, 1, vp_roof);
	PART ( -1, 2, vp_frame_v);
	PART ( -1, 2, vp_window);
	PART ( -1, 2, vp_fuel_tank_gas);

	PART ( -2, -2, vp_frame_v);
	PART ( -2, -2, vp_window);
	PART ( -2, -1, vp_frame_h2);
	PART ( -2, -1, vp_seat);
	PART ( -2, -1, vp_roof);
	PART ( -2, 0, vp_frame_v2);
  PART ( -2, 0, vp_floor_trunk);
	PART ( -2, 0, vp_roof);
	PART ( -2, 1, vp_frame_h2);
	PART ( -2, 1, vp_seat);
	PART ( -2, 1, vp_roof);
	PART ( -2, 2, vp_frame_v);
	PART ( -2, 2, vp_window);
	PART ( -2, 2, vp_fuel_tank_gas);

	PART ( -3, -2, vp_frame_v);
	PART ( -3, -2, vp_window);
	PART ( -3, -1, vp_frame_h2);
	PART ( -3, -1, vp_seat);
	PART ( -3, -1, vp_roof);
	PART ( -3, 0, vp_frame_v2);
  PART ( -3, 0, vp_aisle_v2);
	PART ( -3, 0, vp_roof);
	PART ( -3, 1, vp_frame_h2);
	PART ( -3, 1, vp_seat);
	PART ( -3, 1, vp_roof);
	PART ( -3, 2, vp_frame_v);
	PART ( -3, 2, vp_window);

	PART ( -4, -2, vp_frame_v);
	PART ( -4, -2, vp_window);
	PART ( -4, -1, vp_frame_h2);
	PART ( -4, -1, vp_seat);
	PART ( -4, -1, vp_roof);
	PART ( -4, 0, vp_frame_v2);
  PART ( -4, 0, vp_aisle_v2);
	PART ( -4, 0, vp_roof);
	PART ( -4, 1, vp_frame_h2);
	PART ( -4, 1, vp_seat);
	PART ( -4, 1, vp_roof);
	PART ( -4, 2, vp_frame_v);
	PART ( -4, 2, vp_window);

	PART ( -5, -2, vp_frame_v);
	PART ( -5, -2, vp_window);
	PART ( -5, -1, vp_frame_h2);
	PART ( -5, -1, vp_seat);
	PART ( -5, -1, vp_roof);
	PART ( -5, 0, vp_frame_v2);
  PART ( -5, 0, vp_floor_trunk);
	PART ( -5, 0, vp_roof);
	PART ( -5, 1, vp_frame_h2);
	PART ( -5, 1, vp_seat);
	PART ( -5, 1, vp_roof);
	PART ( -5, 2, vp_frame_v);
	PART ( -5, 2, vp_window);

	PART ( -6, -2, vp_wheel_wide);
	//	PART ( -6, -2, vp_window);
	PART ( -6, -1, vp_frame_h2);
	PART ( -6, -1, vp_seat);
	PART ( -6, -1, vp_roof);
	PART ( -6, 0, vp_frame_v2);
  PART ( -6, 0, vp_aisle_v2);
	PART ( -6, 0, vp_roof);
	PART ( -6, 1, vp_frame_h2);
	PART ( -6, 1, vp_seat);
	PART ( -6, 1, vp_roof);
	PART ( -6, 2, vp_wheel_wide);
	//	PART ( -6, 2, vp_window);

	PART ( -7, -2, vp_frame_v);
	PART ( -7, -2, vp_window);
	PART ( -7, -1, vp_frame_h2);
	PART ( -7, -1, vp_seat);
	PART ( -7, -1, vp_roof);
	PART ( -7, 0, vp_frame_v2);
  PART ( -7, 0, vp_aisle_v2);
	PART ( -7, 0, vp_roof);
	PART ( -7, 1, vp_frame_h2);
	PART ( -7, 1, vp_seat);
	PART ( -7, 1, vp_roof);
	PART ( -7, 2, vp_frame_v);
	PART ( -7, 2, vp_window);

	PART ( -8, -2, vp_frame_h);
	PART ( -8, -2, vp_window);
	PART ( -8, -1, vp_frame_h);
	PART ( -8, -1, vp_window);
	PART ( -8, 0, vp_door);
	PART ( -8, 1, vp_frame_h);
	PART ( -8, 1, vp_window);
	PART ( -8, 2, vp_frame_h);
	PART ( -8, 2, vp_window);

    //        name
    VEHICLE (_("Car"));
    //   o--o
    //   |""|
    //   +##+
    //   +##+
    //   #HH#
    //   o++o

    //   dx, dy,    part_id
    PART (0, 0,     vp_frame_v2);
    PART (0, 0,     vp_seat);
    PART (0, 0,     vp_seatbelt);
    PART (0, 0,     vp_controls);
    PART (0, 0,     vp_roof);
    PART (0, 1,     vp_frame_v2);
    PART (0, 1,     vp_seat);
    PART (0, 1,     vp_seatbelt);
    PART (0, 1,     vp_roof);
    PART (0, -1,    vp_door);
    PART (0, 2,     vp_door);
    PART (-1, 0,     vp_frame_v2);
    PART (-1, 0,     vp_seat);
    PART (-1, 0,     vp_seatbelt);
    PART (-1, 0,     vp_roof);
    PART (-1, 1,     vp_frame_v2);
    PART (-1, 1,     vp_seat);
    PART (-1, 1,     vp_seatbelt);
    PART (-1, 1,     vp_roof);
    PART (-1, -1,    vp_door);
    PART (-1, 2,     vp_door);
    PART (1, 0,     vp_frame_h);
    PART (1, 0,     vp_window);
    PART (1, 0,     vp_head_light);
    PART (1, 1,     vp_frame_h);
    PART (1, 1,     vp_window);
    PART (1, 1,     vp_head_light);
    PART (1, -1,    vp_frame_v);
    PART (1, 2,     vp_frame_v);
    PART (2, 0,     vp_frame_h);
    PART (2, 0,     vp_engine_motor_large);
    PART (2, 1,     vp_frame_h);
    PART (2, -1,    vp_wheel);
    PART (2, 2,     vp_wheel);
    PART (-2, 0,     vp_frame_v);
    PART (-2, 0,     vp_cargo_trunk);
    PART (-2, 0,     vp_roof);
    PART (-2, 1,     vp_frame_v);
    PART (-2, 1,     vp_cargo_trunk);
    PART (-2, 1,     vp_roof);
    PART (-2, -1,    vp_board_v);
    PART (-2, -1,    vp_fuel_tank_batt);
    PART (-2, -1,    vp_solar_panel);
    PART (-2, 2,     vp_board_v);
    PART (-2, 2,     vp_fuel_tank_batt);
    PART (-2, 2,     vp_solar_panel);
    PART (-3, -1,    vp_wheel);
    PART (-3, 0,     vp_door);
    PART (-3, 1,     vp_door);
    PART (-3, 2,     vp_wheel);

     VEHICLE ("RV");
    // - 0 +
    // O---O 2
    // |"""| 1
    // +#=#+ 0
    // |-+-| -1
    // |#H=+ -2
    // |OHO| -3
    // |OHO| -4
    // |#H&| -5
    // +---+ -6

    // dx, dy, part_id
    PART (0, 0, vp_frame_v2);
    PART (0, 0, vp_aisle_h2);
    PART (0, 0, vp_roof);
    PART (0, 0, vp_engine_gas_v6);
    PART (0, 1, vp_frame_v2);
    PART (0, 1, vp_seat);
    PART (0, 1, vp_roof);
    PART (0, 1, vp_seatbelt);
    PART (0, -1, vp_frame_v2);
    PART (0, -1, vp_seat);
    PART (0, -1, vp_roof);
    PART (0, -1, vp_controls);
    PART (0, -1, vp_seatbelt);
    PART (0, 2, vp_door);
    PART (0, -2, vp_door);

    PART (1, 0, vp_frame_v2);
    PART (1, 0, vp_window);
    PART (1, 1, vp_frame_v2);
    PART (1, 1, vp_window);
    PART (1, -1, vp_frame_v2);
    PART (1, -1, vp_window);
    PART (1, 2, vp_frame_v);
    PART (1, -2, vp_frame_v);

    PART (2, 0, vp_frame_h);
    PART (2, 0, vp_head_light);
    PART (2, 1, vp_frame_h);
    PART (2, 1, vp_head_light);
    PART (2, -1, vp_frame_h);
    PART (2, -1, vp_head_light);
    PART (2, 2, vp_wheel_wide);
    PART (2, -2, vp_wheel_wide);

    PART (-1, 0, vp_door_i);
    PART (-1, 1, vp_board_h);
    PART (-1, -1, vp_board_h);
    PART (-1, 2, vp_board_v);
    PART (-1, -2, vp_board_v);

    PART (-2, 1, vp_frame_v2);
    PART (-2, 1, vp_roof);
    PART (-2, 1, vp_aisle_h2);
    PART (-2, 0, vp_frame_v2);
    PART (-2, 0, vp_roof);
    PART (-2, 0, vp_aisle_v2);
    PART (-2, -1, vp_frame_v2);
    PART (-2, -1, vp_roof);
    PART (-2, -1, vp_seat);
    PART (-2, 2, vp_door);
    PART (-2, -2, vp_board_v);

    PART (-3, 0, vp_frame_v2);
    PART (-3, 0, vp_roof);
    PART (-3, 0, vp_aisle_v2);
    PART (-3, 1, vp_wheel_wide_under);
    PART (-3, 1, vp_cargo_trunk);
    PART (-3, 1, vp_roof);
    PART (-3, -1, vp_wheel_wide_under);
    PART (-3, -1, vp_cargo_trunk);
    PART (-3, -1, vp_roof);
    PART (-3, 2, vp_board_v);
    PART (-3, -2, vp_board_v);
    PART (-3, -2, vp_fuel_tank_water);

    PART (-4, 0, vp_frame_v2);
    PART (-4, 0, vp_roof);
    PART (-4, 0, vp_aisle_v2);
    PART (-4, 1, vp_wheel_wide_under);
    PART (-4, 1, vp_cargo_trunk);
    PART (-4, 1, vp_roof);
    PART (-4, -1, vp_wheel_wide_under);
    PART (-4, -1, vp_cargo_trunk);
    PART (-4, -1, vp_roof);
    PART (-4, 2, vp_board_v);
    PART (-4, -2, vp_board_v);
    PART (-4, -2, vp_fuel_tank_batt);

    PART (-5, 0, vp_frame_v2);
    PART (-5, 0, vp_roof);
    PART (-5, 0, vp_aisle_v2);
    PART (-5, 1, vp_frame_v2);
    PART (-5, 1, vp_roof);
    PART (-5, 1, vp_bed);
    PART (-5, -1, vp_frame_v2);
    PART (-5, -1, vp_kitchen_unit);
    PART (-5, -1, vp_roof);
    PART (-5, 2, vp_board_v);
    PART (-5, -2, vp_board_v);
    PART (-5, -2, vp_fuel_tank_gas);

    PART (-6, 0, vp_board_h);
    PART (-6, 1, vp_board_h);
    PART (-6, -1, vp_board_h);
    PART (-6, 2, vp_board_n);
    PART (-6, -2, vp_board_b);

    VEHICLE (_("Shopping Cart"));
    //    #

    //   dx, dy,    part_id
    PART (0, 0, vp_wheel_caster);
    PART (0, 0, vp_cargo_box);

    if (vtypes.size() != num_vehicles)
        debugmsg("%d vehicles, %d types", vtypes.size(), num_vehicles);
}

