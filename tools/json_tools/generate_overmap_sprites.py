#!/usr/bin/env python3
"""
Generate sprites for mapgen IDs
"""
import argparse
import json

from collections import OrderedDict
from pathlib import Path
from typing import Optional, Union

from PIL import Image

from util import import_data


SIZE = 24
TERRAIN_COLOR_NAMES = {}
PALETTES = {}
SCHEME = None
TILE_ENTRY_TEMPLATE = {
    'id': '',
    'fg': '',
    'rotates': True,
}
CREATED_IDS = set()
VARIANTS = dict()
SKIPPED = {
    # 'single_color': set(),
    'duplicate': set(),
    'predecessor_mapgen': list(),
    'no_rows': list(),
}


def get_first_valid(
    value: Union[str, list, OrderedDict],
    default: Optional[str] = None,
) -> str:
    """
    Return first str value from a list [of lists] that is not empty

    TODO: support all quirks properly and get rid of this function
    """
    if isinstance(value, OrderedDict):
        return value.get('fallback')

    while isinstance(value, list):
        for item in value:
            if item:
                value = item
                break
    if value:
        return value
    return default


class TupleJSONDecoder(json.JSONDecoder):
    """
    Decode JSON lists as tuples
    https://stackoverflow.com/a/10889125
    """

    def __init__(self, **kwargs):
        json.JSONDecoder.__init__(self, **kwargs)
        # Use the custom JSONArray
        self.parse_array = self.JSONArray
        # Use the python implemenation of the scanner
        self.scan_once = json.scanner.py_make_scanner(self)

    def JSONArray(self, s_and_end, scan_once, **kwargs):
        values, end = json.decoder.JSONArray(s_and_end, scan_once, **kwargs)
        return tuple(values), end


def generate_image(
    rows: list,
    terrain_dict: dict,
    fill_ter: str,
) -> Image:
    """
    Generate sprite from rows
    """
    image = Image.new('RGBA', [len(rows[0]), len(rows)], 255)
    image_data = image.load()
    for index_x, row in enumerate(rows):
        for index_y, char in enumerate(row):
            terrain_id = get_first_valid(terrain_dict.get(char), fill_ter)
            # TODO: skip on t_secretdoor_metal_c ?

            color = SCHEME['terrain'].get(terrain_id)
            if not color:
                color = SCHEME['colors'].get(
                    TERRAIN_COLOR_NAMES.get(terrain_id))
            if not color:
                color = 0, 0, 0, 0
            try:
                image_data[index_y, index_x] = color
            except TypeError:
                print(color)
                raise

    return image


