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
    vp_roof,
    vp_door,
    vp_window,
    vp_blade_h,
    vp_blade_v,
    vp_spike_h,
    vp_spike_v = vp_spike_h,
    vp_wheel_large,
    vp_wheel,

    vp_engine_gas_small,
    vp_engine_gas_med,
    vp_engine_gas_large,
    vp_engine_motor,
    vp_engine_motor_large,
    vp_engine_plasma,
    vp_fuel_tank_gas,
    vp_fuel_tank_batt,
    vp_fuel_tank_plut,
    vp_fuel_tank_hydrogen,
    vp_cargo_trunk, // over
    vp_cargo_box,   // over

// pure internal parts
    vp_controls,
    vp_muffler,
    vp_seatbelt,
    vp_solar_panel,
    vp_m249,
    vp_flamethrower,
    vp_plasmagun,

// plating -- special case. mounted as internal, work as first line
// of defence and gives color to external part
    vp_steel_plate,
    vp_superalloy_plate,
    vp_spiked_plate,
    vp_hard_plate,

    num_vparts
};

enum vpart_flags
{
    vpf_external,           // can be mounted as external part
    vpf_internal,           // can be mounted inside other part
    vpf_mount_point,        // allows mounting other parts to it
    vpf_mount_inner,        // allows mounting internal parts inside it (or over it)
    vpf_mount_over,         // allows mounting parts like cargo trunk over it
    vpf_opaque,             // can't see through it
    vpf_obstacle,           // can't pass through it
    vpf_openable,           // can open/close it
    vpf_no_reinforce,       // can't reinforce this part with armor plates
    vpf_sharp,              // cutting damage instead of bashing
    vpf_unmount_on_damage,  // when damaged, part is unmounted, rather than broken

// functional flags (only one of each can be mounted per tile)
    vpf_over,               // can be mounted over other part
    vpf_roof,               // is a roof (cover)
    vpf_wheel,              // this part touches ground (trigger traps)
    vpf_seat,               // is seat
    vpf_engine,             // is engine
    vpf_fuel_tank,          // is fuel tank
    vpf_cargo,              // is cargo
    vpf_controls,           // is controls
    vpf_muffler,            // is muffler
    vpf_seatbelt,           // is seatbelt
    vpf_solar_panel,        // is solar panel
    vpf_turret,             // is turret
    vpf_armor,              // is armor plating
    vpf_func_begin  = vpf_over,
    vpf_func_end    = vpf_armor,

    num_vpflags
};

struct vpart_info
{
    const char *name;       // part name
    char sym;               // symbol of part as if it's looking north
    nc_color color;         // color
    char sym_broken;        // symbol of broken part as if it's looking north
    nc_color color_broken;  // color of broken part
    int dmg_mod;            // damage modifier, percent
    int durability;         // durability
    union
    {
        int par1;
        int power;      // engine (top spd), solar panel (% of 1 fuel per turn, can be > 100)
        int size;       // wheel, fuel tank, trunk
        int bonus;      // seatbelt (str), muffler (%)
    };
    union
    {
        int par2;
        int fuel_type;  // engine, fuel tank
    };
    itype_id item;      // corresponding item
    int difficulty;     // installation difficulty (mechanics requirement)
    unsigned long flags;    // flags
};

