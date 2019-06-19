#!/usr/bin/env python3
"Strip line numbers from comments in input .pot or .po file."

from __future__ import print_function

import sys
import os

usage = "USAGE: strip_line_numbers.py <filename> [<filename> ...]"

def strip_pot_file(filename):
    # Check pot file
    if not os.path.isfile(filename):
        print("Invalid filename: %s" % filename)
        sys.exit(1)
    try:
        to_write = open(filename, 'r', encoding="utf-8").readlines()
    except IOError as read_exc:
        print(read_exc)
        sys.exit(1)
    assert(len(to_write) > 1) # Wrong .pot file

    to_write = strip_line_numbers(to_write)
    to_write = strip_repeated_comments(to_write)

    # Write the file back out with the line numbers stripped
    try:
        open(filename, 'w', encoding="utf-8").writelines(to_write)
    except IOError as write_exc:
        print(write_exc)
        sys.exit(1)

def strip_repeated_comments(lines):
    to_remove = []
    ln_length = len(lines)
    for i in range(ln_length):
        line = lines[i]
        if i < (ln_length - 1):
            next_line = lines[i+1]
        else:
            next_line = None
        # only act on line-comments
        if not line.startswith("#:"):
            continue
        if next_line is not None and line == next_line:
            to_remove.append(i)
            continue
    # Remove obsolete strings from list
    to_remove.reverse()
    for line_num in to_remove:
        del lines[line_num]

    return lines

def strip_line_numbers(lines):
    "Strip line numbers from the specified list of strings."

    # strip line-numbers
    for i in range(len(lines)):
        line = lines[i]
        # only act on line-comments
        if not line.startswith("#:"): continue
        # line comments are of the format: "#: <file>:<line> <file>:<line>"
        items = line.split()
        newitems = ["#:"]
        # replace line elements while yanking the line number,
        # and checking for duplicates
        for item in items[1:]:
            newname = item.split(':')[0]
            if not newname in newitems:
                newitems.append(newname)
        # replace the old line with the new
        newline = ' '.join(newitems) + '\n'
        lines[i] = newline

    return lines

if __name__ == "__main__":
    # get filename(s) from sys.argv and strip
    if len(sys.argv) < 2:
        # ideally this would read from stdin and input to stdout,
        # but not need to do that yet.
        print(usage)
        sys.exit(1)

    for filename in sys.argv[1:]:
        strip_pot_file(filename)
