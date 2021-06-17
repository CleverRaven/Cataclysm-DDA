#!/usr/bin/env python3
"""
Outer join game IDs with tileset IDs and output a CSV report
"""
import re

import pandas


GAME_IDS_FILENAME = 'all_game_ids.csv'
TILESET_IDS_FILENAME = 'tileset_ids.csv'
OVERLAY_IDS_FILENAME = 'all_overlay_ids.csv'
OUTPUT_FILENAME = 'tileset_coverage.csv'

REPLACEMENTS = (
    r'^overlay_(.+)_sees_player$',
)

DELETION_RE = (
    r'_season_(spring|summer|autumn|winter)|'
    r'^\[|'
    r'\]$|'
    r'^overlay(_(male|female))?_(effect|mutation|worn|wielded(_corpse)?)_|'
    r'^overlay_|'
    r'^corpse_|'
    r'^vp_|'
    r'(_('
    r'cover|cross|horizontal|horizontal_2|vertical|vertical_2|ne|nw|se|sw'
    r'))?'
    r'(_('
    r'unconnected|left|right|rear|front'
    r'))?'
    r'(_edge)?'
    r'$'
)


def strip_overlay_id(overlay_id):
    """
    Extract game ID from an overlay ID string

    >>> strip_overlay_id('vp_wheel_wood')
    'wheel_wood'
    >>> strip_overlay_id('vp_reinforced_windshield_front_edge')
    'reinforced_windshield'
    >>> strip_overlay_id('vp_wheel_wheelchair')
    'wheel_wheelchair'
    >>> strip_overlay_id('vp_frame_wood_vertical_2_unconnected')
    'frame_wood'

    """
    stripped_id = overlay_id

    for pattern in REPLACEMENTS:
        stripped_id = re.sub(
            pattern,
            r'\g<1>',
            overlay_id,
        )

    stripped_id = re.sub(
        DELETION_RE,
        '',
        stripped_id,
    )

    return stripped_id


def main():
    """
    Executed when the file is run directly
    """
    all_game_ids = pandas.read_csv(
        GAME_IDS_FILENAME,
        warn_bad_lines=True,
    )
    tileset_ids = pandas.read_csv(
        TILESET_IDS_FILENAME,
        header=None,
        names=('tileset_id',),
        warn_bad_lines=True,
    )
    overlay_ids = pandas.read_csv(
        OVERLAY_IDS_FILENAME,
        header=None,
        names=('overlay_id',),
        warn_bad_lines=True,
    )

    tileset_ids['id'] = tileset_ids['tileset_id'].apply(strip_overlay_id)

    result = all_game_ids.merge(
        tileset_ids,
        how='outer',
        on='id',
    )
    result = result.merge(
        overlay_ids,
        how='outer',
        left_on='tileset_id',
        right_on='overlay_id',
    )
    result.to_csv(OUTPUT_FILENAME)


if __name__ == '__main__':
    main()
