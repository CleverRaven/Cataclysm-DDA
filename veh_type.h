#ifndef _VEH_TYPE_H_
#define _VEH_TYPE_H_

#include "color.h"
#include "itype.h"

#ifndef mfb
#define mfb(n) static_cast <unsigned long> (1 << (n))
#endif

/*
  On altering any entries in this enum please add or remove the appropriate entry to the veh_part_names array in tile_id_data.h
*/
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
    vp_reinforced_window,
    vp_blade_h,
    vp_blade_v,
    vp_spike_h,
    vp_spike_v = vp_spike_h,

    vp_wheel,
    vp_wheel_wide,
    vp_wheel_wide_under,
    vp_wheel_bicycle,
    vp_wheel_motorbike,
    vp_wheel_small,
    vp_wheel_caster,

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
    vp_weldrig,
    vp_m249,
    vp_flamethrower,
    vp_plasmagun,
    vp_fusiongun,

// plating -- special case. mounted as internal, work as first line
// of defence and gives color to external part
    vp_steel_plate,
    vp_superalloy_plate,
    vp_spiked_plate,
    vp_hard_plate,

    vp_head_light,

    num_vparts
};

/* Flag info:
 * INTERNAL - Can be mounted inside other parts
 * ANCHOR_POINT - Allows secure seatbelt attachment
 * OVER - Can be mounted over other parts
 * VARIABLE_SIZE - Has 'bigness' for power, wheel radius, etc
 * Other flags are self-explanatory in their names. */
struct vpart_info
{
    std::string id;         // unique identifier for this part
    std::string name;       // part name, user-visible
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
    std::string fuel_type;  // engine, fuel tank
    itype_id item;      // corresponding item
    int difficulty;     // installation difficulty (mechanics requirement)
    std::set<std::string> flags;    // flags

    bool has_flag(const std::string flag) {
        return flags.count(flag) != 0;
    }
};

extern vpart_info vpart_list[num_vparts];
extern std::map<std::string, vpart_id> vpart_enums;
#endif
