#!/usr/bin/env python3

import argparse
import json
import os

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify data directory")
args_dict = vars(args.parse_args())

types = [
    "AMMO",
    "ARMOR",
    "BATTERY",
    "BIONIC_ITEM",
    "BOOK",
    "COMESTIBLE",
    "CONTAINER",
    "ENGINE",
    "GENERIC",
    "GUN",
    "GUNMOD",
    "MAGAZINE",
    "PET_ARMOR",
    "TOOL",
    "TOOLMOD",
    "TOOL_ARMOR",
]

oversized = list()
changed = dict()
affected = dict()
items = dict()
recipes = dict()
group_migration = dict()


def get_id(jo):
    if "id" in jo:
        return jo["id"]
    elif "abstract" in jo:
        return jo["abstract"]


def get_parent_value(id, property, src):
    if src not in items:
        return get_parent_value(id, property, "dda")
        
    if id not in items[src]:
        if src != "dda":
            return get_parent_value(id, property, "dda")
        else:
            return None
    if property not in items[src][id]:
        if "copy-from" in items[src][id]:
            if id == items[src][id]["copy-from"]:
                if src == "dda":
                    return None
                return get_parent_value(id, property, "dda")
            return get_parent_value(items[src][id]["copy-from"], property, src)
        else:
            return None
    return items[src][id][property]


def get_value(jo, property, src):
    if property in jo:
        return jo[property]
    if "copy-from" in jo:
        return get_parent_value(jo["copy-from"], property, src)
    return None


def get_jo(id, src):
    if src not in items:
        if src != "dda":
            return get_jo(id, "dda")
        return None
    if id not in items[src]:
        if src != "dda":
            return get_jo(id, "dda")
        return None
    return items[src][id]


