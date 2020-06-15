"""Utility stuff for json tools.
"""

from __future__ import print_function

import argparse
from collections import Counter, OrderedDict
from fnmatch import fnmatch
import json
import re
import os
from StringIO import StringIO


SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))
JSON_DIR = os.path.normpath(os.path.join(SCRIPT_DIR, "..", "..", "data", "json"))
JSON_FNMATCH = "*.json"



def import_data(json_dir=JSON_DIR, json_fmatch=JSON_FNMATCH):
    """Use a UNIX like file match expression to weed out the JSON files.

    returns tuple, first element containing json read, second element containing
    list of any errors found. error list will be empty if no errors
    """
    data = []
    errors = []
    candidates = None
    for d_descriptor in os.walk(json_dir):
        d = d_descriptor[0]
        for f in d_descriptor[2]:
            if fnmatch(f, json_fmatch):
                json_file = os.path.join(d, f)
                with open(json_file, "r") as file:
                    try:
                        candidates = json.load(file, object_pairs_hook=OrderedDict)
                    except Exception as err:
                        errors.append("Problem reading file %s, reason: %s" % (json_file, err))
                    if type(candidates) != list:
                        if type(candidates) == OrderedDict:
                            data.append(candidates)
                        else:
                            errors.append("Problem parsing data from file %s, reason: expected a list." % json_file)
                    else:
                        data += candidates
    return (data, errors)



def match_primitive_values(item_value, where_value):
    """Perform any odd logic on item matching.
    """
    # Matching interpolation for keyboard constrained input.
    if type(item_value) == str or type(item_value) == unicode:
        # Direct match, and don't convert unicode in Python 2.
        return bool(re.match(where_value, item_value))
    elif type(item_value) == int or type(item_value) == float:
        # match after string conversion
        return bool(re.match(where_value, str(item_value)))
    elif type(item_value) == bool:
        # help conversion to JSON booleans from the commandline
        return bool(re.match(where_value, str(item_value).lower()))
    else:
        return False



def matches_where(item, where_key, where_value):
    """True if:
    where_key exists AND
    where_value somehow matches (either directly or guesstimated).

    False if:
    no where_key passed in
    where_key not in item
    where_key in item but where_value does not match
    """
    if not where_key:
        return True
    if where_key not in item:
        return False
    # So we have some value.
    item_value = item[where_key]
    # Matching interpolation for keyboard constrained input.
    if type(item_value) == list:
        # 1 level deep.
        for next_level in item_value:
            if match_primitive_values(next_level, where_value):
                return True
        # else...
        return False
    elif type(item_value) == dict:
        # Match against the keys of the dictionary... I question my logic.
        # 1 level deep.
        for next_level in item_value:
            if match_primitive_values(next_level, where_value):
                return True
        # else...
        return False
    else:
        return match_primitive_values(item_value, where_value)



def matches_all_wheres(item, where_fn_list):
    """Takes a list of where functions and attempts to match against them.

    Assumes wheres are the type returned from WhereAction, and the function
    accepts the item to match against.

    True if:
    all where's match (effectively AND)

    False if:
    any where's don't match
    """
    for where_fn in where_fn_list:
        if not where_fn(item):
            return False
    # Must be a match.
    return True



class WhereAction(argparse.Action):
    """An argparse action callback.

    Example application:

    parser.add_argument("where",
        action=WhereAction, nargs='*', type=str, help="where exclusions")
    """

    def where_test_factory(self, where_key, where_value):
        """Wrap the where test we are using and return it as a callable function.

        item in the callback is assumed to be what we're testing against.
        """
        def t(item):
            return matches_where(item, where_key, where_value)
        return t

    def __init__(self, option_strings, dest, nargs=None, **kwargs):
        if not nargs:
            raise ValueError("nargs must be declared")
        super(WhereAction, self).__init__(option_strings, dest, nargs=nargs, **kwargs)

    def __call__(self, parser, namespace, values, option_string=None):
        try:
            where_functions = []
            for w in values:
                where_key, where_value = w.split("=")
                where_functions.append(self.where_test_factory(where_key, where_value))
            setattr(namespace, self.dest, where_functions)
        except Exception:
            raise ValueError("Where options are strict. Must be in the form of 'where_key=where_value'")



def key_counter(data, where_fn_list):
    """Count occurences of keys found in data {list of dicts}
    that also match each where_fn_list {list of fns}.

    Returns a tuple of data.
    """
    stats = Counter()
    matching_data = [i for i in data if matches_all_wheres(i, where_fn_list)]
    for item in matching_data:
        # Check each key for nested data
        for key in item.keys():
            # Skip comments
            if key.startswith('//'):
                continue

            val = item[key]
            # If value is an object, tally key.subkey for all object subkeys
            if type(val) == OrderedDict:
                for subkey in val.keys():
                    if not subkey.startswith('//'):
                        stats[key + '.' + subkey] += 1

            # If value is a list of objects, tally key.subkey for each
            elif type(val) == list and all(type(e) == OrderedDict for e in val):
                for obj in val:
                    for subkey in obj.keys():
                        if not subkey.startswith('//'):
                            stats[key + '.' + subkey] += 1

            # For anything else, it only counts as one
            else:
                stats[key] += 1

    return stats, len(matching_data)


