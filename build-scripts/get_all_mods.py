#!/usr/bin/env python3

import sys
import glob
import json

blacklist_filename, = sys.argv[1:]
with open(blacklist_filename) as blacklist_file:
    blacklist = {s.rstrip('\n') for s in blacklist_file.readlines()}

mods = []

for info in glob.glob('data/mods/*/modinfo.json'):
    mod_info = json.load(open(info))
    mods.extend(e["ident"] for e in mod_info if e["type"] == "MOD_INFO")

mods_to_keep = [mod for mod in mods if mod not in blacklist]

print(','.join(mods_to_keep))
