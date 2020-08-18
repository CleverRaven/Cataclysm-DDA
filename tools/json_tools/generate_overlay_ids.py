#!/usr/bin/env python3
"""
Attempts to generate a reasonable set of overlay IDs
that should be defined in a tileset.
Meant to be used along with tools/gfx_tools/list_tileset_ids.py
"""

from itertools import product

from util import import_data


TILESET_OVERLAY_TYPES = {
    'effect_type': {
        'prefix': 'overlay_effect_'
    },
    'mutation': {
        'prefix': 'overlay_mutation_'
    },
    'bionic': {
        'prefix': 'overlay_mutation_'
    },
    'ARMOR': {
        'prefix': 'overlay_worn_'
    },
    'TOOL_ARMOR': {
        'prefix': 'overlay_worn_'
    },
    'GENERIC': {
        'prefix': 'overlay_wielded_'
    },
    'GUN': {
        'prefix': 'overlay_wielded_'
    },
    'TOOL': {
        'prefix': 'overlay_wielded_'
    },
    'ENGINE': {
        'prefix': 'overlay_wielded_'
    },
    'WHEEL': {
        'prefix': 'overlay_wielded_'
    },
    'COMESTIBLE': {
        'prefix': 'overlay_wielded_'
    },
    'MONSTER': {
        'prefix': ''
    }
}


if __name__ == '__main__':
    data = import_data()[0]

    for datum in data:
        datum_type = datum.get('type')
        overlay_data = TILESET_OVERLAY_TYPES.get(datum_type, None)
        if not overlay_data:
            continue

        game_id = datum.get('id')
        flags = datum.get('flags', tuple())

        if not game_id:
            continue
        if datum.get('asbstract'):
            continue
        if 'PSEUDO' in flags or 'NO_DROP' in flags:
            continue
        if datum.get('copy-from') in ('fake_item', 'software'):
            continue

        variable_prefix = ('',)
        if 'BIONIC_TOGGLED' in flags:
            variable_prefix = ('', 'active_')
        if datum_type == 'MONSTER':
            variable_prefix = ('corpse_', 'overlay_wielded_corpse_')

        for p in product(
                variable_prefix,
                (game_id,)):
            output = [overlay_data['prefix'],]
            output.extend(p)
            # print(output)
            print(''.join(output))
