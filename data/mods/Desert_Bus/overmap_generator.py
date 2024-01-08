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

road_type = 2 # 0 for straight road, 1 for windy road, 2 for railroad

dir_path = dirname(getsourcefile(lambda: 0))
overmaps_path = os.path.join(dir_path + "/overmaps")
template_file_name = ""
if road_type == 0:
    template_file_name = "/overmap_template_road.txt"
elif road_type == 1:
    template_file_name = "/overmap_template_road.txt"
elif road_type == 2:
    template_file_name = "/overmap_template_railroad.txt"
template_path = os.path.join(dir_path + template_file_name)
template_first_line = ""
template_second_line = ""
template_third_line = ""
template_fourth_line = ""
with open(template_path) as template_file:
    template_first_line = template_file.readline()
    template_second_line = template_file.readline()
    template_third_line = template_file.readline()
    template_fourth_line = template_file.readline()

for om_y in range(-20, 21):

    if road_type == 0 or road_type == 2:
        full_third_line = template_third_line + ","
        for omt_y in range(0, 178):
            full_third_line = full_third_line + template_third_line + ","
        full_third_line = full_third_line + template_third_line
    elif road_type == 1:
        full_third_line = ""
        #do some windy stuff

    output_file_string = template_first_line + str(om_y) + template_second_line + full_third_line + template_fourth_line
    output_file_string = output_file_string.replace( '\n', '')
    output_file = str.encode(output_file_string)
    output_filename = "overmap_0_" + str(om_y) + ".omap.gz"
    output_path = os.path.join(overmaps_path, output_filename)
    f = gzip.open(output_path, 'wb')
    f.write(output_file)
    f.close()
