#!/usr/bin/env python3
"Extract translatable strings from the .json files in data/json."

import json
import os
import itertools
import subprocess
from optparse import OptionParser
from sys import platform

# Must parse command line arguments here
# 'options' variable is referenced in our defined functions below

parser = OptionParser()
parser.add_option("-v", "--verbose", dest="verbose", help="be verbose")
(options, args) = parser.parse_args()

# Exceptions
class WrongJSONItem(Exception):
    def __init__(self, msg, item):
        self.msg = msg
        self.item = item

    def __str__(self):
        return ("---\nJSON error\n{0}\n--- JSON Item:\n{1}\n---".format(self.msg, self.item))

# there may be some non-json files in data/raw
not_json = {os.path.normpath(i) for i in {
    "sokoban.txt",
    "LOADING_ORDER.md"
}}

git_files_list = {os.path.normpath(i) for i in {
    ".",
}}

# no warning will be given if an untranslatable object is found in those files
warning_suppressed_list = {os.path.normpath(i) for i in {
    "data/json/overmap_terrain.json",
    "data/json/traps.json",
    "data/json/vehicleparts/",
    "data/raw/keybindings.json",
    "data/mods/alt_map_key/overmap_terrain.json",
    "data/mods/DeoxyMod/Deoxy_vehicle_parts.json",
    "data/mods/More_Survival_Tools/start_locations.json",
    "data/mods/NPC_Traits/npc_classes.json",
    "data/mods/Tanks/monsters.json"
}}

def warning_supressed(filename):
    for i in warning_suppressed_list:
        if filename.startswith(i):
            return True
    return False

# these files will not be parsed. Full related path.
ignore_files = {os.path.normpath(i) for i in {
    "data/json/anatomy.json",
    "data/mods/replacements.json",
    "data/raw/color_templates/no_bright_background.json"
}}

# these objects have no translatable strings
ignorable = {
    "behavior",
    "BULLET_PULLING",
    "city_building",
    "colordef",
    "emit",
    "enchantment",
    "EXTERNAL_OPTION",
    "GAME_OPTION",
    "ITEM_BLACKLIST",
    "item_group",
    "ITEM_OPTION",
    "ITEM_WHITELIST",
    "MIGRATION",
    "mod_tileset",
    "monitems",
    "monster_adjustment",
    "MONSTER_BLACKLIST",
    "MONSTER_FACTION",
    "monstergroup",
    "MONSTER_WHITELIST",
    "mutation_type",
    "obsolete_terrain",
    "overlay_order",
    "overmap_connection",
    "overmap_location",
    "overmap_special",
    "profession_item_substitutions",
    "palette",
    "region_overlay",
    "region_settings",
    "requirement",
    "rotatable_symbol",
    "skill_boost",
    "SPECIES",
    "trait_group",
    "uncraft",
    "vehicle_group",
    "vehicle_placement",
    "WORLD_OPTION"
}

# these objects can have their strings automatically extracted.
# insert object "type" here IF AND ONLY IF
# all of their translatable strings are in the following form:
#   "name" member
#   "description" member
#   "name_plural" member
#   "text" member
#   "sound" member
#   "messages" member containing an array of translatable strings
automatically_convertible = {
    "activity_type",
    "AMMO",
    "ammunition_type",
    "ARMOR",
    "BATTERY",
    "bionic",
    "BIONIC_ITEM",
    "BOOK",
    "COMESTIBLE",
    "construction_category",
    "CONTAINER",
    "dream",
    "ENGINE",
    "epilogue",
    "faction",
    "fault",
    "furniture",
    "GENERIC",
    "item_action",
    "ITEM_CATEGORY",
    "json_flag",
    "keybinding",
    "MAGAZINE",
    "map_extra",
    "MOD_INFO",
    "MONSTER",
    "morale_type",
    "morale_type",
    "npc",
    "npc_class",
    "overmap_land_use_code",
    "overmap_terrain",
    "PET_ARMOR",
    "skill",
    "snippet",
    "speech",
    "SPELL",
    "start_location",
    "STATIONARY_ITEM",
    "terrain",
    "TOOL",
    "TOOLMOD",
    "TOOL_ARMOR",
    "tool_quality",
    "tutorial_messages",
    "VAR_VEH_PART",
    "vehicle",
    "vehicle_part",
    "vitamin",
    "WHEEL",
    "help"
}

# for these objects a plural form is needed
needs_plural = {
    "ARMOR",
    "BIONIC_ITEM",
    "BOOK",
    "CONTAINER",
    "GENERIC",
    "GUN",
    "GUNMOD",
    "MONSTER"
    "STATIONARY_ITEM",
    "TOOL",
    "TOOLMOD",
    "TOOL_ARMOR",
    "VAR_VEH_PART",
}

