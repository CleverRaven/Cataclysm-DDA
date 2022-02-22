#!/usr/bin/env python3
"""Print a Markdown or CSV table of JSON values from the given keys.
Run this script with -h for full usage information.

Examples with basic field names:

    %(prog)s --type=TOOL id volume weight longest_side
    %(prog)s --type=ARMOR --format=csv id encumbrance coverage
    %(prog)s --type=COMESTIBLE id fun calories quench healthy

Examples with nested attributes:

    %(prog)s --type=ARMOR name.str pocket_data.moves
    %(prog)s --type=MAGAZINE name.str pocket_data.ammo_restriction

"""

import argparse
import csv
import sys
import util


# Command-line arguments
parser = argparse.ArgumentParser(
    description=__doc__,
    formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument(
    "columns", metavar="column_key", nargs="+",
    help="list of JSON object keys to be columns in the table")
parser.add_argument(
    "--fnmatch",
    default="*.json",
    help="override with glob expression to select a smaller fileset")
parser.add_argument(
    "-f", "--format",
    default="md",
    help="output format: 'md' for markdown, 'csv' for comma-separated")
parser.add_argument(
    "-t", "--type",
    help="only include JSON data matching these types, separated by comma",
    type=lambda s: list([i for i in s.split(',')]))
parser.add_argument(
    "--nonestring",
    default="None",
    help="what to output when value is None")
parser.add_argument(
    "--noheader",
    dest='with_header', action='store_false',
    help="do not output table header")
parser.add_argument(
    "--tileset",
    dest='tileset_types_only', action='store_true',
    help="override --type filter with a set of types required for a tileset")
parser.set_defaults(with_header=True, tileset_types_only=False)


I18N_DICT_KEYS = ('str', 'str_sp', 'str_pl', 'ctxt', '//~')
I18N_DICT_KEYS_SET = set(I18N_DICT_KEYS)
TILESET_TYPES = [
    "AMMO", "ARMOR", "BATTERY", "BIONIC_ITEM", "bionic", "BOOK", "COMESTIBLE",
    "ENGINE", "field_type", "furniture", "gate", "GENERIC", "GUN", "GUNMOD",
    "MAGAZINE", "MONSTER", "mutation", "PET_ARMOR", "SPELL", "terrain", "TOOL",
    "TOOL_ARMOR", "TOOLMOD", "trap", "vehicle_part", "WHEEL"]


def item_values(item, fields, none_string="None"):
    """Return item values from within the given fields, converted to strings.

    Fields may be plain string or numeric values:

        >>> item_values({'name': 'sword', 'length_cm': 90},
        ...             ['name', 'length_cm'])
        ['sword', '90']

    Fields may also be nested objects; subkeys may be referenced like
    key.subkey:

        >>> item_values({'loc': {'x': 5, 'y': 10}}, ['loc.x', 'loc.y'])
        ['5', '10']

    Fields with a nested object list have their values combined with "/":

        >>> item_values({'locs': [{'x': 4, 'y': 8}, {'x': 5, 'y': 10}] },
        ...             ['locs.x', 'locs.y'])
        ['4 / 5', '8 / 10']

    Fields may include both plain and dotted keys:

        >>> item_values({'id': 'd6', 'name': {'str': 'die', 'str_pl': 'dice'}},
        ...             ['id', 'name.str', 'name.str_pl'])
        ['d6', 'die', 'dice']

    """
    values = []
    for field in fields:
        if "." in field:
            subkeys = field.split(".")
        else:
            subkeys = [field]
        # Descend into dotted keys
        it = item
        for subkey in subkeys:
            # Is it a dict with this subkey?
            if isinstance(it, dict) and subkey in it:
                it = it[subkey]
            # Is it a list of dicts having this subkey?
            elif isinstance(it, list) and all(subkey in o for o in it):
                # Pull from all subkeys, or just the one
                if len(it) == 1:
                    if isinstance(it[0], dict):
                        it = it[0][subkey]
                    else:
                        it = it[0]
                else:
                    it = [i[subkey] for i in it]
            # Stop if any subkey is not found
            else:
                it = none_string
                break

        if isinstance(it, dict):
            if set(it.keys()) <= I18N_DICT_KEYS_SET:
                # it dict contains only i18zed values
                first_good_value = None
                for k in I18N_DICT_KEYS:
                    value = it.get(k, None)
                    if value:
                        first_good_value = value
                        break
                values.append("%s" % first_good_value or none_string)
            else:
                # Make dict presentable
                values.append("%s" % it.items())
        # Separate lists with slashes
        elif isinstance(it, list):
            values.append(" / ".join(
                "%s" % i if i is not None else none_string for i in it))
        # Otherwise just force string
        else:
            values.append("%s" % it if it is not None else none_string)

    return values


def get_format_class_by_extension(format_string):
    """

    >>> get_format_class_by_extension('csv')
    <class 'table.CSVFormat'>

    """
    format_name = format_string.upper()
    try:
        format_class = getattr(
            sys.modules[__name__],
            "{}Format".format(format_name))
    except AttributeError:
        sys.exit("Unknown format {}".format(format_name))
    return format_class


class MDFormat:
    """
    Markdown
    | col1 | col2 | col3
    """
    def header(self, columns):
        self.row(columns)
        # Markdown table needs a separator after the header
        print("| --- " * len(columns))

    def row(self, values):
        safe_values = [self.safe_value(v) for v in values]
        print("| " + " | ".join(safe_values))

    def safe_value(self, value):
        """Return value with special characters escaped.

        >>> MDFormat.safe_value(MDFormat, 'x|y|z')
        'x\\\\|y\\\\|z'

        """
        return value.replace('|', '\\|')


class CSVFormat:
    """
    Comma-Separated Values
    col1,col2,"col3,with,commas"
    """
    writer = None

    def __init__(self):
        self.writer = csv.writer(sys.stdout)

    def to_utf8(self, lst):
        return [str(elem).encode('utf-8') for elem in lst]

    def header(self, columns):
        self.row(columns)

    def row(self, values):
        self.writer.writerow(self.to_utf8(values))


class CDDAValues:
    """Worker class that prints table from provided data"""
    output = None

    def __init__(self, format_string):
        format_class = get_format_class_by_extension(format_string)
        self.output = format_class()

    def print_table(self, data, columns, types_filter,
                    none_string, with_header):
        if with_header:
            self.output.header(columns)
        for item in data:
            if types_filter and item.get('type') not in types_filter:
                continue

            self.output.row(item_values(item, columns, none_string))


if __name__ == "__main__":
    args = parser.parse_args()
    if args.tileset_types_only:
        args.type = TILESET_TYPES

    # Get data (don't care about load errors)
    json_data, _ = util.import_data(json_fmatch=args.fnmatch)

    worker = CDDAValues(args.format)
    worker.print_table(
        json_data, args.columns, args.type, args.nonestring, args.with_header)
