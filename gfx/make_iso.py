#!/usr/bin/env python
# pylint: disable=C0103
# pylint: disable=C0301
"""Convert a Cataclysm-DDA tileset to isometric view."""

import os
import json
import argparse
import datetime
import math

# when a 2d sprite 'stands up' on an isometric tile bae, like a tree
# how far up should it be from the bottom corner of the tile?
# offset=0 will align the bottom of the sprite with the bottom (southwest corner) of the square
#  this will make it overlap with the tiles adjacent to the lower left and right
# offset<1 will be a fraction of the isometric tile height
# offset>=1 will be an absolute number of pixels
SPRITE_OFFSET_FROM_BOTTOM = 1.0/8

parser = argparse.ArgumentParser(description='Convert a Cataclysm-DDA tileset to isometric view.')

parser.add_argument('tileset', help='name of the tileset directory to convert')

parser.add_argument('-f', dest='floodfill', action='store_true', help='floodfill tile edges with transparent pixels')

parser.add_argument('-d', dest='dryrun', action='count', help='perform a dry run (more ds, more dry)')

parser.add_argument('-w', dest='wallheight', metavar="W", help='wall height, default 0', type=int, default=0)

args = parser.parse_args()

def iso_ize(tile_num, new_tile_num=None, initial_rotation=0, override=False, height=0):
    """Convert one normal tile image file to one isometric tile image file

    tile_num is the index of the tile in the original sprite sheet
    new_tile_num is an optional different number for the output tile
        used for making rotations of a previously unrotated tile
    initial_rotation rotates the tile before conversion
        used for making rotations of a previously unrotated tile
    override causes conversion to happen even if the target tile has already been created
    """

    if override or (tile_num, initial_rotation) not in converted_tile_ids:
        # TODO insert new tiles into the tile numbers, rather than append to the end
        if new_tile_num == None:
            new_tile_num = tile_num
        converted_tile_ids[(tile_num, initial_rotation)] = True
        convert_command = (
            # ImageMagick convert utility
            'convert ' +
            # start with everything transparent
            '-background transparent ' +
            # load the square sprite
            new_tileset_name + '/tiles/tile-' + "{:0>6d}".format(tile_num) + '.png ' +
            # rotate the square sprite some quarter turns
            '-rotate ' + str(initial_rotation) + ' ' +
            # rotate 45 degrees to start the iso conversion
            '-rotate -45 ' +
            # shrink the image 50% vertically
            '+repage -resize 100%x50% ' +
            # crop down to the target pixel size, trimming some half pixels off the <> shape
            '-crop ' + str(nwidth) + 'x' + str(int(nwidth/2)) + '+2+1 ' +
            # set the image extents to the target pixel size and position of the <> shape in the canvas
            '+repage -extent ' + str(nwidth) + 'x' + str(nheight) + '+0-' + str(nheight-int(nwidth/2)) + ' ' +
            # path and filename for the output isometric sprite
            new_tileset_name + '/tiles/to_merge/tile-' + "{:0>6d}".format(new_tile_num) + '.png'
        )
        print '  iso-izing tiles/tile-' + '{:0>6d}'.format(tile_num) + '.png to to_merge/tile-' + "{:0>6d}".format(new_tile_num) + '.png'
        print convert_command
        if (not args.dryrun) and os.system(convert_command):
            raise # failure

        if height > 0:
            mogrify_command = (
                'convert ' +
                 new_tileset_name + '/tiles/to_merge/tile-' + "{:0>6d}".format(new_tile_num) + '.png ' + 
                '-clone 0 -geometry +0-1 -compose Over -composite ' * height +
                 new_tileset_name + '/tiles/to_merge/tile-' + "{:0>6d}".format(new_tile_num) + '.2.png'
            )            
            print mogrify_command
            if (not args.dryrun) and os.system(mogrify_command):
                raise # failure
            if (not args.dryrun) and os.system('mv ' + 
                new_tileset_name + '/tiles/to_merge/tile-' + "{:0>6d}".format(new_tile_num) + '.2.png ' + 
                new_tileset_name + '/tiles/to_merge/tile-' + "{:0>6d}".format(new_tile_num) + '.png'):
                raise # failure

        return True # success
    return False # didn't try

