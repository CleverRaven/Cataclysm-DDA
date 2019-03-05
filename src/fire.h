#pragma once
#ifndef FIRE_H
#define FIRE_H

#include "units.h"

/**
 * Contains the state of a fire in one tile on one turn
 *
 * Used to pass data between the fire field calculations in @ref map::process_fields_in_submap()
 * and the item::burn() function of each specific item being burned.
 *
 * A structure of this type is created during the burn calculation for a given turn and a given
 * tile.  The calculation iterates through all items on the tile, with this structure serving
 * to accumulate the sum of smoke produced by and fuel contributed by each item in the fire
 * this turn.
 */
struct fire_data {
    fire_data() = default;
    fire_data( const fire_data & ) = default;
    fire_data( int intensity, bool is_contained = false ) : fire_intensity( intensity ),
        contained( is_contained )
    {}
    /** Current intensity of the fire.  This is an input to the calculations */
    int fire_intensity = 0;
    /** Smoke produced by each burning item this turn is summed here. */
    float smoke_produced = 0.0f;
    /** Fuel contributed by each burning item this turn is summed here. */
    float fuel_produced = 0.0f;
    /** The fire is contained and burned for fuel intentionally. */
    bool contained = false;
};

/**
 * Contains burning parameters for a given material.
 *
 * Contains the parameters which define how a given material reacts to being burned.  This is
 * currently used by the calculations in @ref item::burn() to determine smoke produced and fuel
 * values contributed to a fire.
 *
 * A given material will generally contain several of these structures, indexed by fire intensity.
 * This allows materials to react correctly to stronger flames.
 *
 * This structure is populated from json data files in @ref material_type::load()
 */
struct mat_burn_data {
    /** If this is true, an object will not burn or be destroyed by fire. */
    bool immune = false;
    /** If non-zero and lower than item's volume, scale burning by `volume_penalty / volume`. */
    units::volume volume_per_turn = 0_ml;
    /** Fuel contributed per tick when this material burns. */
    float fuel = 0.0f;
    /** Smoke produced per tick when this material burns. */
    float smoke = 0.0f;
    /** Volume of material destroyed per tick when this material burns. */
    float burn = 0.0f;
};

#endif
