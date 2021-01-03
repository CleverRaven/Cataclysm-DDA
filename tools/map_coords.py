#!/usr/bin/env python3

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

    print(f"  Folder:  {min_cx}.{min_cy}.{cz}.map -> {max_cx}.{max_cy}.{cz}.map")
    print("Overmaps:  ", end="")
    for k, v in enumerate(ol):
        if k == len(ol) - 1:
            print(v)
        else:
            print(v, end=", ")


def info_range(r1: tuple, r2: tuple) -> None:
    rv = lambda miv, mav: [miv] if miv == mav else list(range(miv, mav + 1))

    lvx = rv(min(r1[0], r2[0]), max(r1[0], r2[0]))
    lvy = rv(min(r1[1], r2[1]), max(r1[1], r2[1]))
    lvz = rv(min(r1[2], r2[2]), max(r1[2], r2[2]))

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
        description="Prints information given game coordinates or file coordinates",
        formatter_class=argparse.RawTextHelpFormatter,
    )
    parser.add_argument(
        "format",
        type=str,
        nargs="?",
        help="""coordinate format
Accepted formats:
Folders:    x.y.z
    E.g: 1.0.0  -7.3.5  8.-9.-1
Maps:       x.y.z.map
    E.g: 6.1.-3.map  -4.8.1.map  -9.-9.17.map
Coordinate: \"x'(0->179) y'(0->179)\" or \"x'(0->179) y'(0->179) z\"
    E.g: 5'107 3'146    7'98 -7'137 5    -1'0 0'4 -8    
    The quotation mark ( \" ) IS necessary
        """,
    )
    parser.add_argument("--info", action="store_true", help="Print info about maps")
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
                "Invalid range for map coordination, a'b where b is positive from 0 to 179"
            )
        else:
            print("Invalid format, print help to view usage")

        if retInfo:
            cx, cy, cz = retInfo

            print(
                f"     Map:  {cx//180}'{cx - 180 * (cx//180)}  {cy//180}'{cy- 180 * (cy//180)}  {cz}"
            )

            print(f"    File:  {cx//32}.{cy//32}.{cz}/{cx}.{cy}.{cz}.map")

            info_folder(f"{cx//32}.{cy//32}.{cz}")