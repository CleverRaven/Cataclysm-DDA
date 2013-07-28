#!/usr/bin/env python
"Extract translatable strings from the .json files in data/raw."

from __future__ import print_function

import json
import os

# allow running from main directory, or from script subdirectory
if os.path.exists("data/raw"):
    raw_folder = "data/raw"
    to_folder = "lang/json"
elif os.path.exists("../data/raw"):
    raw_folder = "../data/raw"
    to_folder = "../lang/json"
else:
    print("Error: Couldn't find the 'data/raw' subdirectory.")
    exit(1)

def gettextify(string):
    "Put the string in a fake gettext call, and add a newline."
    return "_(%r)\n" % string

# create the output directory, if it does not already exist
if not os.path.exists(to_folder):
    os.mkdir(to_folder)

# extract "name" and "description" fields from json to fake-python
def convert(infilename, outfile):
    "open infilename, read data, write names and descriptions to outfile."
    jsondata = json.loads(open(infilename).read())
    names = [item["name"] for item in jsondata]
    descriptions = [item["description"] for item in jsondata]
    for n, d in zip(names, descriptions):
        outfile.write(gettextify(n))
        outfile.write(gettextify(d))

# data/raw/items/*
with open(os.path.join(to_folder,"json_items.py"), 'w') as items_jtl:
    for filename in os.listdir(os.path.join(raw_folder,"items")):
        jsonfile = os.path.join(raw_folder, "items", filename)
        convert(jsonfile, items_jtl)

# data/raw/skills.json
with open(os.path.join(to_folder, "json_skills.py"), 'w') as skills_jtl:
    jsonfile = os.path.join(raw_folder, "skills.json")
    jsondata = json.loads(open(jsonfile).read())
    names = [item[1] for item in jsondata]
    descriptions = [item[2] for item in jsondata]
    for n, d in zip(names, descriptions):
        skills_jtl.write(gettextify(n))
        skills_jtl.write(gettextify(d))

# data/raw/professions.json
with open(os.path.join(to_folder,"json_professions.py"), 'w') as prof_jtl:
    prof_json = os.path.join(raw_folder, "professions.json")
    convert(prof_json, prof_jtl)

# data/raw/bionics.json
with open(os.path.join(to_folder,"json_bionics.py"), 'w') as bio_jtl:
    bio_json = os.path.join(raw_folder, "bionics.json")
    convert(bio_json, bio_jtl)

# data/raw/snippets.json
with open(os.path.join(to_folder,"json_snippets.py"), 'w') as snip_jtl:
    jsonfile = os.path.join(raw_folder, "snippets.json")
    jsondata = json.loads(open(jsonfile).read())
    snip = jsondata["snippets"]
    texts = [item["text"] for item in snip]
    for t in zip(texts):
        snip_jtl.write(gettextify(t))

# data/raw/materials.json
with open(os.path.join(to_folder,"json_materials.py"), 'w') as mat_jtl:
    jsonfile = os.path.join(raw_folder, "materials.json")
    jsondata = json.loads(open(jsonfile).read())
    names = [item["name"] for item in jsondata]
    verb1 = [item["bash_dmg_verb"] for item in jsondata]
    verb2 = [item["cut_dmg_verb"] for item in jsondata]
    dmgs = [item["dmg_adj"] for item in jsondata]
    for n,v1,v2,d in zip(names,verb1,verb2,dmgs):
        mat_jtl.write(gettextify(n))
        mat_jtl.write(gettextify(v1))
        mat_jtl.write(gettextify(v2))
        mat_jtl.write(gettextify(d[0]))
        mat_jtl.write(gettextify(d[1]))
        mat_jtl.write(gettextify(d[2]))
        mat_jtl.write(gettextify(d[3]))

# data/raw/names.json
with open(os.path.join(to_folder,"json_names.py"), 'w') as name_jtl:
    jsonfile = os.path.join(raw_folder, "names.json")
    jsondata = json.loads(open(jsonfile).read())
    names = [item["name"] for item in jsondata]
    for n in zip(names):
        name_jtl.write(gettextify('<name>' + n[0]))