def offset(n):
    # if we aren't going to iso-ize the tile, move it upwards and enlarge its canvas
    # don't offset a tile that we've previously converted
    if (n, 0) not in converted_tile_ids:
        print"  offsetting " + str(n)
        converted_tile_ids[(n, 0)] = True
        offset_command = (
            # ImageMagic convert utility
            'convert ' +
            # start with everything transparent
            '-background transparent ' + 
            # load the square sprite
            new_tileset_name + '/tiles/tile-' + "{:0>6d}".format(n) + '.png' +
            # optionally fill in the "background" of the image with transparent pixels
            (' -fill transparent -draw "color 0,0 floodfill"' if args.floodfill else '') +
            # set the image extents to the target pixel size and position of the [] shape in the canvas
            ' -extent ' + str(nwidth) + 'x' + str(nheight) +
            '-' + str(int((nwidth-owidth)/2)) + '-' + str(int((nheight-oheight)-flat_sprite_offset)) + ' ' +
            '+repage ' +
            # path and filename for the output offset sprite
            new_tileset_name + '/tiles/to_merge/tile-' + '{:0>6d}'.format(n) + '.png')
        print offset_command
        if (not args.dryrun) and os.system(offset_command):
            raise # failure


def thing_convert(thing, g, ntile, thing_id, rotates, height):
    # a thing can be:
    #   one int, a single sprite
    #   an array of ints, manual rotations of the same sprite
    #   an array of objects with members weight and sprite, each sprite can be
    #     one int, a single sprite
    #     an array of ints, manual rotations of the same sprite

    if type(thing) == int: # just one sprite id
        # sprite id -1 means this is not a valid tile
        if thing == -1:
            return
        # this way one int and a one-int array can be handled the same
        thing = list([thing])

    if len(thing) == 0:
        # no sprites, invalid tile
        return

    if type(thing[0]) == int: # list of sprite id[s]
        if len(thing) == 1:
            # one sprite, iso-ize or offset, possibly with new rotations
            # which tiles should be iso-ized?
            if (
                    # terrain and lighting background, foreground if there's no background
                    (
                        (ntile['id'][0:2] == 't_' or ntile['id'][0:9] == 'lighting_') and
                        (g == 'bg' or 'bg' not in ntile)
                    ) or
                    # furniture background only if there's a foreground
                    (ntile['id'][0:2] == 'f_' and g == 'bg' and 'fg' in ntile) or
                    # vehicle parts and fields
                    ntile['id'][0:3] == 'vp_' or ntile['id'][0:3] == 'fd_' or # vehicle parts and fields
                    # additional_tiles
                    thing_id == 'broken' or
                    thing_id == 'open' or
                    thing_id == 'unconnected' or
                    thing_id == 'center' or
                    thing_id == 'corner' or
                    thing_id == 'edge' or
                    thing_id == 'end_piece' or
                    thing_id == 't_connection' or
                    # any tile that is flagged to be rotated normally
                    rotates or
                    # every tile called recursively
                    thing_id != ntile['id']
               ):
                # iso-ize this tile
                iso_ize(thing[0], override=True, height=height)
                if (
                        rotates or
                        thing_id == 'broken' or
                        thing_id == 'open' or
                        thing_id == 'unconnected' or
                        thing_id == 'center' or
                        thing_id == 'corner' or
                        thing_id == 'edge' or
                        thing_id == 'end_piece' or
                        thing_id == 't_connection'
                    ):
                    print "  and rotating " + str(thing[0])
                    # create 3 new rotated iso-ized tiles, as well
                    for rot in (270, 180, 90):
                        if iso_ize(thing[0], ntile['ntn'], rot, height=height):
                            # append new tile number to sprite list for this tile's fg or bg
                            thing.append(ntile['ntn'])
                            ntile['ntn'] += 1
                            print "next tile number now " + str(ntile['ntn'])
            else:
                offset(thing[0])
        else:
            # multiple sprites, iso-ize each existing rotation of this tile
            for tile_num in thing:
                iso_ize(tile_num, height=height)
    elif type(thing[0]) == dict: # weighted sprite[ set][s]
        for subthing in thing:
            thing_convert(subthing['sprite'], g, ntile, thing_id, rotates, height)


