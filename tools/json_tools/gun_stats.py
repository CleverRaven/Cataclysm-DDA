#!/usr/bin/env python3

import argparse
import json
import os

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args.add_argument(
    "-f", "--format", default="md", dest="format", action="store",
    help="output format: 'md' for markdown, 'csv' for comma-separated")
args.add_argument(
    "-a", "--ammo", default="", dest="ammo", action="store",
    help="ammotype filter - provide an ammotype id")
args_dict = vars(args.parse_args())

all_gun_jos = dict()
all_keys = [
    "weight",
    "volume",
    "longest_side",
    "ammo",
    "ranged_damage",
    "dispersion",
    "modes",
    "pocket_data"
]

csv = False

format_style = args_dict["format"]
if format_style == "csv":
    csv = True
elif format_style == "md":
    csv = False
else:
    print(f"Invalid output format specifier '{format_style}'")


def insert_start_char():
    if not csv:
        return "| "
    return ""


def insert_separator():
    if csv:
        return ","
    else:
        return " | "


def stringify_list(subject):
    if(type(subject)) is not list:
        return subject
    if not csv:
        return str(subject)
    out = str()
    for x in sorted(subject):
        out += str(x)
        out += " - "
    return out


def split_to_units(string):
    units = string.split(' ')

    if len(units) != 2:
        digits = str()
        unit_str = str()
        lastdigit = -1
        for x in range(0, len(string)):
            lastdigit = x
            if string[x].isdigit():
                digits += string[x]
            else:
                break
        for x in range(lastdigit, len(string)):
            unit_str += string[x]
        units = list()
        units.append(digits)
        units.append(unit_str)

    return units


def extract_weight(jo):
    key = "weight"
    if key not in jo:
        return "???"

    weight = split_to_units(jo[key])

    grams = int(weight[0])
    scale = weight[1]

    if scale == 'g':
        return str(grams)
    elif scale == 'kg':
        return str(grams * 1000)


def extract_volume(jo):
    key = "volume"
    if key not in jo:
        return "????"

    volume = split_to_units(jo[key])

    ml = int(volume[0])
    scale = volume[1]

    if scale == 'ml':
        return str(ml)
    elif scale == 'L':
        return str(ml * 1000)


def extract_length(jo):
    key = "longest_side"
    if key not in jo:
        return "???"

    length = split_to_units(jo[key])

    mm = int(length[0])
    scale = length[1]

    if scale == 'mm':
        return str(mm)
    elif scale == 'cm':
        return str(mm * 10)


def extract_ammo(jo):
    key = "ammo"
    if key not in jo:
        return str(0)
    if len(jo[key]) == 1:
        return str(jo[key][0])

    return stringify_list(jo[key])


def extract_modes(jo):
    key = "modes"
    if key not in jo:
        return str(1)

    shots = list()
    for mode in jo[key]:
        shots.append(mode[2])

    if len(shots) == 1:
        return str(shots[0])

    return stringify_list(shots)


def extract_magazines(jo):
    key = "pocket_data"
    if key not in jo:
        return "???"

    for pocket in jo[key]:
        if pocket["pocket_type"] == "MAGAZINE":
            subkey = "ammo_restriction"
            if len(pocket[subkey]) != 1:
                out = str()
                for type_key in pocket[subkey]:
                    out += type_key
                    out += ": "
                    out += str(pocket[subkey][type_key])
                    out += " - "
                return out
            for type_key in pocket[subkey]:
                return str(pocket[subkey][type_key])

        if pocket["pocket_type"] != "MAGAZINE_WELL":
            continue

        if "item_restriction" in pocket:
            return stringify_list(pocket["item_restriction"])
        if "allowed_speedloaders" in pocket:
            return stringify_list(pocket["allowed_speedloaders"])


def extract_accuracy(jo):
    key = "dispersion"
    if key not in jo:
        return str(0)
    return str(jo[key])


