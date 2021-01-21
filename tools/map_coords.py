#!/usr/bin/env python3
"""Translates map coordinate to map file and folder and vice-versa
Run this script with -h for full usage information.

Info about output:
     Map: In game map coordinate number
    File: Where coordinate is found on the maps folder
  Folder: Range of submaps the folder can have
Overmaps: Number of overmaps the folder contains

Examples with map format:

    %(prog)s 6.1.-3.map
    %(prog)s -4.8.1.map
    %(prog)s -9.-9.17.map

Examples using folder format:

    %(prog)s 1.0.0
    %(prog)s -7.3.5
    %(prog)s 8.-9.-1

Examples with map coordinate format:

    %(prog)s "5'107 3'146"
    %(prog)s "7'98 -7'137 5"
    %(prog)s "-1'0 0'4 -8"

Examples of INVALID map coordinate format:

    %(prog)s "3'180 3'17"    -> Number after the ' must be between 0 and 179
    %(prog)s "7'31  5'-17 1" -> Number after the ' must be positive
    %(prog)s 1'13 3'17 1   -> Quotation marks are needed (")
                              to prevent string escaping

Examples with range:

    %(prog)s "0'31 0'0" --range "0'32 0'0"
    %(prog)s "1'5 4'3 5" --range "-7'1 -9'179 -7"
    %(prog)s 7.3.0.map --range -1.73.1.map
"""

import sys
import argparse
import re


def print_info() -> None:
    print(
        """
    The game map is divided in overmaps
    Each overmap is divided in overmap terrain tiles (OTT) in a 180x180 grid
    Each OTT is 24x24 tiles divided into 4 submaps ( terrain )
    Each OTT is a `.map` file
    Each map folder is 32x32 OTTs
    """
    )


def info_map(format: str) -> list:
    mx, cx = [int(n) for n in format.split()[0].split("'")]
    my, cy = [int(n) for n in format.split()[1].split("'")]

    dx = mx * 180 + cx
    dy = my * 180 + cy
    dz = 0

    if len(format.split()) == 3:
        dz = int(format.split()[2])

    return (dx, dy, dz)


def info_file(format: str) -> list:
    cx, cy, cz = [int(n) for n in format.split(".")[:3]]
    return (cx, cy, cz)


def info_folder(format: str) -> None:
    cx, cy, cz = [int(n) for n in format.split(".")]
    min_cx, max_cx = cx * 32, cx * 32 + 31
    min_cy, max_cy = cy * 32, cy * 32 + 31

    o1 = f"{min_cx//180} {min_cy//180}"
    o2 = f"{min_cx//180} {max_cy//180}"
    o3 = f"{max_cx//180} {min_cy//180}"
    o4 = f"{max_cx//180} {max_cy//180}"
    ol = [o1]
    if o2 not in ol:
        ol.append(o2)
    if o3 not in ol:
        ol.append(o3)
    if o4 not in ol:
        ol.append(o4)

    print(f"  Folder:  {min_cx}.{min_cy}.{cz}.map -> "
          f"{max_cx}.{max_cy}.{cz}.map")
    print("Overmaps:  ", end="")
    for k, v in enumerate(ol):
        if k == len(ol) - 1:
            print(v)
        else:
            print(v, end=", ")


def get_range_list(value1: int, value2: int) -> list:
    min_value = min(value1, value2)
    max_value = max(value1, value2)

    if min_value == max_value:
        return [min_value]
    else:
        return list(range(min_value, max_value + 1))


def info_range(r1: tuple, r2: tuple) -> None:

    lvx = get_range_list(r1[0], r2[0])
    lvy = get_range_list(r1[1], r2[1])
    lvz = get_range_list(r1[2], r2[2])

    for z in lvz:
        for y in lvy:
            for x in lvx:
                print(f"{x//32}.{y//32}.{z}/{x}.{y}.{z}.map", end=" ")
    print("")


if __name__ == "__main__":

    if len(sys.argv) > 1:
        for key, arg in enumerate(sys.argv[1:]):
            if arg[0] == "-" and arg[1].isdigit():
                argvCopy = " " + arg
                sys.argv[key + 1] = argvCopy

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawTextHelpFormatter,
    )
    parser.add_argument(
        "format",
        type=str,
        nargs="?",
        help="""coordinate format
Accepted formats:
Folders:    x.y.z
Maps:       x.y.z.map
Coordinate: \"x'(0->179) y'(0->179)\" or \"x'(0->179) y'(0->179) z\"
    Default for z is 0
        """,
    )
    parser.add_argument("--info", action="store_true",
                        help="Print info about maps")
    parser.add_argument(
        "--range", help="Print maps in range using maps or coordinate format"
    )
    args = parser.parse_args(sys.argv[1:])

    if not args.format:
        print("Use --help to view usage")
        sys.exit(1)

    args.format = args.format.strip()

    if args.info:
        print_info()

    elif args.range:
        args.range = args.range.strip()
        matchFile = re.compile(r"^-?\d+\.-?\d+\.-?\d+\.map$")
        matchCoord = re.compile(r"^-?\d'-?\d+\s+-?\d+'-?\d+(\s+-?\d+)?$")

        if re.match(matchFile, args.format):
            if re.match(matchFile, args.range):
                retInfoR = info_file(args.range)

            elif re.match(matchCoord, args.range):
                retInfoR = info_map(args.range)

            else:
                print("Invalid format for range, use --help to view usage")
                sys.exit(1)

            retInfoF = info_file(args.format)

        elif re.match(matchCoord, args.format):
            if re.match(matchFile, args.range):
                retInfoR = info_file(args.range)

            elif re.match(matchCoord, args.range):
                retInfoR = info_map(args.range)

            else:
                print("Invalid format for range, use --help  to view usage")
                sys.exit(1)

            retInfoF = info_map(args.format)

        else:
            print("Invalid format, print help to view usage")
            sys.exit(1)

        info_range(retInfoF, retInfoR)

    else:
        args.format = args.format.strip()

        retInfo = False

        # .map file pattern
        if re.match(r"^-?\d+\.-?\d+\.-?\d+\.map$", args.format):
            retInfo = info_file(args.format)

        # .map folder pattern
        elif re.match(r"^-?\d+\.-?\d+\.-?\d+$", args.format):
            info_folder(args.format)

        # map coordinate pattern
        elif re.match(
            r"^-?\d'(\d{0,2}|1[0-7]\d)\s+-?\d+'(\d{0,2}|1[0-7]\d)(\s+-?\d+)?$",
            args.format,
        ):
            retInfo = info_map(args.format)
        elif re.match(r"^-?\d'-?\d+\s+-?\d+'-?\d+(\s+-?\d+)?$", args.format):
            print(
                "Invalid range for map coordination,"
                " a'b where b is positive from 0 to 179"
            )
        else:
            print("Invalid format, print help to view usage")

        if retInfo:
            cx, cy, cz = retInfo

            print(
                f"     Map:  {cx//180}'{cx - 180 * (cx//180)}"
                f"  {cy//180}'{cy- 180 * (cy//180)}  {cz}"
            )

            print(f"    File:  {cx//32}.{cy//32}.{cz}/{cx}.{cy}.{cz}.map")

            info_folder(f"{cx//32}.{cy//32}.{cz}")
