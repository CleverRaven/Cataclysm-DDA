#!/bin/python

# decompose.py
# Split a gfx tile_config.json into 1000s of little directories, each with their own config
# file and tile image.

import argparse
import copy
import json
import math
import os
import subprocess

try:
    import pyvips
    Vips = pyvips
except ImportError:
    import gi
    gi.require_version('Vips', '8.0')
    from gi.repository import Vips

# stupid stinking Python 2 versus Python 3 syntax
def write_to_json(pathname, data, prettify=False):
    with open(pathname, "w") as fp:
        try:
            json.dump(data, fp)
        except ValueError:
            fp.write(json.dumps(data))

    json_formatter = "./tools/format/json_formatter.cgi"
    if prettify and os.path.isfile(json_formatter):
        cmd = [json_formatter, pathname]
        subprocess.call(cmd)


def find_or_make_dir(pathname):
    try:
        os.stat(pathname)
    except OSError:
        os.mkdir(pathname)


class TileSheetData(object):
    def __init__(self, tilesheet_data, refs):
        self.ts_filename = tilesheet_data.get("file", "")
        #print("working on {}".format(self.ts_filename))
        self.tile_id_to_tile_entrys = {}
        self.sprite_height = tilesheet_data.get("sprite_height", refs.default_height)
        self.sprite_width = tilesheet_data.get("sprite_width", refs.default_width)
        self.sprite_offset_x = tilesheet_data.get("sprite_offset_x", 0)
        self.sprite_offset_y = tilesheet_data.get("sprite_offset_y", 0)
        self.write_dim = self.sprite_width != refs.default_width
        self.write_dim |= self.sprite_height != refs.default_height
        self.write_dim |= self.sprite_offset_x or self.sprite_offset_y
        self.ts_pathname = refs.tileset_pathname + "/" + self.ts_filename
        self.ts_image = Vips.Image.pngload(self.ts_pathname)
        self.ts_width = self.ts_image.width
        self.ts_tiles_per_row = int(math.floor(self.ts_width / self.sprite_width))
        self.ts_height = self.ts_image.height
        self.ts_rows = int(math.floor(self.ts_height / self.sprite_height))
        self.pngnum_min = refs.last_pngnum
        self.pngnum_max = refs.last_pngnum + self.ts_tiles_per_row * self.ts_rows - 1
        #print("\t{}x{}; {} rows; spans {} to {}".format(self.ts_width, self.ts_height,
        #                                                self.ts_rows, self.pngnum_min,
        #                                                self.pngnum_max))
        self.expansions = []
        self.fallback = tilesheet_data.get("ascii")
        refs.last_pngnum = self.pngnum_max + 1
        refs.tspathname_to_tsfilename.setdefault(self.ts_pathname, self.ts_filename)

    def check_for_expansion(self, tile_entry):
        if tile_entry.get("fg", -10) == 0:
            self.expansions.append(tile_entry)
            return True
        return False

    def check_id_valid(self, tile_id):
        if not tile_id:
            return True
        # wielded terrain isn't valid
        if tile_id.startswith("overlay_wielded_t_"):
            return False
        if tile_id.startswith("overlay_wielded_mon_"):
            return False
        if tile_id.startswith("overlay_wielded_fd_"):
            return False
        if tile_id.startswith("overlay_wielded_f_"):
            return False
        # wielding or wearing overlays makes no sense
        if tile_id.startswith("overlay_wielded_overlay"):
            return False
        if tile_id.startswith("overlay_worn_overlay"):
            return False
        return True

    def parse_id(self, tile_entry):
        all_tile_ids = []
        read_tile_ids = tile_entry.get("id")
        valid = True
        if isinstance(read_tile_ids, list):
            for tile_id in read_tile_ids:
                valid &= self.check_id_valid(tile_id)
                if tile_id and valid and tile_id not in all_tile_ids:
                    all_tile_ids.append(tile_id)
        else:
            valid &= self.check_id_valid(read_tile_ids)
            if read_tile_ids and valid and read_tile_ids not in all_tile_ids:
                all_tile_ids.append(read_tile_ids)
        if not valid:
            return None, None
        if not all_tile_ids:
            return "background", ["background"]
        #print("tile {}".format(all_tile_ids[0]))
        return all_tile_ids[0], all_tile_ids

    def parse_index(self, read_pngnums, all_pngnums, refs):
        local_pngnums = []
        if isinstance(read_pngnums, list):
            for pngnum in read_pngnums:
                if isinstance(pngnum, dict):
                    sprite_ids = pngnum.get("sprite", -10)
                    if isinstance(sprite_ids, list):
                        for sprite_id in sprite_ids:
                            if sprite_id < 0 or sprite_id in refs.delete_pngnums:
                                continue
                            if sprite_id not in all_pngnums:
                                all_pngnums.append(sprite_id)
                            if sprite_id not in local_pngnums:
                                local_pngnums.append(sprite_id)
                    else:
                        if sprite_ids < 0 or sprite_ids in refs.delete_pngnums:
                            continue
                        if sprite_ids not in all_pngnums:
                            all_pngnums.append(sprite_ids)
                        if sprite_ids not in local_pngnums:
                            local_pngnums.append(sprite_ids)
                else:
                    if pngnum < 0 or pngnum in refs.delete_pngnums:
                        continue
                    if pngnum not in all_pngnums:
                        all_pngnums.append(pngnum)
                    if pngnum not in local_pngnums:
                        local_pngnums.append(pngnum)
        elif read_pngnums >= 0 and read_pngnums not in refs.delete_pngnums:
            if read_pngnums not in all_pngnums:
                all_pngnums.append(read_pngnums)
            if read_pngnums not in local_pngnums:
                local_pngnums.append(read_pngnums)
        return all_pngnums

    def parse_png(self, tile_entry, refs):
        all_pngnums = []
        fg_id = tile_entry.get("fg", -10)
        all_pngnums = self.parse_index(fg_id, all_pngnums, refs)

        bg_id = tile_entry.get("bg", -10)
        all_pngnums = self.parse_index(bg_id, all_pngnums, refs)

        add_tile_entrys = tile_entry.get("additional_tiles", [])
        for add_tile_entry in add_tile_entrys:
            add_pngnums = self.parse_png(add_tile_entry, refs)
            for add_pngnum in add_pngnums:
                if add_pngnum not in all_pngnums:
                    all_pngnums.append(add_pngnum)
        # print("\tpngs: {}".format(all_pngnums))
        return all_pngnums

    def parse_tile_entry(self, tile_entry, refs):
        if self.check_for_expansion(tile_entry):
            return None
        tile_id, all_tile_ids = self.parse_id(tile_entry)
        if not tile_id:
            # print("no id for {}".format(tile_entry))
            return None
        tile_id = tile_id.replace("/", "_")
        all_pngnums = self.parse_png(tile_entry, refs)
        offset = 0
        for i in range(0, len(all_pngnums)):
            pngnum = all_pngnums[i]
            if pngnum in refs.pngnum_to_pngname:
                continue
            pngname = "{}_{}_{}".format(pngnum, tile_id, i + offset)
            while pngname in refs.pngname_to_pngnum:
                offset += 1
                pngname = "{}_{}_{}".format(pngnum, tile_id, i + offset)
            try:
                refs.pngnum_to_pngname.setdefault(pngnum, pngname)
                refs.pngname_to_pngnum.setdefault(pngname, pngnum)
                refs.add_pngnum_to_tsfilepath(pngnum)
            except TypeError:
                print("failed to parse {}".format(json.dumps(tile_entry, indent=2)))
                raise
        return tile_id

    def summarize(self, tile_info, refs):
        #debug statement to verify pngnum_min and pngnum_max
        #print("{} from {} to {}".format(self.ts_filename, self.pngnum_min, self.pngnum_max))
        if self.fallback:
            refs.ts_data[self.ts_filename] = self
            ts_tile_info = {
                "fallback": True
            }
            tile_info.append({self.ts_filename: ts_tile_info})
            return
        if self.pngnum_max > 0:
            refs.ts_data[self.ts_filename] = self
            ts_tile_info = {
                "//": "indices {} to {}".format(self.pngnum_min, self.pngnum_max)
            }
            if self.write_dim:
                ts_tile_info["sprite_offset_x"] = self.sprite_offset_x
                ts_tile_info["sprite_offset_y"] = self.sprite_offset_y
                ts_tile_info["sprite_width"] = self.sprite_width
                ts_tile_info["sprite_height"] = self.sprite_height
            #print("{}: {}".format(self.ts_filename, json.dumps(ts_tile_info, indent=2)))
            tile_info.append({self.ts_filename: ts_tile_info})

