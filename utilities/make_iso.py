#!/usr/bin/env python3
# pylint: disable=C0103
# pylint: disable=C0301
"""Convert a Cataclysm-DDA tileset to isometric view.
Run this script with -h for full usage information.

Examples:

    %(prog)s gfx/UltimateCataclysm
    %(prog)s gfx/BrownLikeBears

Isometric tiles are saved to a new folder like gfx/UltimateCataclysm_iso.
"""

import os
import json
import argparse
import datetime
import math

# when a 2d sprite 'stands up' on an isometric tile bae, like a tree
# how far up should it be from the bottom corner of the tile?
# offset=0 will align bottom of sprite with bottom (SW corner) of square
# offset<1 will be a fraction of the isometric tile height
# offset>=1 will be an absolute number of pixels
SPRITE_OFFSET_FROM_BOTTOM = 1.0 / 8

parser = argparse.ArgumentParser(
    description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)

parser.add_argument('tileset', help='name of the tileset directory to convert')

parser.add_argument('-f', dest='floodfill', action='store_true',
                    help='use filled background instead of transparent')

args = parser.parse_args()

# Indexed by (tile_num, initial_rotation) tuple, True if iso-converted
converted_tile_ids = dict()


def iso_ize(tile_num, new_tile_num=-1, initial_rotation=0, override=False):
    if override or (tile_num, initial_rotation) not in converted_tile_ids:
        print("  iso-izing " + str(tile_num))
        if new_tile_num == -1:
            new_tile_num = tile_num
        tile_png = "tile-{:0>6d}.png".format(tile_num)
        command = (
            'convert -background transparent ' +
            new_tileset_name + '/tiles/' + tile_png +
            ' -rotate ' + str(initial_rotation) + ' ' +
            '-rotate -45 +repage -resize 100%x50% ' +
            '-crop ' + str(nwidth) + 'x' + str(int(nwidth / 2)) +
            '+2+1 ' +  # TODO: get correct offsets
            '+repage -extent ' + str(nwidth) + 'x' + str(nheight) +
            '+0-' + str(nheight - int(nwidth / 2)) + ' ' +
            new_tileset_name + '/tiles/to_merge/' + tile_png)
        print(command)
        if os.system(command):
            raise RuntimeError("iso-ization failed for %s" % tile_png)
        else:
            converted_tile_ids[(tile_num, initial_rotation)] = True
            return True
    return False


