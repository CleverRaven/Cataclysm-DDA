#!/usr/bin/env python
"Extract translatable strings from the .json files in data/json."

from __future__ import print_function

import json
import os

# Exceptions
class WrongJSONItem(Exception):
    def __init__(self, msg, item):
        self.msg = msg
        self.item = item
    def __str__(self):
        return ("---\nWrong JSON item:\n{0}\n--- JSON Item:\n{1}\n---".format(self.msg, self.item))

# there may be some non-json files in data/raw
not_json = {
    "sokoban.txt",
    "main.lua",
    "preload.lua",
    "LOADING_ORDER.md"
}

# don't parse this files. Full related path.
ignore_files = {
    "data/mods/obsolete-mods.json",
    "data/raw/color_templates/no_bright_background.json"
}

# these objects have no translatable strings
ignorable = {
    "BULLET_PULLING",
    "ITEM_BLACKLIST",
    "MONSTER_BLACKLIST",
    "MONSTER_FACTION",
    "MONSTER_WHITELIST",
    "SPECIES",
    "colordef",
    "epilogue", # FIXME right now this object can't be translated correctly
    "item_group",
    "monitems",
    "monstergroup",
    "npc",      # FIXME right now this object is unextractable
    "overmap_special",
    "recipe_category",
    "recipe_subcategory",
    "region_overlay",
    "region_settings",
    "vehicle_group",
    "vehicle_placement"
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
    "AMMO",
    "ARMOR",
    "ammunition_type",
    "bionic",
    "BIONIC_ITEM",
    "BOOK",
    "COMESTIBLE",
    "construction",
    "CONTAINER",
    "dream",
    "faction",
    "furniture",
    "GENERIC",
    "GUNMOD",
    "GUN",
    "STATIONARY_ITEM",
    "hint",
    "item_action",
    "ITEM_CATEGORY",
    "keybinding",
    "lab_note",
    "MOD_INFO",
    "MONSTER",
    "mutation",
    "overmap_terrain",
    "skill",
    "snippet",
    "speech",
    "start_location",
    "terrain",
    "tool_quality",
    "TOOL",
    "TOOL_ARMOR",
    "trap",
    "tutorial_messages",
    "VAR_VEH_PART",
    "vehicle_part",
    "vehicle",
}

# for these objects a plural form is needed
needs_plural = {
    "AMMO",
    "ARMOR",
    "BIONIC_ITEM",
    "BOOK",
    "COMESTIBLE",
    "CONTAINER",
    "GENERIC",
    "GUNMOD",
    "GUN",
    "STATIONARY_ITEM",
    "TOOL",
    "TOOL_ARMOR",
    "VAR_VEH_PART",
    "MONSTER"
}

# these objects can be automatically converted, but use format strings
use_format_strings = {
    "technique",
}

##
##  SPECIALIZED EXTRACTION FUNCTIONS
##

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
    writestr(outfile, item["name"])
    writestr(outfile, item["description"],
             comment="Description for martial art '%s'" % item["name"])
    onhit_buffs = item.get("onhit_buffs", list())
    static_buffs = item.get("static_buffs", list())
    buffs = onhit_buffs + static_buffs
    for buff in buffs:
        writestr(outfile, buff["name"])
        if buff["name"] == item["name"]:
            c="Description of buff for martial art '%s'" % item["name"]
        else:
            c="Description of buff '%s' for martial art '%s'" % (buff["name"], item["name"])
        writestr(outfile, buff["description"], comment=c)

def extract_effect_type(item):
    # writestr will not write string if it is None.
    outfile = get_outfile("effects")
    name = item.get("name", ())

    if name:
        if len(name) == len(item.get("desc", ())):
            for nm_desc in zip(name, item.get("desc", ())):
                writestr(outfile, nm_desc[0])
                writestr(outfile, nm_desc[1], format_strings=True,
                         comment="Description of effect '{}'.".format(nm_desc[0]))
        else:
            for i in item.get("name", ()):
                writestr(outfile, i)
            for f in ["desc", "reduced_desc"]:
                for i in item.get(f, ()):
                    writestr(outfile, i, format_strings=True)

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

    # aplly and remove memorial messages.
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
    writestr(outfile, name)
    if name:
        msg = item.get("description")
        if msg:
            writestr(outfile, msg, comment="Description for scenario '{}'.".format(name))
        msg = item.get("start_name")
        if msg:
            writestr(outfile, msg, comment="Starting location for scenario '{}'.".format(name))
    else:
        for f in ["description", "start_name"]:
            found = item.get(f, None)
            writestr(outfile, found)