# these objects can be automatically converted, but use format strings
use_format_strings = {
    "technique",
}

# For handling grammatical gender
all_genders = ["f", "m", "n"]

def gender_options(subject):
    return [subject + ":" + g for g in all_genders]

##
##  SPECIALIZED EXTRACTION FUNCTIONS
##

def extract_harvest(item):
    outfile = get_outfile("harvest")
    if "message" in item:
        writestr(outfile, item["message"])

def extract_bodypart(item):
    outfile = get_outfile("bodypart")
    writestr(outfile, item["name"])
    writestr(outfile, item["name"], context="bodypart_accusative")
    if "name_plural" in item:
        writestr(outfile, item["name_plural"])
        writestr(outfile, item["name_plural"], context="bodypart_accusative")
    writestr(outfile, item["encumbrance_text"])
    writestr(outfile, item["heading_singular"])
    writestr(outfile, item["heading_plural"])
    if "hp_bar_ui_text" in item:
        writestr(outfile, item["hp_bar_ui_text"])

def extract_clothing_mod(item):
    outfile = get_outfile("clothing_mod")
    writestr(outfile, item["implement_prompt"])
    writestr(outfile, item["destroy_prompt"])

def extract_construction(item):
    outfile = get_outfile("construction")
    writestr(outfile, item["description"])
    if "pre_note" in item:
        writestr(outfile, item["pre_note"])

def extract_material(item):
    outfile = get_outfile("material")
    writestr(outfile, item["name"])
    writestr(outfile, item["bash_dmg_verb"])
    writestr(outfile, item["cut_dmg_verb"])
    writestr(outfile, item["dmg_adj"][0])
    writestr(outfile, item["dmg_adj"][1])
    writestr(outfile, item["dmg_adj"][2])
    writestr(outfile, item["dmg_adj"][3])

def extract_martial_art(item):
    outfile = get_outfile("martial_art")
    if "name" in item:
        name = item["name"]
        writestr(outfile, name)
    else:
        name = item["id"]
    if "description" in item:
        writestr(outfile, item["description"],
                 comment="Description for martial art '{}'".format(name))
    onhit_buffs = item.get("onhit_buffs", list())
    static_buffs = item.get("static_buffs", list())
    onmove_buffs = item.get("onmove_buffs", list())
    ondodge_buffs = item.get("ondodge_buffs", list())
    buffs = onhit_buffs + static_buffs + onmove_buffs + ondodge_buffs
    for buff in buffs:
        writestr(outfile, buff["name"])
        if buff["name"] == item["name"]:
            c="Description of buff for martial art '{}'".format(name)
        else:
            c="Description of buff '{}' for martial art '{}'".format(buff["name"], name)
        writestr(outfile, buff["description"], comment=c)

def extract_effect_type(item):
    # writestr will not write string if it is None.
    outfile = get_outfile("effects")
    ctxt_name = item.get("name", ())

    if ctxt_name:
        if len(ctxt_name) == len(item.get("desc", ())):
            for nm_desc in zip(ctxt_name, item.get("desc", ())):
                writestr(outfile, nm_desc[0])
                writestr(outfile, nm_desc[1], format_strings=True,
                         comment="Description of effect '{}'.".format(nm_desc[0]))
        else:
            for i in ctxt_name:
                writestr(outfile, i)
            for f in ["desc", "reduced_desc"]:
                for i in item.get(f, ()):
                    writestr(outfile, i, format_strings=True)

    name = None
    if ctxt_name:
        name = [i["str"] if type(i) is dict else i for i in ctxt_name]
    # apply_message
    msg = item.get("apply_message")
    if not name:
        writestr(outfile, msg, format_strings=True)
    else:
        writestr(outfile, msg, format_strings=True,
                 comment="Apply message for effect(s) '{}'.".format(', '.join(name)))

    # remove_message
    msg = item.get("remove_message")
    if not name:
        writestr(outfile, msg, format_strings=True)
    else:
        writestr(outfile, msg, format_strings=True,
                 comment="Remove message for effect(s) '{}'.".format(', '.join(name)))

    # miss messages
    msg = item.get("miss_messages", ())
    if not name:
        for m in msg:
            writestr(outfile, m[0])
    else:
        for m in msg:
            writestr(outfile, m[0],
                     comment="Miss message for effect(s) '{}'.".format(', '.join(name)))
    msg = item.get("decay_messages", ())
    if not name:
        for m in msg:
            writestr(outfile, m[0])
    else:
        for m in msg:
            writestr(outfile, m[0],
                     comment="Decay message for effect(s) '{}'.".format(', '.join(name)))

    # speed_name
    if "speed_name" in item:
        if not name:
            writestr(outfile, item.get("speed_name"))
        else:
            writestr(outfile, item.get("speed_name"), comment="Speed name of effect(s) '{}'.".format(', '.join(name)))

    # apply and remove memorial messages.
    msg = item.get("apply_memorial_log")
    if not name:
        writestr(outfile, msg, context="memorial_male")
        writestr(outfile, msg, context="memorial_female")
    else:
        writestr(outfile, msg, context="memorial_male",
                 comment="Male memorial apply log for effect(s) '{}'.".format(', '.join(name)))
        writestr(outfile, msg, context="memorial_female",
                 comment="Female memorial apply log for effect(s) '{}'.".format(', '.join(name)))
    msg = item.get("remove_memorial_log")
    if not name:
        writestr(outfile, msg, context="memorial_male")
        writestr(outfile, msg, context="memorial_female")
    else:
        writestr(outfile, msg, context="memorial_male",
          comment="Male memorial remove log for effect(s) '{}'.".format(', '.join(name)))
        writestr(outfile, msg, context="memorial_female",
          comment="Female memorial remove log for effect(s) '{}'.".format(', '.join(name)))


