#!/usr/bin/env python
"Extract translatable strings from the .json files in data/json."

from __future__ import print_function

import json
import os

##
##  DATA
##

# there may be some non-json files in data/raw
not_json = {
    "sokoban.txt",
    "main.lua"
}

# these objects have no translatable strings
ignorable = {
    "colordef",
    "ITEM_BLACKLIST",
    "item_group",
    "monstergroup",
    "MONSTER_BLACKLIST",
    "MONSTER_WHITELIST",
    "monitems",
    "npc", # FIXME right now this object is unextractable
    "overmap_special",
    "recipe_category",
    "recipe_subcategory",
    "recipe",
    "region_settings",
    "BULLET_PULLING",
    "SPECIES"
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
    writestr(outfile, item["description"])
    onhit_buffs = item.get("onhit_buffs", list())
    static_buffs = item.get("static_buffs", list())
    buffs = onhit_buffs + static_buffs
    for buff in buffs:
        writestr(outfile, buff["name"])
        writestr(outfile, buff["description"])

def extract_effect_type(item):
    outfile = get_outfile("effects")
    # writestr will not write string if it is None.
    for f in ["name", "desc", "reduced_desc"]:
        for i in item.get(f, ()):
            writestr(outfile, i)
    for f in ["apply_message", "remove_message"]:
        found = item.get(f, ())
        writestr(outfile, found)
    for f in ["miss_messages", "decay_messages"]:
        for i in item.get(f, ()):
            writestr(outfile, i[0])
    for m in [ "remove_memorial_log", "apply_memorial_log"]:
        found = item.get(m, ())
        writestr(outfile, found, context="memorial_male")
        writestr(outfile, found, context="memorial_female")


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
    for f in [ "name", "description", "start_name"]:
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

# these objects need to have their strings specially extracted
extract_specials = {
    "effect_type": extract_effect_type,
    "material": extract_material,
    "martial_art": extract_martial_art,
    "profession": extract_professions,
    "scenario": extract_scenarios,
    "mapgen": extract_mapgen
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
        fs.write(string)
        fs.write("\n")

def get_outfile(json_object_type):
    return os.path.join(to_dir, json_object_type + "_from_json.py")

use_action_msgs = {
    "activate_msg",
    "deactive_msg",
    "out_of_power_msg",
    "msg",
    "friendly_msg",
    "hostile_msg",
    "need_fire_msg",
    "need_charges_msg",
    "non_interactive_msg",
    "unfold_msg",
    "activation_message"
}

def extract_use_action_msgs(outfile, use_action, kwargs):
    """Extract messages for iuse_actor objects. """
    for f in use_action_msgs:
        if f in use_action:
            writestr(outfile, use_action[f], **kwargs)

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
        print(item)
        print("ERROR: Unrecognized object type %r!" % object_type)
        exit(1)
    wrote = False
    if "name" in item:
        if "name_plural" in item:
            writestr(outfile, item["name"], item["name_plural"], **kwargs)
        else:
            if object_type in needs_plural:
                # no name_plural entry in json, use default constructed (name+"s"), as in item_factory.cpp
                writestr(outfile, item["name"], "%ss" % item["name"], **kwargs)
            else:
                writestr(outfile, item["name"], **kwargs)
        wrote = True
    if "use_action" in item:
        extract_use_action_msgs(outfile, item["use_action"], kwargs)
        wrote = True
    if "description" in item:
        writestr(outfile, item["description"], **kwargs)
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
        if os.path.isdir(os.path.join(json_dir, f)):
            dirs.append(f)
        elif f in skiplist:
            continue
        elif f.endswith(".json"):
            extract_all_from_file(os.path.join(json_dir, f))
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
    if hasattr(jsondata, "keys"):
        extract(jsondata, json_file)
    else:
        for jsonobject in jsondata:
            extract(jsonobject, json_file)

def add_fake_types():
    """Add names of fake items and monsters. This is done by hand and must be updated
    manually each time something is added to itypedef.cpp or mtypedef.cpp."""
    outfile = os.path.join(to_dir, "faketypes.py")

    # fake item types
    writestr(outfile, "corpse", "corpses")
    writestr(outfile, "nearby fire")
    writestr(outfile, "cvd machine")
    writestr(outfile, "integrated toolset")
    writestr(outfile, "a smoking device and a source of flame")
    writestr(outfile, "note", "notes")
    writestr(outfile, "misc software")
    writestr(outfile, "MediSoft")
    writestr(outfile, "infection data")
    writestr(outfile, "hackPRO")

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