class ExtractionData(object):
    def __init__(self, ts_filename, refs):
        self.ts_data = refs.ts_data.get(ts_filename)
        self.valid = False
        if not self.ts_data.sprite_width or not self.ts_data.sprite_height:
            return

        self.valid = True

        ts_base = ts_filename.split(".png")[0]
        geometry_dim = "{}x{}".format(self.ts_data.sprite_width, self.ts_data.sprite_height)
        pngs_dir = "/pngs_" +  ts_base + "_{}".format(geometry_dim)
        self.ts_dir_pathname = refs.tileset_pathname + pngs_dir
        find_or_make_dir(self.ts_dir_pathname)
        self.tilenum_in_dir = 256
        self.dir_count = 0
        self.subdir_pathname = ""

    def write_expansions(self):
        for expand_entry in self.ts_data.expansions:
            expansion_id = expand_entry.get("id", "expansion")
            if not isinstance(expansion_id, str):
                continue
            expand_entry_pathname = self.ts_dir_pathname + "/" + expansion_id + ".json"
            write_to_json(expand_entry_pathname, expand_entry)

    def increment_dir(self):
        if self.tilenum_in_dir > 255:
            self.subdir_pathname = self.ts_dir_pathname + "/" + "images{}".format(self.dir_count)
            find_or_make_dir(self.subdir_pathname)
            self.tilenum_in_dir = 0
            self.dir_count += 1
        else:
            self.tilenum_in_dir += 1
        return self.subdir_pathname

    def extract_image(self, png_index, refs):
        if not png_index or refs.extracted_pngnums.get(png_index):
            return
        if png_index not in refs.pngnum_to_pngname:
            return
        pngname = refs.pngnum_to_pngname[png_index]
        ts_pathname = refs.pngnum_to_tspathname[png_index]
        ts_filename = refs.tspathname_to_tsfilename[ts_pathname]
        self.increment_dir()
        tile_data = refs.ts_data[ts_filename]
        file_index = png_index - tile_data.pngnum_min
        y_index = math.floor( file_index / tile_data.ts_tiles_per_row )
        x_index = file_index - y_index * tile_data.ts_tiles_per_row
        tile_off_x = max(0, tile_data.sprite_width * x_index)
        tile_off_y = max(0, tile_data.sprite_height * y_index)
        tile_image = tile_data.ts_image.extract_area(tile_off_x, tile_off_y,
                                                     tile_data.sprite_width,
                                                     tile_data.sprite_height)
        tile_png_pathname = self.subdir_pathname + "/" + pngname + ".png"
        tile_image.pngsave(tile_png_pathname)
        refs.extracted_pngnums[png_index] = True

    def write_images(self, refs):
        for pngnum in range(self.ts_data.pngnum_min, self.ts_data.pngnum_max + 1):
            out_data.extract_image(pngnum, refs)


