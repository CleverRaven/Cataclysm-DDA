#!/usr/bin/env python
"Strip line numbers from comments in input .pot or .po file."

from __future__ import print_function

import sys
import os

usage = "USAGE: strip_line_numbers.py <filename> [<filename> ...]"

def strip_line_numbers(filename):
    "Strip line numbers from the specified .pot or .po file."

    if not os.path.isfile(filename):
        print("invalid filename: %s" % filename)
        exit(1)

    # modifying the file in-place, so just read the whole thing
    lines = open(filename, 'r').readlines()

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

    # write the file back out with the line numbers stripped
    open(filename, 'w').writelines(lines)

if __name__ == "__main__":
    # get filename(s) from sys.argv and strip
    if len(sys.argv) < 2:
        # ideally this would read from stdin and input to stdout,
        # but not need to do that yet.
        print(usage)
        exit(1)

    for filename in sys.argv[1:]:
        strip_line_numbers(filename)

