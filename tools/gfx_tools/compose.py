#!/usr/bin/env python3
'''
Merge all tile entries and PNGs in a compositing tileset directory into
a tile_config.json and tilesheet .png file(s) ready for use in CDDA.

Examples:

    %(prog)s ../CDDA-Tilesets/gfx/Retrodays/
    %(prog)s --use-all ../CDDA-Tilesets/gfx/UltimateCataclysm/

By default, output is written back to the source directory. Pass an output
directory as the last argument to place output files there instead.
'''

import argparse
import json
import os
import subprocess
import sys

from typing import Any, Optional, Tuple, Union

try:
    import pyvips
    Vips = pyvips
except ImportError:
    import gi
    gi.require_version('Vips', '8.0')  # NoQA
    from gi.repository import Vips

PROPERTIES_FILENAME = 'tileset.txt'

PNGSAVE_ARGS = {
    'compression': 9,
    'strip': True,
    'filter': 8,
}

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


def write_to_json(pathname: str, data: Union[dict, list]) -> None:
    '''
    Write data to a JSON file
    '''
    with open(pathname, 'w', encoding="utf-8") as file:
        json.dump(data, file, ensure_ascii=False)

    json_formatter = './tools/format/json_formatter.cgi'
    if os.path.isfile(json_formatter):
        cmd = [json_formatter, pathname]
        subprocess.call(cmd)


def find_or_make_dir(pathname: str) -> None:
    '''
    Autocreate needed directory if it doesn't exist
    TODO: just use pathlib
    '''
    try:
        os.stat(pathname)
    except OSError:
        os.mkdir(pathname)


def list_or_first(iterable: list) -> Any:
    '''
    Strip unneeded container list if there is only one value
    '''
    return iterable[0] if len(iterable) == 1 else iterable


def read_properties(filepath: str) -> dict:
    '''
    tileset.txt reader
    '''
    with open(filepath, 'r', encoding="utf-8") as file:
        pairs = {}
        for line in file.readlines():
            line = line.strip()
            if line and not line.startswith('#'):
                key, value = line.split(':')
                pairs[key.strip()] = value.strip()
    return pairs


class ComposingException(Exception):
    '''
    Base class for all composing exceptions
    '''