class PngRefs(object):
    def __init__(self):
        # dict of png absolute numbers to png names
        self.pngnum_to_pngname = {}
        # dict of pngnames to png numbers; used to control uniqueness
        self.pngname_to_pngnum = {}
        # dict of png absolute numbers to tilesheet paths, used to know where to extract images
        self.pngnum_to_tspathname = {}
        self.tspathname_to_tsfilename = {}
        # list of pngs written out
        self.extracted_pngnums = {}
        # list of pngs to not use
        self.delete_pngnums = []
        # misc data
        self.tileset_pathname = ""
        self.default_width = 16
        self.default_height = 16
        self.last_pngnum = 0
        self.ts_data = {}

    def get_all_data(self, tileset_dirname, delete_pathname):
        self.tileset_pathname = tileset_dirname
        if not tileset_dirname.startswith("gfx/"):
            self.tileset_pathname = "gfx/" + tileset_dirname

        try:
            os.stat(self.tileset_pathname)
        except KeyError:
            print("cannot find a directory {}".format(self.tileset_pathname))
            exit -1

        tileset_confname = refs.tileset_pathname + "/" + "tile_config.json"

        try:
            os.stat(tileset_confname)
        except KeyError:
            print("cannot find a directory {}".format(tileset_confname))
            exit -1

        if delete_pathname:
            with open(delete_pathname) as del_file:
                del_ranges = json.load(del_file)
                for delete_range in del_ranges:
                    if not isinstance(delete_range, list):
                        contineu
                    min_png = delete_range[0]
                    max_png = min_png
                    if len(delete_range) > 1:
                        max_png = delete_range[1]
                    for i in range(min_png, max_png + 1):
                        self.delete_pngnums.append(i)

        with open(tileset_confname) as conf_file:
            return(json.load(conf_file))       

    def add_pngnum_to_tsfilepath(self, pngnum):
        if not isinstance(pngnum, int):
            return
        if pngnum in self.pngnum_to_tspathname:
            return
        for ts_filename, ts_data in self.ts_data.items():
            if pngnum >= ts_data.pngnum_min and pngnum <= ts_data.pngnum_max:
                self.pngnum_to_tspathname.setdefault(pngnum, ts_data.ts_pathname)
                return
        raise KeyError("index %s out of range", pngnum)

    def convert_index(self, read_pngnums, new_index, new_id):
        if isinstance(read_pngnums, list):
            for pngnum in read_pngnums:
                if isinstance(pngnum, dict):
                    sprite_ids = pngnum.get("sprite", -10)
                    if isinstance(sprite_ids, list):
                        new_sprites = []
                        for sprite_id in sprite_ids:
                            if sprite_id >= 0 and sprite_id not in self.delete_pngnums:
                                if not new_id:
                                    new_id = self.pngnum_to_pngname[sprite_id]
                                new_sprites.append(self.pngnum_to_pngname[sprite_id])
                        pngnum["sprite"] = new_sprites
                    else:
                        if sprite_ids >= 0 and sprite_ids not in self.delete_pngnums:
                            if not new_id:
                                new_id = self.pngnum_to_pngname[sprite_ids]
                            pngnum["sprite"] = self.pngnum_to_pngname[sprite_ids]
                    new_index.append(pngnum)
                elif pngnum >= 0 and pngnum not in self.delete_pngnums:
                    if not new_id:
                        new_id = self.pngnum_to_pngname[pngnum]
                    new_index.append(self.pngnum_to_pngname[pngnum])
        elif read_pngnums >= 0 and read_pngnums not in self.delete_pngnums:
            if not new_id:
                new_id = self.pngnum_to_pngname[read_pngnums]
            new_index.append(self.pngnum_to_pngname[read_pngnums])
        return new_id

    def convert_pngnum_to_pngname(self, tile_entry):
        new_fg = []
        new_id = ""
        fg_id = tile_entry.get("fg", -10)
        new_id = self.convert_index(fg_id, new_fg, new_id)
        bg_id = tile_entry.get("bg", -10)
        new_bg = []
        new_id = self.convert_index(bg_id, new_bg, new_id)
        add_tile_entrys = tile_entry.get("additional_tiles", [])
        for add_tile_entry in add_tile_entrys:
            self.convert_pngnum_to_pngname(add_tile_entry)
        tile_entry["fg"] = new_fg
        tile_entry["bg"] = new_bg

        return new_id, tile_entry

    def report_missing(self):
        for pngnum in self.pngnum_to_pngname:
            if not self.extracted_pngnums.get(pngnum):
                print("missing index {}, {}".format(pngnum, self.pngnum_to_pngname[pngnum]))


