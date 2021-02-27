#!/usr/bin/env python3
"Extract translatable strings from the .json files in data/json."

import json
import os
import itertools
import subprocess
from optparse import OptionParser
from sys import platform
from sys import exit

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
        return "---\nJSON error\n{0}\n--- JSON Item:\n{1}\n---".format(
            self.msg, self.item)


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
    "data/json/flags.json",
    "data/json/npcs/npc.json",
    "data/json/overmap_terrain.json",
    "data/json/statistics.json",
    "data/json/traps.json",
    "data/json/vehicleparts/",
    "data/raw/keybindings.json",
    "data/mods/alt_map_key/overmap_terrain.json",
    "data/mods/DeoxyMod/Deoxy_vehicle_parts.json",
    "data/mods/More_Survival_Tools/start_locations.json",
    "data/mods/NPC_Traits/npc_classes.json",
    "data/mods/Tanks/monsters.json",
}}


def warning_supressed(filename):
    for i in warning_suppressed_list:
        if filename.startswith(i):
            return True
    return False


# these files will not be parsed. Full related path.
ignore_files = {os.path.normpath(i) for i in {
    "data/json/anatomy.json",
    "data/json/items/book/abstract.json",
    "data/mods/replacements.json",
    "data/raw/color_templates/no_bright_background.json"
}}

# ignore these directories and their subdirectories
ignore_directories = {os.path.normpath(dir) for dir in {
    "data/mods/TEST_DATA",
}}

# these objects have no translatable strings
ignorable = {
    "ascii_art",
    "ammo_effect",
    "behavior",
    "butchery_requirement",
    "charge_removal_blacklist",
    "city_building",
    "colordef",
    "disease_type",
    "emit",
    "enchantment",
    "event_transformation",
    "EXTERNAL_OPTION",
    "hit_range",
    "ITEM_BLACKLIST",
    "item_group",
    "MIGRATION",
    "mod_tileset",
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
    "region_overlay",
    "region_settings",
    "relic_procgen_data",
    "requirement",
    "rotatable_symbol",
    "SCENARIO_BLACKLIST",
    "scent_type",
    "score",
    "skill_boost",
    "TRAIT_BLACKLIST",
    "trait_group",
    "uncraft",
    "vehicle_group",
    "vehicle_placement",
}

# these objects can have their strings automatically extracted.
# insert object "type" here IF AND ONLY IF
# all of their translatable strings are in the following form:
#   "name" member
#   "description" member
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
    "construction_group",
    "dream",
    "ENGINE",
    "event_statistic",
    "faction",
    "furniture",
    "GENERIC",
    "item_action",
    "ITEM_CATEGORY",
    "json_flag",
    "keybinding",
    "LOOT_ZONE",
    "MAGAZINE",
    "map_extra",
    "MOD_INFO",
    "MONSTER",
    "morale_type",
    "npc",
    "proficiency",
    "npc_class",
    "overmap_land_use_code",
    "overmap_terrain",
    "PET_ARMOR",
    "skill",
    "SPECIES",
    "speech",
    "SPELL",
    "start_location",
    "terrain",
    "TOOL",
    "TOOLMOD",
    "TOOL_ARMOR",
    "tool_quality",
    "vehicle",
    "vehicle_part",
    "vitamin",
    "WHEEL",
    "help",
    "weather_type"
}

# for these objects a plural form is needed
# NOTE: please also change `needs_plural` in `src/item_factory.cpp`
# when changing this list
needs_plural = {
    "AMMO",
    "ARMOR",
    "BATTERY",
    "BIONIC_ITEM",
    "BOOK",
    "COMESTIBLE",
    "ENGINE",
    "GENERIC",
    "GUN",
    "GUNMOD",
    "MAGAZINE",
    "MONSTER",
    "PET_ARMOR",
    "TOOL",
    "TOOLMOD",
    "TOOL_ARMOR",
    "WHEEL",
}

# These objects use a plural form in their description
needs_plural_desc = {
    "event_statistic"
}

# these objects can be automatically converted, but use format strings
use_format_strings = {
    "technique",
}

# For handling grammatical gender
all_genders = ["f", "m", "n"]


def gender_options(subject):
    return [subject + ":" + g for g in all_genders]

