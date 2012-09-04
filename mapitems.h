#ifndef _MAPITEMS_H_
#define _MAPITEMS_H_
enum items_location {
 mi_none,
 mi_field, mi_forest, mi_hive, mi_hive_center,
 mi_road,
 mi_livingroom, mi_kitchen, mi_fridge, mi_home_hw, mi_bedroom, mi_homeguns,
  mi_dresser, mi_dining,
 mi_snacks, mi_fridgesnacks, mi_behindcounter, mi_magazines,
 mi_softdrugs, mi_harddrugs,
 mi_cannedfood, mi_pasta, mi_produce,
 mi_cleaning,
 mi_hardware, mi_tools, mi_bigtools, mi_mischw,
 mi_consumer_electronics,
 mi_sports, mi_camping, mi_allsporting,
 mi_alcohol,
 mi_pool_table,
 mi_trash,
 mi_ammo, mi_pistols, mi_shotguns, mi_rifles, mi_smg, mi_assault, mi_allguns,
  mi_gunxtras,
 mi_shoes, mi_pants, mi_shirts, mi_jackets, mi_winter, mi_bags, mi_allclothes,
 mi_novels, mi_manuals, mi_textbooks,
 mi_cop_weapons, mi_cop_evidence,
 mi_hospital_lab, mi_hospital_samples, mi_surgery,
 mi_office, mi_vault,
 mi_art, mi_pawn, mi_mil_surplus,
 mi_shelter,
 mi_chemistry, mi_teleport, mi_goo, mi_cloning_vat, mi_dissection,
 mi_hydro, mi_electronics, mi_monparts, mi_bionics, mi_bionics_common,
 mi_bots, mi_launchers, mi_mil_rifles, mi_grenades, mi_mil_armor, mi_mil_food,
  mi_mil_food_nodrugs, mi_bionics_mil,
 mi_weapons, mi_survival_armor, mi_survival_tools,
 mi_sewage_plant,
 mi_mine_storage, mi_mine_equipment,
 mi_spiral,
 mi_radio,
 mi_toxic_dump_equipment,
 mi_subway, mi_sewer,
 mi_cavern,
 mi_spider,
 mi_ant_food, mi_ant_egg,
// Monster drops
 mi_biollante, mi_bugs, mi_bees, mi_wasps, mi_robots,
// Map Extras
 mi_helicopter, mi_military, mi_science, mi_rare, mi_stash_food, mi_stash_ammo,
 mi_stash_wood, mi_stash_drugs, mi_drugdealer, mi_wreckage,
// Shopkeeps &c
 mi_npc_hacker,
 mi_trader_avoid,
 num_itloc
};

// This is used only for monsters; they get a list of items_locations, and
// a chance that each one will be used.
struct items_location_and_chance
{
 items_location loc;
 int chance;
 items_location_and_chance (items_location l, int c) {
  loc = l;
  chance = c;
 };
};
#endif 
