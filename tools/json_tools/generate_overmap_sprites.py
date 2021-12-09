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


def get_first_valid(
    value: Union[str, list, OrderedDict],
    default: Optional[str] = None,
) -> str:
    """
    Return first str value from a list [of lists] that is not empty
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
            yield om_ids[row][col], image.crop(box)


def read_scheme(
    color_scheme_path: Path,
) -> None:
    """
    Parse color scheme JSON into the SCHEME global
    """
    with open(color_scheme_path) as filehandler:
        global SCHEME
        SCHEME = json.load(filehandler, cls=TupleJSONDecoder)

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


def read_mapgen_palettes() -> None:
    """
    Fill the PALETTES global
    """
    palette_entries, errors = import_data(
        json_dir=Path('../../data/json/mapgen_palettes/'),
        json_fmatch='*.json',
    )
    if errors:
        print(errors)

    global PALETTES
    for entry in palette_entries:
        palette_id = entry.get('id')
        terrains = entry.get('terrain')
        if entry.get('type') == 'palette' and palette_id and terrains:
            if palette_id in PALETTES:
                print(f'WARNING: duplicate palette id {palette_id}')
            PALETTES[palette_id] = {
                key: value
                for key, value
                in terrains.items()
                if not isinstance(value, dict)
                and not isinstance(value, OrderedDict)
                # TODO: support whatever this is
            }


def get_mapgen_data() -> list:
    """
    Get all mapgen entries
    """
    mapgen_data, errors = import_data(
        json_dir=Path('../../data/json/mapgen/'),
        json_fmatch='*.json',
    )
    if errors:
        print(errors)

    return mapgen_data


def main():
    """
    main
    """
    arg_parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    arg_parser.add_argument(
        'output_dir', type=Path,
        help='Output directory path',
    )
    arg_parser.add_argument(
        'color_scheme', type=Path,  # TODO: default
        help='path to a JSON color SCHEME',
    )
    args_dict = vars(arg_parser.parse_args())
    output_dir = args_dict.get('output_dir')
    color_scheme_path = args_dict.get('color_scheme')

    read_scheme(color_scheme_path)

    read_terrain_color_names()

    read_mapgen_palettes()

    mapgen_data = get_mapgen_data()

    for entry in mapgen_data:
        entry_type = entry.get('type')
        if entry_type != 'mapgen':
            continue

        method = entry.get('method')
        if method != 'json':
            continue

        om_id = entry.get('om_terrain')
        if not om_id:
            continue

        mapgen = entry.get('object')
        if not mapgen:
            continue

        terrain_dict = {}
        palettes = mapgen.get('palettes')
        if palettes:
            for palette_id in palettes:
                if isinstance(palette_id, OrderedDict):
                    palette_id = get_first_valid(palette_id.get('disribution'))
                palette = PALETTES.get(palette_id)
                if palette:
                    terrain_dict.update(palette)
                else:
                    print(f'WARNING: unknown palette {palette_id}')
                # TODO: support nested palettes, two cases found

        terrain_defs = mapgen.get('terrain')
        if terrain_defs:
            terrain_dict.update(terrain_defs)

        if not terrain_dict:
            print(f'skipping {om_id} because of empty terrain defs')
            continue

        rows = mapgen.get('rows')
        if not rows:
            continue

        fill_ter = mapgen.get('fill_ter')

        image = generate_image(
            rows=rows, terrain_dict=terrain_dict, fill_ter=fill_ter)

        if isinstance(om_id, list) and isinstance(om_id[0], list):
            generator = split_image(image=image, om_ids=om_id)
            for submap_id, submap_image in generator:
                submap_image.save(output_dir / f'{submap_id}.png')

        else:
            om_id = get_first_valid(om_id)
            image.save(output_dir / f'{om_id}.png')


if __name__ == '__main__':
    main()