def extract_gun(item):
    outfile = get_outfile("gun")
    if "name" in item:
        item_name = item.get("name")
        writestr(outfile, item_name)
        if "name_plural" in item:
            if item.get("name_plural") != "none":
                writestr(outfile, item_name, item.get("name_plural"))
            else:
                writestr(outfile, item_name)
        else:
            if item.get("type") in needs_plural:
                # no name_plural entry in json, use default constructed (name+"s"), as in item_factory.cpp
                writestr(outfile, item_name, "{}s".format(item_name))
            else:
                writestr(outfile, item_name)
    if "description" in item:
        description = item.get("description")
        writestr(outfile, description)
    if "modes" in item:
        modes = item.get("modes")
        for fire_mode in modes:
            writestr(outfile, fire_mode[1])
    if "skill" in item:
        # legacy code: the "gun type" is calculated in `item::gun_type` and
        # it's basically the skill id, except for archery (which is either
        # bow or crossbow). Once "gun type" is loaded from JSON, it should
        # be extracted directly.
        if not item.get("skill") == "archery":
            writestr(outfile, item.get("skill"), context="gun_type_type")
    if "reload_noise" in item:
        item_reload_noise = item.get("reload_noise")
        writestr(outfile, item_reload_noise)


def extract_gunmod(item):
    outfile = get_outfile("gunmod")
    if "name" in item:
        item_name = item.get("name")
        writestr(outfile, item_name)
        if "name_plural" in item:
            if item.get("name_plural") != "none":
                writestr(outfile, item_name, item.get("name_plural"))
            else:
                writestr(outfile, item_name)
        else:
            if item.get("type") in needs_plural:
                # no name_plural entry in json, use default constructed (name+"s"), as in item_factory.cpp
                writestr(outfile, item_name, "{}s".format(item_name))
            else:
                writestr(outfile, item_name)
    if "description" in item:
        description = item.get("description")
        writestr(outfile, description)
    if "mode_modifier" in item:
        modes = item.get("mode_modifier")
        for fire_mode in modes:
            writestr(outfile, fire_mode[1])
    if "location" in item:
        location = item.get("location")
        writestr(outfile, location)
    if "mod_targets" in item:
        for target in item["mod_targets"]:
            writestr(outfile, target, context="gun_type_type")


def extract_professions(item):
    outfile = get_outfile("professions")
    nm = item["name"]
    if type(nm) == dict:
        writestr(outfile, nm["male"], context="profession_male")
        writestr(outfile, item["description"], context="prof_desc_male",
                 comment="Profession ({}) description".format(nm["male"]))

        writestr(outfile, nm["female"], context="profession_female")
        writestr(outfile, item["description"], context="prof_desc_female",
                 comment="Profession ({0}) description".format(nm["female"]))
    else:
        writestr(outfile, nm, context="profession_male")
        writestr(outfile, item["description"], context="prof_desc_male",
                 comment="Profession (male {}) description".format(nm))

        writestr(outfile, nm, context="profession_female")
        writestr(outfile, item["description"], context="prof_desc_female",
                 comment="Profession (female {}) description".format(nm))

