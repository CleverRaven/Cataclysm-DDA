#!/usr/bin/env python3

#Converts `region_settings` type to `region_settings_new` and components
#Untested for off-repo mods

import argparse
import json
import os

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args.add_argument("modname", action="store", help="provide mod name as suffix")
args_dict = vars(args.parse_args())

def object_key_check(jo, object_key, old_field, new_field):
    if object_key not in jo:
        return None
    if old_field in jo[object_key]:
        jo[object_key][new_field] = jo[object_key][old_field]
        del jo[object_key][old_field]
    return jo

def gen_new(path, modname):
    with open(path, "r", encoding="utf-8") as json_file:
        try:
            json_data = json.load(json_file)
        except json.JSONDecodeError:
            print(f"Json Decode Error at {path}")
            print("Ensure that the file is a JSON file consisting of"
                  " an array of objects!")
            return None
    
    if len(json_data) > 1:
        print("ERROR: can only parse one region_settings object at a time")
    region_obj = json_data[0]
    new_json_data = [region_obj]
    
    #unmodified
    default_oter = region_obj.get("default_oter")
    default_groundcover = region_obj.get("default_groundcover")
    
    overmap_connection_settings = region_obj.get("overmap_connection_settings")
    #modified
    overmap_river_settings = region_obj.get("overmap_river_settings")
    overmap_lake_settings = region_obj.get("overmap_lake_settings")
    overmap_ocean_settings = region_obj.get("overmap_ocean_settings")
    overmap_ravine_settings = region_obj.get("overmap_ravine_settings")
    overmap_forest_settings = region_obj.get("overmap_forest_settings")
    overmap_highway_settings = region_obj.get("overmap_highway_settings")
    forest_trail_settings = region_obj.get("forest_trail_settings")
    city = region_obj.get("city")
    weather = region_obj.get("weather")
    overmap_feature_flag_settings = region_obj.get("overmap_feature_flag_settings")
    
    #complex
    map_extras = region_obj.get("map_extras")
    region_terrain_and_furniture = region_obj.get("region_terrain_and_furniture")
    forest_mapgen_settings = region_obj.get("forest_mapgen_settings")
    
    #loaded?
    loaded_oter = default_oter is not None
    loaded_groundcover = default_groundcover is not None
    loaded_rivers = overmap_river_settings is not None
    loaded_lake = overmap_lake_settings is not None
    loaded_ocean = overmap_ocean_settings is not None
    loaded_ravine = overmap_ravine_settings is not None
    loaded_forest = overmap_forest_settings is not None
    loaded_highway = overmap_highway_settings is not None
    loaded_foresttrail = forest_trail_settings is not None
    loaded_city = city is not None
    loaded_weather = weather is not None
    loaded_ffs = overmap_feature_flag_settings is not None
    loaded_mx = map_extras is not None
    loaded_terfurn = region_terrain_and_furniture is not None
    loaded_forestcomp = forest_mapgen_settings is not None

    new_region_settings = new_fields("region_settings_new", "default", modname, "default", None, False) 
    
    no_rivers = loaded_rivers and overmap_river_settings.get("river_scale") is not None and overmap_river_settings["river_scale"] == 0
    no_lakes = loaded_lake and overmap_lake_settings.get("noise_threshold_lake") is not None and overmap_lake_settings["noise_threshold_lake"] >= 1.0
    no_ocean = loaded_ocean and overmap_ocean_settings.get("noise_threshold_ocean") is not None and overmap_ocean_settings["noise_threshold_ocean"] >= 1.0
    no_forests = loaded_forest and overmap_forest_settings.get("noise_threshold_forest") is not None and overmap_forest_settings["noise_threshold_forest"] >= 1.0
    no_ravines = loaded_ravine and overmap_ravine_settings.get("num_ravines") is not None and overmap_ravine_settings["num_ravines"] == 0
    no_foresttrail = loaded_foresttrail and forest_trail_settings.get("chance") is not None and forest_trail_settings["chance"] == 0
    
    default_setting = "default_" + modname
    if loaded_rivers:
        new_region_settings["rivers"] = "no_rivers" if no_rivers else default_setting
    if loaded_lake:
        new_region_settings["lakes"] = "no_lakes" if no_lakes else default_setting
    if loaded_ocean:
        new_region_settings["ocean"] = "no_ocean" if no_ocean else default_setting
    if loaded_forest:
        new_region_settings["forests"] = "no_forests" if no_forests else default_setting
    #no ravines is DDA default
    if loaded_ravine:
        new_region_settings["ravines"] = "default" if no_ravines else default_setting
    if loaded_foresttrail:
        new_region_settings["forest_trails"] = "no_forest_trails" if no_foresttrail else default_setting
    if loaded_forestcomp:
        new_region_settings["forest_composition"] = default_setting
    #currently, all existing highway settings are copies of DDA
    #if loaded_highway:
    #    new_region_settings["highways"] = default_setting
    if loaded_city:
        new_region_settings["cities"] = default_setting
    if loaded_mx:
        new_region_settings["map_extras"] = default_setting
    if loaded_terfurn:
        new_region_settings["terrain_furniture"] = default_setting
    if loaded_weather:
        new_region_settings["weather"] = default_setting
    
    #all OM connections are copied from DDA
    #new_region_settings["connections"] = overmap_connection_settings
    if loaded_oter:
        new_region_settings["default_oter"] = default_oter
    if loaded_groundcover:
        new_region_settings["default_groundcover"] = default_groundcover
    if loaded_ffs:
        new_region_settings["feature_flag_settings"] = copy_fields(overmap_feature_flag_settings, {})
    
    if not no_rivers:
        new_json_data.append(new_fields("region_settings_river", "default", modname, "default", overmap_river_settings, True))
    if not no_lakes:
        new_json_data.append(new_fields("region_settings_lake", "default", modname, "default", overmap_lake_settings, True))
    if not no_ocean:
        new_json_data.append(new_fields("region_settings_ocean", "default", modname, "default", overmap_ocean_settings, True))
    if not no_ravines:
        new_json_data.append(new_fields("region_settings_ravine", "default", modname, "default", overmap_ravine_settings, True))
    if not no_forests:
        new_json_data.append(new_fields("region_settings_forest", "default", modname, "default", overmap_forest_settings, True))
    if not no_foresttrail:
        new_json_data.append(new_fields("region_settings_forest_trail", "default", modname, "default", forest_trail_settings, True))
    new_json_data.append(new_fields("region_settings_city", "default", modname, "default", city, True))
    new_json_data.append(new_fields("weather_generator", "default", modname, "default", weather, True))

    map_extra_ids = []
    map_extra_settings = new_fields("region_settings_map_extras", "default", modname, "default", None, False)

    if map_extras is not None:
        for k,v in map_extras.items():
            new_map_collection = new_fields("map_extra_collection", k, modname, None, v, True)
            new_json_data.append(new_map_collection)
            map_extra_ids.append(new_map_collection["id"])
        map_extra_settings["extras"] = map_extra_ids
        new_json_data.append(map_extra_settings)
    
    ter_furn_keys = []
    ter_furn_settings = new_fields("region_settings_terrain_furniture", "default", modname, "default", None, False)
    
    if region_terrain_and_furniture is not None:
        ter_results = ter_furn(region_terrain_and_furniture, modname, "ter_id", "terrain")
        new_json_data = new_json_data + ter_results[0]
        ter_furn_keys = ter_furn_keys + ter_results[1]
        
        furn_results = ter_furn(region_terrain_and_furniture, modname, "furn_id", "furniture")
        new_json_data = new_json_data + furn_results[0]
        ter_furn_keys = ter_furn_keys + furn_results[1]
            
        ter_furn_settings["ter_furn"] = ter_furn_keys
        new_json_data.append(ter_furn_settings)
    
    forest_mapgen_new = new_fields("region_settings_forest_mapgen", "default", modname, "default", None, False)
    if forest_mapgen_settings is not None:
        biome_keys = []
        for k,v in forest_mapgen_settings.items():
            returned_pair = forest_biome(k, v, modname)
            if returned_pair[0] is not None:
                new_json_data.append(returned_pair[0])
                biome_keys.append(returned_pair[0]["id"])
            if len(returned_pair[1]) > 0:
                for feature in returned_pair[1]:
                    new_json_data.append(feature)
        
        if len(biome_keys) > 0:
            forest_mapgen_new["biomes"] = biome_keys
        new_json_data.append(forest_mapgen_new)
    
    new_json_data_filtered = []
    new_json_data.append(new_region_settings)
    for jo in new_json_data:
        if len(jo.items()) > 0:
            new_json_data_filtered.append(jo)
    
    return new_json_data_filtered

