#!/usr/bin/env python3

import argparse
import json
import os

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args_dict = vars(args.parse_args())


class portion_data:
    encumbrance = -1
    max_encumbrance = -1
    coverage = -1
    covers = []
    sided = False


def portion_null(dat):
    ret = dat.encumbrance == -1
    ret = ret and dat.max_encumbrance == -1
    ret = ret and dat.coverage == -1
    ret = ret and len(dat.covers) == 0
    ret = ret and not dat.sided
    return ret


def gen_entry(dat):
    added = dict()

    if dat.encumbrance != -1:
        added["encumbrance"] = dat.encumbrance
        if dat.max_encumbrance != -1:
            added["encumbrance"] = [dat.encumbrance, dat.max_encumbrance]
    if dat.coverage != -1:
        added["coverage"] = dat.coverage
    if len(dat.covers) != 0:
        added["covers"] = dat.covers
    if dat.sided:
        added["sided"] = True

    return added


def portionize(jo):
    dat = portion_data()
    if "encumbrance" in jo:
        dat.encumbrance = jo["encumbrance"]
        del jo["encumbrance"]
    if "max_encumbrance" in jo:
        dat.max_encumbrance = jo["max_encumbrance"]
        del jo["max_encumbrance"]
    if "coverage" in jo:
        dat.coverage = jo["coverage"]
        del jo["coverage"]
    if "covers" in jo:
        dat.covers = jo["covers"]
        del jo["covers"]
    if "sided" in jo:
        dat.sided = jo["sided"]
        del jo["sided"]

    if portion_null(dat):
        return None

    if "armor_portion_data" not in jo:
        added = gen_entry(dat)
        jo["armor_portion_data"] = [added]
    else:
        added = gen_entry(dat)
        added["//0"] = "Autogenned for item with non-portion and portion data"
        added["//1"] = "Ensure that this works correctly!"
        jo["armor_portion_data"].insert(0, added)

    return jo


def gen_new(path):
    change = False
    # Having troubles with this script halting?
    # Uncomment below to find the file it's halting on
    #print(path)
    with open(path, "r") as json_file:
        json_data = json.load(json_file)
        for jo in json_data:
            if "type" not in jo:
                continue
            if (jo["type"] != "ARMOR" and
                jo["type"] != "TOOL_ARMOR" and
               "armor_data" not in jo):
                continue

            if "armor_data" in jo:
                new_data = portionize(jo["armor_data"])
                if new_data is not None:
                    jo["armor_data"] = new_data
                    change = True
                continue

            # I don't know python - assiging to jo is not necessary here
            # but is necessary for "armor_data" above
            new_jo = portionize(jo)
            if new_jo is not None:
                change = True

    return json_data if change else None


for root, directories, filenames in os.walk(args_dict["dir"]):
    for filename in filenames:
        path = os.path.join(root, filename)
        if path.endswith(".json"):
            new = gen_new(path)
            if new is not None:
                with open(path, "w") as jf:
                    json.dump(new, jf, ensure_ascii=False)
                os.system(f"./tools/format/json_formatter.cgi {path}")
