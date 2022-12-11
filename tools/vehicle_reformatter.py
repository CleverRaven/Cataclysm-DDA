#!/usr/bin/env python3

import json
import argparse
import copy
import os
import subprocess

LINE_LIMIT = 58


def get_data(argsDict, resource_name):
    resource = []
    resource_sources = argsDict.get(resource_name, [])
    if not isinstance(resource_sources, list):
        resource_sources = [resource_sources]
    for resource_filename in resource_sources:
        if resource_filename.endswith(".json"):
            try:
                with open(
                        resource_filename, encoding="utf-8") as resource_file:
                    resource += json.load(resource_file)
            except FileNotFoundError:
                exit("Failed: could not find {}".format(resource_filename))
        else:
            print("Invalid filename {}".format(resource_filename))
    return resource


# stupid stinking Python 2 versus Python 3 syntax
def write_to_json(pathname, data):
    with open(pathname, "w", encoding="utf-8") as fp:
        try:
            json.dump(data, fp, ensure_ascii=False)
        except ValueError:
            fp.write(json.dumps(data))

    json_formatter = "./tools/format/json_formatter.cgi"
    if os.path.isfile(json_formatter):
        cmd = [json_formatter, pathname]
        subprocess.call(cmd)


def really_add_parts(xpoint, ypoint, xparts, new_parts):
    y_part = xparts.get(ypoint, {})
    if not y_part:
        return
    pretty_parts = []
    pretty_subparts = []
    temp_parts = ""
    for part in y_part["parts"]:
        temp_parts += '"{}"'.format(part)
        # debug - are we getting the line length test string correct?
        # print("{}: {}".format(len(temp_parts), temp_parts))
        if len(temp_parts) > LINE_LIMIT:
            pretty_parts.append(pretty_subparts)
            pretty_subparts = []
            temp_parts = '"{}"'.format(part)
        temp_parts += ", "
        pretty_subparts.append(part)
    pretty_parts.append(pretty_subparts)
    # debug - correct list of parts sublists?
    # print("{}".format(pretty_parts))
    for subpart_list in pretty_parts:
        if len(subpart_list) > 1:
            rev_part = {"x": xpoint, "y": ypoint, "parts": subpart_list}
        else:
            subpart = subpart_list[0]
            if isinstance(subpart, str):
                rev_part = {"x": xpoint, "y": ypoint, "part": subpart}
            else:
                rev_part = {"x": xpoint, "y": ypoint}
                for key, value in subpart.items():
                    if key == "x" or key == "y":
                        continue
                    rev_part[key] = value
        revised_parts.append(rev_part)


def add_parts(revised_parts, xpoint, last_center, new_parts):
    xparts = new_parts.get(xpoint, {})
    if not xparts:
        return 0
    min_y = 122
    max_y = -122
    for ypoint in xparts:
        min_y = min(min_y, ypoint)
        max_y = max(max_y, ypoint)

    if xpoint == 0:
        really_add_parts(xpoint, last_center, xparts, new_parts)
    for ypoint in range(min_y - 1, last_center, 1):
        really_add_parts(xpoint, ypoint, xparts, new_parts)
    if xpoint != 0:
        really_add_parts(xpoint, last_center, xparts, new_parts)
    for ypoint in range(last_center + 1, max_y + 1):
        really_add_parts(xpoint, ypoint, xparts, new_parts)
    return int((min_y + max_y) / 2)


args = argparse.ArgumentParser(
    description="Reformat vehicles using parts arrays.")
args.add_argument("vehicle_sources", action="store", nargs="+",
                  help="specify jsons file to convert to new format.")
argsDict = vars(args.parse_args())

for datafile in argsDict.get("vehicle_sources", []):
    min_x = 122
    max_x = -122
    vehicles = get_data(argsDict, "vehicle_sources")
    for old_vehicle in vehicles:
        #print(old_vehicle.get("id"))
        new_parts = {}
        for part in old_vehicle.get("parts", []):
            part_x = part.get("x", 0)
            part_y = part.get("y", 0)
            min_x = min(min_x, part_x)
            max_x = max(max_x, part_x)
            new_parts.setdefault(part_x, {})
            new_parts[part_x].setdefault(part_y, {"parts": []})
            part_list = []
            if part.get("parts"):
                for new_part in part.get("parts"):
                    new_parts[part_x][part_y]["parts"].append(new_part)
            else:
                if part.get("fuel") or part.get("ammo"):
                    new_part = copy.deepcopy(part)
                    del new_part["x"]
                    del new_part["y"]
                    new_parts[part_x][part_y]["parts"].append(new_part)
                else:
                    new_parts[part_x][part_y]["parts"].append(part.get("part"))

        # debug - did everything get copied over correctly?
        # print(json.dumps(new_parts, indent=2))
        revised_parts = []
        last_center = add_parts(revised_parts, 0, 0, new_parts)
        # print("last center {}".format(last_center))
        for xpoint in range(1, max_x + 1):
            last_center = add_parts(revised_parts, xpoint, last_center,
                                    new_parts)
            # print("x {}, last center {}".format(xpoint, last_center))
        for xpoint in range(-1, min_x - 1, -1):
            last_center = add_parts(revised_parts, xpoint, last_center,
                                    new_parts)

        old_vehicle["parts"] = revised_parts

    write_to_json(datafile, vehicles)
