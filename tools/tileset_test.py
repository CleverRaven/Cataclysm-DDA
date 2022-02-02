#!/usr/bin/env python3
'''
Creates a map folder with one of every item, furniture, terrain and trap.
Everything is spawned starting from submap 0,0.
Run this script with -h for full usage information.

Examples:

    %(prog)s data/json save/worldname
    %(prog)s data/mods/Magiclysm .
    %(prog)s data/mods/Magiclysm data/mods/Aftershock .
    %(prog)s . /tmp -f
'''

import argparse
import pathlib
import json
import math

'''
A map file is divided into 4 sections of 12x12 tiles each
'''

JSON_INFO = {
    'items': {
        'allowed_types': ['GENERIC', 'AMMO', 'MAGAZINE',
                          'MAGAZINE', 'TOOL', 'PET_ARMOR', 'BOOK',
                          'BIONIC_ITEM', 'COMESTIBLE', 'GUN', 'GUNMOD',
                          'BATTERY'],
        'tiles_per': 1
    },
    'furniture': {
        'allowed_types': ['furniture'],
        'tiles_per': 9
    },
    'terrain': {
        'allowed_types': ['terrain'],
        'tiles_per': 9
    },
    'trap': {
        'allowed_types': ['trap'],
        'tiles_per': 9
    }
}


def parse_json(path: list) -> dict:
    data_folders = []
    for data_folder in path:
        data_folder = pathlib.Path(data_folder)
        if not data_folder.exists or not data_folder.is_dir:
            return False
        data_folders.append(data_folder)

    parsed_json = dict()

    for _type in JSON_INFO:
        parsed_json[_type] = []

    for data_folder in data_folders:
        for file in data_folder.glob('**/*.json'):
            json_file = None
            with open(file, 'r') as f:
                json_file = json.load(f)

            if type(json_file) is not list:
                continue

            for _type, info in JSON_INFO.items():
                for entry in json_file:
                    if ('type' not in entry or
                        # abstract item definitions
                        'id' not in entry or
                            entry['type'] not in info['allowed_types']):
                        continue

                    parsed_json[_type].append(entry['id'])

    return parsed_json


