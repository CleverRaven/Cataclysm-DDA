#ifndef _VEH_TYPE_H_
#define _VEH_TYPE_H_

#include "color.h"
#include "itype.h"

#ifndef mfb
#define mfb(n) long(1 << (n))
#endif

enum vpart_id
{
    vp_null = 0,

// external parts
    vp_seat,
    vp_saddle,
    vp_bed,
    vp_frame_h,
    vp_frame_v,
    vp_frame_c,
    vp_frame_y,
    vp_frame_u,
    vp_frame_n,
    vp_frame_b,
    vp_frame_h2,
    vp_frame_v2,
    vp_frame_cover,
    vp_frame_handle,
    vp_board_h,
    vp_board_v,
    vp_board_y,
    vp_board_u,
    vp_board_n,
    vp_board_b,
    vp_aisle_h2,
    vp_aisle_v2,
    vp_floor_trunk,
    vp_roof,
    vp_door,
    vp_door_o,
    vp_door_i,
    vp_window,
    vp_blade_h,
    vp_blade_v,
    vp_spike_h,
    vp_spike_v = vp_spike_h,

    vp_wheel,
    vp_wheel_wide,
    vp_wheel_bicycle,
    vp_wheel_motorbike,
    vp_wheel_small,

    vp_engine_gas_1cyl,
    vp_engine_gas_v2,
    vp_engine_gas_i4,
    vp_engine_gas_v6,
    vp_engine_gas_v8,
    vp_engine_motor,
    vp_engine_motor_large,
    vp_engine_plasma,
    vp_engine_foot_crank,
    vp_fuel_tank_gas,
    vp_fuel_tank_batt,
    vp_fuel_tank_plut,
    vp_fuel_tank_hydrogen,
    vp_fuel_tank_water,
    vp_cargo_trunk, // over
    vp_cargo_box,   // over

// pure internal parts
    vp_controls,
    vp_muffler,
    vp_seatbelt,
    vp_solar_panel,
    vp_kitchen_unit,
    vp_m249,
    vp_flamethrower,
    vp_plasmagun,

// plating -- special case. mounted as internal, work as first line
// of defence and gives color to external part
    vp_steel_plate,
    vp_superalloy_plate,
    vp_spiked_plate,
    vp_hard_plate,

    vp_head_light,

    num_vparts
};

enum vpart_flags
{
    vpf_external,           // can be mounted as external part
    vpf_internal,           // can be mounted inside other part
    vpf_mount_point,        // allows mounting other parts to it
    vpf_mount_inner,        // allows mounting internal parts inside it (or over it)
    vpf_mount_over,         // allows mounting parts like cargo trunk over it
    vpf_anchor_point,       // Allows secure attachment of a seatbelt
    vpf_opaque,             // can't see through it
    vpf_obstacle,           // can't pass through it
    vpf_openable,           // can open/close it
    vpf_no_reinforce,       // can't reinforce this part with armor plates
    vpf_sharp,              // cutting damage instead of bashing
    vpf_unmount_on_damage,  // when damaged, part is unmounted, rather than broken
    vpf_boardable,          // part can carry passengers

// functional flags (only one of each can be mounted per tile)
    vpf_over,               // can be mounted over other part
    vpf_roof,               // is a roof (cover)
    vpf_wheel,              // this part touches ground (trigger traps)
    vpf_seat,               // is seat
    vpf_bed,                // is bed (like seat, but can't be boarded)
    vpf_aisle,               // is aisle (no extra movement cost)
    vpf_engine,             // is engine
    vpf_kitchen,            // is kitchen
    vpf_fuel_tank,          // is fuel tank
    vpf_cargo,              // is cargo
    vpf_controls,           // is controls
    vpf_muffler,            // is muffler
    vpf_seatbelt,           // is seatbelt
    vpf_solar_panel,        // is solar panel
    vpf_turret,             // is turret
    vpf_armor,              // is armor plating
    vpf_light,              // generates light arc
    vpf_variable_size,      // has 'bigness' for power, wheel radius, etc.
    vpf_func_begin  = vpf_over,
    vpf_func_end    = vpf_light,

    num_vpflags
};

struct vpart_info
{
    const char *name;       // part name
    long sym;               // symbol of part as if it's looking north
    nc_color color;         // color
    char sym_broken;        // symbol of broken part as if it's looking north
    nc_color color_broken;  // color of broken part
    int dmg_mod;            // damage modifier, percent
    int durability;         // durability
    union
    {
        int par1;
        int power;      // engine (top spd), solar panel (% of 1 fuel per turn, can be > 100)
        int size;       // fuel tank, trunk
        int wheel_width;// wheel width in inches. car could be 9, bicycle could be 2.
        int bonus;      // seatbelt (str), muffler (%)
    };
    int par2;
    std::string fuel_type;  // engine, fuel tank
    itype_id item;      // corresponding item
    int difficulty;     // installation difficulty (mechanics requirement)
    unsigned long flags;    // flags
};