class Tileset:
    '''
    Referenced sprites memory and handling, tile entries conversion
    '''
    def __init__(
            self,
            source_dir: str,
            output_dir: str,
            use_all: bool = False,
            obsolete_fillers: bool = False,
            palette_copies: bool = False,
            palette: bool = False)\
            -> None:
        self.source_dir = source_dir
        self.output_dir = output_dir
        self.use_all = use_all
        self.obsolete_fillers = obsolete_fillers
        self.palette_copies = palette_copies
        self.palette = palette
        self.output_conf_file = None

        self.pngnum = 0
        self.unreferenced_pngnames = {
            'main': [],
            'filler': [],
        }

        self.pngname_to_pngnum = {'null_image': 0}

        if not os.access(self.source_dir, os.R_OK) \
                or not os.path.isdir(self.source_dir):
            raise ComposingException(
                f'Error: cannot open directory {self.source_dir}')

        self.processed_ids = []
        info_path = os.path.join(self.source_dir, 'tile_info.json')
        self.sprite_width = 16
        self.sprite_height = 16
        self.pixelscale = 1
        self.info = [{}]

        if not os.access(info_path, os.R_OK):
            raise ComposingException(f'Error: cannot open {info_path}')

        with open(info_path, 'r', encoding="utf-8") as file:
            self.info = json.load(file)
            self.sprite_width = self.info[0].get('width', self.sprite_width)
            self.sprite_height = self.info[0].get('height', self.sprite_height)
            self.pixelscale = self.info[0].get('pixelscale', self.pixelscale)

        # TODO: self.errors
        self.error_logged = False

    def determine_conffile(self) -> str:
        '''
        Read JSON value from tileset.txt
        '''
        properties = {}

        for candidate_path in (self.source_dir, self.output_dir):
            properties_path = os.path.join(candidate_path, PROPERTIES_FILENAME)
            if os.access(properties_path, os.R_OK):
                properties = read_properties(properties_path)
                if properties:
                    break

        if not properties:
            raise ComposingException(f'No valid {PROPERTIES_FILENAME} found')

        conf_filename = properties.get('JSON', None)

        if not conf_filename:
            raise ComposingException(
                f'No JSON key found in {PROPERTIES_FILENAME}')

        self.output_conf_file = conf_filename
        return self.output_conf_file

    def compose(self) -> None:
        '''
        Convert a composing tileset into a package readable by the game
        '''
        tileset_confpath = os.path.join(
            self.output_dir, self.determine_conffile())
        typed_sheets = {
            'main': [],
            'filler': [],
            'fallback': [],
        }
        fallback_name = 'fallback.png'

        # loop through tilesheets and parse all configs in subdirectories,
        # create sheet images
        for config_index in range(1, len(self.info)):
            sheet = Tilesheet(self, config_index)

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

                sheet.max_index = self.pngnum

            typed_sheets[sheet_type].append(sheet)

        # combine config data in the correct order
        sheet_configs = typed_sheets['main'] + typed_sheets['filler'] \
            + typed_sheets['fallback']

        # prepare "tiles-new", but remember max index of each sheet in keys
        tiles_new_dict = dict()

        def create_tile_entries_for_unused(
                unused: list,
                fillers: bool)\
                -> None:
            # the list must be empty without use_all
            for unused_png in unused:
                if unused_png in self.processed_ids:
                    if not fillers:
                        print(
                            f'Warning: {unused_png} sprite was not mentioned '
                            'in any tile entry but there is a tile entry '
                            f'for the {unused_png} ID')
                    if fillers and self.obsolete_fillers:
                        print(
                            'Warning: there is a tile entry for '
                            f'{unused_png} in a non-filler sheet')
                    continue
                unused_num = self.pngname_to_pngnum[unused_png]
                sheet_min_index = 0
                for sheet_max_index in tiles_new_dict:
                    if sheet_min_index < unused_num <= sheet_max_index:
                        tiles_new_dict[sheet_max_index]['tiles'].append(
                            {'id': unused_png,
                             'fg': unused_num})
                        self.processed_ids.append(unused_png)
                        break
                    sheet_min_index = sheet_max_index

        main_finished = False

        for sheet in sheet_configs:
            if sheet.is_fallback:
                fallback_name = sheet.name
                continue
            if sheet.is_filler and not main_finished:
                create_tile_entries_for_unused(
                    self.handle_unreferenced_sprites('main'),
                    fillers=False
                )
                main_finished = True
            sheet_entries = []

            for tile_entry in sheet.tile_entries:
                # TODO: pop?
                converted_tile_entry = tile_entry.convert()
                if converted_tile_entry:
                    sheet_entries.append(converted_tile_entry)

            sheet_conf = {
                'file': sheet.name,
                '//': f'range {sheet.first_index} to {sheet.max_index}',
            }

            if not sheet.is_standard():
                sheet_conf['sprite_width'] = sheet.sprite_width
                sheet_conf['sprite_height'] = sheet.sprite_height
                sheet_conf['sprite_offset_x'] = sheet.offset_x
                sheet_conf['sprite_offset_y'] = sheet.offset_y

            sheet_conf['tiles'] = sheet_entries

            tiles_new_dict[sheet.max_index] = sheet_conf

        if not main_finished:
            create_tile_entries_for_unused(
                self.handle_unreferenced_sprites('main'),
                fillers=False,
            )

        create_tile_entries_for_unused(
            self.handle_unreferenced_sprites('filler'),
            fillers=True,
        )

        # finalize "tiles-new" config
        tiles_new = list(tiles_new_dict.values())

        FALLBACK['file'] = fallback_name
        tiles_new.append(FALLBACK)
        output_conf = {
            'tile_info': [{
                'pixelscale': self.pixelscale,
                'width': self.sprite_width,
                'height': self.sprite_height,
            }],
            'tiles-new': tiles_new
        }

        # save the config
        write_to_json(tileset_confpath, output_conf)

    def handle_unreferenced_sprites(self, sheet_type: str) -> list:
        '''
        Either warn about unused sprites or return the list
        '''
        if self.use_all:
            return self.unreferenced_pngnames[sheet_type]

        for pngname in self.unreferenced_pngnames[sheet_type]:
            if pngname in self.processed_ids:
                print(f'Error: {pngname}.png not used when {pngname} ID '
                      'is mentioned in a tile entry')
                self.error_logged = True
            else:
                print(
                    f'Warning: sprite filename {pngname} was not used '
                    f'in any {sheet_type} {self.output_conf_file} entries')
        return []