def new_fields(typename, id, modname, copy_from_id=None, copy=None, init_from_old=False):
    jo = dict()
    if init_from_old is True and copy is None:
        return jo
    jo["type"] = typename
    jo["id"] = id + "_" + modname
    if copy_from_id:
        jo["copy-from"] = copy_from_id
    if copy:
        jo = copy_fields(copy, jo)
    return jo

def copy_fields(old_jo, new_jo):
    for k, v in old_jo.items():
        if not blacklist(k):
            v = check_weighted_list(k, v)
            new_jo[k] = v
    return new_jo
    
def forest_biome(k, v, modname):
    features = []
    feature_ids = []
    biome_obj = None
    if v.get("components") is not None:
        for k2, v2 in v["components"].items():
            returned_pair = forest_feature(k2, v2, modname, k)
            features.append(returned_pair[0])
            feature_ids.append(returned_pair[1])
    
        biome_obj = new_fields("forest_biome_mapgen", "biome_"+k, modname, "biome_"+k+"_"+"default", v)
        old_tf = v.get("terrain_furniture")
        if old_tf is not None:
            new_tf = {}
            for k3, v3 in old_tf.items():
                new_tf[k3] = copy_fields(v3, {})
            biome_obj["terrain_furniture"] = new_tf
        biome_obj["components"] = feature_ids
    return (biome_obj, features)
    
