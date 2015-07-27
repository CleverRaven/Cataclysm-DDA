#!/usr/bin/env python
# pylint: disable=C0103
# pylint: disable=C0301

import os
import json
import argparse
import datetime
import math

# when a 2d sprite 'stands up' on an isometric tile bae, like a tree
# how far up should it be from the bottom corner of the tile?
# offset=0 will align the bottom of the sprite with the bottom (southwest corner) of the square
# offset<1 will be a fraction of the isometric tile height
# offset>=1 will be an absolute number of pixels
SPRITE_OFFSET_FROM_BOTTOM = 1.0/8

parser = argparse.ArgumentParser(description='Convert a Cataclysm-DDA tileset to isometric view.')

parser.add_argument('tileset', help='name of the tileset directory to convert')

parser.add_argument('-f', dest='floodfill', action='store_true')

args = parser.parse_args()

def iso_ize(tile_num, new_tile_num=-1, initial_rotation=0, override=False):
    if override or (tile_num,initial_rotation) not in converted_tile_ids:
        print "  iso-izing " + str(tile_num)
        if new_tile_num == -1:
            new_tile_num = tile_num
        converted_tile_ids[(tile_num,initial_rotation)] = True
        command = ('convert -background transparent ' + new_tileset_name + '/tiles/tile-' + "{:0>6d}".format(tile_num) + '.png ' +
            '-rotate ' + str(initial_rotation) + ' ' +
            '-rotate -45 +repage -resize 100%x50% ' +
            '-crop ' + str(nwidth) + 'x' + str(int(nwidth/2)) + '+2+1 ' + #TODO: get correct offsets
            '+repage -extent ' + str(nwidth) + 'x' + str(nheight) + '+0-' + str(nheight-int(nwidth/2)) + ' ' +
            new_tileset_name + '/tiles/to_merge/tile-' + "{:0>6d}".format(new_tile_num) + '.png')
        print command
        if os.system(command):
            raise
        return True
    return False


# convert one old tile definition to one new tile definition
# recursive for additional-tiles
def tile_convert(otile, main_id, new_tile_number):
    print 'tile_convert ' + main_id + ' ' + str(new_tile_number)
    ntile = dict()
    ntile['id'] = otile['id']
    ntile['ntn'] = int(new_tile_number)

    #TODO: handle different tile transformations based on id
    #TODO: load other json files to base transforms on stats, not just id

    if 'rotates' in otile:
        ntile['rotates'] = otile['rotates']
    if 'multitile' in otile:
        ntile['multitile'] = otile['multitile']

    for g in ('fg','bg'):
        if g not in otile:
            continue
        if type(otile[g]) == int:
            if otile[g] == -1:
                continue
            otile[g] = list([otile[g]])

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
            # iso-ize?
            if (
                (
                 (main_id[0:2] == 't_' or main_id[0:9] == 'lighting_') and # terrain and lighting
                 (g == 'bg' or 'bg' not in otile) # rotate bg, fg iff there's no bg
                ) or
                (main_id[0:2] == 'f_' and g == 'bg' and 'fg' in otile) or
                main_id[0:3] == 'vp_' or main_id[0:3] == 'fd_' or # vehicle parts and fields
                # additional_tiles:
                otile['id'] == 'broken' or
                otile['id'] == 'open' or
                otile['id'] == 'unconnected' or
                otile['id'] == 'center' or
                otile['id'] == 'corner' or
                otile['id'] == 'edge' or
                otile['id'] == 'end_piece' or
                otile['id'] == 't_connection' or
                ('rotates' in otile and otile['rotates'] == True) or
                otile['id'] != main_id
               ):
                # iso-ize this tile
                iso_ize(otile[g][0], override=True)
                if ('rotates' in otile or
                    otile['id'] == 'broken' or
                    otile['id'] == 'open' or
                    otile['id'] == 'unconnected' or
                    otile['id'] == 'center' or
                    otile['id'] == 'corner' or
                    otile['id'] == 'edge' or
                    otile['id'] == 'end_piece' or
                    otile['id'] == 't_connection'):
                    print "  and rotating " + str(otile[g][0])
                    # create 3 new iso-ized tiles, as well
                    for rot in (270,180,90):
                        if iso_ize(otile[g][0], ntile['ntn'], rot):
                            ntile[g].append(ntile['ntn'])
                            ntile['ntn'] += 1
                            print "next tile number now " + str(ntile['ntn'])
            else:
                if (ntile[g][0],0) not in converted_tile_ids:
                    print"  offsetting " + str(ntile[g][0])
                    converted_tile_ids[(ntile[g][0],0)] = True
                    # offset this flat tile
                    command = (
                        'convert -background transparent ' + new_tileset_name + '/tiles/tile-' + "{:0>6d}".format(otile[g][0]) + '.png' +
                        (' -fill transparent -draw "color 0,0 floodfill"' if args.floodfill else '') +
                        ' -extent ' + str(nwidth) + 'x' + str(nheight) +
                        '-' + str(int((nwidth-owidth)/2)) + '-' + str(int((nheight-oheight)-flat_sprite_offset)) + ' ' +
                        '+repage ' +
                        new_tileset_name + '/tiles/to_merge/tile-' + '{:0>6d}'.format(otile[g][0]) + '.png')
                    print command
                    if os.system(command):
                        raise
        else:
            ntile[g] = otile[g]
            # iso-ize each existing rotation of this tile
            for tile_num in ntile[g]:
                iso_ize(tile_num)

    if 'additional_tiles' in otile:
        nta = ntile['additional_tiles'] = list()
        for otile in otile['additional_tiles']:
            print ' handling additional_tile ' + otile['id']
            tile = tile_convert(otile, main_id, ntile['ntn'])
            ntile['ntn'] = tile['ntn']
            del tile['ntn']
            nta.append(tile)

    print "returning tile, next tile number now " + str(ntile['ntn'])
    return ntile