def extract_scenarios(item):
    outfile = get_outfile("scenario")
    # writestr will not write string if it is None.
    name = item.get("name")
    writestr(outfile,
             name,
             context="scenario_male",
             comment="Name for scenario '{}' for a male character".format(name))
    writestr(outfile,
             name,
             context="scenario_female",
             comment="Name for scenario '{}' for a female character".format(name))
    if name:
        msg = item.get("description")
        if msg:
            writestr(outfile,
                     msg,
                     context="scen_desc_male",
                     comment="Description for scenario '{}' for a male character.".format(name))
            writestr(outfile,
                     msg,
                     context="scen_desc_female",
                     comment="Description for scenario '{}' for a female character.".format(name))
        msg = item.get("start_name")
        if msg:
            writestr(outfile,
                     msg,
                     context="start_name",
                     comment="Starting location for scenario '{}'.".format(name))
    else:
        for f in ["description", "start_name"]:
            found = item.get(f, None)
            writestr(outfile, found)

def extract_mapgen(item):
    outfile = get_outfile("mapgen")
    # writestr will not write string if it is None.
    for (objkey, objval) in sorted(item["object"].items(), key=lambda x: x[0]):
        if objkey == "place_specials" or objkey == "place_signs":
            for special in objval:
                for (speckey, specval) in sorted(special.items(), key=lambda x: x[0]):
                    if speckey == "signage":
                        writestr(outfile, specval, comment="Sign")
        elif objkey == "signs":
            obj = objval
            for (k, v) in sorted(objval.items(), key=lambda x: x[0]):
                sign = v.get("signage", None)
                writestr(outfile, sign, comment="Sign")
        elif objkey == "computers":
            for (k, v) in sorted(objval.items(), key=lambda x: x[0]):
                if "name" in v:
                    writestr(outfile, v.get("name"), comment="Computer name")
                if "options" in v:
                    for opt in v.get("options"):
                        writestr(outfile, opt.get("name"), comment="Computer option")

def extract_monster_attack(item):
    outfile = get_outfile("monster_attack")
    if "hit_dmg_u" in item:
        writestr(outfile, item.get("hit_dmg_u"))
    if "hit_dmg_npc" in item:
        writestr(outfile, item.get("hit_dmg_npc"))
    if "no_dmg_msg_u" in item:
        writestr(outfile, item.get("no_dmg_msg_u"))
    if "no_dmg_msg_npc" in item:
        writestr(outfile, item.get("no_dmg_msg_npc"))


def extract_recipes(item):
    outfile = get_outfile("recipe")
    if "book_learn" in item:
        for arr in item["book_learn"]:
            if len(arr) >= 3 and len(arr[2]) > 0:
                writestr(outfile, arr[2])
    if "description" in item:
        writestr(outfile, item["description"])


def extract_recipe_group(item):
    outfile = get_outfile("recipe_group")
    if "recipes" in item:
        for i in item.get("recipes"):
            writestr(outfile, i.get("description"))

def extract_gendered_dynamic_line_optional(line, outfile):
    if "gendered_line" in line:
        msg = line["gendered_line"]
        subjects = line["relevant_genders"]
        options = [gender_options(subject) for subject in subjects]
        for context_list in itertools.product(*options):
            context = " ".join(context_list)
            writestr(outfile, msg, context=context)

def extract_dynamic_line_optional(line, member, outfile):
    if member in line:
        extract_dynamic_line(line[member], outfile)

dynamic_line_string_keys = {
# from `simple_string_conds` in `condition.h`
    "u_male", "u_female", "npc_male", "npc_female",
    "has_no_assigned_mission", "has_assigned_mission", "has_many_assigned_missions",
    "has_no_available_mission", "has_available_mission", "has_many_available_missions",
    "mission_complete", "mission_incomplete", "mission_has_generic_rewards",
    "npc_available", "npc_following", "npc_friend", "npc_hostile",
    "npc_train_skills", "npc_train_styles",
    "at_safe_space", "is_day", "npc_has_activity", "is_outside", "u_has_camp",
    "u_can_stow_weapon", "npc_can_stow_weapon", "u_has_weapon", "npc_has_weapon",
    "u_driving", "npc_driving",
    "has_pickup_list", "is_by_radio", "has_reason",
# yes/no strings for complex conditions
    "yes", "no"
}

def extract_dynamic_line(line, outfile):
    if type(line) == list:
        for l in line:
            extract_dynamic_line(l, outfile)
    elif type(line) == dict:
        extract_gendered_dynamic_line_optional(line, outfile)
        for key in dynamic_line_string_keys:
            extract_dynamic_line_optional(line, key, outfile)
    elif type(line) == str:
        writestr(outfile, line)

def extract_talk_effects(effects, outfile):
    if type(effects) != list:
        effects = [effects]
    for eff in effects:
        if type(eff) == dict:
            if "u_buy_monster" in eff and "name" in eff:
                writestr(outfile, eff["name"], comment="Nickname for creature '{}'".format(eff["u_buy_monster"]))

