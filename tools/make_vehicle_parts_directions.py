import argparse
import json
import os
import subprocess

default_path = "data/json/vehicleparts/rams.json"
default_output = "data/json/vehicleparts/rams2.json"
ids_created_output = "data/json/vehicleparts/new_ids.txt"

ids_to_create_directional_parts_for = []
list_of_created_ids = []

variants = {
    "nw" : "y",
    "vertical" : "j",
    "ne" : "u",
    "sw" : "b",
    "vertical_2" : "H",
    "se" : "n",
    "cover" : "^",
    "horizontal" : "h",
    "horizontal_2" : "=",
    "cross" : "c",
    "box" : "#"
}

def gen_new(path):
    with open(path, "r") as json_file:
        json_data = json.load(json_file)
        for jo in json_data:
            if "id" in jo:
                jo["abstract"] = jo["id"]
                jo.pop("id")
                ids_to_create_directional_parts_for.append(jo["abstract"])

        for key, value in variants.items():
            for ram in ids_to_create_directional_parts_for:
                new_ram_id = ram + "_" + key
                new_ram_data = {
                    "id": new_ram_id,
                    "copy-from": ram,
                    "symbol": value,
                    "type": "vehicle_part"
                }
                json_data.append(new_ram_data)
                list_of_created_ids.append(new_ram_id)
        return json_data
   
path = os.path.join(os.getcwd(), default_path)
output_path = os.path.join(os.getcwd(), default_output)
result = gen_new(path)
print(*ids_to_create_directional_parts_for, sep = "\n")

with open(output_path, "w") as jf:
    json.dump(result, jf, ensure_ascii=False, sort_keys=True)
    subprocess.Popen(['D:/Cataclysm-DDA/tools/format/json_formatter.exe', output_path], shell=True)

with open(ids_created_output, "w") as jf:
    for id in list_of_created_ids:
        jf.write(id + "\n")