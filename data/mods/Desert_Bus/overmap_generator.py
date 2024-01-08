#!/usr/bin/env python3

"""

Generates archived omap files the game can read north/south
from the origin up to the specified range using the template,
(-20, 21) will generate 41 files for example from (0, -20) to (0, 20)

"""
from inspect import getsourcefile
from os.path import dirname
from bitarray import bitarray
import glob
import os
import gzip
import random
import math

filler_type = 'field' # Designed to be field (which gets changed by desert_region)
road_type = 'road' # Designed to be a road or subway, has to be a linear OMT
vertical = True # Whether the road goes north-south or east-west
windy = False # Whether the road gently winds back and forth, forces north-south
overmaps = 41 # Number of overmaps to generate, must be odd

dir_path = dirname(getsourcefile(lambda: 0))
overmaps_path = os.path.join(dir_path + '/overmaps')
if road_type == 'road':
    overmaps_path = os.path.join(overmaps_path + '/road')
    if windy:
        overmaps_path = os.path.join(overmaps_path + '/windy')
elif road_type == 'subway':
    overmaps_path = os.path.join(overmaps_path + '/railroad')
else:
    overmaps_path = os.path.join(overmaps_path + '/custom')

for filename in glob.glob(overmaps_path + '/*.gz'): # delete old maps in case overmaps has changed
            os.remove(filename)

def clamp(x, minimum, maximum):
    return max(minimum, min(x, maximum))

all_oms_bits = []
all_oms_bits_needs_populating = True
most_negative_om_y = -1 * round((overmaps - 1) / 2)
most_positive_om_y = round((overmaps - 1) / 2)

for om_y in range(most_negative_om_y, most_positive_om_y + 1):
    full_om = ''
    if windy:
        if all_oms_bits_needs_populating: # figure out all the windiness in advance
            y = 0
            min_amplitude = 3
            max_amplitude = 15
            max_amplitude_difference = 3 # max change between two winds from the last amplitude
            amplitude = max(min_amplitude, max_amplitude * random.random())
            min_period = 20
            max_period = 110
            max_period_difference = 10 # max change between two winds from the last amplitude
            period = max(min_period, max_period * random.random())
            #x_axis = 89
            #max_x_axis_difference = 10
            direction = 1 # Only do half a sine curve then reset origin and do half a negative sine curve with new period and amplitude
            for row in range((overmaps * 180) + 1):
                x = 89 + round(direction * amplitude * math.sin( ( math.pi * y ) / period ))
                #print("row: " + str(row) + " y: " + str(y) + " x: " + str(x))
                row_bits = 180 * bitarray('0')
                row_bits[x] = True
                all_oms_bits.append(row_bits)
                if (y != 0) and (( x >= 89 and direction == -1 ) or ( x <= 89 and direction == 1 )):
                    if direction == 1:
                        direction = -1
                    else:
                        direction = 1
                    current_origin = row - y
                    minima_maxima = current_origin + (y / 2)
                    for sub_row in range(current_origin, current_origin + y): # Backfill the points with a "curve"
                        #print("sub_row: " + str(sub_row) + ", range: " + str(current_origin) + ", " + str(current_origin + y))
                        if (sub_row != minima_maxima):
                            x1 = all_oms_bits[sub_row].find(True)
                            x2 = all_oms_bits[sub_row + 1].find(True)
                            x1_alt = all_oms_bits[sub_row].find(True, right=True)
                            x2_alt = all_oms_bits[sub_row + 1].find(True, right=True)
                            if abs(x1_alt - x2_alt) < abs(x1 - x2):
                                x1 = x1_alt
                                x2 = x2_alt
                            if x1 > x2:
                                temp = x1
                                x1 = x2
                                x2 = temp
                            if x1 != x2:
                                if ( sub_row == (current_origin + 1) ) and ( sub_row != 0 ):
                                    all_oms_bits[sub_row][x1:x2] = True
                                elif sub_row < minima_maxima:
                                    all_oms_bits[sub_row][x1:x2] = True
                                else:
                                    all_oms_bits[sub_row + 1][x1:x2] = True
                    y = 0
                    period += max_period_difference * 2 * (random.random() - 0.5) # weight these so they go towards the mean of min max not get stuck at the edges
                    amplitude += max_amplitude_difference * 2 * (random.random() - 0.5)
                    period = clamp(round(period), min_period, max_period)
                    amplitude = clamp(round(amplitude), min_amplitude, max_amplitude)
                    #min_x_axis = amplitude * 2
                    #max_x_axis = 180 - amplitude * 2
                    #x_axis += max_x_axis_difference * 2 * (random.random() - 0.5)
                    #x_axis = clamp(round(x_axis), min_x_axis, max_x_axis)
                else:
                    y += 1
            current_origin = (overmaps * 180) - y # Handling for the last partial curve
            minima_maxima = current_origin + (y / 2) # Needs changing to derivative solution
            for sub_row in range(current_origin, current_origin + y):
                        if (sub_row != minima_maxima):
                            x1 = all_oms_bits[sub_row].find(True)
                            x2 = all_oms_bits[sub_row + 1].find(True)
                            x1_alt = all_oms_bits[sub_row].find(True, right=True)
                            x2_alt = all_oms_bits[sub_row + 1].find(True, right=True)
                            if abs(x1_alt - x2_alt) < abs(x1 - x2):
                                x1 = x1_alt
                                x2 = x2_alt
                            if x1 > x2:
                                temp = x1
                                x1 = x2
                                x2 = temp
                            if x1 != x2:
                                if ( sub_row == (current_origin + 1) ) and ( sub_row != 0 ):
                                    all_oms_bits[sub_row][x1:x2] = True
                                elif sub_row < minima_maxima:
                                    all_oms_bits[sub_row][x1:x2] = True
                                else:
                                    all_oms_bits[sub_row + 1][x1:x2] = True
            all_oms_bits_needs_populating = False
        #if not vertical:
            #rotate the mess above?
        for om_row_n in range( (om_y - most_negative_om_y) * 180,(om_y - most_negative_om_y + 1) * 180): # Do this during the curve handling instead so the type of bend needed is still apparent
            om_row_bits = all_oms_bits[om_row_n].copy()
            filler_start_amount = om_row_bits.find(True)
            road_amount = om_row_bits.count(True)
            filler_end_amount = 180 - filler_start_amount - road_amount
            if road_amount == 1:
                om_row = '["' + filler_type + '",' + str(filler_start_amount) + '],["' + road_type + '_ns",1],["' + filler_type + '",' + str(filler_end_amount) + ']'
            elif road_amount == 2:
                om_row = '["' + filler_type + '",' + str(filler_start_amount) + '],["' + road_type + '_es",1],["'+ road_type + '_wn",1],["' + filler_type + '",' + str(filler_end_amount) + ']'
            else:
                om_row = '["' + filler_type + '",' + str(filler_start_amount) + '],["' + road_type + '_es",1],["'+ road_type + '_ew",' + str(road_amount-2) + '],["'+ road_type + '_wn",1],["' + filler_type + '",' + str(filler_end_amount) + ']'
            if om_row_n != (((om_y - most_negative_om_y + 1) * 180) - 1):
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
