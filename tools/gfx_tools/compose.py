#!/bin/python

# compose.py
# Split a gfx directory made of 1000s of little images and files into a set of tilesheets
# and a tile_config.json

import argparse
import copy
import json
import os
import string
import subprocess

try:
    import pyvips
    Vips = pyvips
except ImportError:
    import gi
    gi.require_version('Vips', '8.0')
    from gi.repository import Vips

FALLBACK = {
    "file": "fallback.png",
    "tiles": [],
    "ascii": [
        { "offset": 0, "bold": False, "color": "BLACK" },
        { "offset": 256, "bold": True, "color": "WHITE" },
        { "offset": 512, "bold": False, "color": "WHITE" },
        { "offset": 768, "bold": True, "color": "BLACK" },
        { "offset": 1024, "bold": False, "color": "RED" },
        { "offset": 1280, "bold": False, "color": "GREEN" },
        { "offset": 1536, "bold": False, "color": "BLUE" },
        { "offset": 1792, "bold": False, "color": "CYAN" },
        { "offset": 2048, "bold": False, "color": "MAGENTA" },
        { "offset": 2304, "bold": False, "color": "YELLOW" },
        { "offset": 2560, "bold": True, "color": "RED" },
        { "offset": 2816, "bold": True, "color": "GREEN" },
        { "offset": 3072, "bold": True, "color": "BLUE" },
        { "offset": 3328, "bold": True, "color": "CYAN" },
        { "offset": 3584, "bold": True, "color": "MAGENTA" },
        { "offset": 3840, "bold": True, "color": "YELLOW" }
    ]
}

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


def find_or_make_dir(pathname):
    try:
        os.stat(pathname)
    except OSError:
        os.mkdir(pathname)


class PngRefs(object):
    def __init__(self, tileset_dirname):
        # dict of pngnames to png numbers; used to control uniqueness
        self.pngname_to_pngnum = { "null_image": 0 }
        # dict of png absolute numbers to png names
        self.pngnum_to_pngname = { 0: "null_image" }
        self.pngnum = 0
        self.referenced_pngnames = []
        self.tileset_pathname = tileset_dirname
        if not tileset_dirname.startswith("gfx/"):
            self.tileset_pathname = "gfx/" + tileset_dirname
        if self.tileset_pathname.endswith("/"):
            self.tileset_pathname = self.tileset_pathname[:-1]

        try:
            os.stat(self.tileset_pathname)
        except KeyError:
            print("cannot find a directory {}".format(self.tileset_pathname))
            exit -1

        self.processed_ids = []
        tileset_info_path = self.tileset_pathname + "/tile_info.json"
        self.tileset_width = 16
        self.tileset_height = 16
        self.tileset_info = [{}]
        with open(tileset_info_path, "r") as fp:
            self.tileset_info = json.load(fp)
            self.tileset_width = self.tileset_info[0].get("width")
            self.tileset_height = self.tileset_info[0].get("height")

    def convert_a_pngname_to_pngnum(self, sprite_id, entry):
        if sprite_id and sprite_id != "no_entry":
            new_id = self.pngname_to_pngnum.get(sprite_id, 0)
            if new_id:
                entry.append(new_id)
                if sprite_id not in self.referenced_pngnames:
                    self.referenced_pngnames.append(sprite_id)
                return True
            else:
                print("sprite id '{}' has no matching PNG file.  ".format(sprite_id) +
                      "It will not be added to tile_config.json")
        return False

    def convert_pngname_to_pngnum(self, index):
        new_index = []
        if isinstance(index, list):
            for pngname in index:
                if isinstance(pngname, dict):
                    sprite_ids = pngname.get("sprite")
                    valid = False
                    new_sprites = []
                    if isinstance(sprite_ids, list):
                        new_sprites = []
                        for sprite_id in sprite_ids:
                            valid |= self.convert_a_pngname_to_pngnum(sprite_id, new_sprites)
                        pngname["sprite"] = new_sprites
                    else:
                        valid = self.convert_a_pngname_to_pngnum(sprite_ids, new_sprites)
                        if valid:
                            pngname["sprite"] = new_sprites[0]
                    if valid:
                        new_index.append(pngname)
                else:
                    self.convert_a_pngname_to_pngnum(pngname, new_index)
        else:
            self.convert_a_pngname_to_pngnum(index, new_index)
        if new_index and len(new_index) == 1:
            return new_index[0]
        return new_index

    def convert_tile_entry(self, tile_entry, prefix, is_filler):
        tile_id = tile_entry.get("id")
        id_as_prefix = None
        if tile_id:
            if not isinstance(tile_id, list):
                tile_id = [tile_id]
            id_as_prefix = tile_id[0] + "_"

        if is_filler:
            for an_id in tile_id:
                full_id = prefix + an_id
                if full_id in self.processed_ids:
                    print("skipping filler for {}".format(full_id))
                    return None
        fg_id = tile_entry.get("fg")
        if fg_id:
            tile_entry["fg"] = self.convert_pngname_to_pngnum(fg_id)
        else:
            del tile_entry["fg"]

        bg_id = tile_entry.get("bg")
        if bg_id:
            tile_entry["bg"] = self.convert_pngname_to_pngnum(bg_id)
        else:
            try:
                del tile_entry["bg"]
            except:
                print("Cannot find bg for tile with id {}".format(tile_id))

        add_tile_entrys = tile_entry.get("additional_tiles", [])
        for add_tile_entry in add_tile_entrys:
            self.convert_tile_entry(add_tile_entry, id_as_prefix, is_filler)

        if fg_id or bg_id:
            for an_id in tile_id:
                full_id = prefix + an_id
                if full_id not in self.processed_ids:
                    self.processed_ids.append(full_id)
            return tile_entry
        return None

    def verify(self):
        for pngname, pngnum in self.pngname_to_pngnum.items():
            if pngnum and pngname not in self.referenced_pngnames:
                print("image filename '{}' index '{}'".format(pngname, pngnum) +
                      " was not used in any tile_config.json entries")


