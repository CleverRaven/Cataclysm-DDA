#!/usr/bin/env python3

import os
from optparse import OptionParser
from sys import exit, version_info


from string_extractor.parse import parse_json_file
from string_extractor.pot_export import write_to_pot


parser = OptionParser()
parser.add_option("-i", "--include_dir", dest="include_dir",
                  action="append", type="str",
                  help="include directories")
parser.add_option("-n", "--name", dest="name", help="POT package name")
parser.add_option("-o", "--output", dest="output", help="output file path")
parser.add_option("-r", "--reference", dest="reference",
                  help="reference POT for plural collision avoidance")
parser.add_option("-v", "--verbose", dest="verbose", help="be verbose")
parser.add_option("-X", "--exclude", dest="exclude",
                  action="append", type="str",
                  help="exclude a single file")
parser.add_option("-x", "--exclude_dir", dest="exclude_dir",
                  action="append", type="str",
                  help="exclude directories")

(options, args) = parser.parse_args()

if not (version_info.major >= 3 and version_info.minor >= 7):
    print("Requires Python 3.7 or higher.")
    exit(1)

if not options.output:
    print("Have to specify output file path.")
    exit(1)

if not options.include_dir:
    print("Have to specify at least one search path.")

include_dir = [os.path.normpath(i) for i in options.include_dir]

exclude = [os.path.normpath(i) for i in options.exclude] \
    if options.exclude else []

exclude_dir = [os.path.normpath(i) for i in options.exclude_dir] \
    if options.exclude_dir else []


def extract_all_from_dir(json_dir):
    """Extract strings from every json file in the specified directory,
    recursing into any subdirectories."""
    allfiles = sorted(os.listdir(json_dir))
    dirs = []
    skiplist = [os.path.normpath(".gitkeep")]
    for f in allfiles:
        full_name = os.path.join(json_dir, f)
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
    for i in sorted(include_dir):
        extract_all_from_dir(i)

    with open(options.output, mode="w", encoding="utf-8") as fp:
        write_to_pot(fp, True, options.name, sanitize=options.reference)


main()
