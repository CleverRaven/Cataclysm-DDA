#include "vehicle.h"
#include "game.h"

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

void game::init_vehicles()
{
    vehicle *veh;
    int index = 0;
    int pi;
    vtypes.push_back(new vehicle(this, (vhtype_id)index++)); // veh_null
    vtypes.push_back(new vehicle(this, (vhtype_id)index++)); // veh_custom

#define VEHICLE(nm) { veh = new vehicle(this, (vhtype_id)index++); veh->name = nm; vtypes.push_back(veh); }
#define PART(mdx, mdy, id) { pi = veh->install_part(mdx, mdy, id); \
    if (pi < 0) debugmsg("init_vehicles: '%s' part '%s'(%d) can't be installed to %d,%d", veh->name.c_str(), vpart_list[id].name, veh->parts.size(), mdx, mdy); }

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

    if (vtypes.size() != num_vehicles)
        debugmsg("%d vehicles, %d types", vtypes.size(), num_vehicles);
}

