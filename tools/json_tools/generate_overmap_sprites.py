#!/usr/bin/env python3
"""
Generate sprites for mapgen IDs
"""
import argparse

from pathlib import Path
from PIL import Image

from util import import_data


TERRAIN_COLORS = {
    't_dirt': (111, 111, 0, 255),
    't_grass': (0, 255, 0, 0),
}

COLORS = {
    'black': (0, 0, 0),
    'red': (255, 0, 0),
    'green': (0, 110, 0),
    'brown': (97, 56, 28),
    'blue': (10, 10, 220),
    'magenta': (139, 58, 98),
    'cyan': (0, 150, 180),
    'gray': (150, 150, 150),
    'dark_gray': (99, 99, 99),
    'light_red': (255, 150, 150),
    'light_green': (0, 255, 0),
    'yellow': (255, 255, 0),
    'light_blue': (100, 100, 255),
    'light_magenta': (254, 0, 254),
    'light_cyan': (0, 240, 255),
    'white': (255, 255, 255),
}


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
    args_dict = vars(arg_parser.parse_args())
    output_dir = args_dict.get('output_dir')

    terrain_colors = {}
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
            terrain_colors[terrain_id] = COLORS.get(
                terrain_color, (0, 0, 0, 0))

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
        while isinstance(om_id, list):  # FIXME: support splitting
            om_id = om_id[0]

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

        image = Image.new('RGBA', [len(rows[0]), len(rows)], 255)
        image_data = image.load()

        for index_x, row in enumerate(rows):
            for index_y, char in enumerate(row):
                terrain_id = terrain_dict.get(char, fill_ter)

                while isinstance(terrain_id, list):
                    terrain_id = terrain_id[0]

                color = TERRAIN_COLORS.get(terrain_id)
                if not color:
                    color = terrain_colors.get(terrain_id, (0, 0, 0, 0))
                image_data[index_y, index_x] = color

        image.save(output_dir / f'{om_id}.png')


if __name__ == '__main__':
    main()
