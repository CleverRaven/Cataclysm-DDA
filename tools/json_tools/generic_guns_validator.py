#!/usr/bin/env python3

import os
import sys
import util

SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))
GG_DIR = os.path.normpath(os.path.join(
    SCRIPT_DIR, '..', '..', 'data', 'mods', 'Generic_Guns'))

# We might want to also warn about grenades and rockets, but leaving them
# whitelisted for now.
AMMO_TYPE_WHITELIST = {
    '40x46mm',  # Grenade
    'atgm',  # Rocket
    'atlatl',
    'bolt_ballista',
    'barb',
    'battery',
    'BB',
    'blunderbuss',
    'bolt',
    'cannon',
    'charcoal',
    'chemical_spray',
    'fishspear',
    'flammable',
    'm235',  # Rocket
    'metal_rail',
    'nail',
    'nuts_bolts',
    'oxygen',
    'paintball',
    'pebble',
    'plasma',
    'rock',
    'signal_flare',
    'methanol_fuelcell',
    'weldgas'
}
SKILL_WHITELIST = {
    'archery',
    'launcher',
}
# Whitelisted specific ids are mostly obsolete stuff.
# This could go away if obsolete stuff were explicitly marked as such.
ID_WHITELIST = {
    # Guns
    'coilgun',
    'slamfire_shotgun',
    'slamfire_shotgun_d',
    'ftk93',
    'l_bak_223',
    'pneumatic_shotgun',
    'rifle_223',
    # Magazines
    '223_speedloader5',
    'coin_wrapper',
    'exodiisapramag5',
    'robofac_gun_40mm_3rd',
    'robofac_gun_40mm_5rd',
    'robofac_gun_40mm_10rd',
    'bio_shotgun_gun',
    'gasfilter_med',

    'gasfilter_sm',
    'matchhead_30carbine',
    'matchhead_30carbine_jsp',
    "rebreather_cartridge",
    "rebreather_cartridge_air",
    "rebreather_cartridge_o2",
    "rebreather_cartridge_regen"
}


def items_of_type(data, type):
    result = []
    for i in data:
        if 'type' not in i:
            dump = util.CDDAJSONWriter(i).dumps()
            print("json entry has no 'type' field: " + dump)

            sys.exit(1)
        if i['type'] == type:
            result.append(i)
    return result


def get_ids(items):
    result = set()
    for i in items:
        id = i.get('id', [])
        if isinstance(id, str):
            result.add(id)
        else:
            result.update(id)
    return result


def get_id(i):
    if 'id' in i:
        return i['id']
    if 'abstract' in i:
        return i['abstract']
    raise RuntimeError('item lacks id: %r' % i)


def get_ancestors(items_by_id, item):
    result = [item]
    while 'copy-from' in item and item['copy-from'] in items_by_id:
        item = items_by_id[item['copy-from']]
        if item in result:
            raise RuntimeError(
                'Cyclic dependency in copy-from involving %s' %
                item['copy-from'])
        result.append(item)
    return result


def items_for_which_any_ancestor(items, pred):
    items_by_id = {get_id(i): i for i in items}
    result = []
    for i in items:
        if any(pred(ancestor) for ancestor in get_ancestors(items_by_id, i)):
            result.append(i)
    return result


def items_for_which_all_ancestors(items, pred):
    items_by_id = {get_id(i): i for i in items}
    result = []
    for i in items:
        if all(pred(ancestor) for ancestor in get_ancestors(items_by_id, i)):
            result.append(i)
    return result


def blacklisted_items(data):
    blacklists = items_of_type(data, 'ITEM_BLACKLIST')
    items = set()
    for blacklist in blacklists:
        if 'items' in blacklist:
            items.update(set(blacklist['items']))
    return items


def main():
    core_data, core_errors = util.import_data()
    print('Importing Generic Guns data from %r' % GG_DIR)
    gg_data, gg_errors = util.import_data(GG_DIR)

    if core_errors or gg_errors:
        print('Errors reading json:\n%s' % '\n'.join(core_errors + gg_errors))
        sys.exit(1)

    gg_migrations = get_ids(items_of_type(gg_data, 'MIGRATION'))
    gg_blacklist = blacklisted_items(gg_data)

    core_guns = items_of_type(core_data, 'GUN')

    def is_not_fake_item(i):
        return i.get('copy-from', '') != 'fake_item'

    def is_not_whitelisted_skill(i):
        return 'skill' in i and i['skill'] not in SKILL_WHITELIST

    def has_pockets(i):
        return 'pocket_data' in i

    def lacks_whitelisted_pocket(i):
        return not any(
            pocket.get('ammo_restriction', {}).keys() & AMMO_TYPE_WHITELIST
            for pocket in i.get('pocket_data', []))

    def can_be_unwielded(i):
        return 'NO_UNWIELD' not in i.get('flags', [])

    core_guns = items_for_which_all_ancestors(
        core_guns, is_not_fake_item)
    core_guns = items_for_which_any_ancestor(
        core_guns, is_not_whitelisted_skill)
    core_guns = items_for_which_any_ancestor(
        core_guns, has_pockets)
    core_guns = items_for_which_all_ancestors(
        core_guns, lacks_whitelisted_pocket)
    core_guns = items_for_which_all_ancestors(
        core_guns, can_be_unwielded)

    core_magazines = items_of_type(core_data, 'MAGAZINE')
    core_magazines = items_for_which_all_ancestors(
        core_magazines, lacks_whitelisted_pocket)

    core_ammo = items_of_type(core_data, 'AMMO')

    def is_not_whitelisted_ammo_type(i):
        return 'ammo_type' in i and i['ammo_type'] not in AMMO_TYPE_WHITELIST

    def is_bullet(i):
        return i.get('damage', {}).get('damage_type', '') == 'bullet'

    core_bullets = items_for_which_any_ancestor(core_ammo, is_bullet)
    core_bullets = items_for_which_any_ancestor(
        core_bullets, is_not_whitelisted_ammo_type)

    if (not gg_migrations or not core_guns or not core_magazines or
            not core_ammo):
        print('One of the collections is empty; something has gone wrong with '
              'data collection')
        return 1

    returncode = 0

    def check_missing(items, name):
        ids = get_ids(items) - ID_WHITELIST - gg_blacklist

        missing_migrations = ids - gg_migrations
        if missing_migrations:
            print('Missing Generic Guns migrations for these types of %s:' %
                  name)
            print('\n'.join(sorted(missing_migrations)))
            print()
            nonlocal returncode
            returncode = 1

    check_missing(core_bullets, 'ammo')
    check_missing(core_magazines, 'magazine')
    check_missing(core_guns, 'guns')

    if returncode:
        print('The above errors can be resolved by either adding suitable '
              'migrations to Generic Guns or adding to the whitelists of '
              'things not requiring migration in '
              'tools/json_tools/generic_guns_validator.py')

    return returncode


if __name__ == '__main__':
    sys.exit(main())