def split_image(
    image: Image,
    om_ids: list,
) -> tuple:
    """
    Split image into SIZExSIZE sprites, yield (id, image) pairs
    """
    width, height = image.size
    for row in range(height // SIZE):
        for col in range(width // SIZE):
            box = (col * SIZE, row * SIZE, (col + 1) * SIZE, (row + 1) * SIZE)
            output = image.crop(box)
            yield om_ids[row][col], output


def read_scheme(
    color_scheme_path: Path,
) -> None:
    """
    Parse color scheme JSON into the SCHEME global
    """
    with open(color_scheme_path) as file:
        global SCHEME
        SCHEME = json.load(file, cls=TupleJSONDecoder)

    # TODO: support fallback_predecessor_mapgen
    SCHEME['terrain'][None] = 0, 0, 0, 0


def read_terrain_color_names() -> None:
    """
    Fill the TERRAIN_COLOR_NAMES global
    """
    terrain_data, errors = import_data(
        json_dir=Path('../../data/json/furniture_and_terrain/'),
        json_fmatch='terrain*.json',
    )
    if errors:
        print(errors)

    for terrain in terrain_data:
        terrain_type = terrain.get('type')
        terrain_id = terrain.get('id')
        terrain_color = terrain.get('color')
        if isinstance(terrain_color, list):
            terrain_color = terrain_color[0]
        if terrain_type == 'terrain' and terrain_id and terrain_color:
            TERRAIN_COLOR_NAMES[terrain_id] = terrain_color


def add_palette(
    entry: dict,
) -> None:
    """
    Add new palette to the PALETTES global
    """
    if entry.get('type') != 'palette':
        return

    terrains = entry.get('terrain')
    if terrains is None:
        terrains = {}

    palette_id = entry.get('id')
    global PALETTES

    if palette_id in PALETTES:
        raise Exception(f'WARNING: duplicate palette id {palette_id}')

    PALETTES[palette_id] = {
        key: value for
        key, value in
        terrains.items() if
        not isinstance(value, dict) and
        not isinstance(value, OrderedDict)
        # TODO: support whatever this is
    }


def read_mapgen_palettes() -> None:
    """
    Fill the PALETTES global

    FIXME: read from mapgen folder too
    """
    palette_entries, errors = import_data(
        json_dir=Path('../../data/json/mapgen_palettes/'),
        json_fmatch='*.json',
    )
    if errors:
        print(errors)

    for entry in palette_entries:
        add_palette(entry)


def get_mapgen_data(
    mapgen_dir: Path,
    pattern: str,
) -> list:
    """
    Get all mapgen entries
    """
    mapgen_data, errors = import_data(
        json_dir=mapgen_dir,
        json_fmatch=pattern,
    )
    if errors:
        print(errors)

    return mapgen_data


def output_image(
    name: str,
    image: Image,
    generate_json: bool,
    output_dir: Optional[Path] = None,
    duplicates_dir: Optional[Path] = None,
    # single_terrain: bool = False
) -> None:
    """
    Save image to disk
    """
    if name not in CREATED_IDS:
        # new ID

        # if len(image.getcolors()) < 2 and not single_terrain:
        #    SKIPPED['single_color'].add(name)
        #    return

        if output_dir is None:
            CREATED_IDS.add(name)
            return

        if generate_json:
            tile_entry = TILE_ENTRY_TEMPLATE.copy()
            tile_entry['id'] = name
            tile_entry['fg'] = name
            filepath = output_dir / f'{name}.json'
            with open(filepath, 'w') as file:
                json.dump(tile_entry, file)

        image.save(output_dir / f'{name}.png')
        CREATED_IDS.add(name)
        return

    if not duplicates_dir:
        # skipping as duplicate
        SKIPPED['duplicate'].add(name)
        return

    duplicates_subdir = duplicates_dir / name

    if name not in VARIANTS:
        # newly discovered ID with variants, move the previously generated one
        # into a duplicates subdirectory
        first = output_dir / f'{name}.png'

        duplicates_subdir.mkdir(parents=True, exist_ok=True)
        first.rename(duplicates_subdir / f'{name}.png')
        VARIANTS[name] = 2

    else:
        # ID is known to have variants, just increment the counter
        VARIANTS[name] += 1

    image.save(duplicates_subdir / f'{name}_{VARIANTS[name]}.png')


def main():
    """
    main
    """
    # get arguments
    arg_parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    arg_parser.add_argument(
        'color_scheme', type=Path,  # TODO: default
        help='path to a JSON color SCHEME',
    )
    arg_parser.add_argument(
        'output_dir', nargs='?', type=Path,
        help='Output directory path',
    )
    arg_parser.add_argument(
        '--json', dest='json', action='store_true',
        help='Generate JSON files for correct rotations',
    )
    arg_parser.add_argument(
        '--mapgen-dir', dest='mapgen_dir', type=Path,
        default=Path('../../data/json/mapgen/'),
        help='directory with mapgen entries',
    )
    arg_parser.add_argument(
        '--mapgen-files-pattern', dest='mapgen_files_pattern', type=str,
        default='*.json',
        help='filename template for selecting a subset '
        'of JSON files in the mapgen dir',
    )
    arg_parser.add_argument(
        '--duplicates-dir', dest='duplicates_dir', type=Path,
        help='directory for putting duplicate ID sprites into. '
        'Should not be in the output_dir when using compose.py --use-all',
    )
    args_dict = vars(arg_parser.parse_args())

    output_dir = args_dict.get('output_dir', None)
    duplicates_dir = args_dict.get('duplicates_dir', None)
    color_scheme_path = args_dict.get('color_scheme')
    generate_json = args_dict.get('json')

    # read needed values from the data/json/ subdirectories
    read_scheme(color_scheme_path)

    read_terrain_color_names()

    read_mapgen_palettes()

    mapgen_data = get_mapgen_data(
        mapgen_dir=args_dict.get('mapgen_dir'),
        pattern=args_dict.get('mapgen_files_pattern'),
    )

    # get palettes
    for entry in mapgen_data:
        if entry.get('type') == 'palette':
            add_palette(entry)

    # main loop over mapgens
    for entry in mapgen_data:
        # check type
        if entry.get('type') != 'mapgen':
            continue

        # check method, only json supported currently
        if entry.get('method') != 'json':
            continue

        om_id = entry.get('om_terrain')
        if not om_id or om_id == 'FEMA_entrance':
            continue

        # verify that "object" value is defined
        mapgen = entry.get('object')
        if not mapgen:
            continue

        # combine "palettes" value with "terrain" to get all terrain values
        terrain_dict = {}
        palettes = mapgen.get('palettes')
        if palettes:
            for palette_id in palettes:
                if isinstance(palette_id, OrderedDict):
                    palette_id = get_first_valid(
                        palette_id.get('distribution')
                    )
                palette = PALETTES.get(palette_id)
                if palette is not None:
                    terrain_dict.update(palette)
                else:
                    print(f'WARNING: unknown palette {palette_id}')
                # TODO: support nested palettes, two cases found

        terrain_defs = mapgen.get('terrain')
        if terrain_defs:
            terrain_dict.update(terrain_defs)

        # single_terrain = False
        # if not terrain_dict:
        #    single_terrain = True

        # verify "rows" is not empty
        rows = mapgen.get('rows')
        fill_ter = mapgen.get('fill_ter')

        if not rows:
            if fill_ter:
                rows = [' ' * SIZE] * SIZE
            else:
                if mapgen.get('predecessor_mapgen'):
                    SKIPPED['predecessor_mapgen'].append(om_id)
                else:
                    SKIPPED['no_rows'].append(om_id)
                continue

        # create the sprite
        image = generate_image(
            rows=rows, terrain_dict=terrain_dict, fill_ter=fill_ter)

        # write sprite[s] to the output directory
        if isinstance(om_id, list) and isinstance(om_id[0], list):
            generator = split_image(image=image, om_ids=om_id)
            for submap_id, submap_image in generator:
                output_image(
                    name=submap_id,
                    image=submap_image,
                    generate_json=generate_json,
                    output_dir=output_dir,
                    duplicates_dir=duplicates_dir,
                    # single_terrain=single_terrain,
                )

        else:
            om_id = get_first_valid(om_id)
            output_image(
                name=om_id,
                image=image,
                generate_json=generate_json,
                output_dir=output_dir,
                duplicates_dir=duplicates_dir,
                # single_terrain=single_terrain,
            )


if __name__ == '__main__':
    main()

    print(f'generated {len(CREATED_IDS)} sprites')
    print('Skipped:')
    for reason, skipped_values in SKIPPED.items():
        print(reason, len(skipped_values))
        print(skipped_values or '')