def extract_talk_response(response, outfile):
    if "text" in response:
        writestr(outfile, response["text"])
    if "success" in response:
        extract_talk_response(response["success"], outfile)
    if "failure" in response:
        extract_talk_response(response["failure"], outfile)
    if "speaker_effect" in response:
        speaker_effects = response["speaker_effect"]
        if type(speaker_effects) != list:
            speaker_effects = [speaker_effects]
        for eff in speaker_effects:
            if "effect" in eff:
                extract_talk_effects(eff["effect"], outfile)
    if "effect" in response:
        extract_talk_effects(response["effect"], outfile)

def extract_talk_topic(item):
    outfile = get_outfile("talk_topic")
    if "dynamic_line" in item:
        extract_dynamic_line(item["dynamic_line"], outfile)
    if "responses" in item:
        for r in item["responses"]:
            extract_talk_response(r, outfile)
    if "effect" in item:
        extract_talk_effects(item["effect"], outfile)

def extract_trap(item):
    outfile = get_outfile("trap")
    writestr(outfile, item["name"])
    if "vehicle_data" in item and "sound" in item["vehicle_data"]:
        writestr(outfile, item["vehicle_data"]["sound"], comment="Trap-vehicle collision message for trap '{}'".format(item["name"]))

def extract_missiondef(item):
    outfile = get_outfile("mission_def")
    item_name = item.get("name")
    if item_name is None:
        raise WrongJSONItem("JSON item don't contain 'name' field", item)
    writestr(outfile, item_name)
    if "dialogue" in item:
        dialogue = item.get("dialogue")
        if "describe" in dialogue:
            writestr(outfile, dialogue.get("describe"))
        if "offer" in dialogue:
            writestr(outfile, dialogue.get("offer"))
        if "accepted" in dialogue:
            writestr(outfile, dialogue.get("accepted"))
        if "rejected" in dialogue:
            writestr(outfile, dialogue.get("rejected"))
        if "advice" in dialogue:
            writestr(outfile, dialogue.get("advice"))
        if "inquire" in dialogue:
            writestr(outfile, dialogue.get("inquire"))
        if "success" in dialogue:
            writestr(outfile, dialogue.get("success"))
        if "success_lie" in dialogue:
            writestr(outfile, dialogue.get("success_lie"))
        if "failure" in dialogue:
            writestr(outfile, dialogue.get("failure"))
    if "start" in item and "effect" in item["start"]:
        extract_talk_effects(item["start"]["effect"], outfile)
    if "end" in item and "effect" in item["end"]:
        extract_talk_effects(item["end"]["effect"], outfile)
    if "fail" in item and "effect" in item["fail"]:
        extract_talk_effects(item["fail"]["effect"], outfile)

def extract_mutation(item):
    outfile = get_outfile("mutation")

    item_name = found = item.get("name")
    if found is None:
        raise WrongJSONItem("JSON item don't contain 'name' field", item)
    writestr(outfile, found)

    simple_fields = [ "description" ]

    for f in simple_fields:
        found = item.get(f)
        # Need that check due format string argument
        if found is not None:
            writestr(outfile, found, comment="Description for {}".format(item_name))

    if "attacks" in item:
        attacks = item.get("attacks")
        if type(attacks) is list:
            for i in attacks:
                if "attack_text_u" in i:
                    writestr(outfile, i.get("attack_text_u"))
                if "attack_text_npc" in i:
                    writestr(outfile, i.get("attack_text_npc"))
        else:
            if "attack_text_u" in attacks:
                writestr(outfile, attacks.get("attack_text_u"))
            if "attack_text_npc" in attacks:
                writestr(outfile, attacks.get("attack_text_npc"))

    if "spawn_item" in item:
        writestr(outfile, item.get("spawn_item").get("message"))

def extract_mutation_category(item):
    outfile = get_outfile("mutation_category")

    item_name = found = item.get("name")
    if found is None:
        raise WrongJSONItem("JSON item don't contain 'name' field", item)
    writestr(outfile, found, comment="Mutation class name")

    simple_fields = [ "mutagen_message",
                      "iv_message",
                      "iv_sleep_message",
                      "iv_sound_message",
                      "junkie_message"
                    ]

    for f in simple_fields:
        found = item.get(f)
        # Need that check due format string argument
        if found is not None:
            writestr(outfile, found, comment="Mutation class: {} {}".format(item_name, f))

    found = item.get("memorial_message")
    writestr(outfile, found, context="memorial_male",
             comment="Mutation class: {} Male memorial messsage".format(item_name))
    writestr(outfile, found, context="memorial_female",
             comment="Mutation class: {} Female memorial messsage".format(item_name))


def extract_vehspawn(item):
    outfile = get_outfile("vehicle_spawn")

    found = item.get("spawn_types")
    if not found:
        return

    for st in found:
        writestr(outfile, st.get("description"), comment="Vehicle Spawn Description")