def extract_mapgen(item):
    outfile = get_outfile("mapgen")
    # writestr will not write string if it is None.
    for objkey in item["object"]:
        if objkey == "place_specials":
            for special in item["object"][objkey]:
                for speckey in special:
                    if speckey == "signage":
                        writestr(outfile, special[speckey])

def extract_recipes(item):
    outfile = get_outfile("recipe")
    if "book_learn" in item:
        for arr in item["book_learn"]:
            if len(arr) >= 3 and len(arr[2]) > 0:
                writestr(outfile, arr[2])

def extract_dynamic_line_optional(line, member, outfile):
    if member in line:
        extract_dynamic_line(line[member], outfile)

def extract_dynamic_line(line, outfile):
    if type(line) == list:
        for l in line:
            extract_dynamic_line(l, outfile)
    elif type(line) == dict:
        extract_dynamic_line_optional(line, "u_male", outfile)
        extract_dynamic_line_optional(line, "u_female", outfile)
        extract_dynamic_line_optional(line, "npc_male", outfile)
        extract_dynamic_line_optional(line, "npc_female", outfile)
        extract_dynamic_line_optional(line, "yes", outfile)
        extract_dynamic_line_optional(line, "no", outfile)
    else:
        writestr(outfile, line)

def extract_talk_response(response, outfile):
    if "text" in response:
        writestr(outfile, response["text"])
    if "success" in response:
        extract_talk_response(response["success"], outfile)
    if "failure" in response:
        extract_talk_response(response["failure"], outfile)

def extract_talk_topic(item):
    outfile = get_outfile("talk_topic")
    if "dynamic_line" in item:
        extract_dynamic_line(item["dynamic_line"], outfile)
    for r in item["responses"]:
        extract_talk_response(r, outfile)


def extract_mutation(item):
    outfile = get_outfile("mutation_category")

    item_name = found = item.get("name")
    if found is None:
        raise WrongJSONItem("JSON item don't contain 'name' field", item)
        return
    writestr(outfile, found, comment="Mutation class name")

    simple_fields = [ "mutagen_message",
                      "iv_message",
                      "iv_sound_message",
                      "junkie_message"
                    ]

    for f in simple_fields:
        found = item.get(f)
        # Need that check due format string argument
        if found is not None:
            writestr(outfile, found, comment="Mutation class: {0} {1}".format(item_name, f))

    found = item.get("memorial_message")
    writestr(outfile, found, context="memorial_male",
             comment="Mutation class: {0} Male memorial messsage".format(item_name))
    writestr(outfile, found, context="memorial_female",
             comment="Mutation class: {0} Female memorial messsage".format(item_name))


def extract_vehspawn(item):
    outfile = get_outfile("vehicle_spawn")

    found = item.get("spawn_types")
    if not found:
        return

    for st in found:
        writestr(outfile, st.get("description"), comment="Vehicle Spawn Description")

# these objects need to have their strings specially extracted
extract_specials = {
    "effect_type": extract_effect_type,
    "material": extract_material,
    "martial_art": extract_martial_art,
    "profession": extract_professions,
    "recipe": extract_recipes,
    "scenario": extract_scenarios,
    "mapgen": extract_mapgen,
    "talk_topic": extract_talk_topic,
    "mutation_category": extract_mutation,
    "vehicle_spawn": extract_vehspawn
}

##
##  PREPARATION
##

# allow running from main directory, or from script subdirectory
if os.path.exists("data/json"):
    raw_dir = "data/raw"
    json_dir = "data/json"
    mods_dir = "data/mods"
    to_dir = "lang/json"
elif os.path.exists("../data/json"):
    raw_dir = "../data/raw"
    json_dir = "../data/json"
    mods_dir = "../data/mods"
    to_dir = "../lang/json"
