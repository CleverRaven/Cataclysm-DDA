#pragma once
#ifndef CATA_SRC_TYPE_ID_H
#define CATA_SRC_TYPE_ID_H

// IWYU pragma: begin_exports
#include "int_id.h"
#include "string_id.h"
// IWYU pragma: end_exports

class achievement;
using achievement_id = string_id<achievement>;

class activity_type;
using activity_id = string_id<activity_type>;

struct add_type;
using addiction_id = string_id<add_type>;

class ammunition_type;
using ammotype = string_id<ammunition_type>;

struct ammo_effect;
using ammo_effect_id = int_id<ammo_effect>;
using ammo_effect_str_id = string_id<ammo_effect>;

class anatomy;
using anatomy_id = string_id<anatomy>;

struct attack_vector;
using attack_vector_id = string_id<attack_vector>;

struct bionic_data;
using bionic_id = string_id<bionic_data>;

struct body_part_type;
using bodypart_id = int_id<body_part_type>;
using bodypart_str_id = string_id<body_part_type>;

struct sub_body_part_type;
using sub_bodypart_id = int_id<sub_body_part_type>;
using sub_bodypart_str_id = string_id<sub_body_part_type>;

struct bodygraph;
using bodygraph_id = string_id<bodygraph>;

struct character_modifier;
using character_modifier_id = string_id<character_modifier>;

struct limb_score;
using limb_score_id = string_id<limb_score>;

struct city;
using city_id = string_id<city>;

struct construction_category;
using construction_category_id = string_id<construction_category>;

struct construction_group;
using construction_group_str_id = string_id<construction_group>;

struct clothing_mod;
using clothing_mod_id = string_id<clothing_mod>;

struct crafting_category;
using crafting_category_id = string_id<crafting_category>;

struct effect_on_condition;
using effect_on_condition_id = string_id<effect_on_condition>;

class effect_type;
using efftype_id = string_id<effect_type>;

class scent_type;
using scenttype_id = string_id<scent_type>;

class ascii_art;
using ascii_art_id = string_id<ascii_art>;

struct damage_type;
using damage_type_id = string_id<damage_type>;

struct damage_info_order;
using damage_info_order_id = string_id<damage_info_order>;

class disease_type;
using diseasetype_id = string_id<disease_type>;

class emit;
using emit_id = string_id<emit>;

class enchantment;
using enchantment_id = string_id<enchantment>;

struct end_screen;
using end_screen_id = string_id<end_screen>;

class event_statistic;
using event_statistic_id = string_id<event_statistic>;

class fault;
using fault_id = string_id<fault>;

class fault_fix;
using fault_fix_id = string_id<fault_fix>;

class fault_group;
using fault_group_id = string_id<fault_group>;

struct field_type;
using field_type_id = int_id<field_type>;
using field_type_str_id = string_id<field_type>;

struct furn_t;
using furn_id = int_id<furn_t>;
using furn_str_id = string_id<furn_t>;

class climbing_aid;
using climbing_aid_id = string_id<climbing_aid>;

class gun_mode;
using gun_mode_id = string_id<gun_mode>;

class harvest_drop_type;
using harvest_drop_type_id = string_id<harvest_drop_type>;

class harvest_list;
using harvest_id = string_id<harvest_list>;

class item_category;
using item_category_id = string_id<item_category>;

class Item_spawn_data;
// note: "dynamic" string id, see string_id_params in string_id.h
using item_group_id = string_id<Item_spawn_data>;

struct itype;
using itype_id = string_id<itype>;

class weapon_category;
using weapon_category_id = string_id<weapon_category>;

class ma_buff;
using mabuff_id = string_id<ma_buff>;

class martialart;
using matype_id = string_id<martialart>;

class ma_technique;
using matec_id = string_id<ma_technique>;

class map_extra;
using map_extra_id = string_id<map_extra>;

class mapgen_palette;
using palette_id = string_id<mapgen_palette>;

class material_type;
using material_id = string_id<material_type>;

struct mission_type;
using mission_type_id = string_id<mission_type>;

struct MOD_INFORMATION;
using mod_id = string_id<MOD_INFORMATION>;

struct mon_flag;
using mon_flag_id = int_id<mon_flag>;
// As of 1-1-24 there are 118 flags in the vanilla game.
// Additional space is added for expansion, up to the next word size.
// Note: it is safe to use flags larger than this, it will just be slower.
using mon_flag_id_set = int_id_set<mon_flag, 64 * 3>;
using mon_flag_str_id = string_id<mon_flag>;