#
#  SPECIALIZED EXTRACTION FUNCTIONS
#


def extract_achievement(a):
    outfile = get_outfile(a["type"])
    for f in ("name", "description"):
        if f in a:
            writestr(outfile, a[f])
    for req in a.get("requirements", ()):
        if "description" in req:
            writestr(outfile, req["description"])


def extract_bodypart(item):
    outfile = get_outfile("bodypart")
    # See comments in `body_part_struct::load` of bodypart.cpp about why xxx
    # and xxx_multiple are not inside a single translation object.
    writestr(outfile, item["name"])
    if "name_multiple" in item:
        writestr(outfile, item["name_multiple"])
    writestr(outfile, item["accusative"])
    if "accusative_multiple" in item:
        writestr(outfile, item["accusative_multiple"])
    writestr(outfile, item["encumbrance_text"])
    writestr(outfile, item["heading"])
    writestr(outfile, item["heading_multiple"])
    if "smash_message" in item:
        writestr(outfile, item["smash_message"])
    if "hp_bar_ui_text" in item:
        writestr(outfile, item["hp_bar_ui_text"])


def extract_clothing_mod(item):
    outfile = get_outfile("clothing_mod")
    writestr(outfile, item["implement_prompt"])
    writestr(outfile, item["destroy_prompt"])


def extract_construction(item):
    outfile = get_outfile("construction")
    if "pre_note" in item:
        writestr(outfile, item["pre_note"])


def extract_harvest(item):
    outfile = get_outfile("harvest")
    if "message" in item:
        writestr(outfile, item["message"])


def extract_material(item):
    outfile = get_outfile("material")
    writestr(outfile, item["name"])
    wrote = False
    if "bash_dmg_verb" in item:
        writestr(outfile, item["bash_dmg_verb"])
        wrote = True
    if "cut_dmg_verb" in item:
        writestr(outfile, item["cut_dmg_verb"])
        wrote = True
    if "dmg_adj" in item:
        writestr(outfile, item["dmg_adj"][0])
        writestr(outfile, item["dmg_adj"][1])
        writestr(outfile, item["dmg_adj"][2])
        writestr(outfile, item["dmg_adj"][3])
        wrote = True
    if not wrote and "copy-from" not in item:
        print("WARNING: {}: no mandatory field in item: {}".format(
            "/data/json/materials.json", item))


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
    if "initiate" in item:
        writestr(outfile, item["initiate"], format_strings=True,
                 comment="initiate message for martial art '{}'".format(name))
    onhit_buffs = item.get("onhit_buffs", list())
    static_buffs = item.get("static_buffs", list())
    onmove_buffs = item.get("onmove_buffs", list())
    ondodge_buffs = item.get("ondodge_buffs", list())
    onattack_buffs = item.get("onattack_buffs", list())
    onpause_buffs = item.get("onpause_buffs", list())
    onblock_buffs = item.get("onblock_buffs", list())
    ongethit_buffs = item.get("ongethit_buffs", list())
    onmiss_buffs = item.get("onmiss_buffs", list())
    oncrit_buffs = item.get("oncrit_buffs", list())
    onkill_buffs = item.get("onkill_buffs", list())

    buffs = (onhit_buffs + static_buffs + onmove_buffs + ondodge_buffs +
             onattack_buffs + onpause_buffs + onblock_buffs + ongethit_buffs +
             onmiss_buffs + oncrit_buffs + onkill_buffs)
    for buff in buffs:
        writestr(outfile, buff["name"])
        if buff["name"] == item["name"]:
            c = "Description of buff for martial art '{}'".format(name)
        else:
            c = "Description of buff '{}' for martial art '{}'".format(
                buff["name"], name)
        writestr(outfile, buff["description"], comment=c)


