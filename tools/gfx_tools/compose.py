#!/usr/bin/env python3

# compose.py
# Split a gfx directory made of 1000s of little images and files
# into a set of tilesheets and a tile_config.json

'''
Merge all individal tile_entries and pngs in a tileset's directory
into a tile_config.json and 1 or more tilesheet pngs.
'''

import argparse
import json
import os
import subprocess
import sys

from typing import Any, Tuple, Union

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
        {"offset": 0, "bold": False, "color": "BLACK"},
        {"offset": 256, "bold": True, "color": "WHITE"},
        {"offset": 512, "bold": False, "color": "WHITE"},
        {"offset": 768, "bold": True, "color": "BLACK"},
        {"offset": 1024, "bold": False, "color": "RED"},
        {"offset": 1280, "bold": False, "color": "GREEN"},
        {"offset": 1536, "bold": False, "color": "BLUE"},
        {"offset": 1792, "bold": False, "color": "CYAN"},
        {"offset": 2048, "bold": False, "color": "MAGENTA"},
        {"offset": 2304, "bold": False, "color": "YELLOW"},
        {"offset": 2560, "bold": True, "color": "RED"},
        {"offset": 2816, "bold": True, "color": "GREEN"},
        {"offset": 3072, "bold": True, "color": "BLUE"},
        {"offset": 3328, "bold": True, "color": "CYAN"},
        {"offset": 3584, "bold": True, "color": "MAGENTA"},
        {"offset": 3840, "bold": True, "color": "YELLOW"}
    ]
}

ERROR_LOGGED = False


def write_to_json(pathname: str, data: Union[dict, list]) -> None:
    with open(pathname, 'w') as fp:
        json.dump(data, fp)

    json_formatter = './tools/format/json_formatter.cgi'
    if os.path.isfile(json_formatter):
        cmd = [json_formatter, pathname]
        subprocess.call(cmd)


def find_or_make_dir(pathname: str) -> None:
    try:
        os.stat(pathname)
    except OSError:
        os.mkdir(pathname)


def list_or_first(iterable: list) -> Any:
    return iterable[0] if len(iterable) == 1 else iterable