def object_value_counter(obj):
    """Return a Counter tallying all values in the given {dict},
    recursing into nested dicts and lists.
    """
    return list_value_counter(obj.values())

def list_value_counter(_list):
    """Return a Counter tallying all values in the given {list of dicts}
    or {list of strs}, recursing into nested dicts and lists.
    """
    stats = Counter()
    for elem in _list:
        if type(elem) == OrderedDict:
            stats += object_value_counter(elem)
        elif type(elem) == list:
            stats += list_value_counter(elem)
        else:
            stats[str(elem)] += 1
    return stats

def value_counter(data, search_key, where_fn_list):
    """Takes a search_key {str}, and for values found in data {list of dicts}
    that also match each where_fn_list {list of fns} with those keys,
    counts the number of times the value appears.

    Returns a tuple of data.
    """
    stats = Counter()
    matching_data = [i for i in data if matches_all_wheres(i, where_fn_list)]

    # Search key may use one level of dotted notation to descend into an
    # dict or list-of-dict in the matching data

    # Exactly one dot is allowed to look in parent_key.child_key
    if search_key.count('.') == 1:
        parent_key, child_key = search_key.split('.')
    # With no dots, just use the plain key
    elif search_key.count('.') == 0:
        parent_key = search_key
        child_key = None
    else:
        raise ArgumentError("Only one '.' allowed in search key")

    for item in matching_data:
        if parent_key not in item:
            stats['MISSING'] += 1
            continue

        parent_val = item[parent_key]

        stat_vals = []

        # Get value from parent_key.child_key if it's an object
        if type(parent_val) == OrderedDict and child_key in parent_val:
            stat_vals.append(parent_val[child_key])
        # Get values from parent_key.child_key for objects in a list
        elif type(parent_val) == list and all(type(e) == OrderedDict
                                              for e in parent_val):
            for od in parent_val:
                if child_key in od:
                    stat_vals.append(od[child_key])
        else:
            stat_vals.append(parent_val)

        for val in stat_vals:
            # Cast numbers to strings
            if type(val) == int or type(val) == float:
                stats[str(val)] += 1
            # Pull all values from objects
            elif type(val) == OrderedDict:
                stats += list_value_counter(val.values())
            # Pull values from list of objects or strings
            elif type(val) == list:
                stats += list_value_counter(val)

            else:
                stats[str(val)] += 1

    return stats, len(matching_data)



def ui_values_to_columns(values, screen_width=80):
    """Take a list of strings and output in fixed width columns.
    """
    max_val_len = len(max(values, key=len))+1
    cols = screen_width/max_val_len
    iters = 0
    for v in values:
        print(v.ljust(max_val_len), end=' ')
        iters += 1
        if iters % cols == 0:
            print("")
    print("")



def ui_counts_to_columns(counts):
    """Take a Counter instance and display in single fixed width key:value
    column.
    """
    # Values in left column, counts in right, left column as wide as longest string length.
    key_vals = counts.most_common()
    key_field_len = len(max(list(counts.keys()), key=len))+1
    output_template = "%%-%ds: %%s" % key_field_len
    for k_v in key_vals:
        print(output_template % k_v)



class CDDAJSONWriter(object):
    """Essentially a one-off class used to write CDDA formatted JSON output.

    Expects single object, not a list of objects.

    Probable usage:

        print CDDSJSONWriter(some_json).dumps()
    """
    indent = "  "
    indent_multiplier = 0
    buf = None

    def __init__(self, d, indent_multiplier=0):
        self.d = d
        # Should you wish to change the initial indent for whatever reason.
        self.indent_multiplier = indent_multiplier
        # buf is initialized on a call to dumps

    def indented_write(self, s):
        self.buf.write(self.indent*self.indent_multiplier + s)

    def write_key(self, k):
        self.indented_write("\"%s\": " % k)

    def write_primitive_key_val(self, k, v):
        self.write_key(k)
        self.buf.write(json.dumps(v))

    def list_of_lists(self, k, lol):
        self.write_key(k)
        self.buf.write("[\n")
        lol = lol[:]
        while lol:
            self.indent_multiplier += 1
            inner = lol.pop(0)[:]
            self.indented_write("[\n")
            while inner:
                self.indent_multiplier += 1
                item = inner.pop(0)
                # Print each of these on one line
                self.indented_write(json.dumps(item))
                if inner:
                    self.buf.write(",\n")
                self.indent_multiplier -= 1
            self.buf.write("\n")
            self.indented_write("]")
            if lol:
                self.buf.write(",\n")
            else:
                self.buf.write("\n")
            self.indent_multiplier -=1
        self.indented_write("]")

    def dumps(self):
        """Format the Cataclysm JSON in as friendly of a JSON way as we can.
        """
        if self.buf:
            self.buf.close()
            self.buf = None

        self.buf = StringIO()
        items = self.d.items()
        global indent_multiplier
        self.indented_write("{\n")
        self.indent_multiplier += 1
        while items:
            k, v = items.pop(0)
            # Special cases first.
            if (k == "tools" or k == "components") and type(v) == list:
                self.list_of_lists(k, v)
            else:
                self.write_primitive_key_val(k, v)

            # Trailing comma or not
            if items:
                self.buf.write(",\n")
        self.buf.write("\n")
        self.indent_multiplier -= 1
        self.indented_write("}")
        return self.buf.getvalue()
