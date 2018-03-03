import json
import argparse

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
        partsDef.append({"x": part["mount_dx"],
                         "y": part["mount_dy"],
                         "part": part["id"]})

    vehicleDef = {"id": argsDict["vehicle"],
                   "type": "vehicle",
                   "name": argsDict["vehicle"],
                   "parts": partsDef}

    with open("{}.json".format(argsDict['vehicle']), "w") as vehicleDefJson:
        json.dump(vehicleDef, vehicleDefJson, indent=4)
        print("Vehicle def written.")

writeDef(findVehicle())
