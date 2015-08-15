#! /usr/bin/env python
'''Tool to combine ascii map cells with json templates.

Original design and implementation: acidia (https://github.com/acidia)
Current design and implementation: wormingdead (https://github.com/wormingdead)
'''

from __future__ import division

import argparse
import copy
import json
import os.path

_MAP_CELL_SIZE = 24
_TEMPLATE_JSON_SECTION = "_Building_Utility"
_TEMPLATE_TYPE_CELL_MAP = "cell_map"
_TEMPLATE_TYPE_CELL_NUM = "cell_num"
_TEMPLATE_FUNC_OBJ_REPLACE = "object_replace"
_TEMPLATE_FUNC_STR_FORMAT = "string_format"


def division_split(div, single_list):
    '''Divides a list into a list of lists each containing div amount of
    elements.  The last list will contain less than the div amount if the
    list has a non-divisable amount of elements.
    '''

    ret_list = []
    while single_list:
        ret_list.append(single_list[:div])
        single_list = single_list[div:]

    return ret_list


def internal_append(list_of_lists, appends):
    '''Returns the list created when each element of appends is put into its
    corresponding list in list_of_lists (Corresponding by index).  Stops when
    shortest list is exhausted.
    '''

    return [l + [a] for l, a in zip(list_of_lists, appends)]


def get_map_cells(infile, cell_size):
    '''Converts an ascii map file into a list of cells of size cell_size.
    Raises an Assertion Error if any cell is not the correct size.
    '''
    cell_line_no = 1
    cells_per_line = None
    line_no = None

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
                "{line_no}.".format(
                    infile=infile.name,
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

    if line_no is not None:
        assert line_no % cell_size == 0, \
            "Map {infile} reaches EOF before the completion of cells " \
            "{cell_begin} to {cell_end}.".format(
                infile=infile.name,
                cell_begin=len(all_cells) + 1,
                cell_end=len(all_cells) + cells_per_line)

    return all_cells


def recursive_dict_update(info_dict, list_path, data):
    '''Recurses through the info_dict using the sequence in list_path until
    reaching the end, where data replaces whatever is currently in that part
    of the dict.  This function uses a modifier that has effects beyond the
    scope of the function.
    '''
    if list_path == []:
        return data
    else:
        info_dict[list_path[0]] = recursive_dict_update(
            info_dict.get(list_path[0], {}),
            list_path[1:], data)
        return info_dict


def is_list_of_lists(param):
    '''Returns true if x is a list of lists. False otherwise.  Assumes lists
    are homomorphic.
    '''

    return (isinstance(param, list) and
            len(param) > 0 and
            isinstance(param[0], list))


def template_function_exec(full_dict, settings, data):
    '''Modifies a dictionary based on the setting functions passed in,
    inserting data in specified ways.

    Currently two setting functions:
        _TEMPLATE_FUNC_OBJ_REPLACE completely replaces the old value with the
        new value.

        _TEMPLATE_FUNC_STR_FORMAT uses printf (%) formatting to change the
        string in the dict.

    Settings format is specified in help. Basically, can be a path or list of
    paths per function to where the function should modify.
    '''

    # FUNC_OBJ_REPLACE
    paths = settings.get(_TEMPLATE_FUNC_OBJ_REPLACE, [])

    if is_list_of_lists(paths):
        for path in paths:
            recursive_dict_update(full_dict, path, data)
    else:
        recursive_dict_update(full_dict, paths, data)

    # FUNC_STR_FORMAT
    for string, paths in settings.get(_TEMPLATE_FUNC_STR_FORMAT, {}).items():
        if is_list_of_lists(paths):
            for path in paths:
                recursive_dict_update(full_dict, path, string % data)
        else:
            recursive_dict_update(full_dict, paths, string % data)


def complete_json_file(template_file, all_cells, remove_template=True):
    '''Combines json template with cell list and writes out results.

    Reads and separates json template from template settings.  Then combines
    cells and template, putting template_function_exec output into a list.
    Finally writes out each json template list.
    '''
    json_output_list = []
    json_template = json.load(template_file)

    template_settings = json_template.get(_TEMPLATE_JSON_SECTION)
    if remove_template:
        json_template.pop(_TEMPLATE_JSON_SECTION, None)

    for cell_no, cell in enumerate(all_cells, 1):
        copy_of_template = copy.deepcopy(json_template)
        template_function_exec(
            copy_of_template,
            template_settings.get(_TEMPLATE_TYPE_CELL_MAP, {}),
            cell)
        template_function_exec(
            copy_of_template,
            template_settings.get(_TEMPLATE_TYPE_CELL_NUM, {}),
            cell_no)

        json_output_list.append(copy_of_template)

    # TODO: better output file names
    with open("output_" + os.path.basename(template_file.name),
              "w") as outfile:
        json.dump(json_output_list, outfile, indent=4, separators=(",", ": "),
                  sort_keys=True)


def cli_interface():
    '''Sets up command-line parser, including user documentation and help.'''

    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description="A script for combining multi-cell maps with their json "
        "data.",
        epilog='''template explaination:
  To format the json template, add the key '{section}' to the root
  dictionary.  In that section, create a dictionary with one or more template
  type keys.  Each type key should have a dictionary of function keys.  Each
  function key has its own value format, however they will all have a
  destination or list of destinations of where in the json template to perform
  the function.

  List of types:
    '{cell_map}'        The list of ascii lines representing the map cell.
    '{cell_num}'        The number of the current cell.

  List of functions:
    '{obj_repl}'  A keypath or list of keypaths where the value at the end
                      of the keypath will be replaced with the chosen type.
    '{str_form}'   A dictionary with printf formatted strings as keys which
                      have keypath or list of keypaths as values.  The value at
                      the end of the keypath(s) will be replaced with the
                      string formatted with the type as input.
'''.format(section=_TEMPLATE_JSON_SECTION,
           cell_map=_TEMPLATE_TYPE_CELL_MAP,
           cell_num=_TEMPLATE_TYPE_CELL_NUM,
           obj_repl=_TEMPLATE_FUNC_OBJ_REPLACE,
           str_form=_TEMPLATE_FUNC_STR_FORMAT))

    parser.add_argument(
        "map_file",
        metavar="map-file",
        type=argparse.FileType("r"),
        help="A file that contains an ascii representation of one or more map "
             "cells.")

    parser.add_argument(
        "json_templates",
        metavar="json-file",
        nargs="+",
        type=argparse.FileType("r"),
        help="A json template that will be combined with the map cells to a "
             "properly formatted json map file.  See below for templating "
             "format.")

    return parser


def main(parser):
    '''Combines ascii map file with json template(s).

    Takes a parser that yields a map_file and a list of json_templates.  The
    map_file is split into cells using get_map_cells.  The json_templates are
    combined with the cells using complete_json_file.
    '''
    args = parser.parse_args()

    with args.map_file:
        all_cells = get_map_cells(args.map_file, _MAP_CELL_SIZE)

    for json_template in args.json_templates:
        with json_template:
            try:
                complete_json_file(json_template, all_cells)
            except ValueError as val_err:
                parser.exit(1,
                            "Could not parse json in file {file_name}\n"
                            "{json_err}\n".format(
                                file_name=json_template.name,
                                json_err=str(val_err)))

if __name__ == "__main__":
    main(cli_interface())

