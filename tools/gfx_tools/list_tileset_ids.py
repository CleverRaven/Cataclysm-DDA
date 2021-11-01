#!/usr/bin/env python3
"""
List all IDs that are present in a tileset

./list_tileset_ids.py ../../gfx/UltimateCataclysm/
"""

import argparse
import json
import os


TILE_CONFIG_NAME = 'tile_config.json'
SPRITES_LIST_KEY = 'tiles-new'
TILES_LIST_KEY = 'tiles'

parser = argparse.ArgumentParser(
    description="List all IDs that are present in a tileset")
parser.add_argument("tileset_dir", action="store",
                    help="local name of the tileset directory")
args = parser.parse_args()

tileset_dirname = args.tileset_dir

# loading data
with open(
        os.path.join(tileset_dirname, TILE_CONFIG_NAME),
        encoding="utf-8") as fh:
    sprites = json.load(fh)[SPRITES_LIST_KEY]

for sprite_data in sprites:
    for tile in sprite_data[TILES_LIST_KEY]:
        tile_id_container = tile['id']
        if isinstance(tile_id_container, list):
            for tile_id in tile_id_container:
                print(tile_id)
        else:
            print(tile_id_container)
