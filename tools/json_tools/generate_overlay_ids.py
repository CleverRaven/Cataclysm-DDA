#!/usr/bin/env python3
"""
Attempts to generate a reasonable set of overlay IDs
that should be defined in a tileset.
Meant to be used along with tools/gfx_tools/list_tileset_ids.py

Google Docs Spreadsheet formulas:

Prune IDs from tileset for verification:
=REGEXREPLACE(A2,"_season_(spring|summer|autumn|winter)|_male|_female","")

Search for the pruned ID in overlays
=arrayformula(iferror(VLOOKUP($C2:$C,'overlays'!$A$1:$A,1,FALSE)))

Search in game IDs
=arrayformula(iferror(VLOOKUP($C2:$C,'all game IDs'!$B$2:$B,1,FALSE)))
"""

from itertools import product

from util import import_data


CPP_IDS = (
    'footstep', 'cursor', 'highlight', 'highlight_item', 'line_target',
    'line_trail', 'animation_line', 'animation_hit', 'zombie_revival_indicator'
)
ATTITUDES = ('hostile', 'neutral', 'friendly', 'other')

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
    'AMMO': {
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
    },
    'movement_mode': {
        'prefix': 'overlay_'
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
            output = [overlay_data['prefix']]
            output.extend(p)
            # print(output)
            print(''.join(output))

    for hardcoded_id in CPP_IDS:
        print(hardcoded_id)
    for attitude in ATTITUDES:
        print(f'overlay_{attitude}_sees_player')
