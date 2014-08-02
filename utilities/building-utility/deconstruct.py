#! /usr/bin/env python

from __future__ import division

import argparse
import copy
import json
import os

_MAP_CELL_SIZE = 24
_TEMPLATE_JSON_SECTION = "_Building_Utility"
_TEMPLATE_TYPE_CELL_MAP = "CELL_MAP"
_TEMPLATE_TYPE_CELL_NUM = "CELL_NUM"
_TEMPLATE_FUNC_OBJ_REPLACE = "object_replace"
_TEMPLATE_FUNC_STR_FORMAT = "string_format"


def main():
    # initally clear overmap_terrain_entry.json
    # this is a hack, file operations in general need to be refactored a lot
    with open(os.path.join(os.getcwd(), 'output',
                           'overmap_terrain_entry.json'),
              "w") as outfile:
        pass

    with open(os.path.join(os.getcwd(), 'drawing.txt'),"r") as infile:
        data = '[\n'
        mapCount = 1
        mapList = [""] * 9

        for line in infile:
            lineCT = 0

            while len(line) >= _MAP_CELL_SIZE:
                mapList[lineCT] += '\t\t\t\t"' + line[:_MAP_CELL_SIZE] + '"'

                # magic # of chars to know when to stop adding commas
                if len(mapList[lineCT]) < 735:
                    mapList[lineCT] += ','
                mapList[lineCT] += '\n'

                line = line[_MAP_CELL_SIZE:]
                lineCT += 1

            # magic # of chars that shows no errors occured
            if len(mapList[0]) >= 760:
                for line in mapList:
                    if len(line) > 20:
                        data = writeJSON(mapCount, line, data)
                        writeTerrain(mapCount)
                        mapCount += 1
                mapList = [""] * 9

    # remove last comma
    data = data[:-1] + "\n]\n"
    finalizeEntry(data)

#Takes the mapNum and 'text' is the 24x24 map
def writeJSON(mapNum, text, data):
    entry = ''
    with open(os.path.join(os.getcwd(), 'json_header.txt'),
              "r") as json_header_file:
        entry += json_header_file.read().rstrip()

    entry += str(mapNum)

    with open(os.path.join(os.getcwd(), 'json_middle.txt'),
              "r") as json_middle_file:
        entry += json_middle_file.read()

    entry += text

    with open(os.path.join(os.getcwd(), 'json_footer.txt'),
              "r") as json_footer_file:
        entry += json_footer_file.read().rstrip()

    return data + entry

def finalizeEntry(data):
    with open(os.path.join(os.getcwd(), 'output', 'office_tower.json'),
              "w") as outfile:
        outfile.write(data)
    with open(os.path.join(os.getcwd(), 'output',
                           'overmap_terrain_entry.json'),
              "a") as out_terrain_file:
        out_terrain_file.write("\n")

def writeTerrain(mapNum):
    entry = ''
    with open(os.path.join(os.getcwd(), 'terrain_header.txt'),
              "r") as terrain_header_file:
        entry += terrain_header_file.read().rstrip()

    entry += str(mapNum)

    with open(os.path.join(os.getcwd(), 'terrain_footer.txt'),
              "r") as terrain_footer_file:
        entry += terrain_footer_file.read().rstrip()

    with open(os.path.join(os.getcwd(), 'output',
                           'overmap_terrain_entry.json'),
              "a") as outfile:
        outfile.write(entry)
def division_split(div, single_list):
    "Divides a list into a list of lists each containing div amount of "
    "elements.  The last list will contain less than the div amount if the "
    "list has a non-divisable amount of elements."

    ret_list = []
    while single_list:
        ret_list.append(single_list[:div])
        single_list = single_list[div:]

    return ret_list



if __name__ == "__main__":
    main()