class QuarterMap:

    def __init__(self, row, column):
        self.index = 0
        self.version = 33
        self.terrain = ['t_grass'] * 144
        self.furniture = []
        self.traps = []
        self.items = []
        self.coordinate = [row, column, 0]

    def add_item(self, itype: str) -> None:
        self.items.append({
            'row': self.index % 12,
            'column': self.index // 12,
            'typeid': itype
        })
        self.terrain[self.index] = 't_concrete'
        self.index += 1

    def add_furniture(self, ftype: str) -> None:
        index_values = (1, 4, 7, 10)
        furn_index = index_values[self.index // 4] * 12
        furn_index += index_values[self.index % 4]

        self.furniture.append({
            'row': furn_index % 12,
            'column': furn_index // 12,
            'typeid': ftype
        })

        self.terrain[furn_index] = 't_concrete'
        self.index += 1

    def add_terrain(self, ttype: str) -> None:
        index_values = (0, 3, 6, 9)
        ter_index = index_values[self.index // 4] * 12
        ter_index += index_values[self.index % 4]

        for i in range(3):
            for j in range(3):
                self.terrain[ter_index + 12 * i + j] = ttype
        self.index += 1

    def add_trap(self, trtype: str) -> None:
        index_values = (1, 4, 7, 10)
        trap_index = index_values[self.index // 4] * 12
        trap_index += index_values[self.index % 4]

        self.traps.append({
            'row': trap_index % 12,
            'column': trap_index // 12,
            'typeid': trtype
        })

        self.terrain[trap_index] = 't_concrete'
        self.index += 1

    def as_string(self, ident=0) -> str:
        identstr = '\t' * ident
        retstr = identstr + '{\n'
        retstr += identstr + f'\t"version": {self.version},\n'

        retstr += identstr + '\t"coordinates": [\n'
        retstr += identstr + f'\t\t{self.coordinate[0]},\n'
        retstr += identstr + f'\t\t{self.coordinate[1]},\n'
        retstr += identstr + f'\t\t{self.coordinate[2]}\n'
        retstr += identstr + '\t],\n'

        retstr += identstr * ident + '\t"terrain": [\n'
        for key in range(144):
            if key < len(self.terrain):
                entry = self.terrain[key]
            else:
                entry = 't_grass'
            retstr += '\t\t'
            if type(entry) is str:
                retstr += identstr + f'"{entry}"'
            else:
                retstr += identstr + f'[ "{entry[0]}", {entry[1]} ]'
            if key != 143:
                retstr += ','
            retstr += '\n'
        retstr += identstr + '\t],\n'

        retstr += identstr * ident + '\t"items": [\n'
        for key, entry in enumerate(self.items):
            retstr += '\t\t'
            retstr += identstr + \
                f'{entry["row"]}, {entry["column"]}, ' \
                f'[{{ "typeid": "{entry["typeid"]}" }}]'
            if key != len(self.items) - 1:
                retstr += ','
            retstr += '\n'
        retstr += identstr + '\t],\n'

        retstr += identstr * ident + '\t"furniture": [\n'
        for key, entry in enumerate(self.furniture):
            retstr += identstr
            retstr += "\t\t[ " + \
                f'{entry["row"]}, {entry["column"]}, ' \
                f'"{entry["typeid"]}" ]'
            if key != len(self.furniture) - 1:
                retstr += ','
            retstr += '\n'
        retstr += identstr + '\t],\n'

        retstr += identstr * ident + '\t"traps": [\n'
        for key, entry in enumerate(self.traps):
            retstr += identstr
            retstr += "\t\t[ " + \
                f'{entry["row"]}, {entry["column"]}, ' \
                f'"{entry["typeid"]}" ]'
            if key != len(self.traps) - 1:
                retstr += ','
            retstr += '\n'
        retstr += identstr + '\t]\n'

        retstr += identstr + '}'
        return retstr

    def __str__(self) -> str:
        return self.as_string()


class FullMap:

    def __init__(self, offset_x: int, offset_y: int):
        self._map = []
        self._map.append(QuarterMap(offset_x, offset_y))
        self._map.append(QuarterMap(offset_x, offset_y + 1))
        self._map.append(QuarterMap(offset_x + 1, offset_y))
        self._map.append(QuarterMap(offset_x + 1, offset_y + 1))

    def fill_items(self, items) -> None:
        i = 0
        while True:
            map_index = i // 144
            if i == 576 or i == len(items):
                break

            self._map[map_index].add_item(items[i])
            i += 1

    def fill_furniture(self, furniture) -> None:
        i = 0
        while True:
            map_index = i // 16
            if i == 64 or i == len(furniture):
                break

            self._map[map_index].add_furniture(furniture[i])
            i += 1

    def fill_terrain(self, terrain) -> None:
        i = 0
        while True:
            map_index = i // 16
            if i == 64 or i == len(terrain):
                break

            self._map[map_index].add_terrain(terrain[i])
            i += 1

    def fill_traps(self, traps) -> None:
        i = 0
        while True:
            map_index = i // 16
            if i == 64 or i == len(traps):
                break

            self._map[map_index].add_trap(traps[i])
            i += 1

    def __str__(self) -> str:
        retstr = '[\n'
        for i in range(4):
            qret = self._map[i].as_string(1)
            if i != 3:
                qret += ',\n'
            retstr += qret
        retstr += '\n]'
        return retstr


class PathNotExist(Exception):
    def __init__(self, path: pathlib.Path):
        super().__init__(f'path does not exist: {path}')


class MapsFolderExists(Exception):
    def __init__(self, path: pathlib.Path):
        super().__init__(f'maps folder already exists: {path}')


class HandlerUnhandledType(Exception):
    def __init__(self, key):
        super().__init__(f'key not handled by Handler: {key}')


class Handler:

    def __init__(self, path: pathlib.Path, parsed_ids: dict, dry_run: bool,
                 force: bool):
        self.dry_run = dry_run
        self.force = force
        self.maps_folder = pathlib.Path(path)
        if not self.maps_folder.exists():
            raise PathNotExist(self.maps_folder)
        self.maps_folder = self.maps_folder.joinpath('maps')
        if self.maps_folder.exists() and not (self.dry_run or self.force):
            raise MapsFolderExists(self.maps_folder)

        self._maps = dict()
        square_offset = 0
        for key, value in parsed_ids.items():
            if key not in JSON_INFO:
                raise HandlerUnhandledType(key)

            # 576 = 4 * 12 * 12 = Tiles in a .map file
            total_squares = math.ceil(
                JSON_INFO[key]['tiles_per'] * len(value) / 576)
            square_length = math.ceil(math.sqrt(total_squares))
            amount = 576 // JSON_INFO[key]['tiles_per']

            counter = 0
            for i in range(total_squares):
                row = i % square_length + square_offset
                column = i // square_length
                fmap = FullMap(2 * row, 2 * column)

                if key == 'items':
                    fmap.fill_items(value[counter:counter + amount])
                elif key == 'furniture':
                    fmap.fill_furniture(value[counter:counter + amount])
                elif key == 'terrain':
                    fmap.fill_terrain(value[counter:counter + amount])
                elif key == 'trap':
                    fmap.fill_traps(value[counter:counter + amount])

                self._maps[f'{row}.{column}'] = fmap
                counter += amount  # 4 x ( 12 x 12 )

            square_offset += square_length

    def save_maps(self) -> None:
        for _map in self._maps:
            row = int(_map.split('.')[0])
            column = int(_map.split('.')[1])
            folder = pathlib.Path.joinpath(self.maps_folder,
                                           f'{row//32}.{column//32}.0')
            if not self.dry_run:
                folder.mkdir(parents=True, exist_ok=True)

            file_path = folder.joinpath(f'{row}.{column}.0.map')
            if not self.dry_run:
                with open(file_path, 'w') as f:
                    f.write(self._maps[_map].__str__())
                    f.write('\n')
            else:
                print(file_path)
                print(self._maps[_map], end='\n\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawTextHelpFormatter,
    )

    parser.add_argument('data', nargs='+', type=str,
                        help='json files location')

    parser.add_argument(
        'path', type=str, help='folder where the maps folder will be created'
    )

    parser.add_argument('-f', '--force', action='store_true',
                        help='overwrite the maps folder if it already exists')

    parser.add_argument('-n', '--dry_run',
                        action='store_true',
                        help='print to terminal instead of creating files')

    args = parser.parse_args()
    handler = Handler(args.path, parse_json(args.data), args.dry_run,
                      args.force)
    handler.save_maps()