args = argparse.ArgumentParser(description="Split a tileset's tile_config.json into a directory per tile containing the tile data and png.")
args.add_argument("tileset_dir", action="store",
                  help="local name of the tileset directory under gfx/")
args.add_argument("--delete_file", dest="del_path", action="store",
                  help="local name of file containing lists of ranges of indices to delete")
argsDict = vars(args.parse_args())

tileset_dirname = argsDict.get("tileset_dir", "")
delete_pathname = argsDict.get("del_path", "")

refs = PngRefs()
all_tiles = refs.get_all_data(tileset_dirname, delete_pathname)

all_tilesheet_data = all_tiles.get("tiles-new", [])
tile_info = all_tiles.get("tile_info", {})
if tile_info:
    refs.default_width = tile_info[0].get("width")
    refs.default_height = tile_info[0].get("height")

overlay_ordering = all_tiles.get("overlay_ordering", [])
ts_sequence = []
for tilesheet_data in all_tilesheet_data:
    ts_data = TileSheetData(tilesheet_data, refs)
    ts_data.summarize(tile_info, refs)
    ts_sequence.append(ts_data.ts_filename)

for tilesheet_data in all_tilesheet_data:
    ts_filename = tilesheet_data.get("file", "")
    ts_data = refs.ts_data[ts_filename]
    if ts_data.fallback:
        continue
    tile_id_to_tile_entrys = {}
    all_tile_entry = tilesheet_data.get("tiles", [])
    for tile_entry in all_tile_entry:
        tile_id = ts_data.parse_tile_entry(tile_entry, refs)
        if tile_id:
            tile_id_to_tile_entrys.setdefault(tile_id, [])
            tile_id_to_tile_entrys[tile_id].append(tile_entry)
    ts_data.tile_id_to_tile_entrys = tile_id_to_tile_entrys