class Tilesheet:
    '''
    Tilesheet reading and compositing
    '''
    def __init__(
            self,
            tileset: Tileset,
            config_index: int,
            sheet_width: int = 16) -> None:
        self.sheet_width = sheet_width  # sprites across, could be anything
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
        self.sprites = [self.null_image] if config_index == 1 else []

        self.first_index = self.tileset.pngnum + 1
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

    def walk_dirs(self) -> None:
        '''
        Find and process all JSON and PNG files within sheet directory
        '''
        for subdir_fpath, _, filenames in sorted(
                os.walk(self.subdir_path), key=lambda d: d[0]):
            for filename in sorted(filenames):
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
        if pngname in self.tileset.pngname_to_pngnum:
            if not self.is_filler:
                print(f'Error: duplicate {pngname}.png')
                self.tileset.error_logged = True

            if self.is_filler and self.tileset.obsolete_fillers:
                print(f'Warning: {pngname}.png is already present in a '
                      'non-filler sheet')

            return

        self.sprites.append(self.load_image(filepath))
        self.tileset.pngnum += 1
        self.tileset.pngname_to_pngnum[pngname] = self.tileset.pngnum
        self.tileset.unreferenced_pngnames[
            'filler' if self.is_filler else 'main'].append(pngname)

    def load_image(self, png_path: str) -> pyvips.Image:
        '''
        Load and verify an image using pyvips
        '''
        try:
            image = Vips.Image.pngload(png_path)
        except pyvips.error.Error as exception:
            raise ComposingException(
                f'Cannot load {png_path}: {exception.message}') from None
        except UnicodeDecodeError:
            raise ComposingException(
                f'Cannot load {png_path} with UnicodeDecodeError, '
                'please report your setup at '
                'https://github.com/libvips/pyvips/issues/80') from None
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
            self.tileset.error_logged = True

        return image

    def process_json(self, filepath) -> None:
        '''
        Load and store tile entries from the file
        '''
        with open(filepath, 'r', encoding="utf-8") as file:
            try:
                tile_entries = json.load(file)
            except Exception:
                print(f'error loading {filepath}')
                raise

            if not isinstance(tile_entries, list):
                tile_entries = [tile_entries]
            for input_entry in tile_entries:
                self.tile_entries.append(
                    TileEntry(self, input_entry, filepath))

    def write_composite_png(self) -> bool:
        '''
        Compose and save tilesheet PNG if there are sprites to work with
        '''
        if not self.sprites:
            return False

        # count empty spaces in the last row
        self.tileset.pngnum += self.sheet_width - \
            ((len(self.sprites) % self.sheet_width) or self.sheet_width)

        if self.sprites:
            sheet_image = Vips.Image.arrayjoin(
                self.sprites, across=self.sheet_width)

            pngsave_args = PNGSAVE_ARGS

            if self.tileset.palette:
                pngsave_args['palette'] = True

            sheet_image.pngsave(self.output, **pngsave_args)

            if self.tileset.palette_copies and not self.tileset.palette:
                sheet_image.pngsave(
                    self.output + '8', palette=True, **pngsave_args)

            return True
        return False