def extract_recipe_category(item):
    outfile = get_outfile("recipe_category")

    cid = item.get("id", None)
    if cid:
        if cid == 'CC_NONCRAFT':
            return
        cat_name = cid.split("_")[1]
        writestr(outfile, cat_name, comment="Crafting recipes category name")
    else:
        raise WrongJSONItem("Recipe category must have unique id", item)

    found = item.get("recipe_subcategories", [])
    for subcat in found:
        if subcat == 'CSC_ALL':
            writestr(outfile, 'ALL', comment="Crafting recipes subcategory all")
            continue
        subcat_name = subcat.split('_')[2]
        writestr(outfile, subcat_name,
                 comment="Crafting recipes subcategory of '{}' category".format(cat_name))

def extract_gate(item):
    outfile = get_outfile("gates")
    messages = item.get("messages", {})

    for (k, v) in sorted(messages.items(), key=lambda x: x[0]):
        writestr(outfile, v,
                 comment="'{}' action message of some gate object.".format(k))

def extract_field_type(item):
    outfile = get_outfile("field_type")
    for fd in item.get("intensity_levels"):
       if "name" in fd:
           writestr(outfile,fd.get("name"))
            
def extract_ter_furn_transform_messages(item):
	outfile = get_outfile("ter_furn_transform_messages")
	writestr(outfile,item.get("fail_message"))
	for terrain in item.get("terrain"):
		writestr(outfile,terrain.get("message"))

def extract_skill_display_type(item):
    outfile = get_outfile("skill_display_type")
    writestr(outfile, item["display_string"], comment="display string for skill display type '{}'".format(item["ident"]))

# these objects need to have their strings specially extracted
extract_specials = {
    "harvest" : extract_harvest,
    "body_part": extract_bodypart,
    "clothing_mod": extract_clothing_mod,
    "construction": extract_construction,
    "effect_type": extract_effect_type,
    "GUN": extract_gun,
    "GUNMOD": extract_gunmod,
    "mapgen": extract_mapgen,
    "martial_art": extract_martial_art,
    "material": extract_material,
    "mission_definition": extract_missiondef,
    "monster_attack": extract_monster_attack,
    "mutation": extract_mutation,
    "mutation_category": extract_mutation_category,
    "profession": extract_professions,
    "recipe_category": extract_recipe_category,
    "recipe": extract_recipes,
    "recipe_group": extract_recipe_group,
    "scenario": extract_scenarios,
    "talk_topic": extract_talk_topic,
    "trap": extract_trap,
    "gate": extract_gate,
    "vehicle_spawn": extract_vehspawn,
    "field_type": extract_field_type,
    "ter_furn_transform": extract_ter_furn_transform_messages,
    "skill_display_type": extract_skill_display_type

}

##
##  PREPARATION
##

directories = {os.path.normpath(i) for i in {
    "data/raw",
    "data/json",
    "data/mods",
    "data/core",
    "data/legacy",
    "data/help",
}}
to_dir = os.path.normpath("lang/json")

print("==> Preparing the work space")

# allow running from main directory, or from script subdirectory
if not os.path.exists("data/json"):
    print("Error: Couldn't find the 'data/json' subdirectory.")
    exit(1)

# create the output directory, if it does not already exist
if not os.path.exists(to_dir):
    os.mkdir(to_dir)

# clean any old extracted strings, it will all be redone
for filename in os.listdir(to_dir):
    if not filename.endswith(".py"): continue
    f = os.path.join(to_dir, filename)
    os.remove(f)

##
##  FUNCTIONS
##

def tlcomment(fs, string):
    "Write the string to the file as a comment for translators."
    if len(string) > 0:
        fs.write("#~ {}\n".format(string))

def gettextify(string, context=None, plural=None):
    "Put the string in a fake gettext call, and add a newline."
    if context:
        return "pgettext(%r, %r)\n" % (context, string)
    else:
        if plural:
            return "ngettext(%r, %r, n)\n" % (string, plural)
        else:
            return "_(%r)\n" % string

def writestr(filename, string, plural=None, context=None, format_strings=False, comment=None):
    "Wrap the string and write to the file."
    if type(string) is list and plural is None:
        for entry in string:
            writestr(filename, entry, None, context, format_strings, comment)
        return
    elif type(string) is dict and plural is None:
        if "ctxt" in string:
            writestr(filename, string["str"], None, string["ctxt"], format_strings, comment)
        else:
            writestr(filename, string["str"], None, None, format_strings, comment)
        return

    # don't write empty strings
    if not string: return

    with open(filename, 'a', encoding="utf-8", newline='\n') as fs:
        # Append developers comment
        if comment:
            tlcomment(fs, comment)
        # most of the strings from json don't use string formatting.
        # we must tell xgettext this explicitly
        if not format_strings and "%" in string:
            fs.write("# xgettext:no-python-format\n")
        fs.write(gettextify(string,context=context,plural=plural))