// following symbols will be translated:
// y, u, n, b to NW, NE, SE, SW lines correspondingly
// h, j, c to horizontal, vertical, cross correspondingly
const vpart_info vpart_list[num_vparts] =
{   // name        sym   color    sym_b   color_b  dmg  dur  par1 par2 fuel  item
    { "null part",  '?', c_red,     '?', c_red,     100, 100, 0, 0, "NULL", "null", 0,
        0 },
    { "seat",       '#', c_red,     '*', c_red,     60,  300, 0, 0, "NULL", "seat", 1,
        mfb(vpf_over) | mfb(vpf_seat) | mfb(vpf_boardable) | mfb(vpf_cargo) |
        mfb(vpf_no_reinforce) | mfb(vpf_anchor_point) },
    { "saddle",     '#', c_red,     '*', c_red,     20,  200, 0, 0, "NULL", "saddle", 1,
        mfb(vpf_over) | mfb(vpf_seat) | mfb(vpf_boardable) |
        mfb(vpf_no_reinforce) },
    { "bed",        '#', c_magenta, '*', c_magenta, 60,  300, 0, 0, "NULL", "seat", 1,
        mfb(vpf_over) | mfb(vpf_bed) | mfb(vpf_boardable) |
        mfb(vpf_cargo) | mfb(vpf_no_reinforce) },
    { "frame",      'h', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      'j', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      'c', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      'y', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      'u', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      'n', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      'b', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      '=', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      'H', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      '^', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "handle",     '^', c_ltcyan,  '#', c_ltcyan,  100, 300, 0, 0, "NULL", "frame", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "board",      'h', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, "NULL", "steel_plate", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
    { "board",      'j', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, "NULL", "steel_plate", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
    { "board",      'y', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, "NULL", "steel_plate", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
    { "board",      'u', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, "NULL", "steel_plate", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
    { "board",      'n', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, "NULL", "steel_plate", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
    { "board",      'b', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, "NULL", "steel_plate", 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
    { "aisle",       '=', c_white,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_internal) | mfb(vpf_over) | mfb(vpf_no_reinforce) | mfb(vpf_aisle) | mfb(vpf_boardable) },
    { "aisle",       'H', c_white,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_internal) | mfb(vpf_over) | mfb (vpf_no_reinforce) | mfb(vpf_aisle) | mfb(vpf_boardable) },
    { "floor trunk",       '=', c_white,  '#', c_ltgray,  100, 400, 0, 0, "NULL", "frame", 1,
        mfb(vpf_internal) | mfb(vpf_over) | mfb (vpf_no_reinforce) | mfb(vpf_aisle) | mfb(vpf_boardable) | mfb(vpf_cargo) },
    { "roof",       '#', c_ltgray,  '#', c_dkgray,  100, 1000, 0, 0, "NULL", "steel_plate", 1,
        mfb(vpf_internal) | mfb(vpf_roof) },
    { "door",       '+', c_cyan,    '&', c_cyan,    80,  200, 0, 0, "NULL", "frame", 2,
        mfb(vpf_external) | mfb(vpf_obstacle) | mfb(vpf_openable) | mfb(vpf_boardable) },
    { "opaque door",'+', c_cyan,    '&', c_cyan,    80,  200, 0, 0, "NULL", "frame", 2,
        mfb(vpf_external) | mfb(vpf_obstacle) | mfb(vpf_opaque) | mfb(vpf_openable) | mfb(vpf_boardable) },
    { "internal door", '+', c_cyan, '&', c_cyan,    75,  75, 0, 0, "NULL", "frame", 2,
        mfb(vpf_external) | mfb(vpf_obstacle) | mfb(vpf_opaque) | mfb(vpf_openable) | mfb(vpf_roof) | mfb(vpf_no_reinforce) | mfb(vpf_boardable) },
    { "windshield", '"', c_ltcyan,  '0', c_ltgray,  70,  50, 0, 0, "NULL", "glass_sheet", 1,
        mfb(vpf_over) | mfb(vpf_obstacle) | mfb(vpf_no_reinforce) },
    { "blade",      '-', c_white,   'x', c_white,   250, 100, 0, 0, "NULL", "blade", 2,
        mfb(vpf_external) | mfb(vpf_unmount_on_damage) | mfb(vpf_sharp) | mfb(vpf_no_reinforce) },
    { "blade",      '|', c_white,   'x', c_white,   350, 100, 0, 0, "NULL", "blade", 2,
        mfb(vpf_external) | mfb(vpf_unmount_on_damage) | mfb(vpf_sharp) | mfb(vpf_no_reinforce) },
    { "spike",      '.', c_white,   'x', c_white,   300, 100, 0, 0, "NULL", "spike", 2,
        mfb(vpf_external) | mfb(vpf_unmount_on_damage) | mfb(vpf_sharp) | mfb(vpf_no_reinforce) },

//                                                           wheel_width(inches)
    { "wheel",      '0',    c_dkgray,  'x', c_ltgray,  50,  200, 9, 0, "NULL", "wheel", 4,
        mfb(vpf_external) | mfb (vpf_mount_over) | mfb(vpf_wheel) | mfb(vpf_mount_point) | mfb(vpf_variable_size) },
    { "wide wheel", 'O',     c_dkgray,   'x', c_ltgray,  50,  400, 14, 0, "NULL", "wheel_wide", 5,
        mfb(vpf_external) | mfb (vpf_mount_over) | mfb(vpf_wheel) | mfb(vpf_mount_point) | mfb(vpf_variable_size) },
    { "bicycle wheel",'|',  c_dkgray, 'x', c_ltgray,  50,  40, 2, 0, "NULL", "wheel_bicycle", 1,
        mfb(vpf_external) | mfb (vpf_mount_over) | mfb(vpf_wheel) | mfb(vpf_mount_point) | mfb(vpf_variable_size) },
    { "motorbike wheel",'o',c_dkgray, 'x', c_ltgray,  50,  90, 4, 0, "NULL", "wheel_motorbike", 2,
        mfb(vpf_external) | mfb (vpf_mount_over) | mfb(vpf_wheel) | mfb(vpf_mount_point) | mfb(vpf_variable_size) },
    { "small wheel",    'o',c_dkgray, 'x', c_ltgray,  50,  70, 6, 0, "NULL", "wheel_small", 2,
        mfb(vpf_external) | mfb (vpf_mount_over) | mfb(vpf_wheel) | mfb(vpf_mount_point) | mfb(vpf_variable_size) },
//
    { "1-cylinder engine",    '*', c_ltred,  '#', c_red,     80, 150, 40, 0, "gasoline", "1cyl_combustion", 2,
        mfb(vpf_internal) | mfb(vpf_engine) | mfb(vpf_variable_size) },
    { "V-twin engine",       '*', c_ltred,  '#', c_red,     80, 200, 120, 0, "gasoline", "v2_combustion", 2,
        mfb(vpf_internal) | mfb(vpf_engine) | mfb(vpf_variable_size) },
    { "Inline-4 engine",     '*', c_ltred,  '#', c_red,     80, 300, 300, 0, "gasoline", "i4_combustion", 3,
        mfb(vpf_internal) | mfb(vpf_engine) | mfb(vpf_variable_size) },
    { "V6 engine",       '*', c_ltred,  '#', c_red,     80, 400, 800, 0, "gasoline", "v6_combustion", 4,
        mfb(vpf_internal) | mfb(vpf_engine) | mfb(vpf_variable_size) },
    { "V8 engine",       '*', c_ltred,  '#', c_red,     80, 400, 800, 0, "gasoline", "v8_combustion", 4,
        mfb(vpf_internal) | mfb(vpf_engine) | mfb(vpf_variable_size) },
    { "electric motor",             '*', c_yellow,  '#', c_red,    80, 200, 70, 0, "battery", "motor", 3,
        mfb(vpf_internal) | mfb(vpf_engine) },
    { "large electric motor",       '*', c_yellow,  '#', c_red,    80, 400, 350, 0, "battery", "motor_large", 4,
        mfb(vpf_internal) | mfb(vpf_engine) },
    { "plasma engine",              '*', c_ltblue,  '#', c_red,    80, 250, 400, 0, "plasma", "plasma_engine", 6,
        mfb(vpf_internal) | mfb(vpf_engine) },
    { "Foot pedals",                '*', c_ltgray,  '#', c_red,     50, 50, 70, 0, "muscle", "foot_crank", 1,
        mfb(vpf_internal) | mfb(vpf_engine) },
//                                                                         capacity type
    { "gasoline tank",              'O', c_ltred,  '#', c_red,     80, 150, 3000, 0, "gasoline", "metal_tank", 1,
        mfb(vpf_internal) | mfb(vpf_fuel_tank) },
    { "storage battery",            'O', c_yellow,  '#', c_red,    80, 300, 100000, 0, "battery", "storage_battery", 2,
        mfb(vpf_internal) | mfb(vpf_fuel_tank) },
    { "minireactor",                'O', c_ltgreen,  '#', c_red,    80, 700, 10000, 0, "plutonium", "minireactor", 7,
        mfb(vpf_internal) | mfb(vpf_fuel_tank) },
    { "hydrogen tank",             'O', c_ltblue,  '#', c_red,     80, 150, 3000, 0, "plasma", "metal_tank", 1,
        mfb(vpf_internal) | mfb(vpf_fuel_tank) },
    { "water tank",                 'O', c_ltcyan,  '#', c_red,     80, 150, 400, 0, "water", "metal_tank", 1,
        mfb(vpf_internal) | mfb(vpf_fuel_tank) },
    { "trunk",                      'H', c_brown,  '#', c_brown,    80, 300, 400, 0, "NULL", "frame", 1,
        mfb(vpf_over) | mfb(vpf_cargo) },
    { "box",                        'o', c_brown,  '#', c_brown,    60, 100, 400, 0, "NULL", "frame", 1,
        mfb(vpf_over) | mfb(vpf_cargo) | mfb(vpf_boardable) },

    { "controls",   '$', c_ltgray,  '$', c_red,     10, 250, 0, 0, "NULL", "vehicle_controls", 3,
        mfb(vpf_internal)  | mfb(vpf_controls) },
//                                                          bonus
    { "muffler",    '/', c_ltgray,  '/', c_ltgray,  10, 150, 40, 0, "NULL", "muffler", 2,
        mfb(vpf_internal)  | mfb(vpf_muffler) },
    { "seatbelt",   ',', c_ltgray,  ',', c_red,     10, 200, 25, 0, "NULL", "rope_6", 1,
        mfb(vpf_internal)  | mfb(vpf_seatbelt) },
    { "solar panel", '#', c_yellow,  'x', c_yellow, 10, 20, 30, 0, "NULL", "solar_panel", 6,
        mfb(vpf_over)  | mfb(vpf_solar_panel) },
    { "kitchen unit", '&', c_ltcyan, 'x', c_ltcyan, 10, 20, 0, 0, "NULL", "kitchen_unit", 4,
        mfb(vpf_over) | mfb(vpf_cargo) | mfb(vpf_roof) | mfb(vpf_no_reinforce) | mfb(vpf_obstacle) | mfb(vpf_kitchen) },
    { "mounted M249",         't', c_cyan,    '#', c_cyan,    80, 400, 0, 0, "223", "m249", 6,
        mfb(vpf_over)  | mfb(vpf_turret) | mfb(vpf_cargo) },
    { "mounted flamethrower", 't', c_dkgray,  '#', c_dkgray,  80, 400, 0, 0, "gasoline", "flamethrower", 7,
        mfb(vpf_over)  | mfb(vpf_turret) },
    { "mounted plasma gun", 't', c_ltblue,    '#', c_ltblue,    80, 400, 0, 0, "plasma", "plasma_rifle", 7,
        mfb(vpf_over)  | mfb(vpf_turret) },

    { "steel plating",     ')', c_ltcyan, ')', c_ltcyan, 100, 1000, 0, 0, "NULL", "steel_plate", 3,
        mfb(vpf_internal) | mfb(vpf_armor) },
    { "superalloy plating",')', c_dkgray, ')', c_dkgray, 100, 900, 0, 0, "NULL", "alloy_plate", 4,
        mfb(vpf_internal) | mfb(vpf_armor) },
    { "spiked plating",    ')', c_red,    ')', c_red,    150, 900, 0, 0, "NULL", "spiked_plate", 3,
        mfb(vpf_internal) | mfb(vpf_armor) | mfb(vpf_sharp) },
    { "hard plating",      ')', c_cyan,   ')', c_cyan,   100, 2300, 0, 0, "NULL", "hard_plate", 4,
        mfb(vpf_internal) | mfb(vpf_armor) },
    { "head light",        '*', c_white,  '*', c_white,  10, 20, 480, 0, "NULL", "flashlight", 1,
       mfb(vpf_internal) | mfb(vpf_light) }
};


enum vhtype_id
{
    veh_null = 0,
    veh_custom,

// in-built vehicles
    veh_bicycle,
    veh_motorcycle_chassis,
    veh_motorcycle,
    veh_sandbike,
    veh_sandbike_chassis,
    veh_car,
    veh_car_chassis,
    veh_truck,
    veh_semi,  //6L Semitruck. 10 wheels. Sleeper cab.
    veh_trucktrailer,  //Just a trailer with 8 wheels.
    veh_wagon, // Dwarf Fortress Wagon
    veh_bug,  //Old VW Bug.
    veh_bubblecar,  //360 degree view glass circular vehicle. Underpowered plutonium.
    veh_golfcart,  //Yamaha golf cart.
    veh_scooter,  //Vespa S50 scooter.
    veh_armytruck,  //Army M35A2 6L gas and/or hydrogen engine if commented parts uncommented.
    veh_schoolbus,  //Standard schoolbus
    veh_car_electric, // electric version of standard car.
    veh_rv, //RV with bed and kitchen unit

    num_vehicles
};

#endif
