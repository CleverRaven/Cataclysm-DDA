#! /usr/bin/env python

from __future__ import division

import argparse
import copy
import json

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

    if line_no is not None:
        assert line_no % cell_size == 0, \
            "Map {infile} reaches EOF before the completion of cells " \
            "{cell_begin} to {cell_end}.".format(infile=infile.name,
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
        info_dict[list_path[0]] = \
            recursive_dict_update(info_dict.get(list_path[0], {}),
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
    # TODO: epilog needs actual formatting. see argparse formatterclass
    main_parser = argparse.ArgumentParser(description=
        "A script for combining multi-cell maps with their json data.",
        epilog="To format the json template, add the key '{section}' to the "
        "root dictionary.  In that section, create a dictionary with one or "
        "more template type keys.  Each type key should have a dictionary of "
        "function keys.  Each function key has its own value format, however "
        "they will all have a destination or list of destinations of where "
        "in the json template to perform the function.\n\nList of types:\t"
        "'{cell_map}':\tThe list of ascii lines representing the map cell.\n\t"
        "'{cell_num}':\tThe number of the current cell.\n\nList of functions:"
        "\t'{obj_repl}':\tValue is a keypath or list of keypaths where the "
        "value at the end of the keypath will be replaced with the chosen "
        "type.\n\t'{str_form}'\tValue is a dictionary with printf formatted "
        "strings as keys which have keypath or list of keypaths as values.  "
        "The value at the end of the keypath(s) will be replaced with the "
        "string formatted with the type as input.".format(
            section=_TEMPLATE_JSON_SECTION, cell_map=_TEMPLATE_TYPE_CELL_MAP,
            cell_num=_TEMPLATE_TYPE_CELL_NUM,
            obj_repl=_TEMPLATE_FUNC_OBJ_REPLACE,
            str_form=_TEMPLATE_FUNC_STR_FORMAT))

    main_parser.add_argument("map_file", metavar="map-file",
                        type=argparse.FileType("r"),
                        help="A file that contains an ascii representation "
                        "of one or more map cells.")

    main_parser.add_argument("json_templates", metavar="json-file",
                        nargs="+",
                        type=argparse.FileType("r"),
                        help="A json template that will be combined with the "
                        "map cells to a properly formatted json map file.  "
                        "See below for templating format.")

    args = main_parser.parse_args()

    with args.map_file:
        all_cells = get_map_cells(args.map_file, _MAP_CELL_SIZE)

    for json_template in args.json_templates:
        with json_template:
            try:
                complete_json_file(json_template, all_cells)
            except ValueError as val_err:
                main_parser.exit(1, "Could not parse json in file {file_name}"
                "\n{json_err}\n".format(file_name=json_template.name,
                                        json_err=str(val_err)))

