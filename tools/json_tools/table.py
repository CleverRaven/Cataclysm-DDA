#!/usr/bin/env python2
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
import codecs
import sys
import util

# Avoid (most) unicode frustrations
# https://pythonhosted.org/kitchen/unicode-frustrations.html
UTF8Writer = codecs.getwriter('utf8')
sys.stdout = UTF8Writer(sys.stdout)

# Command-line arguments
parser = argparse.ArgumentParser(
    description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument(
    "columns", metavar="column_key", nargs="+",
    help="list of JSON object keys to be columns in the table")
parser.add_argument(
    "--fnmatch",
    default="*.json",
    help="override with glob expression to select a smaller fileset.")
parser.add_argument(
    "-f", "--format",
    default="md",
    help="output format: 'md' for markdown, 'csv' for comma-separated")
parser.add_argument(
    "-t", "--type",
    help="only include JSON data matching this type")


def safe_value(value, format):
    """Return values with special characters escaped for the given format.

    CSV values are "quoted" if they contain a comma or quote:

        >>> safe_value('bacon, eggs, spam', 'csv')
        '"bacon, eggs, spam"'
        >>> safe_value('Tim "The Enchanter" Jones', 'csv')
        '"Tim ""The Enchanter"" Jones"'

    Markdown values have "|" escaped to prevent breaking table layout:

        >>> safe_value('x|y|z', 'md')
        'x\\\\|y\\\\|z'

    """
    if format == 'md':
        return value.replace('|', '\|')

    elif format == 'csv':
        if ',' in value or '"' in value:
            return '"%s"' % value.replace('"', '""')
        else:
            return value

    else:
        raise ValueError("Unknown format '%s'" % format)


def print_row(values, format, is_header=False):
    """Print a row of string values in markdown or csv format.

        >>> print_row(['name', 'quest', 'favorite color'], 'md')
        | name | quest | favorite color

        >>> print_row(['Lancelot', 'Holy Grail', 'blue'], 'csv')
        Lancelot,Holy Grail,blue

    """
    safe_values = [safe_value(v, format) for v in values]

    # Markdown: | col1 | col2 | col3
    if format == 'md':
        print("| " + " | ".join(safe_values))
        # Markdown table needs a separator after the header
        if is_header:
            print("| --- " * len(safe_values))

    # CSV: col1,col2,col3
    elif format == 'csv':
        print(",".join(safe_values))

    else:
        raise ValueError("Unknown format: '%s'" % format)


def item_values(item, fields):
    """Return item values from within the given fields, converted to strings.

    Fields may be plain string or numeric values:

        >>> item_values({'name': 'sword', 'length_cm': 90},
        ...             ['name', 'length_cm'])
        ['sword', '90']

    Fields may also be nested objects; subkeys may be referenced like key.subkey:

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
                    it = it[0][subkey]
                else:
                    it = [i[subkey] for i in it]
            # Stop if any subkey is not found
            else:
                it = "None"
                break

        # Make dict presentable
        if isinstance(it, dict):
            values.append("%s" % it.items())
        # Separate lists with slashes
        elif isinstance(it, list):
            values.append(" / ".join("%s" % i for i in it))
        # Otherwise just force string
        else:
            values.append("%s" % it)

    return values


if __name__ == "__main__":
    args = parser.parse_args()

    # Get data (don't care about load errors)
    json_data, _ = util.import_data(json_fmatch=args.fnmatch)

    # Header row
    print_row(args.columns, args.format, is_header=True)

    # One row per item, matching type if given
    for item in json_data:
        if args.type and item.get('type') != args.type:
            continue

        print_row(item_values(item, args.columns), args.format)