class TilesheetData(object):
    def __init__(self, subdir_index, refs):
        ts_all = refs.tileset_info[subdir_index]
        self.ts_specs = {}
        for ts_name, ts_spec in ts_all.items():
            self.ts_specs = ts_spec
            self.ts_name = ts_name
            break
        self.ts_path = refs.tileset_pathname + "/" + self.ts_name
        self.tile_entries = []
        self.row_num = 0
        self.width = self.ts_specs.get("sprite_width", refs.tileset_width)
        self.height = self.ts_specs.get("sprite_height", refs.tileset_height)
        self.offset_x = 0
        self.offset_y = 0
        subdir_name = self.ts_name.split(".png")[0] + "_{}x{}".format(self.width, self.height)
        self.subdir_path = refs.tileset_pathname + "/pngs_" + subdir_name
        if not self.standard(refs):
            self.offset_x = self.ts_specs.get("sprite_offset_x", 0)
            self.offset_y = self.ts_specs.get("sprite_offset_y", 0)
        self.null_image = Vips.Image.grey(self.width, self.height)
        self.row_pngs = ["null_image"]
        self.filler = False
        self.fallback = False;
        if self.ts_specs.get("fallback"):
            self.fallback = True
            return
        if self.ts_specs.get("filler"):
            self.filler = True
            return

    def set_first_index(self, refs):
        refs.pngnum += 1
        self.first_index = refs.pngnum
        self.max_index = refs.pngnum

    def standard(self, refs):
        if self.offset_x or self.offset_y:
            return False
        return self.width == refs.tileset_width and self.height == refs.tileset_height

    def merge_row(self, refs):
        spacer = 16 - len(self.row_pngs)
        refs.pngnum += spacer

        in_list = []
        
        for png_pathname in self.row_pngs:
            if png_pathname == "null_image":
                in_list.append(self.null_image)
            else:
                vips_image = Vips.Image.pngload(png_pathname)
                try:
                    if not vips_image.hasalpha():
                        vips_image = vips_image.addalpha()
                except Vips.Error:
                    pass

                if vips_image.width != self.width or vips_image.height != self.height:
                    size_msg = "{} is {}x{}, sheet sprites are {}x{}."
                    print(size_msg.format(png_pathname, vips_image.width, vips_image.height,
                                          self.width, self.height))
                    print("\tsprites in the {} tilesheet may be resized.".format(self.ts_name))
                    print("\tAll sprites in a tilesheet directory should have the same dimensions.")
                in_list.append(vips_image)
        for i in range(0, spacer):
            in_list.append(self.null_image)

        return in_list

    def walk_dirs(self, refs):
        tmp_merged_pngs = []
        for subdir_fpath, dirnames, filenames in os.walk(self.subdir_path):
            #print("{} has dirs {} and files {}".format(subdir_fpath, dirnames, filenames))
            for filename in filenames:
                filepath = subdir_fpath + "/" + filename
                if filename.endswith(".png"):
                    pngname = filename.split(".png")[0]
                    if pngname in refs.pngname_to_pngnum or pngname == "no_entry":
                        print("skipping {}".format(pngname))
                        continue
                    if self.filler and pngname in refs.pngname_to_pngnum:
                        continue
                    self.row_pngs.append(filepath)
                    refs.pngname_to_pngnum[pngname] = refs.pngnum
                    refs.pngnum_to_pngname[refs.pngnum] = pngname
                    refs.pngnum += 1
                    if len(self.row_pngs) > 15:
                        merged = self.merge_row(refs)
                        self.row_num += 1
                        self.row_pngs = []
                        tmp_merged_pngs += merged
                elif filename.endswith(".json"):
                    with open(filepath, "r") as fp:
                        try:
                            tile_entry = json.load(fp)
                        except:
                            print("error loading {}".format(filepath))
                            raise

                        if not isinstance(tile_entry, list):
                            tile_entry = [tile_entry]
                        self.tile_entries += tile_entry
        if self.row_pngs:
            merged = self.merge_row(refs)
            tmp_merged_pngs += merged
        return tmp_merged_pngs

    def finalize_merges(self, merge_pngs):
        out_image = Vips.Image.arrayjoin(merge_pngs, across=16)
        out_image.pngsave(self.ts_path)