def tile_convert(otile, main_id, new_tile_number):
    """Convert one square tile definition to one isometric tile definition

    otile is an object containing information about the square tile
    main_id is a string identifying the top level tile name, otile['id'] before recursion
    new_tile_number is an integer specifying where to start numbering the isometric tile(s)
    """
    print 'tile_convert ' + main_id + ' ' + str(new_tile_number)
    print otile

    # start creating an object to represent the new tile definition
    ntile = dict()
    ntile['id'] = otile['id']
    ntile['ntn'] = int(new_tile_number)

    #TODO: handle different tile transformations based on id
    #TODO: load other json files to base transforms on stats, not just id

    # copy attributes from the square tile
    if 'rotates' in otile:
        ntile['rotates'] = otile['rotates']
    if 'multitile' in otile:
        ntile['multitile'] = otile['multitile']

    # process fg and bg sprites in mostly the same way
    for g in ('fg', 'bg'):
        if g not in otile:
            continue

        # tile ids to iso-ify:
        # vp_*.bg and .fg
        # fd_*.bg and .fg
        # fd_*.additional_tiles.fg and .bg

        # calculate desired height for wall tiles
        height = 0
        if (
            ( main_id[0:6] == 't_wall' ) or 
            ( main_id[0:2] == 't_' and main_id[-5:] == '_wall' )
        ):
            height = args.wallheight

        ntile[g] = otile[g]
        thing_convert(
            ntile[g],
            g, 
            ntile,
            ntile['id'],
            ('rotates' in otile and otile['rotates'] == True),
            height,
        )

    if 'additional_tiles' in otile:
        nta = ntile['additional_tiles'] = list()
        for otile in otile['additional_tiles']:
            print ' handling additional_tile ' + otile['id']
            new_tile = tile_convert(otile, main_id, ntile['ntn'])
            ntile['ntn'] = new_tile['ntn']
            del new_tile['ntn']
            nta.append(new_tile)

    print "returning new tile " + str(len(str(ntile))) + " , next tile number now " + str(ntile['ntn'])
    return ntile


if __name__ == "__main__":
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

    (not args.dryrun) and os.system('rm -rf '+new_tileset_name+'/tiles/*')

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
    file_count = 0

    print 'processing tiles-new'
    for otn in otc['tiles-new']:
        # TODO add support for fallback tiles
        if os.path.basename(otn['file']).find('fallback') > -1:
            continue
        converted_tile_ids = dict()
        if first_filename == '':
            first_filename = os.path.basename(otn['file']) # remember this for tileset.txt
        ntc['tiles-new'].append(dict())
        ntn = ntc['tiles-new'][-1]
        ntn['file'] = os.path.basename(otn['file'])
        ntn['tiles'] = list()

        print ' splitting ' + old_tileset_name + '/' + otn['file']
        # split tile image sheet into individual tile images
        split_command = 'convert -crop '+str(oheight)+'x'+str(owidth)+' ' + old_tileset_name + '/' + otn['file'] +' +repage '+new_tileset_name+'/tiles/tile-%06d.png'
        print split_command
        if (not args.dryrun) and os.system(split_command):
            raise
        (not args.dryrun) and os.system('cp '+new_tileset_name+'/tiles/tile-*.png '+new_tileset_name+'/tiles/to_merge')

        path, dirs, files = os.walk(new_tileset_name+'/tiles').next()
        file_count = len(files)

        # path joining version for other paths
        TILEDIR = new_tileset_name+'/tiles'
        tile_count = len([name for name in os.listdir(TILEDIR) if os.path.isfile(os.path.join(TILEDIR, name))])
        new_tile_number = tile_count

        for otile in otn['tiles']:
            print ' handling ' + otile['id']
            tile = tile_convert(otile, otile['id'], new_tile_number)
            new_tile_number = tile['ntn']
            del tile['ntn']
            ntn['tiles'].append(tile)

        # offset unused tiles
        for n in range(file_count):
            if (n, 0) not in converted_tile_ids:
                offset(n)

        print 'Merging tiles to single image'
        montage_command = ('montage -background transparent "' + new_tileset_name + '/tiles/to_merge/tile-*.png" -tile 16x -geometry +0+0 ' +
                       new_tileset_name + '/' + os.path.basename(otn['file']))
        print montage_command
        if (not args.dryrun) and os.system(montage_command):
            raise

    if args.dryrun>1:
        print ntc
    else:
        with open(new_tileset_name + '/tile_config.json', 'w') as new_tile_config_json_file:
            json.dump(ntc, new_tile_config_json_file, sort_keys=True, indent=2, separators=(',', ': '))

    tileset_txt = (
        '#Generated by make_iso.py from flat tileset ' + old_tileset_name + '\n' +
        '#' + datetime.datetime.now().strftime('%c') + '\n' +
        '#Name of the tileset\n' +
        'NAME: ' + new_tileset_name + '\n' +
        '#Displayed name of the tileset\n' +
        'VIEW: ' + new_tileset_name + '\n' +
        '#JSON Path - Default of gfx/tile_config.json\n' +
        'JSON: tile_config.json\n' +
        '#Tileset Path - Default of gfx/tinytile.png\n' +
        'TILESET: ' + os.path.basename(otn['file']) + '\n'
    )

    if args.dryrun>1:
        print tileset_txt
    else:
        with open(new_tileset_name + '/tileset.txt', 'w') as new_tileset_txt_file:
            new_tileset_txt_file.write(tileset_txt)

