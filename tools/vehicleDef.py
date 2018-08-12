import json
import argparse
from collections import OrderedDict
from math import hypot
import re
import os

args = argparse.ArgumentParser(description="Generate a json definition for a vehicle in a Cataclysm DDA save file.")
args.add_argument("save", action="store", help="specify save file containing vehicle")
args.add_argument("vehicle", nargs="?", help="specify name of vehicle", default=None)
argsDict = vars(args.parse_args())

def writeVehicleTemplates(templates):
    with open("vehicles.json", "w") as vehicleDefJson:
        json.dump(templates, vehicleDefJson, indent=4)
        print("Vehicle defs written.")

def getVehicleTemplates():
    vehicles = []
    for root, directories, filenames in os.walk(argsDict["save"]):
        for filename in filenames: 
            path = os.path.join(root,filename)
            if path.endswith(".map"):
                vehicles += getVehicleInstances(path)
    allTemplates = [buildVehicleDef(vehicle) for vehicle in vehicles]
    return allTemplates

def getVehicleInstances(mapPath):
    vehicles = []
    with open(mapPath) as mapFile:
        mapData = json.load(mapFile)
        for i in range(0, len(mapData)):
            for vehicle in mapData[i]["vehicles"]:
                if argsDict["vehicle"] != None:
                    if argsDict["vehicle"] == vehicle["name"]:
                        vehicles.append(vehicle)
                        print("Found '{}'".format(vehicle["name"]))
                else:
                    vehicles.append(vehicle)
                    print("Found '{}'".format(vehicle["name"]))
    return vehicles

def buildVehicleDef(vehicle):
    partsDef = []
    itemsDef = []
    for part in vehicle["parts"]:
        partsDef.append(OrderedDict([
            ("x", part["mount_dx"]),
            ("y", part["mount_dy"]), 
            ("part", part["id"])
        ]))

        for item in part["items"]:
            itemsDef.append(OrderedDict([
                ("x", part["mount_dx"]),
                ("y", part["mount_dy"]), 
                ("chance", 100), 
                ("items", [item["typeid"]])
            ]))

    frames = [p for p in partsDef if re.match(r'(xl|hd|folding_)?frame', p["part"]) != None]
    everythingElse = [p for p in partsDef if re.match(r'(xl|hd|folding_)?frame', p["part"]) == None]

    frames = sortFrames(frames)

    itemsDef.sort(key=lambda i: (i["x"], i["y"])) 

    vehicleDef = OrderedDict([
        ("id", vehicle["name"]),
        ("type", "vehicle"),
        ("name", vehicle["name"]),
        ("parts", frames + everythingElse),
        ("items", itemsDef)
    ])

    return vehicleDef

def sortFrames(frames):
    sortedFrames = []
    sortedFrames.append(frames.pop())

    while len(frames) > 0:
        nextFrame = frames.pop()
        found = False
        for lastFrame in sortedFrames:
            if adjacent(lastFrame, nextFrame):
                sortedFrames.append(nextFrame)
                found = True
                break

        if not found:
            frames.insert(0, nextFrame)

    return sortedFrames
    
def adjacent(frame1, frame2):
    return hypot(frame1["x"] - frame2["x"], frame1["y"] - frame2["y"]) == 1

writeVehicleTemplates(getVehicleTemplates())