#debug statements to verify pngnum_to_pngname and pngname_to_pngnum
#print("pngnum_to_pngname: {}".format(json.dumps(refs.pngnum_to_pngname, sort_keys=True, indent=2)))
#print("pngname_to_pngnum: {}".format(json.dumps(refs.pngname_to_pngnum, sort_keys=True, indent=2)))
#print("{}".format(json.dumps(file_tile_id_to_tile_entrys, indent=2)))
#print("{}".format(json.dumps(refs.pngnum_to_tspathname, indent=2)))

for ts_filename in ts_sequence:
    out_data = ExtractionData(ts_filename, refs)

    if not out_data.valid:
        continue
    out_data.write_expansions()

    for tile_id, tile_entrys in out_data.ts_data.tile_id_to_tile_entrys.items():
        #print("tile id {} with {} entries".format(tile_id, len(tile_entrys)))
        for tile_entry in tile_entrys:
            subdir_pathname = out_data.increment_dir()
            tile_entry_name, tile_entry = refs.convert_pngnum_to_pngname(tile_entry)
            if not tile_entry_name:
                continue
            tile_entry_pathname = subdir_pathname + "/" + tile_entry_name + ".json"
            write_to_json(tile_entry_pathname, tile_entry)
    out_data.write_images(refs)

if tile_info:
    tile_info_pathname = refs.tileset_pathname + "/" + "tile_info.json"
    write_to_json(tile_info_pathname, tile_info, True)

refs.report_missing()