def extract_move_mode(item):
    outfile = get_outfile("move_modes")
    # Move mode name
    name = item["name"]
    writestr(outfile, name, comment="Move mode name")
    # The character in the move menu
    character = item["character"]
    writestr(outfile, character,
             comment="Move mode character in move mode menu")
    # The character in the panels
    pchar = item["panel_char"]
    writestr(outfile, pchar, comment="movement-type")
    # Successful change message
    change_good_none = item["change_good_none"]
    writestr(outfile, change_good_none,
             comment="Successfully switch to this move mode, no steed")
    # Successful change message (animal steed)
    change_good_animal = item["change_good_animal"]
    writestr(outfile, change_good_animal,
             comment="Successfully switch to this move mode, animal steed")
    # Successful change message (mech steed)
    change_good_mech = item["change_good_mech"]
    writestr(outfile, change_good_mech,
             comment="Successfully switch to this move mode, mech steed")
    if "change_bad_none" in item:
        # Failed change message
        change_bad_none = item["change_bad_none"]
        writestr(outfile, change_bad_none,
                 comment="Failure to switch to this move mode, no steed")
    if "change_bad_animal" in item:
        # Failed change message (animal steed)
        change_bad_animal = item["change_bad_animal"]
        writestr(outfile, change_bad_animal,
                 comment="Failure to switch to this move mode, animal steed")
    if "change_bad_mech" in item:
        # Failed change message (mech steed)
        change_bad_mech = item["change_bad_mech"]
        writestr(outfile, change_bad_mech,
                 comment="Failure to switch to this move mode, mech steed")


def extract_effect_type(item):
    # writestr will not write string if it is None.
    outfile = get_outfile("effects")
    ctxt_name = item.get("name", ())

    if ctxt_name:
        if len(ctxt_name) == len(item.get("desc", ())):
            for nm_desc in zip(ctxt_name, item.get("desc", ())):
                comment = "Description of effect '{}'.".format(nm_desc[0])
                writestr(outfile, nm_desc[0])
                writestr(outfile, nm_desc[1], format_strings=True,
                         comment=comment)
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
        comment = "Apply message for effect(s) '{}'.".format(', '.join(name))
        writestr(outfile, msg, format_strings=True, comment=comment)

    # remove_message
    msg = item.get("remove_message")
    if not name:
        writestr(outfile, msg, format_strings=True)
    else:
        comment = "Remove message for effect(s) '{}'.".format(', '.join(name))
        writestr(outfile, msg, format_strings=True, comment=comment)

    # miss messages
    msg = item.get("miss_messages", ())
    if not name:
        for m in msg:
            writestr(outfile, m[0])
    else:
        for m in msg:
            comment = "Miss message for effect(s) '{}'.".format(
                ', '.join(name))
            writestr(outfile, m[0], comment=comment)
    msg = item.get("decay_messages", ())
    if not name:
        for m in msg:
            writestr(outfile, m[0])
    else:
        for m in msg:
            comment = "Decay message for effect(s) '{}'.".format(
                ', '.join(name))
            writestr(outfile, m[0], comment=comment)

    # speed_name
    if "speed_name" in item:
        if not name:
            writestr(outfile, item.get("speed_name"))
        else:
            comment = "Speed name of effect(s) '{}'.".format(', '.join(name))
            writestr(outfile, item.get("speed_name"), comment=comment)

    # apply and remove memorial messages.
    msg = item.get("apply_memorial_log")
    if not name:
        writestr(outfile, msg, context="memorial_male")
        writestr(outfile, msg, context="memorial_female")
    else:
        comment = "Male memorial apply log for effect(s) '{}'.".format(
            ', '.join(name))
        writestr(outfile, msg, context="memorial_male", comment=comment)
        comment = "Female memorial apply log for effect(s) '{}'.".format(
            ', '.join(name))
        writestr(outfile, msg, context="memorial_female", comment=comment)
    msg = item.get("remove_memorial_log")
    if not name:
        writestr(outfile, msg, context="memorial_male")
        writestr(outfile, msg, context="memorial_female")
    else:
        comment = "Male memorial remove log for effect(s) '{}'.".format(
            ', '.join(name))
        writestr(outfile, msg, context="memorial_male", comment=comment)
        comment = "Female memorial remove log for effect(s) '{}'.".format(
            ', '.join(name))
        writestr(outfile, msg, context="memorial_female", comment=comment)


def extract_gun(item):
    outfile = get_outfile("gun")
    if "name" in item:
        item_name = item.get("name")
        if item["type"] in needs_plural:
            writestr(outfile, item_name, pl_fmt=True)
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
    if "use_action" in item:
        use_action = item.get("use_action")
        item_name = item.get("name")
        extract_use_action_msgs(outfile, use_action, item_name, {})