else:
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
    if type(string) is list and plural is None:
        for entry in string:
            writestr(filename, entry, None, context, format_strings, comment)
        return

    "Wrap the string and write to the file."
    # no empty strings
    if not string: return
    with open(filename,'a') as fs:
        # Append developers comment
        if comment:
            tlcomment(fs, comment)
        # most of the strings from json don't use string formatting.
        # we must tell xgettext this explicitly
        if not format_strings and "%" in string:
            fs.write("# xgettext:no-python-format\n")
        fs.write(gettextify(string,context=context,plural=plural))

def tlcomment(fs, string):
    "Write the string to the file as a comment for translators."
    if len(string) > 0:
        fs.write("#~ ")
        fs.write(string.encode('utf-8'))
        fs.write("\n")

def get_outfile(json_object_type):
    return os.path.join(to_dir, json_object_type + "_from_json.py")

use_action_msgs = {
    "activate_msg",
    "deactive_msg",
    "out_of_power_msg",
    "msg",
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
    "noise_message",
    "voluntary_extinguish_message",
    "charges_extinguish_message",
    "water_extinguish_message",
    "auto_extinguish_message",
    "activation_message",
    "holster_msg",
    "holster_prompt"
}

def extract_use_action_msgs(outfile, use_action, it_name, kwargs):
    """Extract messages for iuse_actor objects. """
    for f in use_action_msgs:
        if f in use_action:
            if it_name:
                writestr(outfile, use_action[f],
                  comment="Use action {} for {}.".format(f, it_name), **kwargs)

# extract commonly translatable data from json to fake-python
def extract(item, infilename):
    """Find any extractable strings in the given json object,
    and write them to the appropriate file."""
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
        raise WrongJSONItem("ERROR: Unrecognized object type '{0}'!".format(object_type), item)
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
                writestr(outfile, name, "%ss" % name, **kwargs)
            else:
                writestr(outfile, name, **kwargs)
        wrote = True
    if "use_action" in item:
        extract_use_action_msgs(outfile, item["use_action"], item.get("name"), kwargs)
        wrote = True
    if "description" in item:
        if name:
            c = "Description for %s" % name
        else:
            c = None
        writestr(outfile, item["description"], comment=c, **kwargs)
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
    if "messages" in item:
        for message in item["messages"]:
            writestr(outfile, message, **kwargs)
            wrote = True
    if "valid_mod_locations" in item:
        for mod_loc in item["valid_mod_locations"]:
            writestr(outfile, mod_loc[0], **kwargs)
            wrote = True
    if not wrote:
        print("WARNING: %s: nothing translatable found in item: %r" % (infilename, item))

def extract_all_from_dir(json_dir):
    """Extract strings from every json file in the specified directory,
    recursing into any subdirectories."""
    allfiles = os.listdir(json_dir)
    allfiles.sort()
    dirs = []
    skiplist = [ ".gitkeep" ]
    for f in allfiles:
        full_name = os.path.join(json_dir, f)
        if os.path.isdir(full_name):
            dirs.append(f)
        elif f in skiplist or full_name in ignore_files:
            continue
        elif f.endswith(".json"):
            extract_all_from_file(full_name)
        elif f not in not_json:
            print("skipping file: %r" % f)
    for d in dirs:
        extract_all_from_dir(os.path.join(json_dir, d))

def extract_all_from_file(json_file):
    print("Loading %s" % json_file)
    "Extract translatable strings from every object in the specified file."
    with open(json_file) as fp:
        jsondata = json.load(fp)
    # it's either an array of objects, or a single object
    try:
        if hasattr(jsondata, "keys"):
            extract(jsondata, json_file)
        else:
            for jsonobject in jsondata:
                extract(jsonobject, json_file)
    except WrongJSONItem as E:
        print("---\nFile: {0}".format(json_file))
        print(E)
        exit(1)

def add_fake_types():
    """Add names of fake items and monsters. This is done by hand and must be updated
    manually each time something is added to itypedef.cpp or mtypedef.cpp."""
    outfile = os.path.join(to_dir, "faketypes.py")

    # fake item types

    # fake monster types
    writestr(outfile, "human", "humans")


##
##  EXTRACTION
##

extract_all_from_dir(json_dir)
extract_all_from_dir(raw_dir)
extract_all_from_dir(mods_dir)
add_fake_types()

# done.