def get_outfile(json_object_type):
    return os.path.join(to_dir, json_object_type + "_from_json.py")

use_action_msgs = {
    "activate_msg",
    "deactive_msg",
    "out_of_power_msg",
    "msg",
    "menu_text",
    "message",
    "friendly_msg",
    "hostile_msg",
    "need_fire_msg",
    "need_charges_msg",
    "non_interactive_msg",
    "unfold_msg",
    "sound_msg",
    "no_deactivate_msg",
    "not_ready_msg",
    "success_message",
    "lacks_fuel_message",
    "failure_message",
    "descriptions",
    "use_message",
    "noise_message",
    "bury_question",
    "done_message",
    "voluntary_extinguish_message",
    "charges_extinguish_message",
    "water_extinguish_message",
    "auto_extinguish_message",
    "activation_message",
    "holster_msg",
    "holster_prompt",
    "verb",
    "gerund"
}

def extract_use_action_msgs(outfile, use_action, it_name, kwargs):
    """Extract messages for iuse_actor objects. """
    for f in sorted(use_action_msgs):
        if type(use_action) is dict and f in use_action:
            if it_name:
                writestr(outfile, use_action[f],
                  comment="Use action {} for {}.".format(f, it_name), **kwargs)
    # Recursively check sub objects as they may contain more messages.
    if type(use_action) is list:
        for i in use_action:
            extract_use_action_msgs(outfile, i, it_name, kwargs)
    elif type(use_action) is dict:
        for (k, v) in sorted(use_action.items(), key=lambda x: x[0]):
            extract_use_action_msgs(outfile, v, it_name, kwargs)

# extract commonly translatable data from json to fake-python
def extract(item, infilename):
    """Find any extractable strings in the given json object,
    and write them to the appropriate file."""
    if not "type" in item:
        raise WrongJSONItem("ERROR: Object doesn't have a type: {}".format(infilename), item)
    object_type = item["type"]
    outfile = get_outfile(object_type)
    kwargs = {}
    if object_type in ignorable:
        return
    elif object_type in use_format_strings:
        kwargs["format_strings"] = True
    elif object_type in extract_specials:
        extract_specials[object_type](item)
        return
    elif object_type not in automatically_convertible:
        raise WrongJSONItem("ERROR: Unrecognized object type '{}'!".format(object_type), item)
    wrote = False
    name = item.get("name") # Used in gettext comments below.
    # Don't extract any record with name = "none".
    if name and name == "none":
        return
    if name:
        if "name_plural" in item:
            if item["name_plural"] != "none":
                writestr(outfile, name, item["name_plural"], **kwargs)
            else:
                writestr(outfile, name, **kwargs)
        else:
            if object_type in needs_plural:
                # no name_plural entry in json, use default constructed (name+"s"), as in item_factory.cpp
                writestr(outfile, name, "{}s".format(name), **kwargs)
            else:
                writestr(outfile, name, **kwargs)
        wrote = True
    if "name_suffix" in item:
        writestr(outfile, item["name_suffix"], **kwargs)
        wrote = True
    if "name_unique" in item:
        writestr(outfile, item["name_unique"], **kwargs)
        wrote = True
    if "job_description" in item:
        writestr(outfile, item["job_description"], **kwargs)
        wrote = True
    if "use_action" in item:
        extract_use_action_msgs(outfile, item["use_action"], item.get("name"), kwargs)
        wrote = True
    if "description" in item:
        if name:
            c = "Description for {}".format(name)
        else:
            c = None
        writestr(outfile, item["description"], comment=c, **kwargs)
        wrote = True
    if "detailed_definition" in item:
        writestr(outfile, item["detailed_definition"], **kwargs)
        wrote = True
    if "sound" in item:
        writestr(outfile, item["sound"], **kwargs)
        wrote = True
    if "snippet_category" in item and type(item["snippet_category"]) is list:
        # snippet_category is either a simple string (the category ident)
        # which is not translated, or an array of snippet texts.
        for entry in item["snippet_category"]:
            # Each entry is a json-object with an id and text
            if type(entry) is dict:
                writestr(outfile, entry["text"], **kwargs)
                wrote = True
            else:
                # or a simple string
                writestr(outfile, entry, **kwargs)
                wrote = True
    if "bash" in item and type(item["bash"]) is dict:
        # entries of type technique have a bash member, too.
        # but it's a int, not an object.
        bash = item["bash"]
        if "sound" in bash:
            writestr(outfile, bash["sound"], **kwargs)
            wrote = True
        if "sound_fail" in bash:
            writestr(outfile, bash["sound_fail"], **kwargs)
            wrote = True
    if "seed_data" in item:
        seed_data = item["seed_data"]
        writestr(outfile, seed_data["plant_name"], **kwargs)
    if "text" in item:
        writestr(outfile, item["text"], **kwargs)
        wrote = True
    if "message" in item:
        writestr(outfile, item["message"], format_strings=True,
                 comment="Message for {} '{}'".format(object_type, name), **kwargs )
        wrote = True
    if "messages" in item:
        for message in item["messages"]:
            writestr(outfile, message, **kwargs)
            wrote = True
    if "valid_mod_locations" in item:
        for mod_loc in item["valid_mod_locations"]:
            writestr(outfile, mod_loc[0], **kwargs)
            wrote = True
    if "info" in item:
       c = "Please leave anything in <angle brackets> unchanged."
       writestr(outfile, item["info"], comment=c, **kwargs)
       wrote = True
    if "verb" in item:
       writestr(outfile, item["verb"], **kwargs)
       wrote = True
    if "special_attacks" in item:
        special_attacks = item["special_attacks"]
        for special_attack in special_attacks:
            if "description" in special_attack:
                writestr(outfile, special_attack["description"], **kwargs)
                wrote = True
            if "monster_message" in special_attack:
                writestr(outfile, special_attack["monster_message"], format_strings=True,
                         comment="Attack message of monster \"{}\"'s spell \"{}\""
                         .format(name, special_attack.get("spell_id")), **kwargs)
                wrote = True
    if "footsteps" in item:
       writestr(outfile, item["footsteps"], **kwargs)
       wrote = True
    if not wrote and not "copy-from" in item:
        if not warning_supressed(infilename):
            print("WARNING: {}: nothing translatable found in item: {}".format(infilename, item))

