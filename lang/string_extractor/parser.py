from .parsers.achievement import parse_achievement
from .parsers.activity_type import parse_activity_type
from .parsers.addiction_type import parse_addiction_type
from .parsers.ammunition_type import parse_ammunition_type
from .parsers.bionic import parse_bionic
from .parsers.body_part import parse_body_part
from .parsers.character_mod import parse_character_mod
from .parsers.city import parse_city
from .parsers.climbing_aid import parse_climbing_aid
from .parsers.clothing_mod import parse_clothing_mod
from .parsers.construction import parse_construction
from .parsers.construction_category import parse_construction_category
from .parsers.construction_group import parse_construction_group
from .parsers.damage_info_order import parse_damage_info_order
from .parsers.damage_type import parse_damage_type
from .parsers.dream import parse_dream
from .parsers.effect_on_condition import parse_effect_on_condition
from .parsers.effect_type import parse_effect_type
from .parsers.enchant import parse_enchant
from .parsers.event_statistic import parse_event_statistic
from .parsers.faction import parse_faction
from .parsers.fault import parse_fault
from .parsers.fault_fix import parse_fault_fix
from .parsers.field_type import parse_field_type
from .parsers.furniture import parse_furniture
from .parsers.gate import parse_gate
from .parsers.generic import parse_generic
from .parsers.gun import parse_gun
from .parsers.gunmod import parse_gunmod
from .parsers.harvest import parse_harvest
from .parsers.help import parse_help
from .parsers.item_action import parse_item_action
from .parsers.item_category import parse_item_category
from .parsers.json_flag import parse_json_flag
from .parsers.keybinding import parse_keybinding
from .parsers.limb_score import parse_limb_score
from .parsers.loot_zone import parse_loot_zone
from .parsers.map_extra import parse_map_extra
from .parsers.mapgen import parse_mapgen
from .parsers.mutation import parse_mutation
from .parsers.nested_category import parse_nested_category
from .parsers.npc import parse_npc
from .parsers.npc_class import parse_npc_class
from .parsers.option_slider import parse_option_slider
from .parsers.overmap_terrain import parse_overmap_terrain
from .parsers.palette import parse_palette
from .parsers.profession import parse_profession
from .parsers.proficiency import parse_proficiency
from .parsers.proficiency_category import parse_proficiency_category
from .parsers.recipe import parse_recipe
from .parsers.recipe_category import parse_recipe_category
from .parsers.recipe_group import parse_recipe_group
from .parsers.magazine import parse_magazine
from .parsers.material import parse_material
from .parsers.martial_art import parse_martial_art
from .parsers.mission_definition import parse_mission_definition
from .parsers.mod_info import parse_mod_info
from .parsers.monster import parse_monster
from .parsers.monster_attack import parse_monster_attack
from .parsers.morale_type import parse_morale_type
from .parsers.movement_mode import parse_movement_mode
from .parsers.mutation_category import parse_mutation_category
from .parsers.overmap_land_use_code import parse_overmap_land_use_code
from .parsers.practice import parse_practice
from .parsers.scenario import parse_scenario
from .parsers.shop_blacklist import parse_shopkeeper_blacklist
from .parsers.skill import parse_skill
from .parsers.skill_display_type import parse_skill_display_type
from .parsers.speech import parse_speech
from .parsers.speed_description import parse_speed_description
from .parsers.species import parse_species
from .parsers.spell import parse_spell
from .parsers.start_location import parse_start_location
from .parsers.snippet import parse_snippet
from .parsers.sub_body_part import parse_sub_body_part
from .parsers.talk_topic import parse_talk_topic
from .parsers.technique import parse_technique
from .parsers.ter_furn_transform import parse_ter_furn_transform
from .parsers.terrain import parse_terrain
from .parsers.tool_quality import parse_tool_quality
from .parsers.trap import parse_trap
from .parsers.vehicle import parse_vehicle
from .parsers.vehicle_part import parse_vehicle_part
from .parsers.vehicle_part_category import parse_vehicle_part_category
from .parsers.vehicle_spawn import parse_vehicle_spawn
from .parsers.vitamin import parse_vitamin
from .parsers.weakpoint_set import parse_weakpoint_set
from .parsers.weapon_category import parse_weapon_category
from .parsers.weather_type import parse_weather_type
from .parsers.widget import parse_widget


def dummy_parser(json, origin):
    return