class TileEntry:
    '''
    Tile entry handling
    '''
    def __init__(self, tilesheet: Tilesheet, data, filepath) -> None:
        self.tilesheet = tilesheet
        self.data = data
        self.filepath = filepath

    def convert(
            self,
            entry: Union[dict, None] = None,
            prefix: str = '')\
            -> Optional[dict]:
        '''
        Recursively compile input into game-compatible objects in-place
        '''
        if entry is None:
            entry = self.data
        tile_id = entry.get('id')
        id_as_prefix = None
        skipping_filler = False

        if tile_id:
            if not isinstance(tile_id, list):
                tile_id = [tile_id]
            id_as_prefix = f'{tile_id[0]}_'

        if self.tilesheet.is_filler:
            for an_id in tile_id:
                full_id = f'{prefix}{an_id}'
                if full_id in self.tilesheet.tileset.processed_ids:
                    if self.tilesheet.tileset.obsolete_fillers:
                        print(
                            f'Warning: skipping filler for {full_id} '
                            f'from {self.filepath}')
                    skipping_filler = True
        fg_layer = entry.get('fg', None)
        if fg_layer:
            entry['fg'] = list_or_first(
                self.convert_entry_layer(fg_layer))
        else:
            entry.pop('fg', None)

        bg_layer = entry.get('bg', None)
        if bg_layer:
            entry['bg'] = list_or_first(
                self.convert_entry_layer(bg_layer))
        else:
            entry.pop('bg', None)

        additional_entries = entry.get('additional_tiles', [])
        for additional_entry in additional_entries:
            # recursive part
            self.convert(additional_entry, id_as_prefix)

        if fg_layer or bg_layer:
            for an_id in tile_id:
                full_id = f'{prefix}{an_id}'
                if full_id not in self.tilesheet.tileset.processed_ids:
                    self.tilesheet.tileset.processed_ids.append(full_id)
                else:
                    if not skipping_filler:
                        print(
                            f'Error: {full_id} encountered more than once, '
                            f'last time in {self.filepath}')
                        self.tilesheet.tileset.error_logged = True
            if skipping_filler:
                return None
            return entry
        print(
            f'skipping empty entry for {prefix}{tile_id} in {self.filepath}')
        return None

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

    def convert_random_variations(self, sprite_names: Union[list, str])\
            -> Tuple[list, bool]:
        '''
        Convert list of random weighted variation objects
        '''
        valid = False
        converted_variations = []

        if isinstance(sprite_names, list):
            # list of rotations
            converted_variations = []
            for sprite_name in sprite_names:
                valid |= self.append_sprite_index(
                    sprite_name, converted_variations)
        else:
            # single sprite
            valid = self.append_sprite_index(
                sprite_names, converted_variations)
        return converted_variations, valid

    def append_sprite_index(self, sprite_name: str, entry: list) -> bool:
        '''
        Get sprite index by sprite name and append it to entry
        '''
        if sprite_name:
            sprite_index = self.tilesheet.tileset\
                .pngname_to_pngnum.get(sprite_name, 0)
            if sprite_index:
                sheet_type = 'filler' if self.tilesheet.is_filler else 'main'
                try:
                    self.tilesheet.tileset\
                        .unreferenced_pngnames[sheet_type].remove(sprite_name)
                except ValueError:
                    pass

                entry.append(sprite_index)
                return True

            print(f'Error: sprite {sprite_name} from {self.filepath} '
                  'has no matching PNG file. It will not be added to '
                  f'{self.tilesheet.tileset.output_conf_file}')
            self.tilesheet.tileset.error_logged = True
        return False


if __name__ == '__main__':
    # read arguments and initialize objects
    arg_parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    arg_parser.add_argument(
        'source_dir',
        help='Tileset source files directory path')
    arg_parser.add_argument(
        'output_dir', nargs='?',
        help='Output directory path')
    arg_parser.add_argument(
        '--use-all', dest='use_all', action='store_true',
        help='Add unused images with id being their basename')
    arg_parser.add_argument(
        '--obsolete-fillers', dest='obsolete_fillers', action='store_true',
        help='Warn about obsoleted fillers')
    arg_parser.add_argument(
        '--palette-copies', dest='palette_copies', action='store_true',
        help='Produce copies of tilesheets quantized to 8bpp colormaps.')
    arg_parser.add_argument(
        '--palette', dest='palette', action='store_true',
        help='Quantize all tilesheets to 8bpp colormaps.')
    args_dict = vars(arg_parser.parse_args())

    # compose the tileset
    try:
        tileset_worker = Tileset(
            args_dict.get('source_dir'),
            args_dict.get('output_dir') or args_dict.get('source_dir'),
            args_dict.get('use_all', False),
            args_dict.get('obsolete_fillers', False),
            args_dict.get('palette_copies', False),
            args_dict.get('palette', False)
        )
        tileset_worker.compose()
    except ComposingException as exception:
        sys.exit(exception)

    if tileset_worker.error_logged:
        sys.exit(1)
