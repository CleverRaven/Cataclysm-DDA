#pragma once
//
//  iexamine.h
//  Cataclysm
//
//  Livingstone
//

#ifndef IEXAMINE_H
#define IEXAMINE_H

#include <string>
#include <list>

#include "string_id.h"

class game;
class item;
class player;
class map;
struct tripoint;
struct itype;
struct mtype;
using mtype_id = string_id<mtype>;

enum hack_result {
    HACK_UNABLE,
    HACK_FAIL,
    HACK_NOTHING,
    HACK_SUCCESS
};

namespace iexamine
{
/**
    * Spawn spiders from a spider egg sack in radius 1 around the egg sack.
    * Transforms the egg sack furntiture into a ruptured egg sack (f_egg_sacke).
    * Also spawns eggs.
    * @param p The player
    * @param examp Location of egg sack
    * @param montype The monster type of the created spiders.
    */
void egg_sack_generic( player &p, const tripoint &examp, const mtype_id &montype );

void none( player &p, const tripoint &examp );

void gaspump( player &p, const tripoint &examp );
void atm( player &p, const tripoint &examp );
void vending( player &p, const tripoint &examp );
void toilet( player &p, const tripoint &examp );
void elevator( player &p, const tripoint &examp );
void controls_gate( player &p, const tripoint &examp );
void cardreader( player &p, const tripoint &examp );
void cvdmachine( player &p, const tripoint &examp );
void rubble( player &p, const tripoint &examp );
void crate( player &p, const tripoint &examp );
void chainfence( player &p, const tripoint &examp );
void bars( player &p, const tripoint &examp );
void portable_structure( player &p, const tripoint &examp );
void pit( player &p, const tripoint &examp );
void pit_covered( player &p, const tripoint &examp );
void fence_post( player &p, const tripoint &examp );
void remove_fence_rope( player &p, const tripoint &examp );
void remove_fence_wire( player &p, const tripoint &examp );
void remove_fence_barbed( player &p, const tripoint &examp );
void slot_machine( player &p, const tripoint &examp );
void safe( player &p, const tripoint &examp );
void gunsafe_ml( player &p, const tripoint &examp );
void gunsafe_el( player &p, const tripoint &examp );
void harvest_furn_nectar( player &p, const tripoint &examp );
void harvest_furn( player &p, const tripoint &examp );
void harvest_ter_nectar( player &p, const tripoint &examp );
void harvest_ter( player &p, const tripoint &examp );
void harvested_plant( player &p, const tripoint &examp );
void locked_object( player &p, const tripoint &examp );
void bulletin_board( player &p, const tripoint &examp );
void fault( player &p, const tripoint &examp );
void pedestal_wyrm( player &p, const tripoint &examp );
void pedestal_temple( player &p, const tripoint &examp );
void door_peephole( player &p, const tripoint &examp );
void fswitch( player &p, const tripoint &examp );
void flower_poppy( player &p, const tripoint &examp );
void flower_bluebell( player &p, const tripoint &examp );
void flower_dahlia( player &p, const tripoint &examp );
void flower_marloss( player &p, const tripoint &examp );
void egg_sackbw( player &p, const tripoint &examp );
void egg_sackcs( player &p, const tripoint &examp );
void egg_sackws( player &p, const tripoint &examp );
void fungus( player &p, const tripoint &examp );
void dirtmound( player &p, const tripoint &examp );
void aggie_plant( player &p, const tripoint &examp );
void tree_hickory( player &p, const tripoint &examp );
void tree_maple( player &p, const tripoint &examp );
void tree_maple_tapped( player &p, const tripoint &examp );
void shrub_marloss( player &p, const tripoint &examp );
void tree_marloss( player &p, const tripoint &examp );
void shrub_wildveggies( player &p, const tripoint &examp );
void recycler( player &p, const tripoint &examp );
void trap( player &p, const tripoint &examp );
void water_source( player &p, const tripoint &examp );
void kiln_empty( player &p, const tripoint &examp );
void kiln_full( player &p, const tripoint &examp );
void fvat_empty( player &p, const tripoint &examp );
void fvat_full( player &p, const tripoint &examp );
void keg( player &p, const tripoint &examp );
void reload_furniture( player &p, const tripoint &examp );
void curtains( player &p, const tripoint &examp );
void sign( player &p, const tripoint &examp );
void pay_gas( player &p, const tripoint &examp );
void climb_down( player &p, const tripoint &examp );
hack_result hack_attempt( player &p );

/**
 * Pour liquid into a keg (furniture) on the map. The transferred charges (if any)
 * will be removed from the liquid item.
 * @return Whether any charges have been transferred at all.
 */
bool pour_into_keg( const tripoint &pos, item &liquid );

/**
 * Check whether there is a keg on the map that can be filled via @ref pour_into_keg.
 */
bool has_keg( const tripoint &pos );

/**
 * Items that appear when a generic plant is harvested. Seed @ref islot_seed.
 * @param type The seed type, must have a @ref itype::seed slot.
 * @param plant_count Number of fruits to generate. For charge-based items, this
 *     specifies multiples of the default charge.
 * @param seed_count Number of seeds to generate.
 * @param byproducts If true, byproducts (like straw, withered plants, see
 * @ref islot_seed::byproducts) are included.
 */
std::list<item> get_harvest_items( const itype &type, int plant_count,
                                   int seed_count, bool byproducts );
} //namespace iexamine

using iexamine_function = void ( * )( player &, const tripoint & );
iexamine_function iexamine_function_from_string( std::string const &function_name );

#endif