# convert one old tile definition to one new tile definition
# recursive for additional-tiles
def tile_convert(otile, main_id, new_tile_number):
    print("tile_convert %s %s" % (main_id, new_tile_number))
    ntile = dict()
    ntile['id'] = otile['id']
    ntile['ntn'] = int(new_tile_number)

    #TODO: handle different tile transformations based on id
    #TODO: load other json files to base transforms on stats, not just id

    if 'rotates' in otile:
        ntile['rotates'] = otile['rotates']
    if 'multitile' in otile:
        ntile['multitile'] = otile['multitile']

    for g in ('fg', 'bg'):
        if g not in otile:
            continue
        if type(otile[g]) == int:
            if otile[g] == -1:
                continue
            otile[g] = list([otile[g]])

        add_tile_ids = [
            'broken',
            'open',
            'unconnected',
            'center',
            'corner',
            'edge',
            'end_piece',
            't_connection',
        ]

        # tile ids to iso-ify:
        # t_*.bg, .fg iff .bg is missing
        # t_*.additional_tiles.bg, .fg iff .bg is missing
        # lighting_*.bg, .fg iff .bg is missing
        # vp_*.bg and .fg
        # fd_*.bg and .fg
        # fd_*.additional_tiles.fg and .bg

        if len(otile[g]) == 0:
            continue
        elif len(otile[g]) == 1:
            ntile[g] = otile[g]
            # FIXME: These are factored out to (slightly) simplify the
            # conditional expression below, but it still needs cleanup
            terrain_or_lighting = isinstance(main_id, str) and (
                main_id.startswith('t_') or main_id.startswith('lighting_'))
            vehicle_part_or_field = isinstance(main_id, str) and (
                main_id.startswith('vp_') or main_id.startswith('fd_'))
            # iso-ize?
            if (
                (terrain_or_lighting and (g == 'bg' or 'bg' not in otile)) or
                (main_id[0:1] == ('f_') and g == 'bg' and 'fg' in otile) or
                vehicle_part_or_field or
                # additional_tiles:
                otile['id'] in add_tile_ids or
                ('rotates' in otile and otile['rotates'] is True) or
                otile['id'] != main_id
            ):
                # iso-ize this tile
                iso_ize(otile[g][0], override=True)
                if ('rotates' in otile or otile['id'] in add_tile_ids):
                    print("  and rotating " + str(otile[g][0]))
                    # create 3 new iso-ized tiles, as well
                    for rot in (270, 180, 90):
                        if iso_ize(otile[g][0], ntile['ntn'], rot):
                            ntile[g].append(ntile['ntn'])
                            ntile['ntn'] += 1
                            print("next tile number now " + str(ntile['ntn']))
            else:
                if (ntile[g][0], 0) not in converted_tile_ids:
                    print("  offsetting " + str(ntile[g][0]))
                    converted_tile_ids[(ntile[g][0], 0)] = True
                    # offset this flat tile
                    tile_png = "tile-{:0>6d}.png".format(otile[g][0])
                    command = (
                        'convert -background transparent ' + new_tiles_dir +
                        '/' + tile_png +
                        (
                            ' -fill transparent -draw "color 0,0 floodfill"'
                            if args.floodfill else ''
                        ) +
                        ' -extent ' + str(nwidth) + 'x' + str(nheight) +
                        '-' + str(int((nwidth - owidth) / 2)) + '-' +
                        str(int((nheight - oheight) - flat_sprite_offset)) +
                        ' +repage ' + new_tiles_dir + '/to_merge/' + tile_png)
                    print(command)
                    if os.system(command):
                        print("! Failed to offset %s, continuing" % tile_png)
                        #raise RuntimeError("Failed to offset %s" % tile_png)
        else:
            ntile[g] = otile[g]
            # iso-ize each existing rotation of this tile
            for tile in ntile[g]:
                # if tile_num is a dict with "weight" and "sprite",
                # take the "sprite" number
                if type(tile) == dict and "sprite" in tile:
                    iso_ize(tile["sprite"])
                elif type(tile) == int:
                    iso_ize(tile)
                else:
                    raise RuntimeError("Unexpected sprite number: %s" % tile)

    if 'additional_tiles' in otile:
        nta = ntile['additional_tiles'] = list()
        for otile in otile['additional_tiles']:
            print(" handling additional_tile '%s'" % otile['id'])
            tile = tile_convert(otile, main_id, ntile['ntn'])
            ntile['ntn'] = tile['ntn']
            del tile['ntn']
            nta.append(tile)

    print("returning tile, next tile number now " + str(ntile['ntn']))
    return ntile


#TODO: bail out if old tileset directory doesn't exist

# FIXME: Rename to _dir, use relative path starting with 'gfx'?
old_tileset_name = os.path.abspath(args.tileset)
new_tileset_name = old_tileset_name + "_iso"
new_tiles_dir = os.path.join(new_tileset_name, 'tiles')
print("New tiles will be output to %s" % new_tiles_dir)

# Ensure empty tiles output directory
os.system('rm -rf ' + new_tiles_dir)

try:
    os.mkdir(new_tileset_name)
except OSError:
    pass

try:
    os.mkdir(os.path.join(new_tileset_name, 'tiles'))
except OSError:
    pass

os.system('rm -rf ' + new_tileset_name + '/tiles/*')

try:
    os.mkdir(os.path.join(new_tiles_dir, 'to_merge'))
except OSError:
    pass

print('reading ' + old_tileset_name + '/tile_config.json')
with open(old_tileset_name + '/tile_config.json') as old_tile_config_json_file:
    otc = json.load(old_tile_config_json_file)