def extract_gunmod(item):
    outfile = get_outfile("gunmod")
    if "name" in item:
        item_name = item.get("name")
        if item["type"] in needs_plural:
            writestr(outfile, item_name, pl_fmt=True)
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
    comment = "Name for scenario '{}' for a male character".format(name)
    writestr(outfile, name, context="scenario_male", comment=comment)
    comment = "Name for scenario '{}' for a female character".format(name)
    writestr(outfile, name, context="scenario_female", comment=comment)
    if name:
        msg = item.get("description")
        if msg:
            comment = ("Description for scenario '{}' for a male "
                       "character.".format(name))
            writestr(outfile, msg, context="scen_desc_male", comment=comment)
            comment = ("Description for scenario '{}' for a female "
                       "character.".format(name))
            writestr(outfile, msg, context="scen_desc_female",
                     comment=comment)
        msg = item.get("start_name")
        if msg:
            comment = "Starting location for scenario '{}'.".format(name)
            writestr(outfile, msg, context="start_name", comment=comment)
    else:
        for f in ["description", "start_name"]:
            found = item.get(f, None)
            writestr(outfile, found)


def items_sorted_by_key(d):
    return sorted(d.items(), key=lambda x: x[0])


def extract_mapgen(item):
    outfile = get_outfile("mapgen")
    # writestr will not write string if it is None.
    for (objkey, objval) in items_sorted_by_key(item["object"]):
        if objkey == "place_specials" or objkey == "place_signs":
            for special in objval:
                for (speckey, specval) in items_sorted_by_key(special):
                    if speckey == "signage":
                        writestr(outfile, specval, comment="Sign")
        elif objkey == "signs":
            for (k, v) in items_sorted_by_key(objval):
                sign = v.get("signage", None)
                writestr(outfile, sign, comment="Sign")
        elif objkey == "computers":
            for (k, v) in items_sorted_by_key(objval):
                if "name" in v:
                    writestr(outfile, v.get("name"), comment="Computer name")
                if "options" in v:
                    for opt in v.get("options"):
                        writestr(outfile, opt.get("name"),
                                 comment="Computer option")
                if "access_denied" in v:
                    writestr(outfile, v.get("access_denied"),
                             comment="Computer access denied warning")


def extract_palette(item):
    outfile = get_outfile("palette")
    if "signs" in item:
        for (k, v) in items_sorted_by_key(item["signs"]):
            sign = v.get("signage", None)
            writestr(outfile, sign, comment="Sign")


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
        if type(item["book_learn"]) is dict:
            for (k, v) in item["book_learn"].items():
                if type(v) is dict and "recipe_name" in v:
                    writestr(outfile, v["recipe_name"])
    if "description" in item:
        writestr(outfile, item["description"])
    if "blueprint_name" in item:
        writestr(outfile, item["blueprint_name"])


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


dynamic_line_string_keys = [
    # from `simple_string_conds` in `condition.h`
    "u_male", "u_female", "npc_male", "npc_female",
    "has_no_assigned_mission", "has_assigned_mission",
    "has_many_assigned_missions", "has_no_available_mission",
    "has_available_mission", "has_many_available_missions",
    "mission_complete", "mission_incomplete", "mission_has_generic_rewards",
    "npc_available", "npc_following", "npc_friend", "npc_hostile",
    "npc_train_skills", "npc_train_styles",
    "at_safe_space", "is_day", "npc_has_activity", "is_outside", "u_has_camp",
    "u_can_stow_weapon", "npc_can_stow_weapon", "u_has_weapon",
    "npc_has_weapon", "u_driving", "npc_driving",
    "has_pickup_list", "is_by_radio", "has_reason",
    # yes/no strings for complex conditions, 'and' list
    "yes", "no", "and"
]


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
                comment = "Nickname for creature '{}'".format(
                    eff["u_buy_monster"])
                writestr(outfile, eff["name"], comment=comment)


