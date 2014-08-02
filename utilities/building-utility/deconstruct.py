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


def internal_append(list_of_lists, appends):
    "Returns the list created when each element of appends is put into its "
    "corresponding list in list_of_lists (Corresponding by index).  Stops when"
    " shortest list is exhausted."

    return [l + [a] for l,a in zip(list_of_lists, appends)]


def get_map_cells(infile, cell_size):
    cell_line_no = 1
    cells_per_line = None

    # all_cells holds completed cells.  cell_list holds incompleted cells.
    all_cells = []
    cell_list = [[]]

    for line_no, line in enumerate(infile, 1):
        # -1 to remove newline char
        line = line[:-1]

        assert len(line) % cell_size == 0, \
            "Map {infile} does not have colums equal to a multiple of the " \
            "map cell size. Error occured on line {line_no}.".format(
                infile=infile.name,
                line_no=line_no)

        if cells_per_line is None:
            cells_per_line = len(line) // cell_size
            # setting up for internal_append
            cell_list *= cells_per_line
        else:
            assert cells_per_line == len(line) // cell_size, \
                "Map {infile} starts new cells before finishing cells " \
                "{cell_begin} to {cell_end}. Error occured on line " \
                "{line_no}.".format(infile=infile.name,
                    cell_begin=len(all_cells) + 1,
                    cell_end=len(all_cells) + cells_per_line,
                    line_no=line_no)

        cell_list = internal_append(cell_list, division_split(cell_size, line))

        cell_line_no += 1

        if cell_line_no > cell_size:
            cell_line_no = 1
            cells_per_line = None

            all_cells.extend(cell_list)
            cell_list = [[]]

    assert line_no % cell_size == 0, \
        "Map {infile} reaches EOF before the completion of cells " \
        "{cell_begin} to {cell_end}.".format(infile=infile.name,
            cell_begin=len(all_cells) + 1,
            cell_end=len(all_cells) + cells_per_line)

    return all_cells


def recursive_dict_update(info_dict, list_path, data):
    "Recurses through the info_dict using the sequence in list_path until "
    "reaching the end, where data replaces whatever is currently in that part "
    "of the dict.  This function uses a modifier that has effects beyond the "
    "scope of the function."
    if list_path == []:
        return data
    else:
        info_dict[list_path[0]] = \
            recursive_dict_update(info_dict.get(list_path[0],{}),
                                  list_path[1:], data)
        return info_dict


def template_update(full_dict, settings, data):
    # OBJ_REPLACE completely replaces the old value with the new value
    paths = settings.get(_TEMPLATE_FUNC_OBJ_REPLACE, [])
    if paths != []:
        # TODO: this is ugly, find a better way
        if isinstance(paths[0], list):
            for path in paths:
                recursive_dict_update(full_dict, path, data)
        else:
            recursive_dict_update(full_dict, paths, data)

    # STR_FORMAT uses printf formatting to change the string in the dict
    for string, paths in settings.get(_TEMPLATE_FUNC_STR_FORMAT, {}).items():
        # TODO: this is ugly, find a better way
        if isinstance(paths[0], list):
            for path in paths:
                recursive_dict_update(full_dict, path, string % data)
        else:
            recursive_dict_update(full_dict, paths, string % data)


def complete_json_file(template_file, all_cells, remove_template=True):
    json_list = []
    json_template = json.load(template_file)

    template_settings = json_template.get(_TEMPLATE_JSON_SECTION)
    if remove_template:
        json_template.pop(_TEMPLATE_JSON_SECTION, None)

    for cell_no, cell in enumerate(all_cells, 1):
        copy_of_template = copy.deepcopy(json_template)
        template_update(copy_of_template,
                        template_settings.get(_TEMPLATE_TYPE_CELL_MAP, {}),
                        cell)
        template_update(copy_of_template,
                        template_settings.get(_TEMPLATE_TYPE_CELL_NUM, {}),
                        cell_no)

        json_list.append(copy_of_template)

    # TODO: better output file names
    with open("updated_" + template_file.name, "w") as outfile:
        json.dump(json_list, outfile, indent=4, separators=(",", ": "),
                  sort_keys=True)


if __name__ == "__main__":
    main()

