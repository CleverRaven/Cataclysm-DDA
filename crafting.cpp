#include <string>
#include <sstream>
#include "input.h"
#include "game.h"
#include "options.h"
#include "output.h"
#include "crafting.h"
#include "inventory.h"

void draw_recipe_tabs(WINDOW *w, craft_cat tab,bool filtered=false);

// This function just defines the recipes used throughout the game.
void game::init_recipes()
{
 int id = -1;
 int tl, cl;

 #define RECIPE(result, category, skill1, skill2, difficulty, time, reversible) \
tl = -1; cl = -1; id++;\
recipes.push_back( new recipe(id, result, category, skill1, skill2, difficulty,\
                              time, reversible) )
 #define TOOL(item, amount)  ++tl; recipes[id]->tools[tl].push_back(component(item,amount))
 #define TOOLCONT(item, amount) recipes[id]->tools[tl].push_back(component(item,amount))
 #define COMP(item, amount)  ++cl; recipes[id]->components[tl].push_back(component(item,amount))
 #define COMPCONT(item, amount) recipes[id]->components[tl].push_back(component(item,amount))

/**
 * Macro Tool Groups
 * Placeholder for imminent better system, this is already ridiculous
 * Usage:
 * TOOL(TG_KNIVES,NULL);
 */

#define TG_KNIVES \
 TOOL("knife_steak", -1); TOOLCONT("knife_combat", -1); \
 TOOLCONT("knife_butcher", -1); TOOLCONT("pockknife", -1); \
 TOOLCONT("scalpel", -1); TOOLCONT("machete", -1); \
 TOOLCONT("broadsword", -1); TOOLCONT("toolset", -1);
#define TG_KNIVES_CONT \
 TOOLCONT("knife_steak", -1); TOOLCONT("knife_combat", -1); \
 TOOLCONT("knife_butcher", -1); TOOLCONT("pockknife", -1); \
 TOOLCONT("scalpel", -1); TOOLCONT("machete", -1); \
 TOOLCONT("broadsword", -1); TOOLCONT("toolset", -1);


/* A recipe will not appear in your menu until your level in the primary skill
 * is at least equal to the difficulty.  At that point, your chance of success
 * is still not great; a good 25% improvement over the difficulty is important
 */

// NON-CRAFTABLE BUT CAN BE DISASSEMBLED (set category to CC_NONCRAFT)
  RECIPE("knife_steak", CC_NONCRAFT, NULL, NULL, 0, 2000, true);
  COMP("spike", 1);

  RECIPE("lawnmower", CC_NONCRAFT, NULL, NULL, 0, 1000, true);
  TOOL("wrench", -1);
  TOOLCONT("toolset", -1);
  COMP("scrap", 8);
  COMP("spring", 2);
  COMP("blade", 2);
  COMP("1cyl_combustion", 1);
  COMP("pipe", 3);

  RECIPE("lighter", CC_NONCRAFT, NULL, NULL, 0, 100, true);
  COMP("pilot_light", 1);

  RECIPE("tshirt", CC_NONCRAFT, "tailor", NULL, 0, 500, true);
  COMP("rag", 5);

  RECIPE("tshirt_fit", CC_NONCRAFT, "tailor", NULL, 0, 500, true);
  COMP("rag", 5);

  RECIPE("tank_top", CC_NONCRAFT, "tailor", NULL, 0, 500, true);
  COMP("rag", 5);

  RECIPE("string_36", CC_NONCRAFT, NULL, NULL, 0, 5000, true);
  TG_KNIVES
  COMP("string_6", 6);

  RECIPE("rope_6", CC_NONCRAFT, "tailor", NULL, 0, 5000, true);
  TG_KNIVES
  COMP("string_36", 6);

  RECIPE("rope_30", CC_NONCRAFT, "tailor", NULL, 0, 5000, true);
  TG_KNIVES
  COMP("rope_6", 5);
  // CRAFTABLE

  // WEAPONS

  RECIPE("makeshift_machete", CC_WEAPON, NULL, NULL, 0, 5000, true);
  COMP("duct_tape", 50);
  COMP("blade", 1);

  RECIPE("makeshift_halberd", CC_WEAPON, NULL, NULL, 0, 5000, true);
  COMP("duct_tape", 100);
  COMP("blade", 1);
  COMP("stick", 1);
  COMPCONT("mop", 1);
  COMPCONT("broom", 1);

  RECIPE("spear_wood", CC_WEAPON, NULL, NULL, 0, 800, false);
  TOOL("hatchet", -1);
  TG_KNIVES_CONT
  COMP("stick", 1);
  COMPCONT("broom", 1);
  COMPCONT("mop", 1);
  COMPCONT("2x4", 1);
  COMPCONT("pool_cue", 1);

  RECIPE("javelin", CC_WEAPON, "survival", NULL, 1, 5000, false);
  TOOL("hatchet", -1);
  TG_KNIVES_CONT
  TOOL("fire", -1);
  COMP("spear_wood", 1);
  COMP("rag", 1);
  COMPCONT("leather", 1);
  COMPCONT("fur", 1);
  COMP("plant_fibre", 20);
  COMPCONT("sinew", 20);

  RECIPE("spear_knife", CC_WEAPON, "stabbing", NULL, 0, 600, true);
  COMP("stick", 1);
  COMPCONT("broom", 1);
  COMPCONT("mop", 1);
  COMP("spike", 1);
  COMP("string_6", 6);
  COMPCONT("string_36", 1);

  RECIPE("longbow", CC_WEAPON, "archery", "survival", 2, 15000, true);
  TOOL("hatchet", -1);
  TG_KNIVES_CONT
  COMP("stick", 1);
  COMP("string_36", 2);
  COMPCONT("sinew", 200);
  COMPCONT("plant_fibre", 200);

  RECIPE("arrow_wood", CC_WEAPON, "archery", "survival", 1, 5000, false);
  TOOL("hatchet", -1);
  TG_KNIVES_CONT
  COMP("stick", 1);
  COMPCONT("broom", 1);
  COMPCONT("mop", 1);
  COMPCONT("2x4", 1);
  COMPCONT("bee_sting", 1);

  RECIPE("nailboard", CC_WEAPON, NULL, NULL, 0, 1000, true);
  TOOL("hatchet", -1);
  TOOLCONT("hammer", -1);
  TOOLCONT("rock", -1);
  TOOLCONT("toolset", -1);
  COMP("2x4", 1);
  COMPCONT("stick", 1);
  COMP("nail", 6);

  RECIPE("nailbat", CC_WEAPON, NULL, NULL, 0, 1000, true);
  TOOL("hatchet", -1);
  TOOLCONT("hammer", -1);
  TOOLCONT("rock", -1);
  TOOLCONT("toolset", -1);
  COMP("bat", 1);
  COMP("nail", 6);

  // molotovs use 750ml of flammable liquids
  RECIPE("molotov", CC_WEAPON, NULL, NULL, 0, 500, false);
  COMP("rag", 1);
  COMP("bottle_glass", 1);
  COMPCONT("flask_glass", 1);
  COMP("whiskey", 14);
  COMPCONT("vodka", 14);
  COMPCONT("rum", 14);
  COMPCONT("tequila", 14);
  COMPCONT("gin", 14);
  COMPCONT("triple_sec", 14);
  COMPCONT("gasoline", 600);

  RECIPE("pipebomb", CC_WEAPON, "mechanics", NULL, 1, 750, false);
  TOOL("hacksaw", -1);
  TOOLCONT("toolset", -1);
  COMP("pipe", 1);
  COMP("gasoline", 200);
  COMPCONT("shot_bird", 6);
  COMPCONT("shot_00", 2);
  COMPCONT("shot_slug", 2);
  COMP("string_36", 1);
  COMPCONT("string_6", 1);

  RECIPE("shotgun_sawn", CC_WEAPON, "gun", NULL, 0, 2000, false);
  TOOL("hacksaw", -1);
  TOOLCONT("toolset", -1);
  COMP("shotgun_d", 1);
  COMPCONT("remington_870", 1);
  COMPCONT("mossberg_500", 1);

  RECIPE("revolver_shotgun", CC_WEAPON, "gun", "mechanics", 2, 6000, false);
  TOOL("hacksaw", -1);
  TOOLCONT("toolset", -1);
  TOOL("welder", 30);
  TOOLCONT("toolset", 3);
  COMP("shotgun_s", 1);

  RECIPE("saiga_sawn", CC_WEAPON, "gun", NULL, 0, 2000, false);
  TOOL("hacksaw", -1);
  TOOLCONT("toolset", -1);
  COMP("saiga_12", 1);

  RECIPE("bolt_wood", CC_WEAPON, "mechanics", "archery", 1, 5000, false);
  TOOL("hatchet", -1);
  TG_KNIVES_CONT
  COMP("stick", 1);
  COMPCONT("broom", 1);
  COMPCONT("mop", 1);
  COMPCONT("2x4", 1);
  COMPCONT("bee_sting", 1);

  RECIPE("crossbow", CC_WEAPON, "mechanics", "archery", 3, 15000, true);
  TOOL("wrench", -1);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  COMP("2x4", 1);
  COMPCONT("stick", 4);
  COMP("hose", 1);

  RECIPE("rifle_22", CC_WEAPON, "mechanics", "gun", 3, 12000, true);
  TOOL("hacksaw", -1);
  TOOLCONT("toolset", -1);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  COMP("pipe", 1);
  COMP("2x4", 1);

  RECIPE("rifle_9mm", CC_WEAPON, "mechanics", "gun", 3, 14000, true);
  TOOL("hacksaw", -1);
  TOOLCONT("toolset", -1);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  COMP("pipe", 1);
  COMP("2x4", 1);

  RECIPE("smg_9mm", CC_WEAPON, "mechanics", "gun", 5, 18000, true);
  TOOL("hacksaw", -1);
  TOOLCONT("toolset", -1);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  TOOL("hammer", -1);
  TOOLCONT("rock", -1);
  TOOLCONT("hatchet", -1);
  TOOLCONT("toolset", -1);
  COMP("pipe", 1);
  COMP("2x4", 2);
  COMP("nail", 4);

  RECIPE("smg_45", CC_WEAPON, "mechanics", "gun", 5, 20000, true);
  TOOL("hacksaw", -1);
  TOOLCONT("toolset", -1);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  TOOL("hammer", -1);
  TOOLCONT("rock", -1);
  TOOLCONT("hatchet", -1);
  TOOLCONT("toolset", -1);
  COMP("pipe", 1);
  COMP("2x4", 2);
  COMP("nail", 4);

  RECIPE("flamethrower_simple", CC_WEAPON, "mechanics", "gun", 6, 12000, true);
  TOOL("hacksaw", -1);
  TOOLCONT("toolset", -1);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  COMP("pilot_light", 2);
  COMP("pipe", 1);
  COMP("hose", 2);
  COMP("bottle_glass", 4);
  COMPCONT("bottle_plastic", 6);

  RECIPE("launcher_simple", CC_WEAPON, "mechanics", "launcher", 6, 6000, true);
  TOOL("hacksaw", -1);
  TOOLCONT("toolset", -1);
  COMP("pipe", 1);
  COMP("2x4", 1);
  COMP("nail", 1);

  RECIPE("shot_he", CC_WEAPON, "mechanics", "gun", 4, 2000, false);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  COMP("superglue", 1);
  COMP("shot_slug", 4);
  COMP("gasoline", 200);

  RECIPE("acidbomb", CC_WEAPON, "cooking", NULL, 1, 10000, false);
  TOOL("hotplate", 5);
  TOOLCONT("toolset", 1);
  COMP("bottle_glass", 1);
  COMPCONT("flask_glass", 1);
  COMP("battery", 500);

  RECIPE("grenade", CC_WEAPON, "mechanics", NULL, 2, 5000, false);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  COMP("pilot_light", 1);
  COMP("superglue", 1);
  COMPCONT("string_36", 1);
  COMP("can_food", 1);
  COMPCONT("can_drink", 1);
  COMPCONT("canister_empty", 1);
  COMP("nail", 30);
  COMPCONT("bb", 100);
  COMP("shot_bird", 6);
  COMPCONT("shot_00", 3);
  COMPCONT("shot_slug", 2);
  COMPCONT("gasoline", 200);
  COMPCONT("gunpowder", 72);

  RECIPE("chainsaw_off", CC_WEAPON, "mechanics", NULL, 4, 20000, true);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  TOOL("hammer", -1);
  TOOLCONT("hatchet", -1);
  TOOL("wrench", -1);
  TOOLCONT("toolset", -1);
  COMP("motor", 1);
  COMP("chain", 1);

  RECIPE("smokebomb", CC_WEAPON, "cooking", "mechanics", 3, 7500, false);
  TOOL("screwdriver", -1);
  TOOLCONT("wrench", -1);
  TOOLCONT("toolset", -1);
  COMP("water", 1);
  COMPCONT("water_clean", 1);
  COMPCONT("salt_water", 1);
  COMP("candy", 1);
  COMPCONT("cola", 1);
  COMP("vitamins", 10);
  COMPCONT("aspirin", 8);
  COMP("canister_empty", 1);
  COMPCONT("can_food", 1);
  COMP("superglue", 1);


  RECIPE("gasbomb", CC_WEAPON, "cooking", "mechanics", 4, 8000, false);
  TOOL("screwdriver", -1);
  TOOLCONT("wrench", -1);
  TOOLCONT("toolset", -1);
  COMP("bleach", 2);
  COMP("ammonia", 2);
  COMP("canister_empty", 1);
  COMPCONT("can_food", 1);
  COMP("superglue", 1);

  RECIPE("nx17", CC_WEAPON, "electronics", "mechanics", 8, 40000, true);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  TOOL("soldering_iron", 6);
  TOOLCONT("toolset", 6);
  COMP("vacutainer", 1);
  COMP("power_supply", 8);
  COMP("amplifier", 8);

  RECIPE("mininuke", CC_WEAPON, "mechanics", "electronics", 10, 40000, true);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  TOOL("wrench", -1);
  TOOLCONT("toolset", -1);
  COMP("can_food", 2);
  COMPCONT("steel_chunk", 2);
  COMPCONT("canister_empty", 1);
  COMP("plut_cell", 6);
  COMP("battery", 2);
  COMP("power_supply", 1);

  RECIPE("9mm", CC_AMMO, "gun", "mechanics", 2, 25000, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("9mm_casing", 50);
  COMP("smpistol_primer", 50);
  COMP("gunpowder", 200);
  COMP("lead", 200);

  RECIPE("9mmP", CC_AMMO, "gun", "mechanics", 4, 12500, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("9mm_casing", 25);
  COMP("smpistol_primer", 25);
  COMP("gunpowder", 125);
  COMP("lead", 100);

  RECIPE("9mmP2", CC_AMMO, "gun", "mechanics", 6, 5000, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("9mm_casing", 10);
  COMP("smpistol_primer", 10);
  COMP("gunpowder", 60);
  COMP("lead", 40);

  RECIPE("38_special", CC_AMMO, "gun", "mechanics", 2, 25000, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("38_casing", 50);
  COMP("smpistol_primer", 50);
  COMP("gunpowder", 250);
  COMP("lead", 250);

  RECIPE("38_super", CC_AMMO, "gun", "mechanics", 4, 12500, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("38_casing", 25);
  COMP("smpistol_primer", 25);
  COMP("gunpowder", 175);
  COMP("lead", 125);

  RECIPE("40sw", CC_AMMO, "gun", "mechanics", 3, 30000, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("40_casing", 50);
  COMP("smpistol_primer", 50);
  COMP("gunpowder", 300);
  COMP("lead", 300);

  RECIPE("10mm", CC_AMMO, "gun", "mechanics", 5, 25000, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("40_casing", 50);
  COMP("lgpistol_primer", 50);
  COMP("gunpowder", 400);
  COMP("lead", 400);

  RECIPE("44magnum", CC_AMMO, "gun", "mechanics", 4, 25000, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("44_casing", 50);
  COMP("lgpistol_primer", 50);
  COMP("gunpowder", 500);
  COMP("lead", 500);

  RECIPE("45_acp", CC_AMMO, "gun", "mechanics", 3, 25000, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("45_casing", 50);
  COMP("lgpistol_primer", 50);
  COMP("gunpowder", 500);
  COMP("lead", 400);

  RECIPE("45_super", CC_AMMO, "gun", "mechanics", 6, 5000, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("45_casing", 10);
  COMP("lgpistol_primer", 10);
  COMP("gunpowder", 120);
  COMP("lead", 100);

  RECIPE("57mm", CC_AMMO, "gun", "mechanics", 4, 50000, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("57mm_casing", 100);
  COMP("smrifle_primer", 100);
  COMP("gunpowder", 400);
  COMP("lead", 200);

  RECIPE("46mm", CC_AMMO, "gun", "mechanics", 4, 50000, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("46mm_casing", 100);
  COMP("smpistol_primer", 100);
  COMP("gunpowder", 400);
  COMP("lead", 200);

  RECIPE("762_m43", CC_AMMO, "gun", "mechanics", 3, 40000, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("762_casing", 80);
  COMP("lgrifle_primer", 80);
  COMP("gunpowder", 560);
  COMP("lead", 400);

  RECIPE("762_m87", CC_AMMO, "gun", "mechanics", 5, 40000, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("762_casing", 80);
  COMP("lgrifle_primer", 80);
  COMP("gunpowder", 640);
  COMP("lead", 400);

  RECIPE("223", CC_AMMO, "gun", "mechanics", 3, 20000, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("223_casing", 40);
  COMP("smrifle_primer", 40);
  COMP("gunpowder", 160);
  COMP("lead", 80);

  RECIPE("556", CC_AMMO, "gun", "mechanics", 5, 20000, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("223_casing", 40);
  COMP("smrifle_primer", 40);
  COMP("gunpowder", 240);
  COMP("lead", 80);

  RECIPE("556_incendiary", CC_AMMO, "gun", "mechanics", 6, 15000, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("223_casing", 30);
  COMP("smrifle_primer", 30);
  COMP("gunpowder", 180);
  COMP("incendiary", 60);

  RECIPE("270", CC_AMMO, "gun", "mechanics", 3, 10000, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("3006_casing", 20);
  COMP("lgrifle_primer", 20);
  COMP("gunpowder", 200);
  COMP("lead", 100);

  RECIPE("3006", CC_AMMO, "gun", "mechanics", 5, 5000, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("3006_casing", 10);
  COMP("lgrifle_primer", 10);
  COMP("gunpowder", 120);
  COMP("lead", 80);

  RECIPE("3006_incendiary", CC_AMMO, "gun", "mechanics", 7, 2500, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("3006_casing", 5);
  COMP("lgrifle_primer", 5);
  COMP("gunpowder", 60);
  COMP("incendiary", 40);

  RECIPE("308", CC_AMMO, "gun", "mechanics", 3, 10000, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("308_casing", 20);
  COMP("lgrifle_primer", 20);
  COMP("gunpowder", 160);
  COMP("lead", 120);

  RECIPE("762_51", CC_AMMO, "gun", "mechanics", 5, 10000, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("308_casing", 20);
  COMP("lgrifle_primer", 20);
  COMP("gunpowder", 200);
  COMP("lead", 120);

  RECIPE("762_51_incendiary", CC_AMMO, "gun", "mechanics", 6, 5000, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("308_casing", 10);
  COMP("lgrifle_primer", 10);
  COMP("gunpowder", 100);
  COMP("incendiary", 60);

  RECIPE("shot_bird", CC_AMMO, "gun", "mechanics", 2, 12500, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("shot_hull", 25);
  COMP("shotgun_primer", 25);
  COMP("gunpowder", 300);
  COMP("lead", 400);

  RECIPE("shot_00", CC_AMMO, "gun", "mechanics", 3, 12500, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("shot_hull", 25);
  COMP("shotgun_primer", 25);
  COMP("gunpowder", 600);
  COMP("lead", 400);

  RECIPE("shot_slug", CC_AMMO, "gun", "mechanics", 3, 12500, false);
  TOOL("press", -1);
  TOOL("fire", -1);
  TOOLCONT("toolset", 1);
  TOOLCONT("hotplate", 4);
  TOOLCONT("press", 2);
  COMP("shot_hull", 25);
  COMP("shotgun_primer", 25);
  COMP("gunpowder", 600);
  COMP("lead", 400);
  /* We need a some Chemicals which arn't implemented to realistically craft this!
  RECIPE("c4", CC_WEAPON, "mechanics", "electronics", 4, 8000);
  TOOL("screwdriver", -1);
  COMP("can_food", 1);
  COMPCONT("steel_chunk", 1);
  COMPCONT("canister_empty", 1);
  COMP("battery", 1);
  COMP("superglue",1);

  COMP("soldering_iron",1);

  COMP("power_supply", 1);
  */

  // FOOD

  RECIPE("water_clean", CC_DRINK, "cooking", NULL, 0, 1000, false);
  TOOL("hotplate", 3);
  TOOLCONT("toolset", 1);
  TOOLCONT("fire", -1);
  TOOL("pan", -1);
  TOOLCONT("pot", -1);
  TOOLCONT("rock_pot", -1);
  COMP("water", 1);

  RECIPE("meat_cooked", CC_FOOD, "cooking", NULL, 0, 5000, false);
  TOOL("hotplate", 7);
  TOOLCONT("toolset", 1);
  TOOLCONT("fire", -1);
  TOOL("pan", -1);
  TOOLCONT("pot", -1);
  TOOLCONT("rock_pot", -1);
  TOOLCONT("spear_wood", -1);
  COMP("meat", 1);

  RECIPE("dogfood", CC_FOOD, "cooking", NULL, 4, 10000, false);
  TOOL("hotplate", 6);
  TOOLCONT("toolset", 1);
  TOOLCONT("fire", -1);
  TOOL("pot", -1);
  TOOLCONT("rock_pot", -1);
  COMP("meat", 1);
  COMP("veggy",1);
  COMPCONT("veggy_wild", 1);
  COMP("water",1);
  COMPCONT("water_clean", 1);

  RECIPE("veggy_cooked", CC_FOOD, "cooking", NULL, 0, 4000, false);
  TOOL("hotplate", 5);
  TOOLCONT("toolset", 1);
  TOOLCONT("fire", -1);
  TOOL("pan", -1);
  TOOLCONT("pot", -1);
  TOOLCONT("rock_pot", -1);
  TOOLCONT("spear_wood", -1);
  COMP("veggy", 1);

  RECIPE("veggy_wild_cooked", CC_FOOD, "cooking", NULL, 0, 4000, false);
  TOOL("hotplate", 5);
  TOOLCONT("toolset", 3);
  TOOLCONT("fire", -1);
  TOOL("pan", -1);
  TOOLCONT("pot", -1);
  TOOLCONT("rock_pot", -1);
  COMP("veggy_wild", 1);

  RECIPE("spaghetti_cooked", CC_FOOD, "cooking", NULL, 0, 10000, false);
  TOOL("hotplate", 4);
  TOOLCONT("toolset", 1);
  TOOLCONT("fire", -1);
  TOOL("pot", -1);
  TOOLCONT("rock_pot", -1);
  COMP("spaghetti_raw", 1);
  COMP("water", 1);
  COMPCONT("water_clean", 1);

  RECIPE("cooked_dinner", CC_FOOD, "cooking", NULL, 0, 5000, false);
  TOOL("hotplate", 3);
  TOOLCONT("toolset", 1);
  TOOLCONT("fire", -1);
  COMP("frozen_dinner", 1);

  RECIPE("macaroni_cooked", CC_FOOD, "cooking", NULL, 1, 10000, false);
  TOOL("hotplate", 4);
  TOOLCONT("toolset", 1);
  TOOLCONT("fire", -1);
  TOOL("pot", -1);
  TOOLCONT("rock_pot", -1);
  COMP("macaroni_raw", 1);
  COMP("water", 1);
  COMPCONT("water_clean", 1);

  RECIPE("potato_baked", CC_FOOD, "cooking", NULL, 1, 15000, false);
  TOOL("hotplate", 3);
  TOOLCONT("toolset", 1);
  TOOLCONT("fire", -1);
  TOOL("pan", -1);
  TOOLCONT("pot", -1);
  TOOLCONT("rock_pot", -1);
  COMP("potato_raw", 1);

  RECIPE("tea", CC_DRINK, "cooking", NULL, 0, 4000, false);
  TOOL("hotplate", 2);
  TOOLCONT("toolset", 1);
  TOOLCONT("fire", -1);
  TOOL("pot", -1);
  TOOLCONT("rock_pot", -1);
  COMP("tea_raw", 1);
  COMP("water", 1);
  COMPCONT("water_clean", 1);

  RECIPE("coffee", CC_DRINK, "cooking", NULL, 0, 4000, false);
  TOOL("hotplate", 2);
  TOOLCONT("toolset", 1);
  TOOLCONT("fire", -1);
  TOOL("pot", -1);
  TOOLCONT("rock_pot", -1);
  COMP("coffee_raw", 1);
  COMP("water", 1);
  COMPCONT("water_clean", 1);

  RECIPE("oj", CC_DRINK, "cooking", NULL, 1, 5000, false);
  TOOL("rock", -1);
  TOOLCONT("toolset", -1);
  COMP("orange", 2);
  COMP("water", 1);
  COMPCONT("water_clean", 1);

  RECIPE("apple_cider", CC_DRINK, "cooking", NULL, 2, 7000, false);
  TOOL("rock", -1);
  TOOLCONT("toolset", 1);
  COMP("apple", 3);

  RECIPE("long_island", CC_DRINK, "cooking", NULL, 1, 7000, false);
  COMP("cola", 1);
  COMP("vodka", 1);
  COMP("gin", 1);
  COMP("rum", 1);
  COMP("tequila", 1);
  COMP("triple_sec", 1);

  RECIPE("jerky", CC_FOOD, "cooking", NULL, 3, 30000, false);
  TOOL("hotplate", 10);
  TOOLCONT("toolset", 1);
  TOOLCONT("fire", -1);
  COMP("salt_water", 1);
  COMPCONT("salt", 4);
  COMP("meat", 1);

  RECIPE("V8", CC_FOOD, "cooking", NULL, 2, 5000, false);
  COMP("tomato", 1);
  COMP("broccoli", 1);
  COMP("zucchini", 1);

  RECIPE("broth", CC_FOOD, "cooking", NULL, 2, 10000, false);
  TOOL("hotplate", 5);
  TOOLCONT("toolset", 1);
  TOOLCONT("fire", -1);
  TOOL("pot", -1);
  TOOLCONT("rock_pot", -1);
  COMP("water", 1);
  COMPCONT("water_clean", 1);
  COMP("broccoli", 1);
  COMPCONT("zucchini", 1);
  COMPCONT("veggy", 1);
  COMPCONT("veggy_wild", 1);

  RECIPE("soup_veggy", CC_FOOD, "cooking", NULL, 2, 10000, false);
  TOOL("hotplate", 5);
  TOOLCONT("toolset", 1);
  TOOLCONT("fire", -1);
  TOOL("pot", -1);
  TOOLCONT("rock_pot", -1);
  COMP("broth", 2);
  COMP("macaroni_raw", 1);
  COMPCONT("potato_raw", 1);
  COMP("tomato", 2);
  COMPCONT("broccoli", 2);
  COMPCONT("zucchini", 2);
  COMPCONT("veggy", 2);
  COMPCONT("veggy_wild", 2);

  RECIPE("soup_meat", CC_FOOD, "cooking", NULL, 2, 10000, false);
  TOOL("hotplate", 5);
  TOOLCONT("toolset", 1);
  TOOLCONT("fire", -1);
  TOOL("pot", -1);
  TOOLCONT("rock_pot", -1);
  COMP("broth", 2);
  COMP("macaroni_raw", 1);
  COMPCONT("potato_raw", 1);
  COMP("meat", 2);

  RECIPE("bread", CC_FOOD, "cooking", NULL, 4, 20000, false);
  TOOL("hotplate", 8);
  TOOLCONT("toolset", 1);
  TOOLCONT("fire", -1);
  TOOL("pot", -1);
  TOOLCONT("rock_pot", -1);
  COMP("flour", 3);
  COMP("water", 2);
  COMPCONT("water_clean", 2);

  RECIPE("pie", CC_FOOD, "cooking", NULL, 3, 25000, false);
  TOOL("hotplate", 6);
  TOOLCONT("toolset", 1);
  TOOLCONT("fire", -1);
  TOOL("pan", -1);
  COMP("flour", 2);
  COMP("strawberries", 2);
  COMPCONT("apple", 2);
  COMPCONT("blueberries", 2);
  COMP("sugar", 2);
  COMP("water", 1);
  COMPCONT("water_clean", 1);

  RECIPE("pizza", CC_FOOD, "cooking", NULL, 3, 20000, false);
  TOOL("hotplate", 8);
  TOOLCONT("toolset", 1);
  TOOLCONT("fire", -1);
  TOOL("pan", -1);
  COMP("flour", 2);
  COMP("veggy", 1);
  COMPCONT("veggy_wild", 1);
  COMPCONT("tomato", 2);
  COMPCONT("broccoli", 1);
  COMP("sauce_pesto", 1);
  COMPCONT("sauce_red", 1);
  COMP("water", 1);
  COMPCONT("water_clean", 1);

  RECIPE("meth", CC_CHEM, "cooking", NULL, 5, 20000, false);
  TOOL("hotplate", 15);
  TOOLCONT("toolset", 1);
  TOOLCONT("fire", -1);
  TOOL("bottle_glass", -1);
  TOOLCONT("hose", -1);
  COMP("dayquil", 2);
  COMPCONT("royal_jelly", 1);
  COMP("aspirin", 40);
  COMP("caffeine", 20);
  COMPCONT("adderall", 5);
  COMPCONT("energy_drink", 2);

  RECIPE("crack",        CC_CHEM, "cooking", NULL,     4, 30000,false);
  TOOL("pot", -1);
  TOOLCONT("rock_pot", -1);
  TOOL("fire", -1);
  TOOLCONT("hotplate", 8);
  TOOLCONT("toolset", 1);
  COMP("water", 1);
  COMPCONT("water_clean", 1);
  COMP("coke", 12);
  COMP("ammonia", 1);

  RECIPE("poppy_sleep",  CC_CHEM, "cooking", "survival", 2, 5000, false);
  TOOL("pot", -1);
  TOOLCONT("rock_pot", -1);
  TOOLCONT("rock", -1);
  TOOL("fire", -1);
  COMP("poppy_bud", 2);
  COMP("poppy_flower", 1);

  RECIPE("poppy_pain",  CC_CHEM, "cooking", "survival", 2, 5000, false);
  TOOL("pot", -1);
  TOOLCONT("rock_pot", -1);
  TOOLCONT("rock", -1);
  TOOL("fire", -1);
  COMP("poppy_bud", 2);
  COMP("poppy_flower", 2);

  RECIPE("royal_jelly", CC_CHEM, "cooking", NULL, 5, 5000, false);
  COMP("honeycomb", 1);
  COMP("bleach", 2);
  COMPCONT("purifier", 1);

  RECIPE("heroin", CC_CHEM, "cooking", NULL, 6, 2000, false);
  TOOL("hotplate", 3);
  TOOLCONT("toolset", 1);
  TOOLCONT("fire", -1);
  TOOL("pan", -1);
  TOOLCONT("pot", -1);
  TOOLCONT("rock_pot", -1);
  COMP("salt_water", 1);
  COMPCONT("salt", 4);
  COMP("oxycodone", 40);

  RECIPE("mutagen", CC_CHEM, "cooking", "firstaid", 8, 10000, false);
  TOOL("hotplate", 25);
  TOOLCONT("toolset", 2);
  TOOLCONT("fire", -1);
  COMP("meat_tainted", 3);
  COMPCONT("veggy_tainted", 5);
  COMPCONT("fetus", 1);
  COMPCONT("arm", 2);
  COMPCONT("leg", 2);
  COMP("bleach", 2);
  COMP("ammonia", 1);

  RECIPE("purifier", CC_CHEM, "cooking", "firstaid", 9, 10000, false);
  TOOL("hotplate", 25);
  TOOLCONT("toolset", 2);
  TOOLCONT("fire", -1);
  COMP("royal_jelly", 4);
  COMPCONT("mutagen", 2);
  COMP("bleach", 3);
  COMP("ammonia", 2);

  // ELECTRONICS

  RECIPE("antenna", CC_ELECTRONIC, NULL, NULL, 0, 3000, false);
  TOOL("hacksaw", -1);
  TOOLCONT("toolset", -1);
  COMP("knife_butter", 2);

  RECIPE("amplifier", CC_ELECTRONIC, "electronics", NULL, 1, 4000, false);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  COMP("transponder", 2);

  RECIPE("power_supply", CC_ELECTRONIC, "electronics", NULL, 1, 6500, false);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  TOOL("soldering_iron", 3);
  TOOLCONT("toolset", 3);
  COMP("amplifier", 2);
  COMP("cable", 20);

  RECIPE("receiver", CC_ELECTRONIC, "electronics", NULL, 2, 12000, true);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  TOOL("soldering_iron", 4);
  TOOLCONT("toolset", 4);
  COMP("amplifier", 2);
  COMP("cable", 10);

  RECIPE("transponder", CC_ELECTRONIC, "electronics", NULL, 2, 14000, true);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  TOOL("soldering_iron", 7);
  TOOLCONT("toolset", 7);
  COMP("receiver", 3);
  COMP("cable", 5);

  RECIPE("flashlight", CC_ELECTRONIC, "electronics", NULL, 1, 10000, true);
  COMP("amplifier", 1);
  COMP("scrap", 4);
  COMPCONT("can_drink", 1);
  COMPCONT("bottle_glass", 1);
  COMPCONT("bottle_plastic", 1);
  COMP("cable", 10);

  RECIPE("soldering_iron", CC_ELECTRONIC, "electronics", NULL, 1, 20000, true);
  COMP("antenna", 1);
  COMPCONT("screwdriver", 1);
  COMPCONT("xacto", 1);
  COMPCONT("knife_butter", 1);
  COMP("power_supply", 1);
  COMP("scrap", 2);

  RECIPE("battery", CC_ELECTRONIC, "electronics", "mechanics", 2, 5000, false);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  COMP("ammonia", 1);
  COMPCONT("lemon", 1);
  COMP("steel_chunk", 1);
  COMPCONT("knife_butter", 1);
  COMPCONT("knife_steak", 1);
  COMPCONT("bolt_steel", 1);
  COMPCONT("scrap", 1);
  COMP("can_drink", 1);
  COMPCONT("can_food", 1);

  RECIPE("coilgun", CC_WEAPON, "electronics", NULL, 3, 25000, true);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  TOOL("soldering_iron", 10);
  TOOLCONT("toolset", 10);
  COMP("pipe", 1);
  COMP("power_supply", 1);
  COMP("amplifier", 1);
  COMP("scrap", 6);
  COMP("cable", 20);

  RECIPE("radio", CC_ELECTRONIC, "electronics", NULL, 2, 25000, true);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  TOOL("soldering_iron", 10);
  TOOLCONT("toolset", 10);
  COMP("receiver", 1);
  COMP("antenna", 1);
  COMP("scrap", 5);
  COMP("cable", 7);

  RECIPE("water_purifier", CC_ELECTRONIC, "mechanics","electronics",3,25000, true);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  COMP("element", 2);
  COMP("bottle_glass", 2);
  COMPCONT("bottle_plastic", 5);
  COMP("hose", 1);
  COMP("scrap", 3);
  COMP("cable", 5);

  RECIPE("hotplate", CC_ELECTRONIC, "electronics", NULL, 3, 30000, true);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  COMP("element", 1);
  COMP("amplifier", 1);
  COMP("scrap", 2);
  COMPCONT("pan", 1);
  COMPCONT("pot", 1);
  COMPCONT("knife_butcher", 2);
  COMPCONT("knife_steak", 6);
  COMPCONT("knife_butter", 6);
  COMPCONT("muffler", 1);
  COMP("cable", 10);

  RECIPE("tazer", CC_ELECTRONIC, "electronics", NULL, 3, 25000, true);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  TOOL("soldering_iron", 10);
  TOOLCONT("toolset", 10);
  COMP("amplifier", 1);
  COMP("power_supply", 1);
  COMP("scrap", 2);

  RECIPE("two_way_radio", CC_ELECTRONIC, "electronics", NULL, 4, 30000, true);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  TOOL("soldering_iron", 14);
  TOOLCONT("toolset", 14);
  COMP("amplifier", 1);
  COMP("transponder", 1);
  COMP("receiver", 1);
  COMP("antenna", 1);
  COMP("scrap", 5);
  COMP("cable", 10);

  RECIPE("electrohack", CC_ELECTRONIC, "electronics", "computer", 4, 35000, true);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  TOOL("soldering_iron", 10);
  TOOLCONT("toolset", 10);
  COMP("processor", 1);
  COMP("RAM", 1);
  COMP("scrap", 4);
  COMP("cable", 10);

  RECIPE("EMPbomb", CC_ELECTRONIC, "electronics", NULL, 4, 32000, false);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  TOOL("soldering_iron", 6);
  TOOLCONT("toolset", 6);
  COMP("superglue", 1);
  COMPCONT("string_36", 1);
  COMP("scrap", 3);
  COMPCONT("can_food", 1);
  COMPCONT("can_drink", 1);
  COMPCONT("canister_empty", 1);
  COMP("power_supply", 1);
  COMPCONT("amplifier", 1);
  COMP("cable", 5);

  RECIPE("mp3", CC_ELECTRONIC, "electronics", "computer", 5, 40000, true);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  TOOL("soldering_iron", 5);
  TOOLCONT("toolset", 5);
  COMP("superglue", 1);
  COMP("antenna", 1);
  COMP("amplifier", 1);
  COMP("cable", 2);

  RECIPE("geiger_off", CC_ELECTRONIC, "electronics", NULL, 5, 35000, true);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  TOOL("soldering_iron", 14);
  TOOLCONT("toolset", 14);
  COMP("power_supply", 1);
  COMP("amplifier", 2);
  COMP("scrap", 6);
  COMP("cable", 10);

  RECIPE("UPS_off", CC_ELECTRONIC, "electronics", NULL, 5, 45000, true);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  TOOL("soldering_iron", 24);
  TOOLCONT("toolset", 24);
  COMP("power_supply", 4);
  COMP("amplifier", 3);
  COMP("scrap", 4);
  COMP("cable", 10);

  RECIPE("bio_battery", CC_ELECTRONIC, "electronics", NULL, 6, 50000, true);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  TOOL("soldering_iron", 20);
  TOOLCONT("toolset", 20);
  COMP("power_supply", 6);
  COMPCONT("UPS_off", 1);
  COMP("amplifier", 4);
  COMP("plut_cell", 1);

  RECIPE("teleporter", CC_ELECTRONIC, "electronics", NULL, 8, 50000, true);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  TOOL("wrench", -1);
  TOOLCONT("toolset", -1);
  TOOL("soldering_iron", 16);
  TOOLCONT("toolset", 16);
  COMP("power_supply", 3);
  COMPCONT("plut_cell", 5);
  COMP("amplifier", 3);
  COMP("transponder", 3);
  COMP("scrap", 10);
  COMP("cable", 20);

// ARMOR
// Feet
  RECIPE("socks", CC_ARMOR, "tailor", NULL, 0, 10000, false);
  TOOL("needle_bone", 4);
  TOOLCONT("sewing_kit",  4);
  COMP("rag", 2);

  RECIPE("mocassins", CC_ARMOR, "tailor", NULL, 1, 30000, false);
  TOOL("needle_bone", 5);
  TOOLCONT("sewing_kit",  5);
  COMP("fur", 2);

  RECIPE("boots_fit", CC_ARMOR, "tailor", NULL, 2, 35000, false);
  TOOL("needle_bone", 5);
  TOOLCONT("sewing_kit", 10);
  COMP("leather", 7);

  RECIPE("boots_chitin", CC_ARMOR, "tailor", NULL, 3,  30000, false);
  COMP("string_36", 1);
  COMPCONT("string_6", 4);
  COMP("chitin_piece", 4);
  COMP("leather", 2);
  COMPCONT("fur", 2);
  COMPCONT("rag", 2);

// Legs
  RECIPE("shorts", CC_ARMOR, "tailor", NULL, 1, 25000, false);
  TOOL("needle_bone", 10);
  TOOLCONT("sewing_kit", 10);
  COMP("rag", 5);

  RECIPE("shorts_cargo", CC_ARMOR, "tailor", NULL, 2, 30000, false);
  TOOL("needle_bone", 12);
  TOOLCONT("sewing_kit", 12);
  COMP("rag", 6);

  RECIPE("jeans_fit", CC_ARMOR, "tailor", NULL, 2, 45000, false);
  TOOL("needle_bone", 10);
  TOOLCONT("sewing_kit", 10);
  COMP("rag", 6);

  RECIPE("pants_cargo_fit", CC_ARMOR, "tailor", NULL, 3, 48000, false);
  TOOL("needle_bone", 16);
  TOOLCONT("sewing_kit", 16);
  COMP("rag", 8);

  RECIPE("long_underpants_fit", CC_ARMOR, "tailor", "survival", 3, 35000, false);
  TOOL("needle_bone", 15);
  TOOLCONT("sewing_kit", 15);
  COMP("rag", 10);

  RECIPE("pants_leather", CC_ARMOR, "tailor", NULL, 4, 50000, false);
  TOOL("needle_bone", 10);
  TOOLCONT("sewing_kit", 10);
  COMP("leather", 10);

  RECIPE("tank_top_fit", CC_ARMOR, "tailor", NULL, 2, 38000, true);
  TOOL("needle_bone", 4);
  TOOLCONT("sewing_kit", 4);
  COMP("rag", 4);

  RECIPE("tshirt_fit", CC_ARMOR, "tailor", NULL, 2, 38000, true);
  TOOL("needle_bone", 4);
  TOOLCONT("sewing_kit", 4);
  COMP("rag", 5);

  RECIPE("hoodie_fit", CC_ARMOR, "tailor", NULL, 3, 40000, false);
  TOOL("needle_bone", 14);
  TOOLCONT("sewing_kit", 14);
  COMP("rag", 12);

  RECIPE("trenchcoat_fit", CC_ARMOR, "tailor", NULL, 3, 42000, false);
  TOOL("needle_bone", 24);
  TOOLCONT("sewing_kit", 24);
  COMP("rag", 11);

  RECIPE("coat_fur", CC_ARMOR, "tailor", NULL, 4, 100000, false);
  TOOL("needle_bone", 20);
  TOOLCONT("sewing_kit", 20);
  COMP("fur", 10);

  RECIPE("jacket_leather_fit", CC_ARMOR, "tailor", NULL, 5, 150000, false);
  TOOL("needle_bone", 30);
  TOOLCONT("sewing_kit", 30);
  COMP("leather", 16);


  RECIPE("gloves_liner_fit", CC_ARMOR, "tailor", NULL, 1, 10000, false);
  TOOL("needle_bone", 2);
  TOOLCONT("sewing_kit", 2);
  COMP("rag", 2);

  RECIPE("gloves_light", CC_ARMOR, "tailor", NULL, 1, 10000, false);
  TOOL("needle_bone", 4);
  TOOLCONT("sewing_kit", 4);
  COMP("rag", 1);

  RECIPE("gloves_fingerless", CC_ARMOR, "tailor", NULL, 0, 16000, false);
  TOOL("scissors", -1);
  TG_KNIVES_CONT
  COMP("gloves_leather", 1);

  RECIPE("gloves_leather", CC_ARMOR, "tailor", NULL, 2, 16000, false);
  TOOL("needle_bone", 6);
  TOOLCONT("sewing_kit", 6);
  COMP("leather", 2);

  RECIPE("armguard_metal", CC_ARMOR, "tailor", NULL, 4,  30000, false);
  TOOL("hammer", -1);
  TOOLCONT("toolset", -1);
  COMP("string_36", 1);
  COMPCONT("string_6", 4);
  COMP("steel_chunk", 2);

  RECIPE("gauntlets_chitin", CC_ARMOR, "tailor", NULL, 3,  30000, false);
  COMP("string_36", 1);
  COMPCONT("string_6", 4);
  COMP("chitin_piece", 4);

  // Face
  RECIPE("mask_filter", CC_ARMOR, "mechanics", "tailor", 1, 5000, true);
  COMP("bag_plastic", 2);
  COMPCONT("bottle_plastic", 1);
  COMP("rag", 2);
  COMPCONT("muffler", 1);
  COMPCONT("bandana", 2);
  COMPCONT("wrapper", 4);

  RECIPE("glasses_safety", CC_ARMOR, "tailor", NULL, 1, 8000, false);
  TOOL("scissors", -1);
  TG_KNIVES
  TOOLCONT("toolset", -1);
  COMP("string_36", 1);
  COMPCONT("string_6", 2);
  COMP("bottle_plastic", 1);


  RECIPE("mask_gas", CC_ARMOR, "tailor", NULL, 3, 20000, true);
  TOOL("wrench", -1);
  TOOLCONT("toolset", -1);
  COMP("goggles_ski", 1);
  COMPCONT("goggles_swim", 2);
  COMP("mask_filter", 3);
  COMPCONT("muffler", 1);
  COMP("hose", 1);

  RECIPE("goggles_nv", CC_ARMOR, "electronics", "tailor", 5, 40000, true);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  COMP("goggles_ski", 1);
  COMPCONT("goggles_welding", 1);
  COMPCONT("mask_gas", 1);
  COMP("power_supply", 1);
  COMP("amplifier", 3);
  COMP("scrap", 5);

  //head
  RECIPE("hat_fur", CC_ARMOR, "tailor", NULL, 2, 40000, false);
  TOOL("needle_bone", 8);
  TOOLCONT("sewing_kit", 8);
  COMP("fur", 3);

  RECIPE("helmet_chitin", CC_ARMOR, "tailor", NULL, 6,  60000, false);
  COMP("string_36", 1);
  COMPCONT("string_6", 5);
  COMP("chitin_piece", 5);

  RECIPE("armor_chitin", CC_ARMOR, "tailor", NULL,  7, 100000, false);
  COMP("string_36", 2);
  COMPCONT("string_6", 12);
  COMP("chitin_piece", 15);

  //Storage
  RECIPE("backpack", CC_ARMOR, "tailor", NULL, 3, 50000, false);
  TOOL("needle_bone", 20);
  TOOLCONT("sewing_kit", 20);
  COMP("rag", 20);
  COMPCONT("fur", 16);
  COMPCONT("leather", 12);


  // SURVIVAL

  RECIPE("primitive_hammer", CC_MISC, "survival", "construction", 0, 5000, false);
  TOOL("rock", -1);
  TOOLCONT("hammer", -1);
  TOOLCONT("toolset", -1);
  COMP("stick", 1);
  COMP("rock", 1);
  COMP("string_6", 2);
  COMPCONT("sinew", 40);
  COMPCONT("plant_fibre", 40);

  RECIPE("needle_bone", CC_MISC, "survival", NULL, 3, 20000, false);
  TOOL("toolset", -1);
  TG_KNIVES_CONT
  COMP("bone", 1);

  RECIPE("digging_stick", CC_MISC, "survival", NULL, 1, 20000, false);
  TG_KNIVES
  TOOLCONT("hatchet", -1);
  COMP("stick", 1);

  RECIPE("ragpouch", CC_ARMOR, "tailor",  NULL, 0, 10000, false);
  TOOL("needle_bone", 20);
  TOOLCONT("sewing_kit", 20);
  COMP("rag", 6);
  COMP("string_36", 1);
  COMPCONT("string_6", 6);
  COMPCONT("sinew", 20);
  COMPCONT("plant_fibre", 20);

  RECIPE("leather_pouch", CC_ARMOR, "tailor",  "survival", 2, 10000, false);
  TOOL("needle_bone", 20);
  TOOLCONT("sewing_kit", 20);
  COMP("leather", 6);
  COMP("string_36", 1);
  COMPCONT("string_6", 6);
  COMPCONT("sinew", 20);
  COMPCONT("plant_fibre", 20);

  RECIPE("rock_pot", CC_MISC, "survival", "cooking", 2, 20000, false);
  TOOL("hammer", -1);
  TOOLCONT("primitive_hammer", -1);
  TOOLCONT("toolset", -1);
  COMP("rock", 3);
  COMP("sinew", 80);
  COMPCONT("plant_fibre", 80);
  COMPCONT("string_36", 1);

  RECIPE("primitive_shovel", CC_MISC, "survival", "construction", 2, 60000, false);
  TOOL("primitive_hammer", -1);
  TOOLCONT("hammer", -1);
  TOOLCONT("toolset", -1);
  COMP("stick", 1);
  COMP("rock", 1);
  COMP("string_6", 2);
  COMPCONT("sinew", 40);
  COMPCONT("plant_fibre", 40);

  RECIPE("primitive_axe", CC_MISC, "survival", "construction", 3, 60000, false);
  TOOL("primitive_hammer", -1);
  TOOLCONT("hammer", -1);
  TOOLCONT("toolset", -1);
  COMP("stick", 1);
  COMP("rock", 1);
  COMP("string_6", 2);
  COMPCONT("sinew", 40);
  COMPCONT("plant_fibre", 40);

  RECIPE("waterskin", CC_MISC, "tailor", "survival", 2, 30000, false);
  TOOL("sewing_kit", 60);
  TOOLCONT("needle_bone", 60);
  COMP("sinew", 40);
  COMPCONT("plant_fibre", 40);
  COMPCONT("string_36", 1);
  COMP("leather", 6);
  COMPCONT("fur", 6);


  RECIPE("shelter_kit", CC_MISC, "survival", "construction", 2, 50000, false);
  TOOL("sewing_kit", 200);
  TOOLCONT("needle_bone", 200);
  COMP("stick", 10);
  COMP("leather", 20);
  COMP("string_6", 10);
  COMPCONT("sinew", 500);
  COMPCONT("plant_fibre", 500);

  RECIPE("shelter_kit", CC_MISC, "survival", "tailoring", 0, 20000, false);
  TOOL("sewing_kit", 50);
  TOOLCONT("needle_bone", 50);
  COMP("stick", 3);
  COMP("leather", 4);
  COMP("sinew", 60);
  COMPCONT("plant_fibre", 60);
  COMPCONT("string_6", 1);
  COMP("damaged_shelter_kit", 1);

  RECIPE("snare_trigger", CC_MISC, "survival", NULL, 1, 2000, false);
  TG_KNIVES
  COMP("stick", 1);

  RECIPE("light_snare_kit", CC_MISC, "survival", "traps", 1, 5000, true);
  COMP("snare_trigger", 1);
  COMP("string_36", 1);

  RECIPE("heavy_snare_kit", CC_MISC, "survival", "traps", 3, 8000, true);
  COMP("snare_trigger", 1);
  COMP("rope_6", 1);

  // MISC
  RECIPE("kitchen_unit", CC_MISC, "mechanics", NULL, 4, 60000, true);
  TOOL("wrench", -1);
  TOOLCONT("toolset", -1);
  TOOL("hammer", -1);
  TOOLCONT("toolset", -1);
  TOOL("welder", 100);
  TOOLCONT("toolset", 10);
  TOOL("hacksaw", -1);
  TOOLCONT("toolset", -1);
  COMP("pipe", 2);
  COMP("steel_chunk", 16);
  COMPCONT("steel_plate", 2);
  COMP("hotplate", 1);
  COMP("pot", 1);
  COMP("pan", 1);

  RECIPE("rag", CC_MISC, NULL, NULL, 0, 3000, false);
  TOOL("fire", -1);
  TOOLCONT("hotplate", 3);
  TOOLCONT("toolset", 1);
  COMP("water", 1);
  COMPCONT("water_clean", 1);
  COMP("rag_bloody", 1);

  RECIPE("sheet", CC_MISC, NULL, NULL, 0, 10000, false);
  TOOL("sewing_kit", 50);
  COMP("rag", 20);

  RECIPE("vehicle_controls", CC_MISC, "mechanics", NULL, 3, 30000, true);
  TOOL("wrench", -1);
  TOOLCONT("toolset", -1);
  TOOL("hammer", -1);
  TOOLCONT("toolset", -1);
  TOOL("welder", 50);
  TOOLCONT("toolset", 5);
  TOOL("hacksaw", -1);
  TOOLCONT("toolset", -1);
  COMP("pipe", 10);
  COMP("steel_chunk", 12);
  COMP("wire", 3);


  RECIPE("thread", CC_MISC, "tailor", NULL, 1, 3000, false);
  COMP("string_6", 1);

  RECIPE("string_6", CC_MISC, NULL, NULL, 0, 5000, true);
  COMP("thread", 50);

  RECIPE("string_36", CC_MISC, NULL, NULL, 0, 5000, true);
  COMP("string_6", 6);

  RECIPE("rope_6", CC_MISC, "tailor", NULL, 0, 5000, true);
  COMP("string_36", 6);

  RECIPE("rope_30", CC_MISC, "tailor", NULL, 0, 5000, true);
  COMP("rope_6", 5);

  RECIPE("torch",        CC_MISC, NULL,    NULL,     0, 2000, false);
  COMP("stick", 1);
  COMPCONT("2x4", 1);
  COMPCONT("splinter", 1);
  COMPCONT("pool_cue", 1);
  COMPCONT("torch_done", 1);
  COMP("gasoline", 200);
  COMPCONT("vodka", 7);
  COMPCONT("rum", 7);
  COMPCONT("whiskey", 7);
  COMPCONT("tequila", 7);
  COMPCONT("gin", 7);
  COMPCONT("triple_sec", 7);
  COMP("rag", 1);

  RECIPE("candle",       CC_MISC, NULL,    NULL,     0, 5000, false);
  TOOL("lighter", 5);
  TOOLCONT("fire", -1);
  TOOLCONT("toolset", 1);
  COMP("can_food", -1);
  COMP("wax", 2);
  COMP("string_6", 1);

  RECIPE("spike",     CC_MISC, NULL, NULL, 0, 3000, false);
  TOOL("hammer", -1);
  TOOLCONT("toolset", -1);
  COMP("knife_combat", 1);
  COMPCONT("steel_chunk", 3);
  COMPCONT("scrap", 9);

  RECIPE("blade",     CC_MISC, NULL, NULL, 0, 3000, false);
  TOOL("hammer", -1);
  TOOLCONT("toolset", -1);
  COMP("broadsword", 1);
  COMPCONT("machete", 1);
  COMPCONT("pike", 1);

  RECIPE("superglue", CC_MISC, "cooking", NULL, 2, 12000, false);
  TOOL("hotplate", 5);
  TOOLCONT("toolset", 1);
  TOOLCONT("fire", -1);
  COMP("water", 1);
  COMPCONT("water_clean", 1);
  COMP("bleach", 1);
  COMPCONT("ant_egg", 1);

  RECIPE("steel_lump", CC_MISC, "mechanics", NULL, 0, 5000, true);
  TOOL("welder", 20);
  TOOLCONT("toolset", 1);
  COMP("steel_chunk", 4);

  RECIPE("2x4", CC_MISC, NULL, NULL, 0, 8000, false);
  TOOL("saw", -1);
  COMP("stick", 1);

  RECIPE("frame", CC_MISC, "mechanics", NULL, 1, 8000, true);
  TOOL("welder", 50);
  TOOLCONT("toolset", 2);
  COMP("steel_lump", 3);

  RECIPE("sheet_metal", CC_MISC, "mechanics", NULL, 2, 4000, true);
  TOOL("welder", 20);
  TOOLCONT("toolset", 1);
  COMP("scrap", 4);

  RECIPE("steel_plate", CC_MISC, "mechanics", NULL,4, 12000, true);
  TOOL("welder", 100);
  TOOLCONT("toolset", 4);
  COMP("steel_lump", 8);

  RECIPE("spiked_plate", CC_MISC, "mechanics", NULL, 4, 12000, true);
  TOOL("welder", 120);
  TOOLCONT("toolset", 5);
  COMP("steel_lump", 8);
  COMP("steel_chunk", 4);
  COMPCONT("scrap", 8);

  RECIPE("hard_plate", CC_MISC, "mechanics", NULL, 4, 12000, true);
  TOOL("welder", 300);
  TOOLCONT("toolset", 12);
  COMP("steel_lump", 24);

  RECIPE("crowbar", CC_MISC, "mechanics", NULL, 1, 1000, false);
  TOOL("hatchet", -1);
  TOOLCONT("hammer", -1);
  TOOLCONT("rock", -1);
  TOOLCONT("toolset", -1);
  COMP("pipe", 1);

  RECIPE("bayonet", CC_MISC, "gun", NULL, 1, 500, true);
  COMP("spike", 1);
  COMP("string_36", 1);

  RECIPE("tripwire", CC_MISC, "traps", NULL, 1, 500, false);
  COMP("string_36", 1);
  COMP("superglue", 1);

  RECIPE("board_trap", CC_MISC, "traps", NULL, 2, 2500, true);
  TOOL("hatchet", -1);
  TOOLCONT("hammer", -1);
  TOOLCONT("rock", -1);
  TOOLCONT("toolset", -1);
  COMP("2x4", 3);
  COMP("nail", 20);

  RECIPE("beartrap", CC_MISC, "mechanics", "traps", 2, 3000, true);
  TOOL("wrench", -1);
  TOOLCONT("toolset", -1);
  COMP("scrap", 3);
  COMP("spring", 1);

  RECIPE("crossbow_trap", CC_MISC, "mechanics", "traps", 3, 4500, true);
  COMP("crossbow", 1);
  COMP("bolt_steel", 1);
  COMPCONT("bolt_wood", 4);
  COMP("string_6", 2);
  COMPCONT("string_36", 1);

  RECIPE("shotgun_trap", CC_MISC, "mechanics", "traps", 3, 5000, true);
  COMP("shotgun_sawn", 1);
  COMP("shot_00", 2);
  COMP("string_36", 1);
  COMPCONT("string_6", 2);

  RECIPE("blade_trap", CC_MISC, "mechanics", "traps", 4, 8000, true);
  TOOL("wrench", -1);
  TOOLCONT("toolset", -1);
  COMP("motor", 1);
  COMP("blade", 1);
  COMP("string_36", 1);

  RECIPE("boobytrap", CC_MISC, "mechanics", "traps",3,5000, false);
  COMP("grenade",1);

  COMP("string_6",1);

  COMP("can_food",1);


  RECIPE("landmine", CC_MISC, "traps", "mechanics", 5, 10000, false);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  COMP("superglue", 1);
  COMP("can_food", 1);
  COMPCONT("steel_chunk", 1);
  COMPCONT("canister_empty", 1);
  COMPCONT("scrap", 4);
  COMP("nail", 100);
  COMPCONT("bb", 200);
  COMP("shot_bird", 30);
  COMPCONT("shot_00", 15);
  COMPCONT("shot_slug", 12);
  COMPCONT("gasoline", 600);
  COMPCONT("grenade", 1);
  COMPCONT("gunpowder", 72);

  RECIPE("brazier", CC_MISC, "mechanics", NULL, 1, 2000, false);
  TOOL("hatchet", -1);
  TOOLCONT("hammer", -1);
  TOOLCONT("rock", -1);
  TOOLCONT("toolset", -1);
  COMP("sheet_metal",1);


  RECIPE("bandages", CC_MISC, "firstaid", NULL, 1, 500, false);
  COMP("rag", 3);
  COMP("superglue", 1);
  COMPCONT("duct_tape", 5);
  COMP("vodka", 7);
  COMPCONT("rum", 7);
  COMPCONT("whiskey", 7);
  COMPCONT("tequila", 7);
  COMPCONT("gin", 7);
  COMPCONT("triple_sec", 7);

  RECIPE("silencer", CC_MISC, "mechanics", NULL, 1, 650, false);
  TOOL("hacksaw", -1);
  TOOLCONT("toolset", -1);
  COMP("muffler", 1);
  COMPCONT("rag", 4);
  COMP("pipe", 1);

  RECIPE("pheromone", CC_MISC, "cooking", NULL, 3, 1200, false);
  TOOL("hotplate", 18);
  TOOLCONT("toolset", 9);
  TOOLCONT("fire", -1);
  COMP("meat_tainted", 1);
  COMP("ammonia", 1);

  RECIPE("laser_pack", CC_MISC, "electronics", NULL, 5, 10000, true);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  COMP("superglue", 1);
  COMP("plut_cell", 1);

  RECIPE("bot_manhack", CC_MISC, "electronics", "computer", 6, 8000, true);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  TOOL("soldering_iron", 10);
  TOOLCONT("toolset", 10);
  COMP("spike", 2);
  COMP("processor", 1);
  COMP("RAM", 1);
  COMP("power_supply", 1);
  //  COMP("battery", 400);
  COMPCONT("plut_cell", 1);
  //  COMP("scrap", 15);

  RECIPE("bot_turret", CC_MISC, "electronics", "computer", 7, 9000, true);
  TOOL("screwdriver", -1);
  TOOLCONT("toolset", -1);
  TOOL("soldering_iron", 14);
  TOOLCONT("toolset", 14);
  COMP("smg_9mm", 1);
  COMPCONT("uzi", 1);
  COMPCONT("tec9", 1);
  COMPCONT("calico", 1);
  COMPCONT("hk_mp5", 1);
  COMP("processor", 2);
  COMP("RAM", 2);
  COMP("power_supply", 1);
  //  COMP("battery", 500);
  COMPCONT("plut_cell", 1);
  //  COMP("scrap", 30);
}

bool game::crafting_allowed()
{
    if (u.morale_level() < MIN_MORALE_CRAFT) 
    {	// See morale.h
        add_msg("Your morale is too low to craft...");
        return false;
    }
    
    return true;
}

void game::recraft()
{
 if(u.lastrecipe == NULL)
 {
  popup("Craft something first");
 }
 else if (making_would_work(u.lastrecipe))
 {
  make_craft(u.lastrecipe);
 }
}

//TODO clean up this function to give better status messages (e.g., "no fire available")
bool game::making_would_work(recipe *making)
{
    if (!crafting_allowed())
    {
    	return false;
    }
    
    if(can_make(making))
    {
        if (itypes[(making->result)]->m1 == LIQUID)
        {
            if (u.has_watertight_container() || u.has_matching_liquid(itypes[making->result]->id)) 
            {
                return true;
            } 
            else 
            {
                popup("You don't have anything to store that liquid in!");
            }
        }
        else 
        {
            return true;
        }
    }
    else
    {
        popup("You can no longer make that craft!");
    }
    
    return false;
}
bool game::can_make(recipe *r)
{
    inventory crafting_inv = crafting_inventory();
    if((r->sk_primary != NULL && u.skillLevel(r->sk_primary) < r->difficulty) || (r->sk_secondary != NULL && u.skillLevel(r->sk_secondary) <= 0))
    {
        return false;
    }
    // under the assumption that all comp and tool's array contains all the required stuffs at the start of the array

    // check all tools
    for(int i = 0 ; i < 20 ; i++)
    {
        // if current tool is null(size 0), assume that there is no more after it.
        if(r->tools[i].size()==0)
        {
            break;
        }
        bool has_tool = false;
        for(int j = 0 ; j < r->tools[i].size() ; j++)
        {
            itype_id type = r->tools[i][j].type;
            int req = r->tools[i][j].count;
            if((req<= 0 && crafting_inv.has_amount(type,1)) || (req > 0 && crafting_inv.has_charges(type,req)))
            {
                has_tool = true;
                break;
            }
        }
        if(!has_tool)
        {
            return false;
        }
    }
    // check all components
    for(int i = 0 ; i < 20 ; i++)
    {
        if(r->components[i].size() == 0)
        {
            break;
        }
        bool has_comp = false;
        for(int j = 0 ; j < r->components[i].size() ; j++)
        {
            itype_id type = r->components[i][j].type;
            int req = r->components[i][j].count;
            if (itypes[type]->count_by_charges() && req > 0)
            {
                if (crafting_inv.has_charges(type, req))
                {
                    has_comp = true;
                    break;
                }
            }
            else if (crafting_inv.has_amount(type, abs(req)))
            {
                has_comp = true;
                break;
            }
        }
        if(!has_comp)
        {
            return false;
        }
    }
    return true;
}

void game::craft()
{
    if (!crafting_allowed())
    {
    	return;
    }

    recipe *rec = select_crafting_recipe();
    if (rec)
    {
        make_craft(rec);
    }
}

void game::long_craft()
{
    if (!crafting_allowed())
    {
    	return;
    }
    
    recipe *rec = select_crafting_recipe();
    if (rec)
    {
        make_all_craft(rec);
    }
}

recipe* game::select_crafting_recipe()
{
	WINDOW *w_head = newwin( 3, 80, (TERMY > 25) ? (TERMY-25)/2 : 0, (TERMX > 80) ? (TERMX -80)/2 : 0);
    WINDOW *w_data = newwin(22, 80, 3 + ((TERMY > 25) ? (TERMY-25)/2 : 0), (TERMX  > 80) ? (TERMX -80)/2 : 0);
    craft_cat tab = CC_WEAPON;
    std::vector<recipe*> current;
    std::vector<bool> available;
    item tmp;
    int line = 0, xpos, ypos;
    bool redraw = true;
    bool done = false;
    recipe *chosen = NULL;
    InputEvent input;

    inventory crafting_inv = crafting_inventory();
    std::string filterstring = "";
    do {
        if (redraw) 
        { // When we switch tabs, redraw the header
            redraw = false;
            line = 0;
            draw_recipe_tabs(w_head, tab, (filterstring == "")?false:true);
            current.clear();
            available.clear();
            // Set current to all recipes in the current tab; available are possible to make
            pick_recipes(current, available, tab,filterstring);
        }

        // Clear the screen of recipe data, and draw it anew
        werase(w_data);
        if(filterstring != "")
        {
            mvwprintz(w_data, 19, 5, c_white, "?: Describe, [F]ind , [R]eset");
        }
        else
        {
            mvwprintz(w_data, 19, 5, c_white, "?: Describe, [F]ind");
        }
        mvwprintz(w_data, 20, 5, c_white, "Press <ENTER> to attempt to craft object.");
        for (int i = 0; i < 80; i++) 
        {
            mvwputch(w_data, 21, i, c_ltgray, LINE_OXOX);
            if (i < 21) 
            {
                mvwputch(w_data, i, 0, c_ltgray, LINE_XOXO);
                mvwputch(w_data, i, 79, c_ltgray, LINE_XOXO);
            }
        }

        mvwputch(w_data, 21,  0, c_ltgray, LINE_XXOO); // _|
        mvwputch(w_data, 21, 79, c_ltgray, LINE_XOOX); // |_
        wrefresh(w_data);

        int recmin = 0, recmax = current.size();
        if(recmax > MAX_DISPLAYED_RECIPES)
        {
            if (line <= recmin + 9) 
            {
                for (int i = recmin; i < recmin + MAX_DISPLAYED_RECIPES; i++) 
                {
                    mvwprintz(w_data, i - recmin, 2, c_dkgray, "");	// Clear the line
                    if (i == line)
                    {
                        mvwprintz(w_data, i - recmin, 2, (available[i] ? h_white : h_dkgray),
                        itypes[current[i]->result]->name.c_str());
                    }
                    else
                    {
                        mvwprintz(w_data, i - recmin, 2, (available[i] ? c_white : c_dkgray),
                        itypes[current[i]->result]->name.c_str());
                    }
                }
            } 
            else if (line >= recmax - 9) 
            {
                for (int i = recmax - MAX_DISPLAYED_RECIPES; i < recmax; i++) 
                {
                    mvwprintz(w_data, 18 + i - recmax, 2, c_ltgray, "");	// Clear the line
                    if (i == line)
                    {
                        mvwprintz(w_data, 18 + i - recmax, 2, (available[i] ? h_white : h_dkgray),
                        itypes[current[i]->result]->name.c_str());
                    }
                    else
                    {
                        mvwprintz(w_data, 18 + i - recmax, 2, (available[i] ? c_white : c_dkgray),
                        itypes[current[i]->result]->name.c_str());
                    }
                }
            } 
            else 
            {
                for (int i = line - 9; i < line + 9; i++) 
                {
                    mvwprintz(w_data, 9 + i - line, 2, c_ltgray, "");	// Clear the line
                    if (i == line)
                    {
                        mvwprintz(w_data, 9 + i - line, 2, (available[i] ? h_white : h_dkgray),
                        itypes[current[i]->result]->name.c_str());
                    }
                    else
                    {
                        mvwprintz(w_data, 9 + i - line, 2, (available[i] ? c_white : c_dkgray),
                        itypes[current[i]->result]->name.c_str());
                    }
                }
            }
        } 
        else
        {
            for (int i = 0; i < current.size() && i < 23; i++) 
            {
                if (i == line)
                {
                    mvwprintz(w_data, i, 2, (available[i] ? h_white : h_dkgray),
                    itypes[current[i]->result]->name.c_str());
                }
                else
                {
                    mvwprintz(w_data, i, 2, (available[i] ? c_white : c_dkgray),
                    itypes[current[i]->result]->name.c_str());
                }
            }
        }
        if (current.size() > 0) 
        {
            nc_color col = (available[line] ? c_white : c_dkgray);
            mvwprintz(w_data, 0, 30, col, "Primary skill: %s",
            (current[line]->sk_primary == NULL ? "N/A" :
            current[line]->sk_primary->name().c_str()));
            mvwprintz(w_data, 1, 30, col, "Secondary skill: %s",
            (current[line]->sk_secondary == NULL ? "N/A" :
            current[line]->sk_secondary->name().c_str()));
            mvwprintz(w_data, 2, 30, col, "Difficulty: %d", current[line]->difficulty);
            if (current[line]->sk_primary == NULL)
            {
                mvwprintz(w_data, 3, 30, col, "Your skill level: N/A");
            }
            else
            {
                mvwprintz(w_data, 3, 30, col, "Your skill level: %d",
                // Macs don't seem to like passing this as a class, so force it to int
                (int)u.skillLevel(current[line]->sk_primary));
            }
            if (current[line]->time >= 1000)
            {
                mvwprintz(w_data, 4, 30, col, "Time to complete: %d minutes",
                int(current[line]->time / 1000));
            }
            else
            {
                mvwprintz(w_data, 4, 30, col, "Time to complete: %d turns",
                int(current[line]->time / 100));
            }
            mvwprintz(w_data, 5, 30, col, "Tools required:");
            if (current[line]->tools[0].size() == 0) 
            {
                mvwputch(w_data, 6, 30, col, '>');
                mvwprintz(w_data, 6, 32, c_green, "NONE");
                ypos = 6;
            } 
            else 
            {
                ypos = 5;
                // Loop to print the required tools
                for (int i = 0; i < 5 && current[line]->tools[i].size() > 0; i++) 
                {
                    ypos++;
                    xpos = 32;
                    mvwputch(w_data, ypos, 30, col, '>');
                    for (int j = 0; j < current[line]->tools[i].size(); j++) 
                    {
                        itype_id type = current[line]->tools[i][j].type;
                        int charges = current[line]->tools[i][j].count;
                        nc_color toolcol = c_red;

                        if (charges < 0 && crafting_inv.has_amount(type, 1))
                        {
                            toolcol = c_green;
                        }
                        else if (charges > 0 && crafting_inv.has_charges(type, charges))
                        {
                            toolcol = c_green;
                        }

                            std::stringstream toolinfo;
                            toolinfo << itypes[type]->name + " ";
                        if (charges > 0)
                        {
                            toolinfo << "(" << charges << " charges) ";
                        }
                        std::string toolname = toolinfo.str();
                        if (xpos + toolname.length() >= 80) 
                        {
                            xpos = 32;
                            ypos++;
                        }
                        mvwprintz(w_data, ypos, xpos, toolcol, toolname.c_str());
                        xpos += toolname.length();
                        if (j < current[line]->tools[i].size() - 1) 
                        {
                            if (xpos >= 77) 
                            {
                            xpos = 32;
                            ypos++;
                            }
                            mvwprintz(w_data, ypos, xpos, c_white, "OR ");
                            xpos += 3;
                        }
                    }
                }
            }
        // Loop to print the required components
            ypos++;
            mvwprintz(w_data, ypos, 30, col, "Components required:");
            for (int i = 0; i < 5; i++) 
            {
                if (current[line]->components[i].size() > 0) 
                {
                    ypos++;
                    mvwputch(w_data, ypos, 30, col, '>');
                }
                xpos = 32;
                for (int j = 0; j < current[line]->components[i].size(); j++) 
                {
                    int count = current[line]->components[i][j].count;
                    itype_id type = current[line]->components[i][j].type;
                    nc_color compcol = c_red;
                    if (itypes[type]->count_by_charges() && count > 0)  
                    {
                        if (crafting_inv.has_charges(type, count))
                        {
                            compcol = c_green;
                        }
                    } 
                    else if (crafting_inv.has_amount(type, abs(count)))
                    {
                        compcol = c_green;
                    }
                    std::stringstream dump;
                    dump << abs(count) << "x " << itypes[type]->name << " ";
                    std::string compname = dump.str();
                    if (xpos + compname.length() >= 80) 
                    {
                        ypos++;
                        xpos = 32;
                    }
                    mvwprintz(w_data, ypos, xpos, compcol, compname.c_str());
                    xpos += compname.length();
                    if (j < current[line]->components[i].size() - 1) 
                    {
                        if (xpos >= 77) 
                        {
                            ypos++;
                            xpos = 32;
                        }
                        mvwprintz(w_data, ypos, xpos, c_white, "OR ");
                        xpos += 3;
                    }
                }
            }
        }

        wrefresh(w_data);
        input = get_input();
        switch (input) 
        {
            case DirectionW:
            case DirectionUp:
                if (tab == CC_WEAPON)
                {
                    tab = CC_MISC;
                }
                else
                {
                    tab = craft_cat(int(tab) - 1);
                }
                redraw = true;
                break;
            case DirectionE:
            case DirectionDown:
                if (tab == CC_MISC)
                {
                    tab = CC_WEAPON;
                }
                else
                {
                    tab = craft_cat(int(tab) + 1);
                }
                redraw = true;
                break;
            case DirectionS:
                line++;
                break;
            case DirectionN:
                line--;
                break;
            case Confirm:
                if (!available[line])
                {
                    popup("You can't do that!");
                }
                else
                {// is player making a liquid? Then need to check for valid container
                    if (itypes[current[line]->result]->m1 == LIQUID)
                    {
                        if (u.has_watertight_container() || u.has_matching_liquid(itypes[current[line]->result]->id)) 
                        {
                            chosen = current[line];
                            done = true;
                            break;
                        } 
                        else 
                        {
                            popup("You don't have anything to store that liquid in!");
                        }
                    }
                    else 
                    {
                        chosen = current[line];
                        done = true;
                    }
                }
                break;
            case Help:
                tmp = item(itypes[current[line]->result], 0);
                full_screen_popup(tmp.info(true).c_str());
                redraw = true;
                break;
            case Filter:
                filterstring = string_input_popup("Search :",55,filterstring);
                redraw = true;
                break;
            case Reset:
                filterstring = "";
                redraw = true;
                break;

        }
        if (line < 0)
        {
            line = current.size() - 1;
        }
        else if (line >= current.size())
        {
            line = 0;
        }
    } while (input != Cancel && !done);

    werase(w_head);
    werase(w_data);
    delwin(w_head);
    delwin(w_data);
    refresh_all();
    
    return chosen;
}

void draw_recipe_tabs(WINDOW *w, craft_cat tab,bool filtered)
{
    werase(w);
    for (int i = 0; i < 80; i++)
    {
        mvwputch(w, 2, i, c_ltgray, LINE_OXOX);
    }

    mvwputch(w, 2,  0, c_ltgray, LINE_OXXO); // |^
    mvwputch(w, 2, 79, c_ltgray, LINE_OOXX); // ^|
    if(!filtered)
    {
        draw_tab(w,  2, "WEAPONS", (tab == CC_WEAPON) ? true : false);
        draw_tab(w, 13, "AMMO",    (tab == CC_AMMO)   ? true : false);
        draw_tab(w, 21, "FOOD",    (tab == CC_FOOD)   ? true : false);
        draw_tab(w, 29, "DRINKS",  (tab == CC_DRINK)  ? true : false);
        draw_tab(w, 39, "CHEMS",   (tab == CC_CHEM)   ? true : false);
        draw_tab(w, 48, "ELECTRONICS", (tab == CC_ELECTRONIC) ? true : false);
        draw_tab(w, 63, "ARMOR",   (tab == CC_ARMOR)  ? true : false);
        draw_tab(w, 72, "MISC",    (tab == CC_MISC)   ? true : false);
    }
    else
    {
        draw_tab(w,  2, "Searched", true);
    }

    wrefresh(w);
}

inventory game::crafting_inventory(){
 inventory crafting_inv;
 crafting_inv.form_from_map(this, point(u.posx, u.posy), PICKUP_RANGE);
 crafting_inv += u.inv;
 crafting_inv += u.weapon;
 if (u.has_bionic("bio_tools")) {
  item tools(itypes["toolset"], turn);
  tools.charges = u.power_level;
  crafting_inv += tools;
 }
 return crafting_inv;
}

void game::pick_recipes(std::vector<recipe*> &current,
                        std::vector<bool> &available, craft_cat tab,std::string filter)
{
    inventory crafting_inv = crafting_inventory();

    bool have_tool[5], have_comp[5];

    current.clear();
    available.clear();
    for (int i = 0; i < recipes.size(); i++) 
    {
        if(filter == "")
        {
            // Check if the category matches the tab, and we have the requisite skills
            if (recipes[i]->category == tab &&
                    (recipes[i]->sk_primary == NULL ||
                    u.skillLevel(recipes[i]->sk_primary) >= recipes[i]->difficulty) &&
                    (recipes[i]->sk_secondary == NULL ||
                    u.skillLevel(recipes[i]->sk_secondary) > 0))
            {
                if (recipes[i]->difficulty >= 0 )
                {
                    current.push_back(recipes[i]);
                }
            }
            available.push_back(false);
        }
        else
        {
            // if filter is on , put everything here
            if ((recipes[i]->sk_primary == NULL ||
                    u.skillLevel(recipes[i]->sk_primary) >= recipes[i]->difficulty) &&
                    (recipes[i]->sk_secondary == NULL ||
                    u.skillLevel(recipes[i]->sk_secondary) > 0))
            {
                if (recipes[i]->difficulty >= 0 && itypes[recipes[i]->result]->name.find(filter) != std::string::npos)
                {
                    current.push_back(recipes[i]);
                }
            }
            available.push_back(false);
        }
    }
    for (int i = 0; i < current.size() && i < 51; i++) 
    {
        //Check if we have the requisite tools and components
        if(can_make(current[i]))
        {
            available[i] = true;
        }
    }
}

void game::make_craft(recipe *making)
{
 u.assign_activity(this, ACT_CRAFT, making->time, making->id);
 u.moves = 0;
 u.lastrecipe = making;
}


void game::make_all_craft(recipe *making)
{
 u.assign_activity(this, ACT_LONGCRAFT, making->time, making->id);
 u.moves = 0;
 u.lastrecipe = making;
}

void game::complete_craft()
{
 recipe* making = recipes[u.activity.index]; // Which recipe is it?

// # of dice is 75% primary skill, 25% secondary (unless secondary is null)
 int skill_dice = u.skillLevel(making->sk_primary) * 3;
 if (making->sk_secondary == NULL)
   skill_dice += u.skillLevel(making->sk_primary);
 else
   skill_dice += u.skillLevel(making->sk_secondary);
// Sides on dice is 16 plus your current intelligence
 int skill_sides = 16 + u.int_cur;

 int diff_dice = making->difficulty * 4; // Since skill level is * 4 also
 int diff_sides = 24;	// 16 + 8 (default intelligence)

 int skill_roll = dice(skill_dice, skill_sides);
 int diff_roll  = dice(diff_dice,  diff_sides);

 if (making->sk_primary)
  u.practice(making->sk_primary, making->difficulty * 5 + 20);
 if (making->sk_secondary)
  u.practice(making->sk_secondary, 5);

// Messed up badly; waste some components.
 if (making->difficulty != 0 && diff_roll > skill_roll * (1 + 0.1 * rng(1, 5))) {
  add_msg("You fail to make the %s, and waste some materials.",
          itypes[making->result]->name.c_str());
  for (int i = 0; i < 5; i++) {
   if (making->components[i].size() > 0) {
    std::vector<component> copy = making->components[i];
    for (int j = 0; j < copy.size(); j++)
     copy[j].count = rng(0, copy[j].count);
    consume_items(copy);
   }
   if (making->tools[i].size() > 0)
    consume_tools(making->tools[i]);
  }
  u.activity.type = ACT_NULL;
  return;
  // Messed up slightly; no components wasted.
 } else if (diff_roll > skill_roll) {
  add_msg("You fail to make the %s, but don't waste any materials.",
          itypes[making->result]->name.c_str());
  //this method would only have been called from a place that nulls u.activity.type,
  //so it appears that it's safe to NOT null that variable here.
  //rationale: this allows certain contexts (e.g. ACT_LONGCRAFT) to distinguish major and minor failures
  return;
 }
// If we're here, the craft was a success!
// Use up the components and tools
 for (int i = 0; i < 5; i++) {
  if (making->components[i].size() > 0)
   consume_items(making->components[i]);
  if (making->tools[i].size() > 0)
   consume_tools(making->tools[i]);
 }

  // Set up the new item, and pick an inventory letter
 int iter = 0;
 item newit(itypes[making->result], turn, nextinv);

 // for food items
 if (newit.is_food())
  {
    int bday_tmp = turn % 3600;		// fuzzy birthday for stacking reasons
    newit.bday = int(turn) + 3600 - bday_tmp;

		if (newit.has_flag(IF_EATEN_HOT)) {	// hot foods generated
			newit.item_flags |= mfb(IF_HOT);
			newit.active = true;
			newit.item_counter = 600;
		}
  }
 if (!newit.craft_has_charges())
  newit.charges = 0;
 do {
  newit.invlet = nextinv;
  advance_nextinv();
  iter++;
 } while (u.has_item(newit.invlet) && iter < 52);
 //newit = newit.in_its_container(&itypes);
 if (newit.made_of(LIQUID))
  handle_liquid(newit, false, false);
 else {
// We might not have space for the item
  if (iter == 52 || u.volume_carried()+newit.volume() > u.volume_capacity()) {
   add_msg("There's no room in your inventory for the %s, so you drop it.",
             newit.tname().c_str());
   m.add_item(u.posx, u.posy, newit);
  } else if (u.weight_carried() + newit.volume() > u.weight_capacity()) {
   add_msg("The %s is too heavy to carry, so you drop it.",
           newit.tname().c_str());
   m.add_item(u.posx, u.posy, newit);
  } else {
   u.i_add(newit);
   add_msg("%c - %s", newit.invlet, newit.tname().c_str());
  }
 }
}

void game::consume_items(std::vector<component> components)
{
// For each set of components in the recipe, fill you_have with the list of all
// matching ingredients the player has.
 std::vector<component> player_has;
 std::vector<component> map_has;
 std::vector<component> mixed;
 std::vector<component> player_use;
 std::vector<component> map_use;
 std::vector<component> mixed_use;
 inventory map_inv;
 map_inv.form_from_map(this, point(u.posx, u.posy), PICKUP_RANGE);

 for (int i = 0; i < components.size(); i++) {
  itype_id type = components[i].type;
  int count = abs(components[i].count);
  bool pl = false, mp = false;


  if (itypes[type]->count_by_charges() && count > 0)
  {
   if (u.has_charges(type, count)) {
    player_has.push_back(components[i]);
    pl = true;
   }
   if (map_inv.has_charges(type, count)) {
    map_has.push_back(components[i]);
    mp = true;
   }
   if (!pl && !mp && u.charges_of(type) + map_inv.charges_of(type) >= count)
    mixed.push_back(components[i]);
  }

  else { // Counting by units, not charges

   if (u.has_amount(type, count)) {
    player_has.push_back(components[i]);
    pl = true;
   }
   if (map_inv.has_amount(type, count)) {
    map_has.push_back(components[i]);
    mp = true;
   }
   if (!pl && !mp && u.amount_of(type) + map_inv.amount_of(type) >= count)
    mixed.push_back(components[i]);

  }
 }

 if (player_has.size() + map_has.size() + mixed.size() == 1) { // Only 1 choice
  if (player_has.size() == 1)
   player_use.push_back(player_has[0]);
  else if (map_has.size() == 1)
   map_use.push_back(map_has[0]);
  else
   mixed_use.push_back(mixed[0]);

 } else { // Let the player pick which component they want to use
  std::vector<std::string> options; // List for the menu_vec below
// Populate options with the names of the items
  for (int i = 0; i < map_has.size(); i++) {
   std::string tmpStr = itypes[map_has[i].type]->name + " (nearby)";
   options.push_back(tmpStr);
  }
  for (int i = 0; i < player_has.size(); i++)
   options.push_back(itypes[player_has[i].type]->name);
  for (int i = 0; i < mixed.size(); i++) {
   std::string tmpStr = itypes[mixed[i].type]->name +" (on person & nearby)";
   options.push_back(tmpStr);
  }

  if (options.size() == 0) // This SHOULD only happen if cooking with a fire,
   return;                 // and the fire goes out.

// Get the selection via a menu popup
  int selection = menu_vec("Use which component?", options) - 1;
  if (selection < map_has.size())
   map_use.push_back(map_has[selection]);
  else if (selection < map_has.size() + player_has.size()) {
   selection -= map_has.size();
   player_use.push_back(player_has[selection]);
  } else {
   selection -= map_has.size() + player_has.size();
   mixed_use.push_back(mixed[selection]);
  }
 }

 for (int i = 0; i < player_use.size(); i++) {
  if (itypes[player_use[i].type]->count_by_charges() &&
      player_use[i].count > 0)
   u.use_charges(player_use[i].type, player_use[i].count);
  else
   u.use_amount(player_use[i].type, abs(player_use[i].count),
                   (player_use[i].count < 0));
 }
 for (int i = 0; i < map_use.size(); i++) {
  if (itypes[map_use[i].type]->count_by_charges() &&
      map_use[i].count > 0)
   m.use_charges(point(u.posx, u.posy), PICKUP_RANGE,
                    map_use[i].type, map_use[i].count);
  else
   m.use_amount(point(u.posx, u.posy), PICKUP_RANGE,
                   map_use[i].type, abs(map_use[i].count),
                   (map_use[i].count < 0));
 }
 for (int i = 0; i < mixed_use.size(); i++) {
  if (itypes[mixed_use[i].type]->count_by_charges() &&
      mixed_use[i].count > 0) {
   int from_map = mixed_use[i].count - u.charges_of(mixed_use[i].type);
   u.use_charges(mixed_use[i].type, u.charges_of(mixed_use[i].type));
   m.use_charges(point(u.posx, u.posy), PICKUP_RANGE,
                    mixed_use[i].type, from_map);
  } else {
   bool in_container = (mixed_use[i].count < 0);
   int from_map = abs(mixed_use[i].count) - u.amount_of(mixed_use[i].type);
   u.use_amount(mixed_use[i].type, u.amount_of(mixed_use[i].type),
                   in_container);
   m.use_amount(point(u.posx, u.posy), PICKUP_RANGE,
                   mixed_use[i].type, from_map, in_container);
  }
 }
}

void game::consume_tools(std::vector<component> tools)
{
 bool found_nocharge = false;
 inventory map_inv;
 map_inv.form_from_map(this, point(u.posx, u.posy), PICKUP_RANGE);
 std::vector<component> player_has;
 std::vector<component> map_has;
// Use charges of any tools that require charges used
 for (int i = 0; i < tools.size() && !found_nocharge; i++) {
  itype_id type = tools[i].type;
  if (tools[i].count > 0) {
   int count = tools[i].count;
   if (u.has_charges(type, count))
    player_has.push_back(tools[i]);
   if (map_inv.has_charges(type, count))
    map_has.push_back(tools[i]);
  } else if (u.has_amount(type, 1) || map_inv.has_amount(type, 1))
   found_nocharge = true;
 }
 if (found_nocharge)
  return; // Default to using a tool that doesn't require charges

 if (player_has.size() == 1 && map_has.size() == 0)
  u.use_charges(player_has[0].type, player_has[0].count);
 else if (map_has.size() == 1 && player_has.size() == 0)
  m.use_charges(point(u.posx, u.posy), PICKUP_RANGE,
                   map_has[0].type, map_has[0].count);
 else { // Variety of options, list them and pick one
// Populate the list
  std::vector<std::string> options;
  for (int i = 0; i < map_has.size(); i++) {
   std::string tmpStr = itypes[map_has[i].type]->name + " (nearby)";
   options.push_back(tmpStr);
  }
  for (int i = 0; i < player_has.size(); i++)
   options.push_back(itypes[player_has[i].type]->name);

  if (options.size() == 0) // This SHOULD only happen if cooking with a fire,
   return;                 // and the fire goes out.

// Get selection via a popup menu
  int selection = menu_vec("Use which tool?", options) - 1;
  if (selection < map_has.size())
   m.use_charges(point(u.posx, u.posy), PICKUP_RANGE,
                    map_has[selection].type, map_has[selection].count);
  else {
   selection -= map_has.size();
   u.use_charges(player_has[selection].type, player_has[selection].count);
  }
 }
}

void game::disassemble(char ch)
{
  if (!ch) {
    ch = inv("Disassemble item:");
  }
  if (ch == 27) {
    add_msg("Never mind.");
    return;
  }
  if (!u.has_item(ch)) {
    add_msg("You don't have item '%c'!", ch);
    return;
  }

  item* dis_item = &u.i_at(ch);

  if (OPTIONS[OPT_QUERY_DISASSEMBLE] && !(query_yn("Really disassemble your %s?", dis_item->tname(this).c_str())))
    return;

  for (int i = 0; i < recipes.size(); i++) {
    if (dis_item->type == itypes[recipes[i]->result] && recipes[i]->reversible)
    // ok, a valid recipe exists for the item, and it is reversible
    // assign the activity
    {
      // check tools are available
      // loop over the tools and see what's required...again
      inventory crafting_inv = crafting_inventory();
      bool have_tool[5];
      for (int j = 0; j < 5; j++)
      {
        have_tool[j] = false;
        if (recipes[i]->tools[j].size() == 0) // no tools required, may change this
          have_tool[j] = true;
        else
        {
          for (int k = 0; k < recipes[i]->tools[j].size(); k++)
          {
            itype_id type = recipes[i]->tools[j][k].type;
            int req = recipes[i]->tools[j][k].count;	// -1 => 1

            if ((req <= 0 && crafting_inv.has_amount (type, 1)) ||
              (req >  0 && crafting_inv.has_charges(type, req)))
            {
              have_tool[j] = true;
              k = recipes[i]->tools[j].size();
            }
            // if crafting recipe required a welder, disassembly requires a hacksaw or super toolkit
            if (type == "welder")
            {
              if (crafting_inv.has_amount("hacksaw", 1) ||
                  crafting_inv.has_amount("toolset", 1))
                have_tool[j] = true;
              else
                have_tool[j] = false;
            }
          }

          if (!have_tool[j])
          {
            int req = recipes[i]->tools[j][0].count;
            if (recipes[i]->tools[j][0].type == "welder")
                add_msg("You need a hack saw to disassemble this.");
            else
            {
              if (req <= 0)
              {
                add_msg("You need a %s to disassemble this.",
                itypes[recipes[i]->tools[j][0].type]->name.c_str());
              }
              else
              {
                add_msg("You need a %s with %d charges to disassemble this.",
                itypes[recipes[i]->tools[j][0].type]->name.c_str(), req);
              }
            }
          }
        }
      }
      // all tools present, so assign the activity
      if (have_tool[0] && have_tool[1] && have_tool[2] && have_tool[3] &&
      have_tool[4])
      {
        u.assign_activity(this, ACT_DISASSEMBLE, recipes[i]->time, recipes[i]->id);
        u.moves = 0;
        std::vector<int> dis_items;
        dis_items.push_back(ch);
        u.activity.values = dis_items;
      }
      return; // recipe exists, but no tools, so do not start disassembly
    }
  }
  // no recipe exists, or the item cannot be disassembled
  add_msg("This item cannot be disassembled!");
}

void game::complete_disassemble()
{
  // which recipe was it?
  recipe* dis = recipes[u.activity.index]; // Which recipe is it?
  item* dis_item = &u.i_at(u.activity.values[0]);

  add_msg("You disassemble the item into its components.");
  // remove any batteries or ammo first
    if (dis_item->is_gun() && dis_item->curammo != NULL && dis_item->ammo_type() != AT_NULL)
    {
      item ammodrop;
      ammodrop = item(dis_item->curammo, turn);
      ammodrop.charges = dis_item->charges;
      if (ammodrop.made_of(LIQUID))
        handle_liquid(ammodrop, false, false);
      else
        m.add_item(u.posx, u.posy, ammodrop);
    }
    if (dis_item->is_tool() && dis_item->charges > 0 && dis_item->ammo_type() != AT_NULL)
    {
      item ammodrop;
      ammodrop = item(itypes[default_ammo(dis_item->ammo_type())], turn);
      ammodrop.charges = dis_item->charges;
      if (ammodrop.made_of(LIQUID))
        handle_liquid(ammodrop, false, false);
      else
        m.add_item(u.posx, u.posy, ammodrop);
    }
    u.i_rem(u.activity.values[0]);  // remove the item

  // consume tool charges
  for (int j = 0; j < 5; j++)
  {
    if (dis->tools[j].size() > 0)
    consume_tools(dis->tools[j]);
  }

  // add the components to the map
  // Player skills should determine how many components are returned

  // adapting original crafting formula to check if disassembly was successful
  // # of dice is 75% primary skill, 25% secondary (unless secondary is null)
  int skill_dice = 2 + u.skillLevel(dis->sk_primary) * 3;
   if (dis->sk_secondary == NULL)
     skill_dice += u.skillLevel(dis->sk_primary);
   else
     skill_dice += u.skillLevel(dis->sk_secondary);
  // Sides on dice is 16 plus your current intelligence
   int skill_sides = 16 + u.int_cur;

   int diff_dice = dis->difficulty;
   int diff_sides = 24;	// 16 + 8 (default intelligence)

   // disassembly only nets a bit of practice
   if (dis->sk_primary)
    u.practice(dis->sk_primary, (dis->difficulty) * 2);
   if (dis->sk_secondary)
    u.practice(dis->sk_secondary, 2);

  for (int j = 0; j < 5; j++)
  {
    if (dis->components[j].size() != 0)
    {
      int compcount = dis->components[j][0].count;
      bool comp_success = (dice(skill_dice, skill_sides) > dice(diff_dice,  diff_sides));
      do
      {
        item newit(itypes[dis->components[j][0].type], turn);
        // skip item addition if component is a consumable like superglue
        if (dis->components[j][0].type == "superglue" || dis->components[j][0].type == "duct_tape")
          compcount--;
        else
        {
          if (newit.count_by_charges())
          {
            if (dis->difficulty == 0 || comp_success)
              m.spawn_item(u.posx, u.posy, itypes[dis->components[j][0].type], 0, 0, compcount);
            else
              add_msg("You fail to recover a component.");
            compcount = 0;
          } else
          {
            if (dis->difficulty == 0 || comp_success)
              m.add_item(u.posx, u.posy, newit);
            else
              add_msg("You fail to recover a component.");
            compcount--;
          }
        }
      } while (compcount > 0);
    }
  }
}