def extract_talk_response(response, outfile):
    if "text" in response:
        writestr(outfile, response["text"])
    if "truefalsetext" in response:
        writestr(outfile, response["truefalsetext"]["true"])
        writestr(outfile, response["truefalsetext"]["false"])
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
    if "repeat_responses" in item:
        rr = item["repeat_responses"]
        if type(rr) is dict and "response" in rr:
            extract_talk_response(rr["response"], outfile)
        elif type(rr) is list:
            for r in rr:
                if "response" in r:
                    extract_talk_response(r["response"], outfile)
    if "effect" in item:
        extract_talk_effects(item["effect"], outfile)


def extract_trap(item):
    outfile = get_outfile("trap")
    writestr(outfile, item["name"])
    if "vehicle_data" in item and "sound" in item["vehicle_data"]:
        comment = "Trap-vehicle collision message for trap '{}'".format(
            item["name"])
        writestr(outfile, item["vehicle_data"]["sound"], comment=comment)


def extract_missiondef(item):
    outfile = get_outfile("mission_def")
    item_name = item.get("name")
    if item_name is None:
        raise WrongJSONItem("JSON item don't contain 'name' field", item)
    writestr(outfile, item_name)
    if "description" in item:
        comment = "Description for mission '{}'".format(item_name)
        writestr(outfile, item["description"], comment=comment)
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

    item_name_or_id = found = item.get("name")
    if found is None:
        if "copy-from" in item:
            item_name_or_id = item["id"]
        else:
            raise WrongJSONItem("JSON item don't contain 'name' field", item)
    else:
        writestr(outfile, found)

    simple_fields = ["description"]

    for f in simple_fields:
        found = item.get(f)
        # Need that check due format string argument
        if found is not None:
            comment = "Description for {}".format(item_name_or_id)
            writestr(outfile, found, comment=comment)

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

    if "ranged_mutation" in item:
        writestr(outfile, item.get("ranged_mutation").get("message"))


def extract_mutation_category(item):
    outfile = get_outfile("mutation_category")

    item_name = found = item.get("name")
    if found is None:
        raise WrongJSONItem("JSON item don't contain 'name' field", item)
    writestr(outfile, found, comment="Mutation class name")

    simple_fields = ["mutagen_message",
                     "iv_message",
                     "iv_sleep_message",
                     "iv_sound_message",
                     "junkie_message"
                     ]

    for f in simple_fields:
        found = item.get(f)
        # Need that check due format string argument
        if found is not None:
            comment = "Mutation class: {} {}".format(item_name, f)
            writestr(outfile, found, comment=comment)

    found = item.get("memorial_message")
    comment = "Mutation class: {} Male memorial messsage".format(item_name)
    writestr(outfile, found, context="memorial_male", comment=comment)
    comment = "Mutation class: {} Female memorial messsage".format(item_name)
    writestr(outfile, found, context="memorial_female", comment=comment)


def extract_vehspawn(item):
    outfile = get_outfile("vehicle_spawn")

    found = item.get("spawn_types")
    if not found:
        return

    for st in found:
        writestr(outfile, st.get("description"),
                 comment="Vehicle Spawn Description")


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
            writestr(outfile, 'ALL',
                     comment="Crafting recipes subcategory all")
            continue
        subcat_name = subcat.split('_')[2]
        comment = "Crafting recipes subcategory of '{}' category".format(
            cat_name)
        writestr(outfile, subcat_name, comment=comment)


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
            writestr(outfile, fd.get("name"))


def extract_ter_furn_transform_messages(item):
    outfile = get_outfile("ter_furn_transform_messages")
    if "fail_message" in item:
        writestr(outfile, item.get("fail_message"))
    if "terrain" in item:
        for terrain in item.get("terrain"):
            writestr(outfile, terrain.get("message"))
    if "furniture" in item:
        for furniture in item.get("furniture"):
            writestr(outfile, furniture.get("message"))


def extract_skill_display_type(item):
    outfile = get_outfile("skill_display_type")
    comment = "display string for skill display type '{}'".format(item["id"])
    writestr(outfile, item["display_string"], comment=comment)


