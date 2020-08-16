#!/usr/bin/env python3
"""
Attempts to generate a reasonable set of overlay IDs
that should be defined in a tileset.
Meant to be used along with tools/gfx_tools/list_tileset_ids.py
"""

from itertools import product

from util import import_data


PREFIX = 'overlay_'
GENDERS = ['male_', 'female_']
TILESET_OVERLAY_TYPES = {
    'effect_type': {
        'prefix': 'effect_'
    },
    'mutation': {
        'prefix': 'mutation_',
        'gendered': True,
    },
    'bionic': {
        'prefix': 'mutation_',
        'gendered': True,
    },
    'ARMOR': {
        'prefix': 'worn_',
        'gendered': True,
    },
    'TOOL_ARMOR': {
        'prefix': 'worn_',
        'gendered': True,
    },
    'GENERIC': {
        'prefix': 'wielded_'
    },
    'GUN': {
        'prefix': 'wielded_'
    },
    'TOOL': {
        'prefix': 'wielded_'
    },
    'ENGINE': {
        'prefix': 'wielded_'
    },
    'WHEEL': {
        'prefix': 'wielded_'
    },
    'COMESTIBLE': {
        'prefix': 'wielded_'
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
            variable_prefix = ('active_', '')
        genders = GENDERS if overlay_data.get('gendered') else ('',)

        for p in product(
                genders,
                (overlay_data['prefix'],),
                variable_prefix,
                (game_id,)):
            output = [PREFIX]
            output.extend(p)
            # print(output)
            print(''.join(output))