parsers = {
    "achievement": parse_achievement,
    "activity_type": parse_activity_type,
    "addiction_type": parse_addiction_type,
    "ammo": parse_generic,
    "ammo_effect": dummy_parser,
    "ammunition_type": parse_ammunition_type,
    "anatomy": dummy_parser,
    "armor": parse_generic,
    "ascii_art": dummy_parser,
    "battery": parse_generic,
    "behavior": dummy_parser,
    "bionic": parse_bionic,
    "bionic_item": parse_generic,
    "bionic_migration": dummy_parser,
    "body_graph": dummy_parser,
    "body_part": parse_body_part,
    "book": parse_generic,
    "butchery_requirement": dummy_parser,
    "character_mod": parse_character_mod,
    "charge_removal_blacklist": dummy_parser,
    "city": parse_city,
    "city_building": dummy_parser,
    "climbing_aid": parse_climbing_aid,
    "clothing_mod": parse_clothing_mod,
    "comestible": parse_generic,
    "colordef": dummy_parser,
    "conduct": parse_achievement,
    "connect_group": dummy_parser,
    "construction": parse_construction,
    "construction_category": parse_construction_category,
    "construction_group": parse_construction_group,
    "damage_info_order": parse_damage_info_order,
    "damage_type": parse_damage_type,
    "dream": parse_dream,
    "disease_type": dummy_parser,
    "effect_on_condition": parse_effect_on_condition,
    "effect_type": parse_effect_type,
    "emit": dummy_parser,
    "enchantment": parse_enchant,
    "engine": parse_generic,
    "event_statistic": parse_event_statistic,
    "event_transformation": dummy_parser,
    "external_option": dummy_parser,
    "faction": parse_faction,
    "fault": parse_fault,
    "fault_fix": parse_fault_fix,
    "field_type": parse_field_type,
    "furniture": parse_furniture,
    "gate": parse_gate,
    "generic": parse_generic,
    "gun": parse_gun,
    "gunmod": parse_gunmod,
    "harvest": parse_harvest,
    "harvest_drop_type": dummy_parser,
    "help": parse_help,
    "hit_range": dummy_parser,
    "item_action": parse_item_action,
    "item_category": parse_item_category,
    "item_blacklist": dummy_parser,
    "item_group": dummy_parser,
    "jmath_function": dummy_parser,
    "json_flag": parse_json_flag,
    "keybinding": parse_keybinding,
    "limb_score": parse_limb_score,
    "loot_zone": parse_loot_zone,
    "magazine": parse_magazine,
    "map_extra": parse_map_extra,
    "mapgen": parse_mapgen,
    "martial_art": parse_martial_art,
    "material": parse_material,
    "migration": dummy_parser,
    "mission_definition": parse_mission_definition,
    "mod_info": parse_mod_info,
    "mod_tileset": dummy_parser,
    "monster": parse_monster,
    "monster_adjustment": dummy_parser,
    "monster_attack": parse_monster_attack,
    "monster_blacklist": dummy_parser,
    "monster_faction": dummy_parser,
    "monster_flag": dummy_parser,
    "monster_whitelist": dummy_parser,
    "monstergroup": dummy_parser,
    "mood_face": dummy_parser,
    "morale_type": parse_morale_type,
    "movement_mode": parse_movement_mode,
    "mutation": parse_mutation,
    "mutation_category": parse_mutation_category,
    "mutation_type": dummy_parser,
    "nested_category": parse_nested_category,
    "npc": parse_npc,
    "npc_class": parse_npc_class,
    "oter_id_migration": dummy_parser,
    "option_slider": parse_option_slider,
    "overlay_order": dummy_parser,
    "overmap_connection": dummy_parser,
    "overmap_land_use_code": parse_overmap_land_use_code,
    "overmap_location": dummy_parser,
    "overmap_special": dummy_parser,
    "overmap_special_migration": dummy_parser,
    "overmap_terrain": parse_overmap_terrain,
    "palette": parse_palette,
    "pet_armor": parse_generic,
    "playlist": dummy_parser,
    "practice": parse_practice,
    "profession": parse_profession,
    "profession_group": dummy_parser,
    "profession_item_substitutions": dummy_parser,
    "proficiency": parse_proficiency,
    "proficiency_category": parse_proficiency_category,
    "recipe": parse_recipe,
    "recipe_category": parse_recipe_category,
    "recipe_group": parse_recipe_group,
    "region_overlay": dummy_parser,
    "region_settings": dummy_parser,
    "relic_procgen_data": dummy_parser,
    "requirement": dummy_parser,
    "rotatable_symbol": dummy_parser,
    "scenario": parse_scenario,
    "scenario_blacklist": dummy_parser,
    "scent_type": dummy_parser,
    "score": dummy_parser,
    "shopkeeper_blacklist": parse_shopkeeper_blacklist,
    "shopkeeper_consumption_rates": dummy_parser,
    "skill": parse_skill,
    "skill_boost": dummy_parser,
    "skill_display_type": parse_skill_display_type,
    "snippet": parse_snippet,
    "sound_effect": dummy_parser,
    "speech": parse_speech,
    "speed_description": parse_speed_description,
    "species": parse_species,
    "spell": parse_spell,
    "start_location": parse_start_location,
    "sub_body_part": parse_sub_body_part,
    "talk_topic": parse_talk_topic,
    "technique": parse_technique,
    "temperature_removal_blacklist": dummy_parser,
    "ter_furn_transform": parse_ter_furn_transform,
    "terrain": parse_terrain,
    "trait_blacklist": dummy_parser,
    "trait_group": dummy_parser,
    "trait_migration": dummy_parser,
    "trap": parse_trap,
    "tool": parse_generic,
    "tool_armor": parse_generic,
    "tool_quality": parse_tool_quality,
    "toolmod": parse_generic,
    "uncraft": dummy_parser,
    "var_migration": dummy_parser,
    "vehicle": parse_vehicle,
    "vehicle_group": dummy_parser,
    "vehicle_part": parse_vehicle_part,
    "vehicle_part_category": parse_vehicle_part_category,
    "vehicle_part_migration": dummy_parser,
    "vehicle_placement": dummy_parser,
    "vehicle_spawn": parse_vehicle_spawn,
    "vitamin": parse_vitamin,
    "weakpoint_set": parse_weakpoint_set,
    "weapon_category": parse_weapon_category,
    "weather_type": parse_weather_type,
    "wheel": parse_generic,
    "widget": parse_widget
}