def extract_fault(item):
    outfile = get_outfile("fault")
    writestr(outfile, item["name"])
    comment = "description for fault '{}'".format(item["name"])
    writestr(outfile, item["description"], comment=comment)
    for method in item["mending_methods"]:
        if "name" in method:
            comment = "name of mending method for fault '{}'".format(
                item["name"])
            writestr(outfile, method["name"], comment=comment)
        if "description" in method:
            comment = ("description for mending method '{}' of fault "
                       "'{}'".format(method["name"], item["name"]))
            writestr(outfile, method["description"], comment=comment)
        if "success_msg" in method:
            comment = ("success message for mending method '{}' of fault "
                       "'{}'".format(method["name"], item["name"]))
            writestr(outfile, method["success_msg"], format_strings=True,
                     comment=comment)


def extract_snippets(item):
    outfile = get_outfile("snippet")
    text = item["text"]
    if type(text) is not list:
        text = [text]
    for snip in text:
        if type(snip) is str:
            writestr(outfile, snip)
        else:
            writestr(outfile, snip["text"])


def extract_vehicle_part_category(item):
    outfile = get_outfile("vehicle_part_categories")
    name = item.get("name")
    short_name = item.get("short_name")
    comment = item.get("//")
    short_comment = "(short name, optimal 1 symbol) " + comment
    writestr(outfile, name, comment=comment)
    writestr(outfile, short_name, comment=short_comment)


# these objects need to have their strings specially extracted
extract_specials = {
    "achievement": extract_achievement,
    "body_part": extract_bodypart,
    "clothing_mod": extract_clothing_mod,
    "conduct": extract_achievement,
    "construction": extract_construction,
    "effect_type": extract_effect_type,
    "fault": extract_fault,
    "GUN": extract_gun,
    "GUNMOD": extract_gunmod,
    "harvest": extract_harvest,
    "mapgen": extract_mapgen,
    "martial_art": extract_martial_art,
    "material": extract_material,
    "mission_definition": extract_missiondef,
    "monster_attack": extract_monster_attack,
    "movement_mode": extract_move_mode,
    "mutation": extract_mutation,
    "mutation_category": extract_mutation_category,
    "palette": extract_palette,
    "profession": extract_professions,
    "recipe_category": extract_recipe_category,
    "recipe": extract_recipes,
    "recipe_group": extract_recipe_group,
    "scenario": extract_scenarios,
    "snippet": extract_snippets,
    "talk_topic": extract_talk_topic,
    "trap": extract_trap,
    "gate": extract_gate,
    "vehicle_spawn": extract_vehspawn,
    "field_type": extract_field_type,
    "ter_furn_transform": extract_ter_furn_transform_messages,
    "skill_display_type": extract_skill_display_type,
    "vehicle_part_category": extract_vehicle_part_category,
}

#
#  PREPARATION
#

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
    if not filename.endswith(".py"):
        continue
    f = os.path.join(to_dir, filename)
    os.remove(f)

#
#  FUNCTIONS
#


def tlcomment(fs, string):
    "Write the string to the file as a comment for translators."
    if len(string) > 0:
        for line in string.splitlines():
            fs.write("#~ {}\n".format(line))


def gettextify(string, context=None, plural=None):
    "Put the string in a fake gettext call, and add a newline."
    if context:
        if plural:
            return "npgettext(%r, %r, %r, n)\n" % (context, string, plural)
        else:
            return "pgettext(%r, %r)\n" % (context, string)
    else:
        if plural:
            return "ngettext(%r, %r, n)\n" % (string, plural)
        else:
            return "_(%r)\n" % string


