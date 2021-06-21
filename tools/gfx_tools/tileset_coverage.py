#!/usr/bin/env python3
"""
Outer join game IDs with tileset IDs and output a CSV report
"""
import re
import sys

from typing import Union

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


def strip_overlay_id(overlay_id: str) -> str:
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


def get_data() -> tuple:
    """
    Load datasets with game IDs, tileset IDs and overlay IDs
    """
    all_game_ids = pandas.read_csv(
        GAME_IDS_FILENAME,
        warn_bad_lines=True,
    )
    overlay_ids = pandas.read_csv(
        OVERLAY_IDS_FILENAME,
        header=None,
        names=('overlay_id',),
        warn_bad_lines=True,
    )
    tileset_ids = pandas.read_csv(
        TILESET_IDS_FILENAME,
        header=None,
        names=('tileset_id',),
        warn_bad_lines=True,
    )
    return all_game_ids, overlay_ids, tileset_ids


def merge_datasets(
        all_game_ids: Union[dict, pandas.DataFrame],
        overlay_ids: Union[dict, pandas.DataFrame],
        tileset_ids: Union[dict, pandas.DataFrame],
        ) -> pandas.DataFrame:
    """
    Match IDs between game data, overlays and tileset

    >>> merge_datasets({
    ...     'id': ['a', 'b', 'c'],
    ...     'desc': ['desc a', 'desc b', 'desc c'],
    ... }, {
    ...     'overlay_id': [
    ...         'overlay_worn_a', 'overlay_worn_b', 'overlay_worn_c',
    ...         'overlay_wielded_a', 'overlay_wielded_b', 'overlay_wielded_c',
    ...     ],
    ... }, {
    ...     'tileset_id': ['a', 'b', 'overlay_worn_a', 'overlay_wielded_b'],
    ... })
      id    desc         overlay_id         tileset_id
    0  a  desc a                NaN                  a
    1  a     NaN     overlay_worn_a     overlay_worn_a
    2  a     NaN  overlay_wielded_a                NaN
    3  b  desc b                NaN                  b
    4  b     NaN     overlay_worn_b                NaN
    5  b     NaN  overlay_wielded_b  overlay_wielded_b
    6  c  desc c                NaN                NaN
    7  c     NaN     overlay_worn_c                NaN
    8  c     NaN  overlay_wielded_c                NaN
    """
    all_game_ids = pandas.DataFrame(all_game_ids)
    overlay_ids = pandas.DataFrame(overlay_ids)
    tileset_ids = pandas.DataFrame(tileset_ids)

    tileset_ids['id'] = tileset_ids['tileset_id'].apply(strip_overlay_id)

    # FIXME: output the original ID in the generate_overlay_ids.py
    overlay_ids['id'] = overlay_ids['overlay_id'].apply(strip_overlay_id)

    # game_data = pandas.merge(all_game_ids, overlay_ids, on='id')  # TODO

    result = all_game_ids.merge(
        tileset_ids,
        how='outer',
        on='id',
    )
    result = result.merge(
        overlay_ids,
        how='outer',
        left_on=['id', 'tileset_id'],
        right_on=['id', 'overlay_id'],
    )
    return result


def write_output(data: pandas.DataFrame, output_filename: str) -> int:
    """
    Write the resulting DataFrame to a file
    """
    result = data.to_csv(output_filename)
    if result is None:
        # error
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(write_output(merge_datasets(*get_data()), OUTPUT_FILENAME))
