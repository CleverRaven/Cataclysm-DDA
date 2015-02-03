//
//  iexamine.h
//  Cataclysm
//
//  Livingstone
//

#ifndef IEXAMINE_H
#define IEXAMINE_H

class game;
class item;
class player;
class map;

namespace iexamine {
/**
    * Spawn spiders from a spider egg sack in radius 1 around the egg sack.
    * Transforms the egg sack furntiture into a ruptured egg sack (f_egg_sacke).
    * Also spawns eggs.
    * @param montype The monster type of the created spiders.
    */
void egg_sack_generic(player *p, map *m, int examx, int examy, const std::string &montype);
void pick_plant(player *p, map *m, int examx, int examy, std::string itemType, int new_ter,
                bool seeds = false);

void none (player *p, map *m, int examx, int examy);

void gaspump (player *p, map *m, int examx, int examy);
void atm (player *p, map *m, int examx, int examy);
void vending (player *p, map *m, int examx, int examy);
void toilet (player *p, map *m, int examx, int examy);
void elevator (player *p, map *m, int examx, int examy);
void controls_gate(player *p, map *m, int examx, int examy);
void cardreader (player *p, map *m, int examx, int examy);
void rubble (player *p, map *m, int examx, int examy);
void chainfence (player *p, map *m, int examx, int examy);
void bars(player *p, map *m, int examx, int examy);
void portable_structure(player *p, map *m, int examx, int examy);
void pit (player *p, map *m, int examx, int examy);
void pit_covered (player *p, map *m, int examx, int examy);
void fence_post (player *p, map *m, int examx, int examy);
void remove_fence_rope (player *p, map *m, int examx, int examy);
void remove_fence_wire (player *p, map *m, int examx, int examy);
void remove_fence_barbed (player *p, map *m, int examx, int examy);
void slot_machine (player *p, map *m, int examx, int examy);
void safe (player *p, map *m, int examx, int examy);
void gunsafe_ml(player *p, map *m, int examx, int examy);
void gunsafe_el(player *p, map *m, int examx, int examy);
void bulletin_board (player *p, map *m, int examx, int examy);
void fault (player *p, map *m, int examx, int examy);
void pedestal_wyrm (player *p, map *m, int examx, int examy);
void pedestal_temple (player *p, map *m, int examx, int examy);
void door_peephole (player *p, map *m, int examx, int examy);
void fswitch (player *p, map *m, int examx, int examy);
void flower_poppy (player *p, map *m, int examx, int examy);
void flower_bluebell (player *p, map *m, int examx, int examy);
void flower_dahlia (player *p, map *m, int examx, int examy);
void flower_datura (player *p, map *m, int examx, int examy);
void flower_marloss (player *p, map *m, int examx, int examy);
void flower_dandelion (player *p, map *m, int examx, int examy);
void egg_sackbw(player *p, map *m, int examx, int examy);
void egg_sackws(player *p, map *m, int examx, int examy);
void fungus (player *p, map *m, int examx, int examy);
void dirtmound (player *p, map *m, int examx, int examy);
void aggie_plant (player *p, map *m, int examx, int examy);
void harvest_tree_shrub (player *p, map *m, int examx, int examy);
void tree_pine (player *p, map *m, int examx, int examy);
void tree_blackjack (player *p, map *m, int examx, int examy);
void shrub_blueberry (player *p, map *m, int examx, int examy);
void shrub_strawberry (player *p, map *m, int examx, int examy);
void shrub_marloss (player *p, map *m, int examx, int examy);
void tree_marloss(player *p, map *m, int examx, int examy);
void shrub_wildveggies (player *p, map *m, int examx, int examy);
void recycler (player *p, map *m, int examx, int examy);
void trap(player *p, map *m, int examx, int examy);
void water_source (player *p, map *m, int examx, int examy);
void swater_source (player *p, map *m, int examx, int examy);
void acid_source (player *p, map *m, int examx, int examy);
void kiln_empty (player *p, map *m, int examx, int examy);
void kiln_full (player *p, map *m, int examx, int examy);
void fvat_empty (player *p, map *m, int examx, int examy);
void fvat_full (player *p, map *m, int examx, int examy);
void keg (player *p, map *m, int examx, int examy);
void reload_furniture (player *p, map *m, int examx, int examy);
void curtains (player *p, map *m, int examx, int examy);
void sign (player *p, map *m, int examx, int examy);
void pay_gas (player *p, map *m, int examx, int examy);

} //namespace iexamine

using iexamine_function = void (*)(player *, map *, int, int);
iexamine_function iexamine_function_from_string(std::string const &function_name);

#endif
