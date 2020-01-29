#!/bin/python

# png_update.py
# Rename a png and update all references to it.

import argparse
import copy
import json
import os
import string
import subprocess

# stupid stinking Python 2 versus Python 3 syntax
def write_to_json(pathname, data):
    with open(pathname, "w") as fp:
        try:
            json.dump(data, fp)
        except ValueError:
            fp.write(json.dumps(data))

    json_formatter = "./tools/format/json_formatter.cgi"
    if os.path.isfile(json_formatter):
        cmd = [json_formatter, pathname]
        subprocess.call(cmd)


def convert_index(old_index, old_name, new_name):
    changed = False
    new_indexes = []
    if isinstance(old_index, list):
        for pngname in old_index:
            if isinstance(pngname, dict):
                sprite_ids = pngname.get("sprite")
                new_sprites = []
                if isinstance(sprite_ids, list):
                    for sprite_id in sprite_ids:
                        if sprite_id == old_name:
                            new_sprites.append(new_name)
                            changed = True
                        else:
                            new_sprites.append(sprite_id)
                    if new_sprites:
                        pngname["sprite"] = new_sprites
                elif sprite_ids == old_name:
                    pngname["sprite"] = new_name
                    changed = True
                else:
                    pngname["sprite"] = sprite_ids
                new_indexes.append(pngname)
            elif pngname == old_name:
                new_indexes.append(new_name)
                changed = True
            else:
                new_indexes.append(pngname)
    elif old_index == old_name:
        new_indexes.append(new_name)
        changed = True
    else:
        new_indexes.append(old_index)

    if new_indexes and len(new_indexes) == 1:
        return new_indexes[0], changed
    return new_indexes, changed


def convert_tile_entry(tile_entry, old_name, new_name):
    if not isinstance(tile_entry, dict):
        return tile_entry, False

    changed = False
    old_fg_id = tile_entry.get("fg", [])
    if old_fg_id:
        tile_entry["fg"], fg_changed = convert_index(old_fg_id, old_name, new_name)
        changed = fg_changed

    old_bg_id = tile_entry.get("bg", [])
    if old_bg_id:
        tile_entry["bg"], bg_changed = convert_index(old_bg_id, old_name, new_name)
        changed |= bg_changed

    add_tile_entrys = tile_entry.get("additional_tiles", [])
    new_tile_entrys = []
    for add_tile_entry in add_tile_entrys:
        changed_tile_entry, add_changed = convert_tile_entry(add_tile_entry, old_name, new_name)
        new_tile_entrys.append(changed_tile_entry)
        changed |= add_changed
    if new_tile_entrys:
        tile_entry["additional_tiles"] = new_tile_entrys
    return tile_entry, changed     


def convert_tile_entry_file(file_path, old_name, new_name):
    changed = False
    tile_data = []
    new_tile_data = []
    with open(file_path, "r") as fp:
        tile_data = json.load(fp)
    if not isinstance(tile_data, list):
        tile_data = [tile_data]
    for tile_entry in tile_data:
        new_entry, entry_changed = convert_tile_entry(tile_entry, old_name, new_name)
        new_tile_data.append(new_entry)
        changed = entry_changed or changed
    if changed:
        if len(new_tile_data) == 1:
            new_tile_data = new_tile_data[0]
        write_to_json(file_path, new_tile_data)

args = argparse.ArgumentParser(description="Rename a png file, its associated tile_entry.json, and update all other tile_entry.json in the tileset dir to reflect the new name.")
args.add_argument("tileset_dir", action="store",
                  help="local name of the tileset directory under gfx/")
args.add_argument("old_name", action="store",
                  help="old name of the png file")
args.add_argument("new_name", action="store",
                  help="new name of the png file")
argsDict = vars(args.parse_args())

tileset_dirname = argsDict.get("tileset_dir", "")
tmp_old_name = argsDict.get("old_name", "")
tmp_new_name = argsDict.get("new_name", "")

old_name = tmp_old_name
new_name = tmp_new_name
if tmp_old_name.endswith(".png"):
    old_name = tmp_old_name[:-4]
if tmp_new_name.endswith(".png"):
    new_name = tmp_new_name[:-4]

old_name_json = old_name + ".json"
old_name_png = old_name + ".png"
new_name_json = new_name + ".json"
new_name_png = new_name + ".png"

if not tileset_dirname.startswith("gfx/"):
    tileset_dirname = "gfx/" + tileset_dirname
if tileset_dirname.endswith("/"):
    tileset_dirname = tileset_dirname[:-1]

print("In " + tileset_dirname + ", renaming " + old_name + " to " + new_name)
for png_dirname in os.listdir(tileset_dirname):
    if not png_dirname.startswith("pngs_"):
        continue
    png_path = tileset_dirname + "/" + png_dirname
    for subdir_fpath, dirnames, filenames in os.walk(png_path):
        for filename in filenames:
            old_path = subdir_fpath + "/" + filename
            if filename.endswith(".json"):
                convert_tile_entry_file(old_path, old_name, new_name)
            if filename == old_name_png:
                new_path = subdir_fpath + "/" + new_name_png
                os.rename(old_path, new_path)
            elif filename == old_name_json:
                new_path = subdir_fpath + "/" + new_name_json
                os.rename(old_path, new_path)
            