class monfaction;
using mfaction_id = int_id<monfaction>;
using mfaction_str_id = string_id<monfaction>;

struct MonsterGroup;
using mongroup_id = string_id<MonsterGroup>;

class morale_type_data;
using morale_type = string_id<morale_type_data>;

struct mtype;
using mtype_id = string_id<mtype>;

class nested_mapgen;
using nested_mapgen_id = string_id<nested_mapgen>;

class npc_class;
using npc_class_id = string_id<npc_class>;

class npc_template;
using npc_template_id = string_id<npc_template>;

class faction;
using faction_id = string_id<faction>;

struct option_slider;
using option_slider_id = string_id<option_slider>;

struct oter_t;
using oter_id = int_id<oter_t>;
using oter_str_id = string_id<oter_t>;

struct oter_type_t;
using oter_type_id = int_id<oter_type_t>;
using oter_type_str_id = string_id<oter_type_t>;

class oter_vision;
using oter_vision_id = string_id<oter_vision>;

class overmap_connection;
using overmap_connection_id = string_id<overmap_connection>;

struct overmap_location;
using overmap_location_id = string_id<overmap_location>;

class overmap_special;
using overmap_special_id = string_id<overmap_special>;

struct overmap_special_migration;
using overmap_special_migration_id = string_id<overmap_special_migration>;

class profession;
using profession_id = string_id<profession>;

struct profession_group;
using profession_group_id = string_id<profession_group>;

class recipe;
using recipe_id = string_id<recipe>;

struct requirement_data;
using requirement_id = string_id<requirement_data>;

class score;
using score_id = string_id<score>;

struct shopkeeper_cons_rates;
using shopkeeper_cons_rates_id = string_id<shopkeeper_cons_rates>;

struct shopkeeper_blacklist;
using shopkeeper_blacklist_id = string_id<shopkeeper_blacklist>;

class Skill;
using skill_id = string_id<Skill>;

class SkillDisplayType;
using skill_displayType_id = string_id<SkillDisplayType>;

struct species_type;
using species_id = string_id<species_type>;

class speed_description;
using speed_description_id = string_id<speed_description>;

class mood_face;
using mood_face_id = string_id<mood_face>;

class magic_type;
using magic_type_id = string_id<magic_type>;

class spell_type;
using spell_id = string_id<spell_type>;

class start_location;
using start_location_id = string_id<start_location>;

class move_mode;
using move_mode_id = string_id<move_mode>;

struct mutation_category_trait;
using mutation_category_id = string_id<mutation_category_trait>;

struct proficiency_category;
using proficiency_category_id = string_id<proficiency_category>;

class proficiency;
using proficiency_id = string_id<proficiency>;

class relic_procgen_data;
using relic_procgen_id = string_id<relic_procgen_data>;

struct ter_t;
using ter_id = int_id<ter_t>;
using ter_str_id = string_id<ter_t>;

class ter_furn_transform;
using ter_furn_transform_id = string_id<ter_furn_transform>;

class Trait_group;
namespace trait_group
{
using Trait_group_tag = string_id<Trait_group>;
// note: "dynamic" string id, see string_id_params in string_id.h
} // namespace trait_group

struct trap;
using trap_id = int_id<trap>;
using trap_str_id = string_id<trap>;

struct mutation_branch;
using trait_id = string_id<mutation_branch>;

class update_mapgen;
using update_mapgen_id = string_id<update_mapgen>;

struct quality;
using quality_id = string_id<quality>;

class VehicleGroup;
using vgroup_id = string_id<VehicleGroup>;

class vitamin;
using vitamin_id = string_id<vitamin>;

class vpart_info;
using vpart_id = string_id<vpart_info>;

struct vehicle_prototype;
using vproto_id = string_id<vehicle_prototype>;

struct weather_type;
using weather_type_id = string_id<weather_type>;

class zone_type;
using zone_type_id = string_id<zone_type>;

class translation;
using snippet_id = string_id<translation>;

struct construction;
using construction_id = int_id<construction>;
using construction_str_id = string_id<construction>;

class json_flag;
using flag_id = string_id<json_flag>;

using json_character_flag = string_id<json_flag>;

struct jmath_func;
using jmath_func_id = string_id<jmath_func>;

class widget;
using widget_id = string_id<widget>;

struct weakpoints;
using weakpoints_id = string_id<weakpoints>;

struct connect_group;
using connect_group_id = string_id<connect_group>;

#endif // CATA_SRC_TYPE_ID_H
