import json
import argparse
from collections import OrderedDict
from math import hypot
import re

args = argparse.ArgumentParser(description="Generate a json definition for a vehicle in a Cataclysm DDA save file.")
args.add_argument("save", action="store", help="specify save file containing vehicle")
args.add_argument("vehicle", action="store", help="specify name of vehicle")
argsDict = vars(args.parse_args())

def findVehicle():
    with open(argsDict["save"]) as saveFile:
        saveData = json.load(saveFile)
        for i in range(0, len(saveData)):
            for vehicle in saveData[i]["vehicles"]:
                if (argsDict["vehicle"] == vehicle["name"]):
                    print("Vehicle {} found".format(vehicle['name']))
                    return vehicle

def writeDef(vehicle):
    partsDef = []
    for part in vehicle["parts"]:
        partsDef.append(OrderedDict([
            ("x", part["mount_dx"]),
            ("y", part["mount_dy"]), 
            ("part", part["id"])
        ]))

    frames = [p for p in partsDef if re.match(r'(xl)?frame', p["part"]) != None]
    everythingElse = [p for p in partsDef if re.match(r'(xl)?frame', p["part"]) == None]

    frames = sortFrames(frames)

    vehicleDef = OrderedDict([
        ("id", argsDict["vehicle"]),
        ("type", "vehicle"),
        ("name", argsDict["vehicle"]),
        ("parts", frames + everythingElse)
    ])

    with open("{}.json".format(argsDict['vehicle']), "w") as vehicleDefJson:
        json.dump(vehicleDef, vehicleDefJson, indent=4)
        print("Vehicle def written.")

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

writeDef(findVehicle())