def extract_recoil(jo):
    key = "recoil"
    if key not in jo:
        return str(0)
    return str(jo[key])


def extract_damage(jo):
    key = "ranged_damage"
    if key not in jo:
        return str(0)
    ret = 0
    if type(jo[key]) is not list:
        return str(jo[key]["amount"])

    for instance in jo[key]:
        ret += instance["amount"]

    return str(ret)


def print_stats(jo):
    if "id" not in jo:
        return

    out = insert_start_char()
    out += jo["id"]
    out += insert_separator()
    out += extract_weight(jo)
    out += insert_separator()
    out += extract_volume(jo)
    out += insert_separator()
    out += extract_length(jo)
    out += insert_separator()
    ammo = extract_ammo(jo)
    out += ammo
    out += insert_separator()
    out += extract_modes(jo)
    out += insert_separator()
    out += extract_magazines(jo)
    out += insert_separator()
    out += extract_accuracy(jo)
    out += insert_separator()
    out += extract_recoil(jo)
    out += insert_separator()
    out += extract_damage(jo)
    out += insert_separator()

    if args_dict["ammo"] != "" and args_dict["ammo"] not in ammo:
        return

    print(out)


def print_header():
    header_keys = [
        "id",
        "weight",
        "volume",
        "length",
        "ammotype",
        "modes",
        "magazines",
        "dispersion",
        "recoil",
        "damage"
    ]

    out = insert_start_char()
    for key in header_keys:
        out += key
        out += insert_separator()

    print(out)
    if csv:
        return
    out = insert_start_char()
    for x in range(0, len(header_keys)):
        out += "---"
        out += insert_separator()
    print(out)


def extract_jos(path):
    with open(path, "r", encoding="utf-8") as json_file:
        try:
            json_data = json.load(json_file)
        except json.JSONDecodeError:
            print(f"Json Decode Error at {path}")
            print("Ensure that the file is a JSON file consisting of"
                  " an array of objects!")
            return None

        for jo in json_data:
            if type(jo) is not dict:
                print(f"Incorrectly formatted JSON data at {path}")
                print("Ensure that the file is a JSON file consisting"
                      " of an array of objects!")
                break

            if ("type" not in jo or
               jo["type"] != "GUN"):
                continue

            ident = jo["id"] if "id" in jo else jo["abstract"]
            all_gun_jos[ident] = jo


def recurse_extract_keys(jo, dat):
    filled = True
    for key in all_keys:
        if key not in dat:
            filled = False

    if filled:
        return

    for key in all_keys:
        if key not in dat and key in jo:
            dat[key] = jo[key]

    if "copy-from" not in jo:
        return

    parent = jo["copy-from"]
    if parent == "fake_item":
        return

    recurse_extract_keys(all_gun_jos[parent], dat)


def do_copy_from(jo):
    if "copy-from" not in jo:
        return jo

    dat = dict()
    recurse_extract_keys(jo, dat)

    for key in dat:
        jo[key] = dat[key]

    return jo


for root, directories, filenames in os.walk(args_dict["dir"]):
    for filename in filenames:
        if not filename.endswith(".json"):
            continue

        path = os.path.join(root, filename)
        extract_jos(path)

for gun in all_gun_jos:
    new = do_copy_from(all_gun_jos[gun])
    all_gun_jos[gun] = new

all_guns = []
for key in all_gun_jos:
    all_guns.append(all_gun_jos[key])


def ident(jo):
    return jo["id"] if "id" in jo else jo["abstract"]


def identify(jo):
    if "ammo" not in jo:
        #print(f"No ammo: {name}")
        return "$$$"

    if type(jo["ammo"]) is list:
        #if len(jo["ammo"]) != 1:
        #    print(f"Many ammo: {name}")
        return jo["ammo"][0]

    return jo["ammo"]


sorted(all_guns, key=lambda jo: identify(jo))
print_header()
for jo in all_guns:
    print_stats(jo)
