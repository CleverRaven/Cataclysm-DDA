#!/usr/bin/env python
from __future__ import print_function
import sys

version_major = sys.version_info[0]

def p2_check(f):
    count = 1
    try:
        for ln in f:
            ln.decode("utf-8")
            count = count + 1
    except IOError as err:
        print(err)
        return False

    except UnicodeDecodeError as UE:
        print("Unicode error on line {0}:\n{1}".format(count, UE[1]), end="")
        print("{0:>{1}}{2}".format("^", UE[2], "^" * (UE[3]-UE[2] + 1) ))
        return False

    return True

def print_encode_error(unicode_err, counter):
    chunk = unicode_err.object
    err_line = counter + chunk.count(b'\n', 0, unicode_err.start)
    line_start = chunk.rfind(b'\n', 0, unicode_err.start) + 1
    line_end = chunk.find(b'\n', line_start)
    print("Unicode error on line {0}:".format(err_line))
    # Use RAW write because this is bytes class
    sys.stdout.buffer.write(chunk[line_start:line_end + 1])
    x_num = unicode_err.end - unicode_err.start + 2
    x_start = unicode_err.start - line_start + 2
    print("{0:>{1}}".format("^" * x_num, x_start))

def p3_check(f):
    count = 1
    try:
        for ln in f:
            count = count + 1
    except IOError as err:
        print(err)
        return False
    except UnicodeError as UE:
        print_encode_error(UE, count)
        return False

    return True

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: {} [FILENAME]".format(sys.argv[0]))
        sys.exit(1)
    with open(sys.argv[1]) as pot_file:
        if version_major == 2:
            if not p2_check(pot_file):
                sys.exit(1)
        elif version_major == 3:
            if not p3_check(pot_file):
                sys.exit(1)
        else:
            print("Unsupported python version.")
            sys.exit(1)
