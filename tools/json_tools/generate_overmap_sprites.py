#!/usr/bin/env python3
"""
Generate sprites for mapgen IDs
"""
import argparse
import json

from pathlib import Path
from typing import Optional, Union

from PIL import Image

from util import import_data


TERRAIN_COLOR_NAMES = {}
SCHEME = None


def get_first_valid(
    value: Union[str, list],
    default: Optional[str] = None,
) -> str:
    """
    Return first str value from a list [of lists] that is not empty
    """
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
        'color_scheme', type=Path,
        help='path to a JSON color SCHEME',
    )
    args_dict = vars(arg_parser.parse_args())
    output_dir = args_dict.get('output_dir')
    color_scheme_path = args_dict.get('color_scheme')
    with open(color_scheme_path) as filehandler:
        global SCHEME
        SCHEME = json.load(filehandler, cls=TupleJSONDecoder)

    # TODO: support fallback_predecessor_mapgen
    SCHEME['terrain'][None] = (0, 0, 0, 0)

    terrain_data, errors = import_data(
        json_dir=Path('../../data/json/furniture_and_terrain/'),
        json_fmatch='terrain*.json'
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

    mapgen_data, errors = import_data(
        json_dir=Path('../../data/json/mapgen/'),
        json_fmatch='*.json'
    )
    if errors:
        print(errors)

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

        palettes = mapgen.get('palettes')
        if palettes:
            continue  # FIXME: mapgen palettes support

        terrain_dict = mapgen.get('terrain')
        rows = mapgen.get('rows')
        if not rows:
            continue

        fill_ter = mapgen.get('fill_ter')

        om_id = get_first_valid(om_id)

        image = generate_image(
            rows=rows, terrain_dict=terrain_dict, fill_ter=fill_ter)

        image.save(output_dir / f'{om_id}.png')


if __name__ == '__main__':
    main()
