#!/usr/bin/env python3

"""

Generates archived omap files the game can read north/south
from the origin up to the specified range using the template,
(-20, 21) will generate 41 files for example from (0, -20) to (0, 20)

"""
from inspect import getsourcefile
from os.path import dirname
from bitarray import bitarray
import os
import gzip
import random
import math

filler_type = 'field' # Designed to be field (which gets changed by desert_region)
road_type = 'subway' # Designed to be a road or subway, has to be a linear OMT
vertical = True # Whether the road goes north-south or east-west
windy = True # Whether the road gently winds back and forth, forces north-south
overmaps = 21 # Number of overmaps to generate, must be odd

dir_path = dirname(getsourcefile(lambda: 0))
overmaps_path = os.path.join(dir_path + '/overmaps')

def clamp(x, minimum, maximum):
    return max(minimum, min(x, maximum))

full_om = ''
all_oms_bits = []

for om_y in range(-1 * round((overmaps - 1) / 2), round((overmaps - 1) / 2) + 1):
    if windy:
        if om_y == (-1 * round((overmaps - 1) / 2)): # figure out all the windiness in advance
            empty_row_bits = 180 * bitarray('0')
            y = 0
            min_amplitude = 10
            max_amplitude = 30
            max_amplitude_difference = 5 # max change between two winds from the last amplitude
            amplitude = min_amplitude + (max_amplitude * random.random())
            min_period = 5
            max_period = 20
            max_period_difference = 3 # max change between two winds from the last amplitude
            period = min_period + (max_period * random.random())
            direction = 1 # only do half a sine curve then reset origin and do half a negative sine curve with new period and amplitude
            for row in range((overmaps * 180) + 1):
                x = 89 + round(direction * amplitude * math.sin( ( math.pi * y ) / period ))
                row_bits = empty_row_bits
                row_bits[x] = True
                all_oms_bits.append(row_bits)
                if x == 89: # probably need to check if it crossed too
                    if direction == 1:
                        direction = -1
                    else:
                        direction = 1
                    minmax = round(y / 2)
                    for sub_row in range(minmax): # Backfill the points with a "curve"
                        sub_row_bits1 = all_oms_bits[row - y + sub_row]
                        sub_row_bits2 = all_oms_bits[row - y + sub_row + 1 ]
                        x1 = sub_row_bits1.find(True)
                        x2 = sub_row_bits2.find(True)
                        sub_row_bits1[x1:x2] = True
                        all_oms_bits[row - y + sub_row] = sub_row_bits1
                    for sub_row in range(minmax + 1, y + 1):
                        sub_row_bits1 = all_oms_bits[row - y + minmax + sub_row + 1]
                        sub_row_bits2 = all_oms_bits[row - y + minmax + sub_row + 2]
                        x1 = sub_row_bits1.find(True)
                        x2 = sub_row_bits2.find(True)
                        sub_row_bits2[x1:x2] = True
                        all_oms_bits[row - y + minmax + sub_row + 2] = sub_row_bits2 # do x1 and x2 need to be in order?
                    y = 0
                    period += max_period_difference * 2 * (random.random() - 0.5) # weight these so they go towards the mean of min max not get stuck at the edges
                    amplitude += max_amplitude_difference * 2 * (random.random() - 0.5)
                    period = clamp(round(period), min_period, max_period)
                    amplitude = clamp(round(amplitude), min_amplitude, max_amplitude)
                y += 1
        #if not vertical:
            #rotate the mess above?
        for om_row_n in range(om_y * 180,(om_y + 1) * 180):
            om_row_bits = all_oms_bits[om_row_n]
            filler_start_amount = om_row_bits.find(True)
            road_amount = om_row_bits.count(True)
            filler_end_amount = 180 - filler_start - road
            om_row = '["' + filler_type + '",' + str(filler_start_amount) + '],["' + road_type + '_ns",' + str(road_amount) + '],["' + filler_type + '",' + str(filler_end_amount) + ']' # need to make the road bend at the ends
            if om_row_n != ((om_y + 1) * 180):
                full_om = full_om + om_row + ','
            else:
                full_om = full_om + om_row
    else:
        if vertical:
            om_row = '["' + filler_type + '",89],["' + road_type + '_ns",1],["' + filler_type + '",90]'
            for omt_y in range(0, 179):
                full_om = full_om + om_row + ','
            full_om = full_om + om_row
        else:
            om_row = '["' + filler_type + '",180]'
            for omt_y in range(0, 90):
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
