#!/usr/bin/env python
"Extract translatable strings from the .json files in data/raw."

from __future__ import print_function

import json
import os

## DATA

# some .json files have no translatable strings. ignore them.
ignore = ["item_groups.json", "monstergroups.json", "recipes.json", "sokoban.txt"]

## PREPARATION

# allow running from main directory, or from script subdirectory
if os.path.exists("data/raw"):
    raw_folder = "data/raw"
    json_folder = "data/json"
    to_folder = "lang/json"
elif os.path.exists("../data/raw"):
    raw_folder = "../data/raw"
    json_folder = "../data/json"
    to_folder = "../lang/json"
else:
    print("Error: Couldn't find the 'data/raw' subdirectory.")
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

def gettextify(string):
    "Put the string in a fake gettext call, and add a newline."
    return "_(%r)\n" % string

def writestr(fs, string):
    "Wrap the string and write to the file."
    # no empty strings
    if not string: return
    # none of the strings from json use string formatting.
    # we must tell xgettext this explicitly
    if "%" in string: fs.write("# xgettext:no-python-format\n")
    fs.write(gettextify(string))

def tlcomment(fs, string):
    "Write the string to the file as a comment for translators."
    fs.write("#~ ")
    fs.write(string)
    fs.write("\n")

# extract "name" and "description" fields from json to fake-python
def convert(infilename, outfile):
    "open infilename, read data, write names and descriptions to outfile."
    jsondata = json.loads(open(infilename).read())
    names = [item["name"] for item in jsondata]
    descriptions = [item["description"] for item in jsondata]
    for n, d in zip(names, descriptions):
        writestr(outfile, n)
        writestr(outfile, d)

## EXTRACTION

extracted = []

# data/raw/items/*
with open(os.path.join(to_folder,"json_items.py"), 'w') as items_jtl:
    for filename in os.listdir(os.path.join(raw_folder,"items")):
        jsonfile = os.path.join(raw_folder, "items", filename)
        convert(jsonfile, items_jtl)
extracted.append("items")

# data/json/skills.json
with open(os.path.join(to_folder, "json_skills.py"), 'w') as skills_jtl:
    jsonfile = os.path.join(json_folder, "skills.json")
    convert(jsonfile, skills_jtl)
extracted.append("skills.json")

# data/json/professions.json
with open(os.path.join(to_folder,"json_professions.py"), 'w') as prof_jtl:
    prof_json = os.path.join(json_folder, "professions.json")
    convert(prof_json, prof_jtl)
extracted.append("professions.json")

# data/json/bionics.json
with open(os.path.join(to_folder,"json_bionics.py"), 'w') as bio_jtl:
    bio_json = os.path.join(json_folder, "bionics.json")
    convert(bio_json, bio_jtl)
extracted.append("bionics.json")

# data/json/snippets.json
with open(os.path.join(to_folder,"json_snippets.py"), 'w') as snip_jtl:
    jsonfile = os.path.join(json_folder, "snippets.json")
    jsondata = json.loads(open(jsonfile).read())
    for item in jsondata:
        writestr(snip_jtl, item["text"])
extracted.append("snippets.json")

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

# data/raw/names.json
with open(os.path.join(to_folder,"json_names.py"), 'w') as name_jtl:
    jsonfile = os.path.join(raw_folder, "names.json")
    jsondata = json.loads(open(jsonfile).read())
    for item in jsondata:
        if not "name" in item: continue # it probably is
        tlinfo = ["proper name"]
        if "gender" in item:
            tlinfo.append("gender=" + item["gender"])
        if "usage" in item:
            tlinfo.append("usage=" + item["usage"])
        if len(tlinfo) > 1: # then add it as a translator comment
            tlcomment(name_jtl, '; '.join(tlinfo))
        writestr(name_jtl, "<name>" + item["name"])
extracted.append("names.json")

# data/json/mutations.json
with open(os.path.join(to_folder,"json_mutations.py"), 'w') as mut_jtl:
    jsonfile = os.path.join(json_folder, "mutations.json")
    jsondata = json.loads(open(jsonfile).read())
    for item in jsondata:
        writestr(mut_jtl, item["name"])
        writestr(mut_jtl, item["description"])
extracted.append("mutations.json")

# data/json/dreams.json
with open(os.path.join(to_folder,"json_dreams.py"), 'w') as dream_jtl:
    jsonfile = os.path.join(json_folder, "dreams.json")
    jsondata = json.loads(open(jsonfile).read())
    for item in jsondata:
        for message in item["messages"]:
            writestr(dream_jtl, message)
extracted.append("dreams.json")

# data/raw/vehicle_parts.json
with open(os.path.join(to_folder,"json_vehicle_parts.py"),'w') as veh_jtl:
    jsonfile = os.path.join(raw_folder, "vehicle_parts.json")
    jsondata = json.loads(open(jsonfile).read())
    for item in jsondata:
        writestr(veh_jtl, item["name"])
extracted.append("vehicle_parts.json")

# data/raw/parrot.json
with open(os.path.join(to_folder,"json_parrot.py"),'w') as parrot_jtl:
    jsonfile = os.path.join(raw_folder, "parrot.json")
    jsondata = json.loads(open(jsonfile).read())
    for item in jsondata:
        writestr(parrot_jtl, item["sound"])
extracted.append("parrot.json")

# data/raw/vehicles.json
with open(os.path.join(to_folder,"json_vehicles.py"),'w') as veh_jtl:
    jsonfile = os.path.join(raw_folder, "vehicles.json")
    jsondata = json.loads(open(jsonfile).read())
    for item in jsondata:
        writestr(veh_jtl, item["name"])
extracted.append("vehicles.json")

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

