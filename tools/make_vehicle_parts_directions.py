import json
import os
import subprocess

default_path = "data/json/vehicleparts/doors.json"
default_output = "data/json/vehicleparts/doors.json"
ids_created_output = "data/json/vehicleparts/new_ids.txt"

ids_to_create_directional_parts_for = []
list_of_created_ids = []

variants = {
    "default_look" : "M",
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

#variants = {
#    "default_look" : "M",
#    "up" : "n",
#    "down" : "u",
#    "left" : "d",
#    "right" : "b",
#}

def gen_new(path):
    with open(path, "r") as json_file:
        json_data = json.load(json_file)
        for jo in json_data:
            if "id" in jo and jo["type"] == "vehicle_part":
                jo["abstract"] = jo["id"] + "_abstract"
                original_symbol = ""
                if "symbol" in jo:
                    original_symbol = jo["symbol"]
                
                ids_to_create_directional_parts_for.append({"id":jo["id"], "symbol": original_symbol})
                jo.pop("id")

        for key, value in variants.items():
            for ram in ids_to_create_directional_parts_for:
                new_ram_id = ram["id"] + "_" + key
                copyfrom = ram["id"] + "_abstract"
                if(key == "default_look"):
                    new_ram_id = ram["id"]
                    if(ram["symbol"] != ""):
                        value = ram["symbol"]

                new_ram_data = {
                    "id": new_ram_id,
                    "copy-from": copyfrom,
                    "symbol": value,
                    "type": "vehicle_part"
                }
                json_data.append(new_ram_data)
                list_of_created_ids.append(new_ram_id)
        return json_data
   
path = os.path.join(os.getcwd(), default_path)
output_path = os.path.join(os.getcwd(), default_output)
result = gen_new(path)

json_formatter_path = os.path.join(os.getcwd(), "tools/format/json_formatter.exe")

with open(output_path, "w") as jf:
    json.dump(result, jf, ensure_ascii=False, sort_keys=True)
    subprocess.Popen([json_formatter_path, output_path], shell=True)

with open(ids_created_output, "w") as jf:
    for id in list_of_created_ids:
        jf.write(id + "\n")