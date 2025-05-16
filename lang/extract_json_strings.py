#!/usr/bin/env python3

import os
from optparse import OptionParser
from sys import exit, version_info

from string_extractor.parse import parse_json_file
from string_extractor.pot_export import write_to_pot, sanitize


parser = OptionParser()
parser.add_option("-i", "--include_dir", dest="include_dir",
                  action="append", type="str",
                  help="include directories")
parser.add_option("-n", "--name", dest="name", help="POT package name")
parser.add_option("-r", "--reference", dest="reference",
                  help="reference POT for plural collision avoidance, "
                  "also strings from JSON are appended to this file")
parser.add_option("-v", "--verbose", dest="verbose", help="be verbose")
parser.add_option("-X", "--exclude", dest="exclude",
                  action="append", type="str",
                  help="exclude a single file")
parser.add_option("-x", "--exclude_dir", dest="exclude_dir",
                  action="append", type="str",
                  help="exclude directories")
parser.add_option("-D", "--obsolete_paths", dest="obsolete_paths",
                  action="append", type="str",
                  help="obsolete directories or files")

(options, args) = parser.parse_args()

if not (version_info.major >= 3 and version_info.minor >= 7):
    print("Requires Python 3.7 or higher.")
    exit(1)

if not options.reference:
    print("Have to specify reference file path.")
    exit(1)

if not options.include_dir:
    print("Have to specify at least one search path.")
    exit(1)

include_dir = [os.path.normpath(i) for i in options.include_dir]

exclude = [os.path.normpath(i) for i in options.exclude] \
    if options.exclude else []

exclude_dir = [os.path.normpath(i) for i in options.exclude_dir] \
    if options.exclude_dir else []

obsolete_paths = [os.path.normpath(i) for i in options.obsolete_paths] \
    if options.obsolete_paths else []


def extract_all_from_dir(json_dir):
    """Extract strings from every json file in the specified directory,
    recursing into any subdirectories."""
    allfiles = sorted(os.listdir(json_dir))
    dirs = []
    skiplist = [os.path.normpath(".gitkeep")]
    for f in allfiles:
        full_name = os.path.join(json_dir, f)
        if full_name in [i for i in include_dir if i != json_dir]:
            # Skip other included directories;
            # They will be extracted later and appended to
            # the end of the shared list of strings;
            continue
        if os.path.isdir(full_name):
            dirs.append(f)
        elif f in skiplist or full_name in exclude:
            continue
        elif any(full_name.startswith(dir) for dir in exclude_dir):
            continue
        elif f.endswith(".json"):
            parse_json_file(full_name)
    for d in dirs:
        extract_all_from_dir(os.path.join(json_dir, d))


def main():
    for i in include_dir:
        extract_all_from_dir(i)

    sanitize(options.reference, options.name)

    with open(options.reference, mode="a", encoding="utf-8") as fp:
        write_to_pot(fp, obsolete_paths=obsolete_paths)


main()