#TODO: bail out if old tileset directory doesn't exist

old_tileset_name = args.tileset
new_tileset_name = old_tileset_name + "_iso"

try:
    os.mkdir(new_tileset_name)
except:
    pass

try:
    os.mkdir(new_tileset_name + '/tiles')
except:
    pass

os.system('rm -rf '+new_tileset_name+'/tiles/*')

try:
    os.mkdir(new_tileset_name + '/tiles/to_merge')
except:
    pass

print 'reading ' + old_tileset_name + '/tile_config.json'
with open(old_tileset_name + '/tile_config.json') as old_tile_config_json_file:
    otc = json.load(old_tile_config_json_file)

oheight = otc['tile_info'][0]['height']
owidth = otc['tile_info'][0]['width']

# iso tile width is enough to hold a 45 degree rotated original tile
nwidth = int(owidth * math.sqrt(2)/2)*2
# iso tile height is enough to hold a vertical offset 2d tile
if SPRITE_OFFSET_FROM_BOTTOM < 1:
    flat_sprite_offset = int(nwidth * SPRITE_OFFSET_FROM_BOTTOM / 2)
else:
    flat_sprite_offset = int(SPRITE_OFFSET_FROM_BOTTOM / 2)
print "flat sprite offset " + str(flat_sprite_offset)
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

print 'processing tiles-new'
for otn in otc['tiles-new']:
    # need to add support for fallback tiles
    if os.path.basename(otn['file']).find('fallback') > -1:
        continue
    converted_tile_ids = dict()
    if first_filename == '':
        first_filename = os.path.basename(otn['file']) # remember this for tileset.txt
    ntc['tiles-new'].append(dict())
    ntn = ntc['tiles-new'][-1]
    ntn['file'] = 'gfx/' + new_tileset_name + '/' + os.path.basename(otn['file'])
    ntn['tiles'] = list()

    print ' splitting ../'+otn['file']
    # split tile image sheet into individual tile images
    command = 'convert -crop '+str(oheight)+'x'+str(owidth)+' ../'+otn['file']+' +repage '+new_tileset_name+'/tiles/tile-%06d.png'
    print command
    if os.system(command):
        raise
    os.system('cp '+new_tileset_name+'/tiles/tile-*.png '+new_tileset_name+'/tiles/to_merge')

    # path joining version for other paths
    TILEDIR = new_tileset_name+'/tiles'
    tile_count = len([name for name in os.listdir(TILEDIR) if os.path.isfile(os.path.join(TILEDIR, name))])
    new_tile_number = tile_count

    for otile in otn['tiles']:
        print ' handling ' + otile['id']
        tile = tile_convert(otile,otile['id'],new_tile_number)
        new_tile_number = tile['ntn']
        del tile['ntn']
        ntn['tiles'].append(tile)

    print 'Merging tiles to single image'
    command = ('montage -background transparent "' + new_tileset_name + '/tiles/to_merge/tile-*.png" -tile 16x -geometry +0+0 ' +
     new_tileset_name + '/' + os.path.basename(otn['file']))
    print command
    if os.system(command):
        raise

with open(new_tileset_name + '/tile_config.json', 'w') as new_tile_config_json_file:
    json.dump(ntc, new_tile_config_json_file, sort_keys=True, indent=2, separators=(',', ': '))

#TODO: replace tiles.png with first filename from json
with open(new_tileset_name + '/tileset.txt', 'w') as new_tileset_txt_file:
    new_tileset_txt_file.write(
        '#Generated by make_iso.py from flat tileset ' + old_tileset_name + '\n' +
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

