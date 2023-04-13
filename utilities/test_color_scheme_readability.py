#!/usr/bin/env python3

import sys
import argparse
import json


def translate(value, value_min_range, value_max_range, min_range, max_range):
    value_span = value_max_range - value_min_range
    span = max_range - min_range
    scaled = float(value - value_min_range) / float(value_span)
    return min_range + (scaled * span)


def rgb_as_int(rgb1, rgb2):
    n_rgb1 = tuple([translate(c, 0, 255, 0, 1) for c in rgb1])
    n_rgb2 = tuple([translate(c, 0, 255, 0, 1) for c in rgb2])
    return rgb(n_rgb1, n_rgb2)


# https://www.w3.org/TR/WCAG21/#dfn-contrast-ratio
def rgb(rgb1, rgb2):
    for r, g, b in (rgb1, rgb2):
        if not 0.0 <= r <= 1.0:
            raise ValueError("r is out of valid range (0.0 - 1.0)")
        if not 0.0 <= g <= 1.0:
            raise ValueError("g is out of valid range (0.0 - 1.0)")
        if not 0.0 <= b <= 1.0:
            raise ValueError("b is out of valid range (0.0 - 1.0)")

    l1 = _relative_luminance(*rgb1)
    l2 = _relative_luminance(*rgb2)

    if l1 > l2:
        contrast = (l1 + 0.05) / (l2 + 0.05)
    else:
        contrast = (l2 + 0.05) / (l1 + 0.05)

    return round(contrast, 3)


# https://www.w3.org/TR/WCAG21/#dfn-relative-luminance
def _relative_luminance(r, g, b):
    r = _linearize(r)
    g = _linearize(g)
    b = _linearize(b)

    return 0.2126 * r + 0.7152 * g + 0.0722 * b


def _linearize(v):
    if v <= 0.03928:
        return v / 12.92
    else:
        return ((v + 0.055) / 1.055) ** 2.4


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-j",
        "--colors",
        dest="base_colors",
        type=str,
        help="path to base colors json",
        default="",
    )
    parser.add_argument(
        "-p",
        "--print",
        dest="print",
        type=str,
        help="""print out color pairs, you'll need terminal with 24-bit color
        support enabled; enabled by default""",
        default="yes",
        choices=["yes", "no"],
    )
    parser.add_argument(
        "-s",
        "--sort",
        dest="sort",
        action="store_true",
        help="""should the list of color pairs be ordered in descending
        contrast?""",
        default=False,
    )
    parser.add_argument(
        "-f",
        "--filter",
        dest="filter",
        type=str,
        help="Color to filter on"
    )
    args = parser.parse_args(args=None if sys.argv[1:] else ["--help"])

    with open(args.base_colors, "rt") as f:
        bc_json = json.load(f)[0]

    colors = []
    column_width = 0
    for record in bc_json:
        if record != "type":
            column_width = max(column_width, len(record) + 2)
            pair = {
                "name": record,
                "rgb": (
                    bc_json[record][0],
                    bc_json[record][1],
                    bc_json[record][2]
                ),
            }
            colors.append(pair)
    ls = []
    for c1 in colors:
        if args.filter and c1["name"] != args.filter:
            continue
        for c2 in colors:
            if c1["name"] != c2["name"]:
                ls.append((rgb_as_int(c1["rgb"], c2["rgb"]), c1, c2))
    if args.sort:
        ls = sorted(ls, key=lambda x: x[0], reverse=True)
    for v, c1, c2 in ls:
        if v >= 7.0:
            v = "AAA"
        elif v >= 4.5:
            v = "AA"
        s_color_1 = f"{c1['name']:>{column_width}s}"
        s_color_2 = f"{c2['name']:>{column_width}s}"
        s = f"{s_color_1} {s_color_2} {v:>7}"
        if args.print == "yes":
            s += (f" \x1b[48;2;{c1['rgb'][0]};{c1['rgb'][1]};{c1['rgb'][2]}m"
                  f"\x1b[38;2;{c2['rgb'][0]};{c2['rgb'][1]};{c2['rgb'][2]}m"
                  f" {c2['rgb'][0]:3} {c2['rgb'][1]:3} {c2['rgb'][2]:3}"
                  " \x1b[0m")
        print(s)