# `context` is deprecated and only for use in legacy code. Use
# `class translation` to read the text in c++ and specify the context in json
# instead.
def writestr(filename, string, context=None, format_strings=False,
             comment=None, pl_fmt=False, _local_fp_cache=dict()):
    "Wrap the string and write to the file."
    if type(string) is list:
        for entry in string:
            writestr(filename, entry, context, format_strings, comment, pl_fmt)
        return
    elif type(string) is dict:
        if "//~" in string:
            if comment is None:
                comment = string["//~"]
            else:
                comment = "{}\n{}".format(comment, string["//~"])
        if context is None:
            context = string.get("ctxt")
        elif "ctxt" in string:
            raise WrongJSONItem("ERROR: 'ctxt' found in json when `context` "
                                "parameter is specified", string)
        str_pl = None
        if pl_fmt:
            if "str_pl" in string:
                str_pl = string["str_pl"]
            elif "str_sp" in string:
                str_pl = string["str_sp"]
            else:
                # no "str_pl" entry in json, assuming regular plural form as in
                # translations.cpp
                str_pl = "{}s".format(string["str"])
        elif "str_pl" in string or "str_sp" in string:
            raise WrongJSONItem(
                "ERROR: 'str_pl' and 'str_sp' not supported here", string)
        if "str" in string:
            str_singular = string["str"]
        elif "str_sp" in string:
            str_singular = string["str_sp"]
        else:
            raise WrongJSONItem("ERROR: 'str' or 'str_sp' not found", string)
    elif type(string) is str:
        if len(string) == 0:
            # empty string has special meaning for gettext, skip it
            return
        str_singular = string
        if pl_fmt:
            # no "str_pl" entry in json, assuming regular plural form as in
            # translations.cpp
            str_pl = "{}s".format(string)
        else:
            str_pl = None
    elif string is None:
        return
    else:
        raise WrongJSONItem(
            "ERROR: value is not a string, dict, list, or None", string)

    if filename in _local_fp_cache:
        fs = _local_fp_cache[filename]
    else:
        fs = open(filename, 'a', encoding="utf-8", newline='\n')
        _local_fp_cache[filename] = fs
    # Append developers comment
    if comment:
        tlcomment(fs, comment)
    # most of the strings from json don't use string formatting.
    # we must tell xgettext this explicitly
    contains_percent = ("%" in str_singular or
                        (str_pl is not None and "%" in str_pl))
    if not format_strings and contains_percent:
        fs.write("# xgettext:no-python-format\n")
    fs.write(gettextify(str_singular, context=context, plural=str_pl))


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
                comment = "Use action {} for {}.".format(f, it_name)
                writestr(outfile, use_action[f], comment=comment, **kwargs)
    # Recursively check sub objects as they may contain more messages.
    if type(use_action) is list:
        for i in use_action:
            extract_use_action_msgs(outfile, i, it_name, kwargs)
    elif type(use_action) is dict:
        for (k, v) in sorted(use_action.items(), key=lambda x: x[0]):
            extract_use_action_msgs(outfile, v, it_name, kwargs)


found_types = set()
known_types = (ignorable | use_format_strings | extract_specials.keys() |
               automatically_convertible)