// following symbols will be translated: 
// y, u, n, b to NW, NE, SE, SW lines correspondingly
// h, j, c to horizontal, vertical, cross correspondingly
const vpart_info vpart_list[num_vparts] =
{   // name        sym   color    sym_b   color_b  dmg  dur  par1 par2  item
    { "null part",  '?', c_red,     '?', c_red,     100, 100, 0, 0, itm_null, 0,
        0 },
    { "seat",       '#', c_red,     '*', c_red,     60,  300, 0, 0, itm_seat, 1,
        mfb(vpf_over) | mfb(vpf_seat) | mfb(vpf_no_reinforce) },
    { "frame",      'h', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, itm_frame, 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      'j', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, itm_frame, 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      'c', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, itm_frame, 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      'y', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, itm_frame, 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      'u', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, itm_frame, 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      'n', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, itm_frame, 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      'b', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, itm_frame, 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      '=', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, itm_frame, 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      'H', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, itm_frame, 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "frame",      '^', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, itm_frame, 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "handle",     '^', c_ltcyan,  '#', c_ltcyan,  100, 300, 0, 0, itm_frame, 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) },
    { "board",      'h', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, itm_steel_plate, 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
    { "board",      'j', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, itm_steel_plate, 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
    { "board",      'y', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, itm_steel_plate, 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
    { "board",      'u', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, itm_steel_plate, 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
    { "board",      'n', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, itm_steel_plate, 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
    { "board",      'b', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, itm_steel_plate, 1,
        mfb(vpf_external) | mfb(vpf_mount_point) | mfb (vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
    { "roof",       '#', c_ltgray,  '#', c_dkgray,  100, 1000, 0, 0, itm_steel_plate, 1,
        mfb(vpf_internal) | mfb(vpf_roof) },
    { "door",       '+', c_cyan,    '&', c_cyan,    80,  200, 0, 0, itm_frame, 2,
        mfb(vpf_external) | mfb(vpf_obstacle) | mfb(vpf_openable) },
    { "windshield", '"', c_ltcyan,  '0', c_ltgray,  70,  50, 0, 0, itm_glass_sheet, 1,
        mfb(vpf_over) | mfb(vpf_obstacle) | mfb(vpf_no_reinforce) },
    { "blade",      '-', c_white,   'x', c_white,   250, 100, 0, 0, itm_machete, 2,
        mfb(vpf_external) | mfb(vpf_unmount_on_damage) | mfb(vpf_sharp) | mfb(vpf_no_reinforce) },
    { "blade",      '|', c_white,   'x', c_white,   350, 100, 0, 0, itm_machete, 2,
        mfb(vpf_external) | mfb(vpf_unmount_on_damage) | mfb(vpf_sharp) | mfb(vpf_no_reinforce) },
    { "spike",      '.', c_white,   'x', c_white,   300, 100, 0, 0, itm_spear_knife, 2,
        mfb(vpf_external) | mfb(vpf_unmount_on_damage) | mfb(vpf_sharp) | mfb(vpf_no_reinforce) },

//                                                           size
    { "large wheel",'0', c_dkgray,  'x', c_ltgray,  50,  300, 30, 0, itm_big_wheel, 3,
        mfb(vpf_external) | mfb (vpf_mount_over) | mfb(vpf_wheel) | mfb(vpf_mount_point) },
    { "wheel",      'o', c_dkgray,  'x', c_ltgray,  50,  200, 10, 0, itm_wheel, 2,
        mfb(vpf_external) | mfb (vpf_mount_over) | mfb(vpf_wheel) | mfb(vpf_mount_point) },
//                                                                         power type
    { "1L combustion engine",       '*', c_ltred,  '#', c_red,     80, 200, 120, AT_GAS, itm_combustion_small, 2,
        mfb(vpf_internal) | mfb(vpf_engine) },
    { "2.5L combustion engine",     '*', c_ltred,  '#', c_red,     80, 300, 300, AT_GAS, itm_combustion, 3,
        mfb(vpf_internal) | mfb(vpf_engine) },
    { "6L combustion engine",       '*', c_ltred,  '#', c_red,     80, 400, 800, AT_GAS, itm_combustion_large, 4,
        mfb(vpf_internal) | mfb(vpf_engine) },
    { "electric motor",             '*', c_yellow,  '#', c_red,    80, 200, 70, AT_BATT, itm_motor, 3,
        mfb(vpf_internal) | mfb(vpf_engine) },
    { "large electric motor",       '*', c_yellow,  '#', c_red,    80, 400, 350, AT_BATT, itm_motor_large, 4,
        mfb(vpf_internal) | mfb(vpf_engine) },
    { "plasma engine",              '*', c_ltblue,  '#', c_red,    80, 250, 400, AT_PLASMA, itm_plasma_engine, 6,
        mfb(vpf_internal) | mfb(vpf_engine) },
//                                                                         capacity type
    { "gasoline tank",              'O', c_ltred,  '#', c_red,     80, 150, 3000, AT_GAS, itm_metal_tank, 1,
        mfb(vpf_internal) | mfb(vpf_fuel_tank) },
    { "storage battery",            'O', c_yellow,  '#', c_red,    80, 300, 1000, AT_BATT, itm_storage_battery, 2,
        mfb(vpf_internal) | mfb(vpf_fuel_tank) },
    { "minireactor",                'O', c_ltgreen,  '#', c_red,    80, 700, 10000, AT_PLUT, itm_minireactor, 7,
        mfb(vpf_internal) | mfb(vpf_fuel_tank) },
    { "hydrogene tank",             'O', c_ltblue,  '#', c_red,     80, 150, 3000, AT_PLASMA, itm_metal_tank, 1,
        mfb(vpf_internal) | mfb(vpf_fuel_tank) },
    { "trunk",                      'H', c_brown,  '#', c_brown,    80, 300, 400, 0, itm_frame, 1,
        mfb(vpf_over) | mfb(vpf_cargo) },
    { "box",                        'o', c_brown,  '#', c_brown,    60, 100, 400, 0, itm_frame, 1,
        mfb(vpf_over) | mfb(vpf_cargo) },

    { "controls",   '$', c_ltgray,  '$', c_red,     10, 250, 0, 0, itm_vehicle_controls, 3,
        mfb(vpf_internal)  | mfb(vpf_controls) },
//                                                          bonus
    { "muffler",    '/', c_ltgray,  '/', c_ltgray,  10, 150, 40, 0, itm_muffler, 2,
        mfb(vpf_internal)  | mfb(vpf_muffler) },
    { "seatbelt",   ',', c_ltgray,  ',', c_red,     10, 200, 25, 0, itm_rope_6, 1,
        mfb(vpf_internal)  | mfb(vpf_seatbelt) },
    { "solar panel", '#', c_yellow,  'x', c_yellow, 10, 20, 30, 0, itm_solar_panel, 6,
        mfb(vpf_over)  | mfb(vpf_solar_panel) },

    { "mounted M249",         't', c_cyan,    '#', c_cyan,    80, 400, 0, AT_223, itm_m249, 6,
        mfb(vpf_over)  | mfb(vpf_turret) | mfb(vpf_cargo) },
    { "mounted flamethrower", 't', c_dkgray,  '#', c_dkgray,  80, 400, 0, AT_GAS, itm_flamethrower, 7,
        mfb(vpf_over)  | mfb(vpf_turret) },
    { "mounted plasma gun", 't', c_ltblue,    '#', c_ltblue,    80, 400, 0, AT_PLASMA, itm_plasma_rifle, 7,
        mfb(vpf_over)  | mfb(vpf_turret) },

    { "steel plating",     ')', c_ltcyan, ')', c_ltcyan, 100, 1000, 0, 0, itm_steel_plate, 3,
        mfb(vpf_internal) | mfb(vpf_armor) },
    { "superalloy plating",')', c_dkgray, ')', c_dkgray, 100, 900, 0, 0, itm_alloy_plate, 4,
        mfb(vpf_internal) | mfb(vpf_armor) },
    { "spiked plating",    ')', c_red,    ')', c_red,    150, 900, 0, 0, itm_spiked_plate, 3,
        mfb(vpf_internal) | mfb(vpf_armor) | mfb(vpf_sharp) },
    { "hard plating",      ')', c_cyan,   ')', c_cyan,   100, 2300, 0, 0, itm_hard_plate, 4,
        mfb(vpf_internal) | mfb(vpf_armor) }
};


enum vhtype_id
{
    veh_null = 0,
    veh_custom,

// in-built vehicles
    veh_motorcycle,
    veh_sandbike,
    veh_car,
    veh_truck,

    num_vehicles
};

#endif
