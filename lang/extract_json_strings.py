#!/usr/bin/env python
"Extract translatable strings from the .json files in data/raw."

from __future__ import print_function

import json
import os

## DATA

# some .json files have no translatable strings. ignore them.
ignore = ["item_groups.json", "monstergroups.json", "recipes.json",
          "sokoban.txt", "colors.json", "species.json",
          "halloween_special.json"]

# keep a list of the files that have been extracted
extracted = []

## PREPARATION

# allow running from main directory, or from script subdirectory
if os.path.exists("data/json"):
    raw_folder = "data/raw"
    json_folder = "data/json"
    to_folder = "lang/json"
elif os.path.exists("../data/json"):
    raw_folder = "../data/raw"
    json_folder = "../data/json"
    to_folder = "../lang/json"
else:
    print("Error: Couldn't find the 'data/json' subdirectory.")
    exit(1)

# create the output directory, if it does not already exist
if not os.path.exists(to_folder):
    os.mkdir(to_folder)

# clean any old extracted strings, it will all be redone
for filename in os.listdir(to_folder):
    if not filename.endswith(".py"): continue
    f = os.path.join(to_folder, filename)
    os.remove(f)

## FUNCTIONS

def gettextify(string, context=None):
    "Put the string in a fake gettext call, and add a newline."
    if context:
        return "pgettext(%r, %r)\n" % (context, string)
    else:
        return "_(%r)\n" % string

def writestr(fs, string, context=None, format_strings=False):
    "Wrap the string and write to the file."
    # no empty strings
    if not string: return
    # none of the strings from json use string formatting.
    # we must tell xgettext this explicitly
    if not format_strings and "%" in string:
        fs.write("# xgettext:no-python-format\n")
    fs.write(gettextify(string,context=context))

def tlcomment(fs, string):
    "Write the string to the file as a comment for translators."
    fs.write("#~ ")
    fs.write(string)
    fs.write("\n")

# extract commonly translatable data from json to fake-python
def convert(infilename, outfile, **kwargs):
    "Open infilename, read data, write translatables to outfile."
    jsondata = json.loads(open(infilename).read())
    for item in jsondata:
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

def autoextract(name, **kwargs):
    "Automatically extract from the named json file in data/json."
    infilename = name + ".json"
    outfilename = os.path.join(to_folder, "json_" + name + ".py")
    with open(outfilename, 'w') as py_out:
        jsonfile = os.path.join(json_folder, infilename)
        convert(jsonfile, py_out, **kwargs)
    extracted.append(infilename)

## EXTRACTION

# automatically extractable files in data/json
autoextract("skills")
autoextract("professions")
autoextract("bionics")
autoextract("snippets")
autoextract("mutations")
autoextract("dreams")
autoextract("migo_speech")
autoextract("lab_notes")
autoextract("hints")
autoextract("furniture")
autoextract("terrain")
autoextract("monsters")
autoextract("vehicle_parts")
autoextract("vehicles")
autoextract("techniques", format_strings=True)
autoextract("tutorial")
autoextract("tool_qualities")

# data/json/items/*
with open(os.path.join(to_folder,"json_items.py"), 'w') as items_jtl:
    for filename in os.listdir(os.path.join(json_folder,"items")):
        try:
            jsonfile = os.path.join(json_folder, "items", filename)
            convert(jsonfile, items_jtl)
        except ValueError:
            print(filename)
            raise
extracted.append("items")

# data/json/materials.json
with open(os.path.join(to_folder,"json_materials.py"), 'w') as mat_jtl:
    jsonfile = os.path.join(json_folder, "materials.json")
    jsondata = json.loads(open(jsonfile).read())
    names = [item["name"] for item in jsondata]
    verb1 = [item["bash_dmg_verb"] for item in jsondata]
    verb2 = [item["cut_dmg_verb"] for item in jsondata]
    dmgs = [item["dmg_adj"] for item in jsondata]
    for n,v1,v2,d in zip(names,verb1,verb2,dmgs):
        writestr(mat_jtl, n)
        writestr(mat_jtl, v1)
        writestr(mat_jtl, v2)
        writestr(mat_jtl, d[0])
        writestr(mat_jtl, d[1])
        writestr(mat_jtl, d[2])
        writestr(mat_jtl, d[3])
extracted.append("materials.json")

# data/json/names.json
with open(os.path.join(to_folder,"json_names.py"), 'w') as name_jtl:
    jsonfile = os.path.join(json_folder, "names.json")
    jsondata = json.loads(open(jsonfile).read())
    for item in jsondata:
        if not "name" in item: continue # it probably is
        tlinfo = ["proper name"]
        context = None
        if "gender" in item:
            tlinfo.append("gender=" + item["gender"])
        if "usage" in item:
            u = item["usage"]
            tlinfo.append("usage=" + u)
            # note: these must match the context ids in name.cpp
            if u == "given": context = "Given Name"
            elif u == "family": context = "Family Name"
            elif u == "universal": context = "Either Name"
            elif u == "backer": context = "Full Name"
            elif u == "city": context = "City Name"
        # add the translator comment
        if len(tlinfo) > 1:
            tlcomment(name_jtl, '; '.join(tlinfo))
        writestr(name_jtl, item["name"], context=context)
extracted.append("names.json")

# data/raw/keybindings.json
with open(os.path.join(to_folder,"json_keybindings.py"),'w') as keys_jtl:
    jsonfile = os.path.join(raw_folder, "keybindings.json")
    convert(jsonfile, keys_jtl)
extracted.append("keybindings.json")

# data/json/martialarts.json
with open(os.path.join(to_folder,"json_martialarts.py"),'w') as martial_jtl:
    jsonfile = os.path.join(json_folder, "martialarts.json")
    jsondata = json.loads(open(jsonfile).read())
    for item in jsondata:
        writestr(martial_jtl, item["name"])
        writestr(martial_jtl, item["description"])
        onhit_buffs = item.get("onhit_buffs", list())
        static_buffs = item.get("static_buffs", list())
        buffs = onhit_buffs + static_buffs
        for buff in buffs:
            writestr(martial_jtl, buff["name"])
            writestr(martial_jtl, buff["description"])
extracted.append("martialarts.json")


## please add any new .json files to extract just above here.
## make sure you extract the right thing from the right place.

# SANITY

all_files = os.listdir(raw_folder) + os.listdir(json_folder)
not_found = []
for f in all_files:
    if not f in extracted and not f in ignore:
        not_found.append(f)

if not_found:
    if len(not_found) == 1:
        print("WARNING: Unrecognized raw file!")
        print(not_found)
        print("Does it have translatable strings in it?")
        print("Add it to lang/extract_json_strings.py then try again.")
    else:
        print("WARNING: Unrecognized raw files!")
        print(not_found)
        print("Do they have translatable strings in them?")
        print("Add them to lang/extract_json_strings.py then try again.")
    exit(1)