class Tileset:
    '''
    Sprites handling and referenced images memory
    '''
    def __init__(self, source_dir: str, output_dir: str) -> None:
        self.source_dir = source_dir
        self.output_dir = output_dir

        self.pngnum = 1
        self.referenced_pngnames = []

        # bijectional dicts of pngnames to png numbers and vice versa
        # used to control uniqueness
        self.pngname_to_pngnum = {'null_image': 0}
        self.pngnum_to_pngname = {0: 'null_image'}

        if not os.access(self.source_dir, os.R_OK) \
                or not os.path.isdir(self.source_dir):
            sys.exit(f'Error: cannot open directory {self.source_dir}')

        self.processed_ids = []
        info_path = os.path.join(self.source_dir, 'tile_info.json')
        self.sprite_width = 16
        self.sprite_height = 16
        self.info = [{}]
        if not os.access(info_path, os.R_OK):
            sys.exit(f'Error: cannot open {info_path}')
        with open(info_path, 'r') as fp:
            self.info = json.load(fp)
            self.sprite_width = self.info[0].get('width')
            self.sprite_height = self.info[0].get('height')

    def append_sprite_index(self, sprite_name: str, entry: list) -> bool:
        '''
        Get sprite index by sprite name and append it to entry
        '''
        if sprite_name:
            sprite_index = self.pngname_to_pngnum.get(sprite_name, 0)
            if sprite_index:
                entry.append(sprite_index)
                if sprite_name not in self.referenced_pngnames:
                    self.referenced_pngnames.append(sprite_name)
                return True
            else:
                print(f'Error: sprite {sprite_name} has no matching PNG file.'
                      ' It will not be added to tile_config.json')
                global ERROR_LOGGED
                ERROR_LOGGED = True
        return False

    def convert_entry_layer(self, entry_layer: Union[list, str]) -> list:
        '''
        Convert sprite names to sprite indexes in one fg or bg tile entry part
        '''
        output = []

        if isinstance(entry_layer, list):
            # "fg": [ "f_fridge_S", "f_fridge_W", "f_fridge_N", "f_fridge_E" ]
            for layer_part in entry_layer:
                if isinstance(layer_part, dict):
                    # weighted random variations
                    variations, valid = self.convert_random_variations(
                        layer_part.get('sprite'))
                    if valid:
                        layer_part['sprite'] = \
                            variations[0] if len(variations) == 1 \
                            else variations
                        output.append(layer_part)
                else:
                    self.append_sprite_index(layer_part, output)
        else:
            # "bg": "t_grass"
            self.append_sprite_index(entry_layer, output)

        return output

    def convert_random_variations(self, sprites: Union[list, str])\
            -> Tuple[list, bool]:
        '''
        Convert list of random weighted variation objects
        '''
        valid = False
        converted_variations = []

        if isinstance(sprites, list):
            # list of rotations
            converted_variations = []
            for sprite_name in sprites:
                valid |= self.append_sprite_index(
                    sprite_name, converted_variations)
        else:
            # single sprite
            valid = self.append_sprite_index(
                sprites, converted_variations)
        return converted_variations, valid

    def convert_tile_entry(self, entry: dict, prefix: str, is_filler: bool)\
            -> None:
        '''
        Recursively compile input into game-compatible objects in-place
        '''
        tile_id = entry.get('id')
        id_as_prefix = None
        if tile_id:
            if not isinstance(tile_id, list):
                tile_id = [tile_id]
            id_as_prefix = f'{tile_id[0]}_'

        if is_filler:
            for an_id in tile_id:
                full_id = f'{prefix}{an_id}'
                if full_id in self.processed_ids:
                    print(f'Info: skipping filler for {full_id}')
                    return None
        fg_layer = entry.get('fg')
        if fg_layer:
            entry['fg'] = list_or_first(
                self.convert_entry_layer(fg_layer))
        else:
            del entry['fg']

        bg_layer = entry.get('bg')
        if bg_layer:
            entry['bg'] = list_or_first(
                self.convert_entry_layer(bg_layer))
        else:
            try:
                del entry['bg']
            except Exception:
                print(f'Error: Cannot find bg for tile with id {tile_id}')
                global ERROR_LOGGED
                ERROR_LOGGED = True

        add_tile_entrys = entry.get('additional_tiles', [])
        for add_tile_entry in add_tile_entrys:
            # recursive part
            self.convert_tile_entry(add_tile_entry, id_as_prefix, is_filler)

        if fg_layer or bg_layer:
            for an_id in tile_id:
                full_id = f'{prefix}{an_id}'
                if full_id not in self.processed_ids:
                    self.processed_ids.append(full_id)
            return entry
        return None  # TODO: option to warn

    def find_unused(self, use_all: bool = False) -> dict:
        '''
        Find unused images and either warn about them or return the list
        '''
        unused = dict()
        for pngname, pngnum in self.pngname_to_pngnum.items():
            if pngnum and pngname not in self.referenced_pngnames:
                if use_all:
                    unused[pngname] = pngnum
                else:
                    print(
                        f'Warning: image filename {pngname} index {pngnum}'
                        'was not used in any tile_config.json entries')
        return unused