# extract commonly translatable data from json to fake-python
def extract(item, infilename):
    """Find any extractable strings in the given json object,
    and write them to the appropriate file."""
    if "type" not in item:
        return
    object_type = item["type"]
    found_types.add(object_type)
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
        raise WrongJSONItem(
            "ERROR: Unrecognized object type '{}'!".format(object_type), item)
    if object_type not in known_types:
        print("WARNING: known_types does not contain object type '{}'".format(
              object_type))
    wrote = False
    name = item.get("name")  # Used in gettext comments below.
    # Don't extract any record with name = "none".
    if name and name == "none":
        return
    if name:
        if object_type in needs_plural:
            writestr(outfile, name, pl_fmt=True, **kwargs)
        else:
            writestr(outfile, name, **kwargs)
        wrote = True
        if type(name) is dict and "str" in name:
            singular_name = name["str"]
        else:
            singular_name = name

    def do_extract(item):
        wrote = False
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
            extract_use_action_msgs(outfile, item["use_action"],
                                    singular_name, kwargs)
            wrote = True
        if "conditional_names" in item:
            for cname in item["conditional_names"]:
                c = "Conditional name for {} when {} matches {}".format(
                    name, cname["type"], cname["condition"])
                writestr(outfile, cname["name"], comment=c,
                         format_strings=True, pl_fmt=True, **kwargs)
                wrote = True
        if "description" in item:
            if name:
                c = "Description for {}".format(singular_name)
            else:
                c = None
            if object_type in needs_plural_desc:
                writestr(outfile, item["description"], comment=c, pl_fmt=True,
                         **kwargs)
            else:
                writestr(outfile, item["description"], comment=c, **kwargs)
            wrote = True
        if "detailed_definition" in item:
            writestr(outfile, item["detailed_definition"], **kwargs)
            wrote = True
        if "sound" in item:
            writestr(outfile, item["sound"], **kwargs)
            wrote = True
        if "sound_description" in item:
            comment = "description for the sound of spell '{}'".format(name)
            writestr(outfile, item["sound_description"], comment=comment,
                     **kwargs)
            wrote = True
        if ("snippet_category" in item and
                type(item["snippet_category"]) is list):
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
            wrote = True
        if "relic_data" in item and "name" in item["relic_data"]:
            writestr(outfile, item["relic_data"]["name"], **kwargs)
            wrote = True
        if "text" in item:
            writestr(outfile, item["text"], **kwargs)
            wrote = True
        if "message" in item:
            writestr(outfile, item["message"], format_strings=True,
                     comment="Message for {} '{}'".format(object_type, name),
                     **kwargs)
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
        if "restriction" in item:
            writestr(outfile, item["restriction"], **kwargs)
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
                    comment = (
                        "Attack message of monster \"{}\"'s spell \"{}\""
                        .format(name["str"] if name and "str" in name
                                else name,
                                special_attack.get("spell_id")))
                    writestr(outfile, special_attack["monster_message"],
                             format_strings=True, comment=comment, **kwargs)
                    wrote = True
                if "targeting_sound" in special_attack:
                    writestr(outfile, special_attack["targeting_sound"],
                             **kwargs)
                    wrote = True
                if "no_ammo_sound" in special_attack:
                    writestr(outfile, special_attack["no_ammo_sound"],
                             **kwargs)
                    wrote = True
        if "footsteps" in item:
            writestr(outfile, item["footsteps"], **kwargs)
            wrote = True
        if "revert_msg" in item:
            writestr(outfile, item["revert_msg"], **kwargs)
            wrote = True
        return wrote
    wrote |= do_extract(item)
    if "extend" in item:
        wrote |= do_extract(item["extend"])
    if not wrote and "copy-from" not in item:
        if not warning_supressed(infilename):
            print("WARNING: {}: nothing translatable found in item: "
                  "{}".format(infilename, item))


def extract_all_from_dir(json_dir):
    """Extract strings from every json file in the specified directory,
    recursing into any subdirectories."""
    allfiles = os.listdir(json_dir)
    allfiles.sort()
    dirs = []
    skiplist = [os.path.normpath(".gitkeep")]
    for f in allfiles:
        full_name = os.path.join(json_dir, f)
        if os.path.isdir(full_name):
            dirs.append(f)
        elif f in skiplist or full_name in ignore_files:
            continue
        elif any(full_name.startswith(dir) for dir in ignore_directories):
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


def prepare_git_file_list():
    command_str = "git ls-files"
    res = None
    if platform == "win32":
        res = subprocess.Popen(command_str, shell=True, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)
    else:
        res = subprocess.Popen(command_str, shell=True, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, close_fds=True)
    output = res.stdout.readlines()
    res.communicate()
    if res.returncode != 0:
        print("'git ls-files' command exited with non-zero exit code: "
              "{}".format(res.returncode))
        exit(1)
    for f in output:
        if len(f) > 0:
            git_files_list.add(os.path.normpath(f[:-1].decode('utf8')))


#
#  EXTRACTION
#

ignored_types = []

# first, make sure we aren't erroneously ignoring types
for ignored in ignorable:
    if ignored in automatically_convertible:
        ignored_types.append(ignored)
    if ignored in extract_specials:
        ignored_types.append(ignored)

if len(ignored_types) != 0:
    print("ERROR: Some types set to be both extracted and ignored:")
    for ignored in ignored_types:
        print(ignored)
    exit(-1)

print("==> Generating the list of all Git tracked files")
prepare_git_file_list()
print("==> Parsing JSON")
for i in sorted(directories):
    print("----> Traversing directory {}".format(i))
    extract_all_from_dir(i)
print("==> Finalizing")
if len(known_types - found_types) != 0:
    print("WARNING: type {} not found in any JSON objects".format(
        known_types - found_types))
if len(needs_plural - found_types) != 0:
    print("WARNING: type {} from needs_plural not found in any JSON "
          "objects".format(needs_plural - found_types))

print("Output files in %s" % to_dir)

# done.