oheight = otc['tile_info'][0]['height']
owidth = otc['tile_info'][0]['width']

# iso tile width is enough to hold a 45 degree rotated original tile
nwidth = int(owidth * math.sqrt(2) / 2) * 2
# iso tile height is enough to hold a vertical offset 2d tile
if SPRITE_OFFSET_FROM_BOTTOM < 1:
    flat_sprite_offset = int(nwidth * SPRITE_OFFSET_FROM_BOTTOM / 2)
else:
    flat_sprite_offset = int(SPRITE_OFFSET_FROM_BOTTOM / 2)
print("flat sprite offset " + str(flat_sprite_offset))
nheight = flat_sprite_offset + oheight

# struct that will become the new tileset_config.json
ntc = dict()
ntc['tile_info'] = list()
ntc['tile_info'].append(dict())
ntc['tile_info'][0]['height'] = nheight
ntc['tile_info'][0]['width'] = nwidth
ntc['tile_info'][0]['iso'] = True

first_filename = ''
ntc['tiles-new'] = list()
tile_count = 0
new_tiles = list()

print('processing tiles-new')
for otn in otc['tiles-new']:
    filename = os.path.join(old_tileset_name, otn['file'])
    print(filename)
    base_filename = os.path.basename(filename)
    # need to add support for fallback tiles
    if 'fallback' in base_filename:
        continue
    if first_filename == '':
        first_filename = base_filename  # remember this for tileset.txt
    ntc['tiles-new'].append(dict())
    ntn = ntc['tiles-new'][-1]
    ntn['file'] = os.path.join(new_tileset_name, base_filename)
    ntn['tiles'] = list()

    # split tile image sheet into individual tile images
    command = (
        'convert -crop ' + str(oheight) + 'x' + str(owidth) + ' ' + filename +
        ' +repage ' + new_tiles_dir + '/tile-%06d.png')

    print(command)
    if os.system(command):
        raise RuntimeError("Failed to split %s into tile images" % filename)

    os.system(
        'cp ' + new_tiles_dir + '/tile-*.png ' + new_tiles_dir + '/to_merge')

    # path joining version for other paths
    tile_count = len([
        name for name in os.listdir(new_tiles_dir)
        if os.path.isfile(os.path.join(new_tiles_dir, name))
    ])
    new_tile_number = tile_count

    for otile in otn['tiles']:
        print(" handling '%s'" % otile['id'])
        tile = tile_convert(otile, otile['id'], new_tile_number)
        new_tile_number = tile['ntn']
        del tile['ntn']
        ntn['tiles'].append(tile)

    print('Merging tiles to single image')
    command = (
        'montage -background transparent "' + new_tileset_name +
        '/tiles/to_merge/tile-*.png" -tile 16x -geometry +0+0 ' +
        new_tileset_name + '/' + base_filename)
    print(command)
    if os.system(command):
        raise RuntimeError("Failed to merge tiles into %s" % new_tileset_name)

with open(new_tileset_name + '/tile_config.json', 'w') as tile_config_json:
    json.dump(
        ntc, tile_config_json,
        sort_keys=True, indent=2, separators=(',', ': '))

#TODO: replace tiles.png with first filename from json
with open(new_tileset_name + '/tileset.txt', 'w') as new_tileset_txt_file:
    new_tileset_txt_file.write(
        '#Generated by make_iso.py from flat tileset ' +
        old_tileset_name + '\n' +
        '#' + datetime.datetime.now().strftime('%c') + '\n' +
        '#Name of the tileset\n' +
        'NAME: ' + new_tileset_name + '\n' +
        '#Displayed name of the tileset\n' +
        'VIEW: ' + new_tileset_name + '\n' +
        '#JSON Path - Default of gfx/tile_config.json\n' +
        'JSON: ' + new_tileset_name + '/tile_config.json\n' +
        '#Tileset Path - Default of gfx/tinytile.png\n' +
        'TILESET: ' + new_tileset_name + '/tiles.png\n'
    )