class Tilesheet:
    '''
    Tilesheet reading and compositing
    '''
    def __init__(
            self,
            tileset: Tileset,
            config_index: int,
            sheet_width: int = 16) -> None:
        self.sheet_width = sheet_width
        tilesheet_config_obj = tileset.info[config_index]
        self.name = next(iter(tilesheet_config_obj))
        self.specs = tilesheet_config_obj[self.name] or {}
        self.tileset = tileset

        self.sprite_width = self.specs.get(
            'sprite_width', tileset.sprite_width)
        self.sprite_height = self.specs.get(
            'sprite_height', tileset.sprite_height)
        self.offset_x = self.specs.get('sprite_offset_x', 0)
        self.offset_y = self.specs.get('sprite_offset_y', 0)

        self.is_fallback = self.specs.get('fallback', False)
        self.is_filler = not self.is_fallback \
            and self.specs.get('filler', False)

        output_root = self.name.split('.png')[0]
        dir_name = \
            f'pngs_{output_root}_{self.sprite_width}x{self.sprite_height}'
        self.subdir_path = os.path.join(tileset.source_dir, dir_name)

        self.output = os.path.join(tileset.output_dir, self.name)

        self.tile_entries = []
        self.null_image = \
            Vips.Image.grey(self.sprite_width, self.sprite_height)
        self.images_grid = [self.null_image] if config_index == 1 else []

    def set_first_index(self) -> None:
        '''
        Set initial indexes.
        '''
        self.first_index = self.tileset.pngnum
        self.max_index = self.tileset.pngnum

    def is_standard(self) -> bool:
        '''
        Check whether output object needs a non-standard size or offset config
        '''
        if self.offset_x or self.offset_y:
            return False
        if self.sprite_width != self.tileset.sprite_width:
            return False
        if self.sprite_height != self.tileset.sprite_height:
            return False
        return True

    def load_image(self, png_path: str) -> pyvips.Image:
        '''
        Load and verify an image using pyvips
        '''
        image = Vips.Image.pngload(png_path)
        if image.interpretation != 'srgb':
            image = image.colourspace('srgb')

        try:
            if not image.hasalpha():
                image = image.addalpha()
        except Vips.Error as vips_error:
            print(f'{png_path}: {vips_error}')

        try:
            if image.get_typeof('icc-profile-data') != 0:
                image = image.icc_transform('srgb')
        except Vips.Error as vips_error:
            print(f'{png_path}: {vips_error}')

        if (image.width != self.sprite_width or
                image.height != self.sprite_height):
            print(
                f'Error: {png_path} is {image.width}x{image.height}, but '
                f'{self.name} sheet sprites have to be '
                f'{self.sprite_width}x{self.sprite_height}.')
            global ERROR_LOGGED
            ERROR_LOGGED = True

        return image

    def walk_dirs(self) -> None:
        '''
        Find and process all JSON and PNG files within sheet directory
        '''
        for subdir_fpath, dirnames, filenames in os.walk(self.subdir_path):
            for filename in filenames:
                filepath = os.path.join(subdir_fpath, filename)
                if filename.endswith('.png'):
                    self.process_png(filepath, filename)
                elif filename.endswith('.json'):
                    self.process_json(filepath)

    def process_png(self, filepath, filename) -> None:
        '''
        Verify image root name is unique, load it and register
        '''
        pngname = filename.split('.png')[0]
        if (pngname in self.tileset.pngname_to_pngnum):
            print(f'skipping {pngname}')
            return
        if self.is_filler and pngname in self.tileset.pngname_to_pngnum:
            return  # TODO: option to warn

        self.images_grid.append(self.load_image(filepath))
        self.tileset.pngname_to_pngnum[pngname] = self.tileset.pngnum
        self.tileset.pngnum_to_pngname[self.tileset.pngnum] = pngname
        self.tileset.pngnum += 1

    def process_json(self, filepath) -> None:
        '''
        Load and store tile entries from the file
        '''
        with open(filepath, 'r') as fp:
            try:
                tile_entries = json.load(fp)
            except Exception:
                print(f'error loading {filepath}')
                raise

            if not isinstance(tile_entries, list):
                tile_entries = [tile_entries]
            self.tile_entries += tile_entries

    def write_composite_png(self) -> bool:
        '''
        Compose and save tilesheet PNG
        '''
        if not self.images_grid:
            return False

        # fill the last row
        empty_spaces = self.sheet_width - (
            len(self.images_grid) % self.sheet_width)
        self.images_grid.extend([self.null_image for i in range(empty_spaces)])
        self.tileset.pngnum += empty_spaces

        if self.images_grid:
            sheet_image = Vips.Image.arrayjoin(
                self.images_grid, across=self.sheet_width)
            sheet_image.pngsave(self.output)
            return True
        return False