def is_official_mod(full_path):
    for i in official_mods:
        if full_path.find(i) != -1:
            return True
    return False

def extract_all_from_dir(json_dir):
    """Extract strings from every json file in the specified directory,
    recursing into any subdirectories."""
    allfiles = os.listdir(json_dir)
    allfiles.sort()
    dirs = []
    skiplist = [ os.path.normpath(".gitkeep") ]
    for f in allfiles:
        full_name = os.path.join(json_dir, f)
        if os.path.isdir(full_name):
            dirs.append(f)
        elif f in skiplist or full_name in ignore_files:
            continue
        elif f.endswith(".json"):
            if full_name in git_files_list:
                extract_all_from_file(full_name)
            else:
                if options.verbose:
                    print("Skipping untracked file: '{}'".format(full_name))
        elif f not in not_json:
            if options.verbose:
                print("Skipping file: '{}'".format(f))
    for d in dirs:
        extract_all_from_dir(os.path.join(json_dir, d))

def extract_all_from_file(json_file):
    "Extract translatable strings from every object in the specified file."
    if options.verbose:
        print("Loading {}".format(json_file))

    with open(json_file, encoding="utf-8") as fp:
        jsondata = json.load(fp)
    # it's either an array of objects, or a single object
    try:
        if hasattr(jsondata, "keys"):
            extract(jsondata, json_file)
        else:
            for jsonobject in jsondata:
                extract(jsonobject, json_file)
    except WrongJSONItem as E:
        print("---\nFile: '{0}'".format(json_file))
        print(E)
        exit(1)

def add_fake_types():
    """Add names of fake items and monsters. This is done by hand and must be updated
    manually each time something is added to itypedef.cpp or mtypedef.cpp."""
    outfile = os.path.join(to_dir, os.path.normpath("faketypes.py"))

    # fake item types

    # fake monster types
    writestr(outfile, "human", "humans")

def prepare_git_file_list():
    command_str = "git ls-files"
    res = None;
    if platform == "win32":
        res = subprocess.Popen(command_str, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    else:
        res = subprocess.Popen(command_str, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, close_fds=True)
    output = res.stdout.readlines()
    res.communicate()
    if res.returncode != 0:
        print("'git ls-files' command exited with non-zero exit code: {}".format(res.returncode))
        exit(1)
    for f in output:
        if len(f) > 0:
            git_files_list.add(os.path.normpath(f[:-1].decode('utf8')))

##
##  EXTRACTION
##

print("==> Generating the list of all Git tracked files")
prepare_git_file_list()
print("==> Parsing JSON")
for i in sorted(directories):
    print("----> Traversing directory {}".format(i))
    extract_all_from_dir(i)
print("==> Finalizing")
add_fake_types()

# done.