def remove_charges(jo, src):
    phase = get_value(jo, "phase", src)
    if phase is not None and phase != "solid":
        count = get_value(jo, "count", src)
        if count is None:
           jo["count"] = 1
           return jo
        return

    if "stackable" in jo and jo["stackable"]:
        return

    if src not in affected:
        affected[src] = dict()
    copy_from = None
    if "copy-from" in jo:
        copy_from = jo["copy-from"]
    affected[src][get_id(jo)] = copy_from

    has_volume = "volume" in jo
    has_charges = "count" in jo or "stack_size" in jo

    if not has_volume and not has_charges:
        return

    charges = get_value(jo, "count", src)
    stack_size = get_value(jo, "stack_size", src)
    if stack_size is None:
        if charges is None:
            return
        stack_size = charges
    if charges is None:
        charges = -1

    vol = get_value(jo, "volume", src)
    vol_arr = vol.split()
    if len(vol_arr) != 2:
        pos = vol.find("ml")
        unit = "ml"
        if pos == -1:
            pos = vol.find("L")
            unit = "L"
        vol = vol[0:pos]
    else:
        vol = vol_arr[0]
        unit = vol_arr[1]
    vol = int(vol)
    if unit == "L":
        vol *= 1000
        unit = "ml"

    if vol / stack_size < 1:
        oversized.append(get_id(jo))

    vol = max(1, vol // stack_size)

    if (vol % 1000 == 0):
        vol = vol // 1000
        unit = "L"

    jo["volume"] = str(vol) + " " + unit
    if "count" in jo:
        jo.pop("count")
    if "stack_size" in jo:
        jo.pop("stack_size")
    if src not in changed:
        changed[src] = dict()
    changed[src][get_id(jo)] = [charges, stack_size]

    return jo


def multiply_values(lhs, rhs):
    left_list = type(lhs) is list
    right_list = type(rhs) is list
    if left_list or right_list:
        ret = list()
        val_min = 0
        val_max = 0
        if left_list:
            val_min = lhs[0]
            val_max = lhs[1]
        else:
            val_min = lhs
            val_max = lhs
        if right_list:
            val_min *= rhs[0]
            val_max *= rhs[1]
        else:
            val_min *= rhs
            val_max *= rhs
        ret.append(val_min)
        ret.append(val_max)
    else:
        ret = lhs * rhs

    return ret


def get_min(entry, property):
    min = -1

    if property in entry:
        min = entry[property]
        if type(min) is list:
            min = min[0]

    if property + "-min" in entry:
        min = entry.pop(property + "-min")

    return min


def get_max(entry, property):
    max = -1

    if property in entry:
        max = entry.pop(property)
        if type(max) is list:
            max = max[1]

    if property + "-max" in entry:
        max = entry.pop(property + "-max")

    return max


def get_changed(id, src):
    if src not in changed:
        if src != "dda":
            return get_changed(id, "dda")
        return None

    if id in changed[src]:
        return changed[src][id]

    if id in changed["dda"]:
        return changed["dda"][id]

    return None


def get_maybe_changed(id, src):
    changed = get_changed(id, src)
    if changed is not None:
        return changed

    if src not in affected:
        if src != "dda":
            return get_maybe_changed(id, "dda")
        return None

    if id in affected[src] and affected[src][id] is not None:
        return get_affected_parent(affected[src][id], src)

    return None


def get_affected_parent(copy_from, src):
    if src in changed and copy_from in changed[src]:
        return changed[src][copy_from]

    if src not in items or copy_from not in items[src]:
        if src == "dda":
            return None
        return get_affected_parent(copy_from, "dda")

    if "copy-from" not in items[src][copy_from]:
        return None

    if copy_from == items[src][copy_from]["copy-from"]:
        if src == "dda":
            return None
        return get_affected_parent(copy_from, "dda")

    return get_affected_parent(items[src][copy_from]["copy-from"], src)


def make_group(entry, default_container):
    jo = dict()
    jo["type"] = "item_group"
    new_entry = dict()
    new_entry["item"] = entry.pop("item")
    if default_container is not None:
        new_entry["container-item"] = "null"
    id_count_min = 0
    id_count_max = 0
    group_count = ("charges" in entry and type(entry["charges"]) is list and
                   entry["charges"][0] == 0 and entry["charges"][1] == 1)
    if group_count:
        entry["count"] = entry.pop("charges")
    if "charges" in entry:
        if type(entry["charges"]) is not list and entry["charges"] <= 1:
            entry.pop("charges")
        else:
            new_entry["count"] = entry.pop("charges")
            if type(new_entry["count"]) is list:
                id_count_min = new_entry["count"][0]
                id_count_max = new_entry["count"][1]
            else:
                id_count_min = new_entry["count"]
                id_count_max = new_entry["count"]
    if "charges-min" in entry:
        new_entry["count-min"] = entry.pop("charges-min")
        id_count_min = new_entry["count-min"]
    if "charges-max" in entry:
        new_entry["count-max"] = entry.pop("charges-max")
        id_count_max = new_entry["count-max"]
    jo["id"] = new_entry["item"] + "_" + entry["container-item"] + "_"
    if id_count_max == id_count_min:
        if id_count_max <= 0:
            id_count_max = 1
        jo["id"] += str(id_count_max)
    elif id_count_max <= 0:
        jo["id"] += str(id_count_min) + "_inf"
    else:
        jo["id"] += str(id_count_min) + "_" + str(id_count_max)
    jo["subtype"] = "collection"
    jo["//"] = "This group was created automatically and may contain errors."
    jo["container-item"] = entry.pop("container-item")
    jo["entries"] = [new_entry]
    return jo


def has_group(id, src):
    if src not in group_migration:
        if src != "dda":
            return has_group(id, "dda")
        return False
    if id not in group_migration[src]:
        if src != "dda":
            return has_group(id, "dda")
        return False
    return True


def adjust_item_group(jo, src):
    item_array = []
    if "entries" in jo:
        item_array = jo["entries"]
    elif "items" in jo:
        item_array = jo["items"]
    else:
        return

    ret = [jo]
    change = False
    for entry in item_array:
        if "item" not in entry:
            continue

        use_src = src
        if src not in affected:
            use_src = "dda"
        affected_item = None
        is_ammo = False
        if entry["item"] in affected[use_src]:
            affected_item = entry["item"]
        elif "ammo-item" in entry and entry["ammo-item"] in affected[use_src]:
            is_ammo = True
            affected_item = entry["ammo-item"]

        if affected_item is None:
            continue

        has_charges = ("charges" in entry or "charges-min" in entry or
                       "charges-max" in entry)

        base_item = get_jo(affected_item, src)
        if not has_charges:
            base_charges = get_value(base_item, "charges", src)
            if base_charges is not None and base_charges > 1:
                entry["charges"] = base_charges
            else:
                continue

        if not is_ammo:
            default_container = get_value(base_item, "container", src)
            if ("container-item" not in entry and
                    default_container is not None and
                    default_container != "null"):
                entry["container-item"] = default_container

        if "container-item" in entry:
            entry["entry-wrapper"] = entry.pop("container-item")

        change = True
        charge_arr = "charge" in entry and type(entry["charge"]) is list
        make_min = ("count-min" in entry or "charges-min" in entry or
                    ("count-max" in entry and charge_arr))
        make_max = ("count-max" in entry or "charges-max" in entry or
                    ("count-min" in entry and charge_arr))

        changes = get_maybe_changed(affected_item, src)
        default = 1
        if changes is not None:
            default = changes[0]

        if make_min:
            # get_min must be called before get_max
            count_min = get_min(entry, "count")
            charges_min = get_min(entry, "charges")
            if charges_min < 0 and default > 1:
                charges_min = default
            if count_min < 0:
                if charges_min >= 0:
                    entry["count-min"] = charges_min
            else:
                if charges_min >= 0:
                    entry["count-min"] = count_min * charges_min
                else:
                    entry["count-min"] = count_min
        if make_max:
            count_max = get_max(entry, "count")
            charges_max = get_max(entry, "charges")
            if charges_max < 0 and default > 1:
                charges_max = default
            if count_max < 0:
                if charges_max >= 0:
                    entry["count-max"] = charges_max
            else:
                if charges_max >= 0:
                    entry["count-max"] = count_max * charges_max
                else:
                    entry["count-max"] = count_max
        if not make_min and not make_max:
            if "count" in entry:
                if "charges" in entry:
                    entry["count"] = multiply_values(entry["count"],
                                                     entry.pop("charges"))
                elif default > 1:
                    entry["count"] = multiply_values(entry["count"], default)
                else:
                    change = False
            else:
                if "charges" in entry:
                    charges = entry.pop("charges")
                    if type(charges) is list or charges > 1:
                        entry["count"] = charges
                elif default > 1:
                    entry["count"] = default
                else:
                    change = False

    if change:
        return ret


def get_parent_charges(result, src):
    if src not in recipes:
        if src != "dda":
            return get_parent_charges(result, "dda")
        return None

    if result not in recipes[src]:
        if src != "dda":
            return get_parent_charges(result, "dda")
        return None

    if "charges" in recipes[src][result]:
        return recipes[src][result]["charges"]

    if "copy-from" in recipes[src][result]:
        return get_parent_charges(recipes[src][result]["copy-from"], src)

    return None


def adjust_recipe(jo, src):
    if "charges" in jo or "result" not in jo:
        return

    aff_it = get_maybe_changed(jo["result"], src)

    if aff_it is None or aff_it[0] <= 1:
        return

    if "copy-from" in jo:
        charges = get_parent_charges(jo["copy-from"], src)
        if charges is not None:
            return

    jo["charges"] = aff_it[0]
    return jo


def gen_new(path, src, is_item=True):
    change = False
    new_datas = list()
    # Having troubles with this script halting?
    # Uncomment below to find the file it's halting on
    #print(path)
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

            if "type" not in jo:
                continue

            new_data = None
            if is_item and jo["type"] == "AMMO":
                new_data = remove_charges(jo, src)
            if not is_item and jo["type"] == "item_group":
                new_groups = adjust_item_group(jo, src)
                if type(new_groups) is list and len(new_groups) > 0:
                    new_data = new_groups[0]
                    if len(new_groups) > 1:
                        new_datas.extend(new_groups[1:])
                else:
                    new_data = new_groups
            if not is_item and jo["type"] == "recipe":
                new_data = adjust_recipe(jo, src)

            if new_data is not None:
                jo = new_data
                change = True

    if len(new_datas) > 0:
        json_data.extend(new_datas)
    return json_data if change else None


def format_json(path):
    file_path = os.path.dirname(__file__)
    format_path_linux = os.path.join(file_path, "../format/json_formatter.cgi")
    path_win = "../format/JsonFormatter-vcpkg-static-Release-x64.exe"
    format_path_win = os.path.join(file_path, path_win)
    if os.path.exists(format_path_linux):
        os.system(f"{format_path_linux} {path}")
    elif os.path.exists(format_path_win):
        os.system(f"{format_path_win} {path}")
    else:
        print("No json formatter found")


def collect_data(path, src):
    with open(path, "r", encoding="utf-8") as json_file:
        try:
            json_data = json.load(json_file)
        except json.JSONDecodeError:
            print(f"Json Decode Error at {path}")
            print("Ensure that the file is a JSON file consisting of"
                  " an array of objects!")
            return

        for jo in json_data:
            if type(jo) is not dict:
                print(f"Incorrectly formatted JSON data at {path}")
                print("Ensure that the file is a JSON file consisting"
                      " of an array of objects!")
                break

            if "type" not in jo:
                continue

            if jo["type"] in types:
                item = dict()
                if "volume" in jo:
                    item["volume"] = jo["volume"]
                if "count" in jo:
                    item["count"] = jo["count"]
                if "stack_size" in jo:
                    item["stack_size"] = jo["stack_size"]
                if "copy-from" in jo:
                    item["copy-from"] = jo["copy-from"]
                if "container" in jo:
                    item["container"] = jo["container"]
                if "phase" in jo:
                    item["phase"] = jo["phase"]

                if src not in items:
                    items[src] = dict()
                items[src][get_id(jo)] = item

            if jo["type"] == "recipe" and "result" in jo:
                recipe = dict()
                if "charges" in jo:
                    recipe["charges"] = jo["charges"]
                if "copy-from" in jo:
                    recipe["copy-from"] = jo["copy-from"]

                if src not in recipes:
                    recipes[src] = dict()
                rec_id = jo["result"]
                if "id_suffix" in jo:
                    rec_id += "_" + jo["id_suffix"]
                recipes[src][rec_id] = recipe


# collect item data with source
json_path = os.path.join(args_dict["dir"], "json")
for root, directories, filenames in os.walk(json_path):
    for filename in filenames:
        if not filename.endswith(".json"):
            continue

        path = os.path.join(root, filename)
        collect_data(path, "dda")

for f in os.scandir(os.path.join(args_dict["dir"], "mods")):
    if f.is_dir():
        for root, directories, filenames in os.walk(f.path):
            for filename in filenames:
                if not filename.endswith(".json"):
                    continue

                path = os.path.join(root, filename)
                collect_data(path, f.name)

# adjust items
for root, directories, filenames in os.walk(json_path):
    for filename in filenames:
        if not filename.endswith(".json"):
            continue

        path = os.path.join(root, filename)
        new = gen_new(path, "dda")
        if new is not None:
            with open(path, "w", encoding="utf-8") as jf:
                json.dump(new, jf, ensure_ascii=False)
            format_json(path)

for f in os.scandir(os.path.join(args_dict["dir"], "mods")):
    if f.is_dir():
        for root, directories, filenames in os.walk(f.path):
            for filename in filenames:
                if not filename.endswith(".json"):
                    continue

                path = os.path.join(root, filename)
                new = gen_new(path, f.name)
                if new is not None:
                    with open(path, "w", encoding="utf-8") as jf:
                        json.dump(new, jf, ensure_ascii=False)
                    format_json(path)

# adjust itemgroups
for root, directories, filenames in os.walk(json_path):
    for filename in filenames:
        if not filename.endswith(".json"):
            continue

        path = os.path.join(root, filename)
        new = gen_new(path, "dda", False)
        if new is not None:
            with open(path, "w", encoding="utf-8") as jf:
                json.dump(new, jf, ensure_ascii=False)
            format_json(path)

for f in os.scandir(os.path.join(args_dict["dir"], "mods")):
    if f.is_dir():
        for root, directories, filenames in os.walk(f.path):
            for filename in filenames:
                if not filename.endswith(".json"):
                    continue

                path = os.path.join(root, filename)
                new = gen_new(path, f.name, False)
                if new is not None:
                    with open(path, "w", encoding="utf-8") as jf:
                        json.dump(new, jf, ensure_ascii=False)
                    format_json(path)

with open('oversized_charge_removal.txt', 'w') as outfile:
    outfile.write('\n'.join(id for id in oversized))

with open('removed_charges.txt', 'w') as outfile:
    header = 'id | previous default charges | previous stack_size | src\n'
    outfile.write(header)
    for src, it in changed.items():
        outfile.write('\n'.join(f'{k} {v[0]} {v[1]} {src}' for k, v in
                                it.items()) + '\n')

with open('itemgroup_migration.txt', 'w') as outfile:
    header = 'new group id | src\n'
    outfile.write(header)
    for src, groups in group_migration.items():
        outfile.write('\n'.join(f'{group} {src}' for group in groups) + '\n')