if __name__ == '__main__':
    # read arguments and initialize objects
    arg_parser = argparse.ArgumentParser(description=__doc__)
    arg_parser.add_argument(
        'source_dir',
        help='Tileset source files directory path')
    arg_parser.add_argument(
        'output_dir', nargs='?',
        help='Output directory path')
    arg_parser.add_argument(
        '--use-all', dest='use_all', action='store_true',
        help='Add unused images with id being their basename')
    args_dict = vars(arg_parser.parse_args())

    source_dir = args_dict.get('source_dir')
    output_dir = args_dict.get('output_dir') or source_dir
    tileset_confpath = os.path.join(output_dir, 'tile_config.json')
    use_all = args_dict.get('use_all', False)

    tileset = Tileset(source_dir, output_dir)

    typed_sheets = {
        'main': [],
        'filler': [],
        'fallback': [],
    }
    fallback_name = 'fallback.png'

    # loop through tilesheets and parse all configs in subdirectories,
    # create sheet images
    for config_index in range(1, len(tileset.info)):
        sheet = Tilesheet(tileset, config_index)
        sheet.set_first_index()

        if sheet.is_filler:
            sheet_type = 'filler'
        elif sheet.is_fallback:
            sheet_type = 'fallback'
        else:
            sheet_type = 'main'

        print(f'Info: parsing {sheet_type} tilesheet {sheet.name}')
        if sheet_type != 'fallback':
            sheet.walk_dirs()

            # write output PNGs
            if not sheet.write_composite_png():
                continue

            sheet.max_index = tileset.pngnum

        typed_sheets[sheet_type].append(sheet)

    # combine config data in correct order
    sheet_configs = typed_sheets['main'] + typed_sheets['filler'] \
        + typed_sheets['fallback']

    # preparing "tiles-new", but remembering max index of each sheet in keys
    tiles_new_dict = dict()

    for sheet in sheet_configs:
        if sheet.is_fallback:
            fallback_name = sheet.name
            continue
        sheet_entries = []

        for tile_entry in sheet.tile_entries:
            converted_tile_entry = tileset.convert_tile_entry(
                tile_entry, '', sheet.is_filler)
            if converted_tile_entry:
                sheet_entries.append(converted_tile_entry)

        sheet_conf = {
            'file': sheet.name,
            'tiles': sheet_entries,
            '//': f'range {sheet.first_index} to {sheet.max_index}'
        }

        if not sheet.is_standard():
            sheet_conf['sprite_width'] = sheet.sprite_width
            sheet_conf['sprite_height'] = sheet.sprite_height
            sheet_conf['sprite_offset_x'] = sheet.offset_x
            sheet_conf['sprite_offset_y'] = sheet.offset_y

        tiles_new_dict[sheet.max_index] = sheet_conf

    # find unused images
    unused = tileset.find_unused(use_all)

    # unused list must be empty without use_all
    for unused_png in unused:
        unused_num = tileset.pngname_to_pngnum[unused_png]
        sheet_min_index = 0
        for sheet_max_index in tiles_new_dict.keys():
            if sheet_min_index < unused_num <= sheet_max_index:
                tiles_new_dict[sheet_max_index]['tiles'].append(
                    {'id': unused_png.split('.png')[0],
                     'fg': unused_num})
                break
            sheet_min_index = sheet_max_index

    # finalizing "tiles-new" config
    tiles_new = [v for v in tiles_new_dict.values()]

    FALLBACK['file'] = fallback_name
    tiles_new.append(FALLBACK)
    output_conf = {
        'tile_info': [{
            'width': tileset.sprite_width,
            'height': tileset.sprite_height
        }],
        'tiles-new': tiles_new
    }

    # save the config
    write_to_json(tileset_confpath, output_conf)

    if ERROR_LOGGED:
        sys.exit(1)
