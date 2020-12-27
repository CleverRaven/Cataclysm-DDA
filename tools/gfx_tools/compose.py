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


def write_to_json(pathname, data):
    with open(pathname, 'w') as fp:
        json.dump(data, fp)

    json_formatter = './tools/format/json_formatter.cgi'
    if os.path.isfile(json_formatter):
        cmd = [json_formatter, pathname]
        subprocess.call(cmd)


def find_or_make_dir(pathname):
    try:
        os.stat(pathname)
    except OSError:
        os.mkdir(pathname)


class Tileset:
    '''
    Sprites handling and referenced images memory
    '''
    def __init__(self, source_dir, output_dir):
        self.source_dir = source_dir
        self.output_dir = output_dir

        self.pngnum = 0
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

    def convert_a_pngname_to_pngnum(self, sprite_id, entry):
        if sprite_id and sprite_id != 'no_entry':
            new_id = self.pngname_to_pngnum.get(sprite_id, 0)
            if new_id:
                entry.append(new_id)
                if sprite_id not in self.referenced_pngnames:
                    self.referenced_pngnames.append(sprite_id)
                return True
            else:
                print(f'Error: sprite id {sprite_id} has no matching PNG file.'
                      ' It will not be added to tile_config.json')
                global ERROR_LOGGED
                ERROR_LOGGED = True
        return False

    def convert_pngname_to_pngnum(self, index):
        new_index = []
        if isinstance(index, list):
            for pngname in index:
                if isinstance(pngname, dict):
                    sprite_ids = pngname.get('sprite')
                    valid = False
                    new_sprites = []
                    if isinstance(sprite_ids, list):
                        new_sprites = []
                        for sprite_id in sprite_ids:
                            valid |= self.convert_a_pngname_to_pngnum(
                                sprite_id, new_sprites)
                        pngname['sprite'] = new_sprites
                    else:
                        valid = self.convert_a_pngname_to_pngnum(
                            sprite_ids, new_sprites)
                        if valid:
                            pngname['sprite'] = new_sprites[0]
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
        '''
        Compile input JSON into objects for the output JSON config
        '''
        tile_id = tile_entry.get('id')
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
        fg_id = tile_entry.get('fg')
        if fg_id:
            tile_entry['fg'] = self.convert_pngname_to_pngnum(fg_id)
        else:
            del tile_entry['fg']

        bg_id = tile_entry.get('bg')
        if bg_id:
            tile_entry['bg'] = self.convert_pngname_to_pngnum(bg_id)
        else:
            try:
                del tile_entry['bg']
            except Exception:
                print(f'Error: Cannot find bg for tile with id {tile_id}')
                global ERROR_LOGGED
                ERROR_LOGGED = True

        add_tile_entrys = tile_entry.get('additional_tiles', [])
        for add_tile_entry in add_tile_entrys:
            self.convert_tile_entry(add_tile_entry, id_as_prefix, is_filler)

        if fg_id or bg_id:
            for an_id in tile_id:
                full_id = f'{prefix}{an_id}'
                if full_id not in self.processed_ids:
                    self.processed_ids.append(full_id)
            return tile_entry
        return None  # TODO: option to warn

    def find_unused(self, use_all=False):
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
    def __init__(self, subdir_index, tileset):
        tilesheet_config_obj = tileset.info[subdir_index]
        self.name = next(iter(tilesheet_config_obj))
        self.specs = tilesheet_config_obj[self.name] or {}

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
        self.row_num = 0
        self.null_image = \
            Vips.Image.grey(self.sprite_width, self.sprite_height)
        self.row_pngs = ['null_image']

    def set_first_index(self, tileset):
        '''
        Increment global index and set local indexes.
        Global index can be decremented later if tilesheet does not contain
        any output images.
        '''
        tileset.pngnum += 1
        self.first_index = tileset.pngnum
        self.max_index = tileset.pngnum

    def is_standard(self, tileset):
        '''
        Check whether output object needs a non-standard size or offset config
        '''
        if self.offset_x or self.offset_y:
            return False
        if self.sprite_width != tileset.sprite_width:
            return False
        if self.sprite_height != tileset.sprite_height:
            return False
        return True

    def load_image(self, png_path):
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

    def prepare_images_row(self, tileset):
        '''
        Prepare a list of 16 Vips images that will be one row
        '''
        spacer = 16 - len(self.row_pngs)
        tileset.pngnum += spacer

        row = []

        for png_path in self.row_pngs:
            if png_path == 'null_image':
                row.append(self.null_image)
            else:
                image = self.load_image(png_path)
                row.append(image)

        # fill the last row
        row.extend([self.null_image for i in range(spacer)])

        return row

    def walk_dirs(self, tileset):
        images_grid = []
        for subdir_fpath, dirnames, filenames in os.walk(self.subdir_path):
            for filename in filenames:
                filepath = os.path.join(subdir_fpath, filename)
                if filename.endswith('.png'):
                    pngname = filename.split('.png')[0]
                    if (pngname in tileset.pngname_to_pngnum or
                            pngname == 'no_entry'):
                        print(f'skipping {pngname}')
                        continue
                    if self.is_filler and pngname in tileset.pngname_to_pngnum:
                        continue  # TODO: option to warn
                    self.row_pngs.append(filepath)
                    tileset.pngname_to_pngnum[pngname] = tileset.pngnum
                    tileset.pngnum_to_pngname[tileset.pngnum] = pngname
                    tileset.pngnum += 1
                    if len(self.row_pngs) > 15:
                        images_row = self.prepare_images_row(tileset)
                        self.row_num += 1
                        self.row_pngs = []
                        images_grid += images_row
                elif filename.endswith('.json'):
                    with open(filepath, 'r') as fp:
                        try:
                            tile_entry = json.load(fp)
                        except Exception:
                            print(f'error loading {filepath}')
                            raise

                        if not isinstance(tile_entry, list):
                            tile_entry = [tile_entry]
                        self.tile_entries += tile_entry
        if self.row_pngs:
            if self.row_num == 0 and self.row_pngs == ['null_image']:
                return []
            images_row = self.prepare_images_row(tileset)
            images_grid += images_row
        return images_grid

    def create_sheet(self, images_grid):
        '''
        Compose and save tilesheet PNG
        '''
        if images_grid:
            sheet_image = Vips.Image.arrayjoin(images_grid, across=16)
            sheet_image.pngsave(self.output)


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
    for subdir_index in range(1, len(tileset.info)):
        sheet = Tilesheet(subdir_index, tileset)
        sheet.set_first_index(tileset)

        if sheet.is_filler:
            sheet_type = 'filler'
        elif sheet.is_fallback:
            sheet_type = 'fallback'
        else:
            sheet_type = 'main'

        print(f'Info: parsing {sheet_type} tilesheet {sheet.name}')
        if sheet_type != 'fallback':
            images_grid = sheet.walk_dirs(tileset)

            if not images_grid:
                # no images in the tilesheet, revert pngnum
                tileset.pngnum -= 1
                continue

            # write output PNGs
            sheet.create_sheet(images_grid)

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

        if not sheet.is_standard(tileset):
            sheet_conf['sprite_width'] = sheet.sprite_width
            sheet_conf['sprite_height'] = sheet.sprite_height
            sheet_conf['sprite_offset_x'] = sheet.offset_x
            sheet_conf['sprite_offset_y'] = sheet.offset_y

        tiles_new_dict[sheet.max_index] = sheet_conf

    # fing unused images
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
