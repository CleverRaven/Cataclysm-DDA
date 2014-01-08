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
}

# these objects have no translatable strings
ignorable = {
    "colordef",
    "item_group",
    "mapgen",
    "monstergroup",
    "monitems",
    "recipe_category",
    "recipe_subcategory",
    "recipe",
    "region_settings",
    "SPECIES"
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
    "AMMO",
    "ARMOR",
    "bionic",
    "BOOK",
    "COMESTIBLE",
    "construction",
    "CONTAINER",
    "dream",
    "furniture",
    "GENERIC",
    "GUNMOD",
    "GUN",
    "hint",
    "ITEM_CATEGORY",
    "keybinding",
    "lab_note",
    "MONSTER",
    "mutation",
    "overmap_terrain",
    "skill",
    "snippet",
    "speech",
    "terrain",
    "tool_quality",
    "TOOL",
    "trap",
    "tutorial_messages",
    "vehicle_part",
    "vehicle",
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
    for f in [ "name", "desc", "apply_message"]:
        found = item.get(f, None)
        writestr(outfile, found)
    for m in [ "remove_memorial_log", "apply_memorial_log"]:
        found = item.get(m, None)
        writestr(outfile, found, comment="Memorial file message")

def extract_professions(item):
    outfile = get_outfile("professions")
    nm = item["name"]
    if type(nm) == dict:
        writestr(outfile, nm["male"], comment="Male profession name")
        writestr(outfile, nm["female"], comment="Female profession name")
        writestr(outfile, item["description"],
         comment="Profession ({0}/{1}) description".format(nm["male"], nm["female"]))
    else:
        writestr(outfile, nm, comment="Profession name")
        writestr(outfile, item["description"],
         comment="Profession ({0}) description".format(nm))

# these objects need to have their strings specially extracted
extract_specials = {
    "effect_type": extract_effect_type,
    "material": extract_material,
    "martial_art": extract_martial_art,
    "profession": extract_professions
}

##
##  PREPARATION
##

# allow running from main directory, or from script subdirectory
if os.path.exists("data/json"):
    raw_dir = "data/raw"
    json_dir = "data/json"
    to_dir = "lang/json"
elif os.path.exists("../data/json"):
    raw_dir = "../data/raw"
    json_dir = "../data/json"
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

def gettextify(string, context=None):
    "Put the string in a fake gettext call, and add a newline."
    if context:
        return "pgettext(%r, %r)\n" % (context, string)
    else:
        return "_(%r)\n" % string

def writestr(filename, string, context=None, format_strings=False, comment=None):
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
        fs.write(gettextify(string,context=context))

def tlcomment(fs, string):
    "Write the string to the file as a comment for translators."
    if len(string) > 0:
        fs.write("#~ ")
        fs.write(string)
        fs.write("\n")

def get_outfile(json_object_type):
    return os.path.join(to_dir, json_object_type + "_from_json.py")

# extract commonly translatable data from json to fake-python
def extract(item):
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
        writestr(outfile, item["name"], **kwargs)
        wrote = True
    if "description" in item:
        writestr(outfile, item["description"], **kwargs)
        wrote = True
    if "sound" in item:
        writestr(outfile, item["sound"], **kwargs)
        wrote = True
    if "text" in item:
        writestr(outfile, item["text"], **kwargs)
        wrote = True
    if "messages" in item:
        for message in item["messages"]:
            writestr(outfile, message, **kwargs)
            wrote = True
    if not wrote:
        print("WARNING: %s: nothing translatable found in item: %r" % (infilename, item))

def extract_all_from_dir(json_dir):
    """Extract strings from every json file in the specified directory,
    recursing into any subdirectories."""
    allfiles = os.listdir(json_dir)
    allfiles.sort()
    dirs = []
    for f in allfiles:
        if os.path.isdir(os.path.join(json_dir, f)):
            dirs.append(f)
        elif f.endswith(".json"):
            extract_all_from_file(os.path.join(json_dir, f))
        elif f not in not_json:
            print("skipping file: %r" % f)
    for d in dirs:
        extract_all_from_dir(os.path.join(json_dir, d))

def extract_all_from_file(json_file):
    "Extract translatable strings from every object in the specified file."
    jsondata = json.loads(open(json_file).read())
    # it's either an array of objects, or a single object
    if hasattr(jsondata, "keys"):
        extract(jsondata)
    else:
        for jsonobject in jsondata:
            extract(jsonobject)

##
##  EXTRACTION
##

extract_all_from_dir(json_dir)
extract_all_from_dir(raw_dir)

# done.
