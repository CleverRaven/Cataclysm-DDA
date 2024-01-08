#!/usr/bin/env python3

"""

Generates archived omap files the game can read north/south
from the origin up to the specified range using the template,
(-20, 21) will generate 41 files for example from (0, -20) to (0, 20)

"""
from inspect import getsourcefile
from os.path import dirname
import os
import gzip
import random

filler_type = 'field' # Designed to be field (which gets changed by desert_region)
road_type = 'subway' # Designed to be a road or subway, has to be a linear OMT
vertical = True # Whether the road goes north-south or east-west
windy = True # Whether the road gently winds back and forth, forces north-south

dir_path = dirname(getsourcefile(lambda: 0))
overmaps_path = os.path.join(dir_path + '/overmaps')

for om_y in range(-20, 21):

    if windy:
        om_straighaway_row = '["' + filler_type + '",89],["' + road_type + '_ns",1],["' + filler_type + '",90]'
        road_x = 89
        chance_to_wind = 0.1 # chance to begin a wind
        minimum_wind = 3 # minimum distance a wind can vary from the centre
        maximum_wind = 10
        winding_east = False # winding east of the centre
        winding_west = False
        winding_away = False # winding away from the centre
        winding_back = False
        road_width = 1
        full_om = om_straighaway_row + ','
        for omt_y in range(0, 178):
            if ( 1 + omt_y + maximum_wind ) >= 180:
                # prevent new winds and wind back
            else:
                winding = winding_east or winding_west
                if road_x == 89:
                    if winding_back:
                        winding_east = False
                        winding_west = False
                        winding_back = False
                        road_width = 1
                    elif random.random() < 0.1:
                        winding = True
                        winding_away = True
                        road_width = 2
                if not winding:
                    full_om = full_om + om_straighaway_row + ','
                elif winding_away:
                    om_row = '["' + filler_type + '",' + str(road_x - (road_width - 1) ) + '],["' + road_type + '_ns",' + str(road_width) + '],["' + filler_type + '",' + str(180 - road_x) + ']'
                    full_om = full_om + om_row + ','
                    road_x += road_width # check for passing centre
                    offset = abs(road_x - 89)
                    if offset > minimum_wind and ( offset >= maximum_wind or random.random < ( offset / maximum_wind ) ):
                        winding_away = False
                        road_width = 1
                    elif random.random() < 0.5:
                        road_width = 1
                    else:
                        road_width = 2
                elif winding_back:
                    om_row = '["' + filler_type + '",' + str(road_x) + '],["' + road_type + '_ns",' + str(road_width) + '],["' + filler_type + '",' + str(180 - road_x - (road_width - 1) ) + ']'
                    full_om = full_om + om_row + ','
                    road_x += road_width # check for passing centre
                else:
                    om_row = '["' + filler_type + '",' + str(road_x) + '],["' + road_type + '_ns",' + str(road_width) + '],["' + filler_type + '",' + str(180 - road_x) + ']'
                    full_om = full_om + om_row + ','
                    if random.random() < 0.4:
                        winding_back = True

        full_om = full_om + om_straighaway_row
    else:
        if vertical:
            om_row = '["' + filler_type + '",89],["' + road_type + '_ns",1],["' + filler_type + '",90]'
            full_om = om_row + ','
            for omt_y in range(0, 178):
                full_om = full_om + om_row + ','
            full_om = full_om + om_row
        else:
            om_row = '["' + filler_type + '",180]'
            full_om = om_row + ','
            for omt_y in range(0, 89):
                full_om = full_om + om_row + ','
            full_om = full_om + '["' + road_type + '_ew",180],'
            for omt_y in range(0, 90):
                full_om = full_om + om_row + ','
            full_om = full_om + om_row

    output_file_string = '[{"type":"overmap","om_pos":[0,' + str(om_y) + '],"layers":[' + full_om + ']}]'
    output_file_string = output_file_string.replace( '\n', '')
    output_file = str.encode(output_file_string)
    output_filename = 'overmap_0_' + str(om_y) + '.omap.gz'
    output_path = os.path.join(overmaps_path, output_filename)
    f = gzip.open(output_path, 'wb')
    f.write(output_file)
    f.close()