def forest_feature(k, v, modname, biome_key):
    feature_obj = new_fields("forest_biome_feature", k+"_"+biome_key, modname, k+"_"+biome_key, v)
    return (feature_obj, feature_obj["id"])
    
def ter_furn(region_terrain_and_furniture, modname, id, type):
    returned_obj = []
    returned_keys = []
    for k,v in region_terrain_and_furniture[type].items():
        key_string = "default_" + k
        ter_obj = new_fields("region_terrain_furniture", key_string, "")
        ter_obj["id"] = ter_obj["id"][:-1]
        ter_obj[id] = k
        ter_obj["replace_with_"+type] = convert_weighted_list(v)
        returned_obj.append(ter_obj)
        returned_keys.append(key_string)
    return (returned_obj, returned_keys)
    
def blacklist(blacklist_key):
    blacklist = ["clear_blacklist", "clear_whitelist", "clear_groundcover", "clear_components", "clear_types", "clear_terrain_furniture", "clear_furniture"]
    return blacklist_key in blacklist

def check_weighted_list(k, jv):
    weighted_keys = ["trailheads", "houses", "parks", "shops", "types", "extras"]
    if k not in weighted_keys:
        return jv
    #v is assumed to be dict
    return convert_weighted_list(jv)

def convert_weighted_list(jv):
    weighted_list = []
    for k, v in jv.items():
        weighted_list.append([k, v])
    return weighted_list

if __name__ == "__main__":
    
    new = gen_new(args_dict["dir"], args_dict["modname"])
    if new is not None:
        with open(args_dict["dir"], "w", encoding="utf-8") as jf:
            json.dump(new, jf, ensure_ascii=False)
    