args = argparse.ArgumentParser(description="Merge all the individal tile_entries and pngs in a tileset's directory into a tile_config.json and 1 or more tilesheet pngs.")
args.add_argument("tileset_dir", action="store",
                  help="local name of the tileset directory under gfx/")
argsDict = vars(args.parse_args())

tileset_dirname = argsDict.get("tileset_dir", "")

refs = PngRefs(tileset_dirname)

all_ts_data = []
fallback_name = "fallback.png"

for subdir_index in range(1, len(refs.tileset_info)):
    ts_data = TilesheetData(subdir_index, refs)
    if not ts_data.filler and not ts_data.fallback:
        ts_data.set_first_index(refs)
        print("parsing tilesheet {}".format(ts_data.ts_name))
        tmp_merged_pngs = ts_data.walk_dirs(refs)

        ts_data.finalize_merges(tmp_merged_pngs)

        ts_data.max_index = refs.pngnum
        all_ts_data.append(ts_data)

for subdir_index in range(1, len(refs.tileset_info)):
    ts_data = TilesheetData(subdir_index, refs)
    if ts_data.filler:
        ts_data.set_first_index(refs)
        print("parsing filler tilesheet {}".format(ts_data.ts_name))
        ts_data.first_index = refs.pngnum
        tmp_merged_pngs = ts_data.walk_dirs(refs)

        ts_data.finalize_merges(tmp_merged_pngs)

        ts_data.max_index = refs.pngnum
        all_ts_data.append(ts_data)

for subdir_index in range(1, len(refs.tileset_info)):
    ts_data = TilesheetData(subdir_index, refs)
    if ts_data.fallback:
        ts_data.set_first_index(refs)
        print("parsing fallback tilesheet {}".format(ts_data.ts_name))
        all_ts_data.append(ts_data)

#print("pngname to pngnum {}".format(json.dumps(refs.pngname_to_pngnum, indent=2)))
#print("pngnum to pngname {}".format(json.dumps(refs.pngnum_to_pngname, sort_keys=True, indent=2)))

tiles_new = []
    
for ts_data in all_ts_data:
    if ts_data.fallback:
        fallback_name = ts_data.ts_name
        continue
    ts_tile_entries = []
    for tile_entry in ts_data.tile_entries:
        converted_tile_entry = refs.convert_tile_entry(tile_entry, "", ts_data.filler)
        if converted_tile_entry:
            ts_tile_entries.append(converted_tile_entry)
    ts_conf = {
        "file": ts_data.ts_name,
        "tiles": ts_tile_entries,
        "//": "range {} to {}".format(ts_data.first_index, ts_data.max_index)
    }
    if not ts_data.standard(refs):
        ts_conf["sprite_width"] = ts_data.width
        ts_conf["sprite_height"] = ts_data.height
        ts_conf["sprite_offset_x"] = ts_data.offset_x
        ts_conf["sprite_offset_y"] = ts_data.offset_y

    #print("\tfinalizing tilesheet {}".format(ts_name))
    tiles_new.append(ts_conf)

FALLBACK["file"] = fallback_name
tiles_new.append(FALLBACK)
conf_data = {
    "tile_info": [{
        "width": refs.tileset_width,
        "height": refs.tileset_height
    }],
    "tiles-new": tiles_new
}
tileset_confpath = refs.tileset_pathname + "/" + "tile_config.json"
write_to_json(tileset_confpath, conf_data)

refs.verify()
