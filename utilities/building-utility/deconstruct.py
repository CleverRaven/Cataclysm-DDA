# This program runs well with PYTHON 2.5 but may not
# run without an error with PYTHON 2.2
#
# I (acidia) just made this as a quick utility to change a character map in
# drawing.txt into a json that the game can process.  You'll have to modify the
# paths in this folder unless you decide to put all the files in
# 'c:\building-utility' and create a folder called 'output' for it to drop the
# finished result in.  This is in no way polished, it just saved me a huge
# amount of time creating 9x9 buildings so I figured someone else might be able
# to use it.

import os


def main():
    mapCount = 1
    with open(os.path.join(os.getcwd(), 'drawing.txt'),"r") as infile:
        data = '[\n'
        mapList = [""] * 9
        for line in infile:
            lineCT = 0
            while len(line) > 23:
                mapList[lineCT] += '\t\t\t\t"' + line[:24] + '"'

                if len(mapList[lineCT]) < 725:
                    mapList[lineCT] += ','
                mapList[lineCT] += '\n'

                line = line[24:]
                lineCT += 1
            if len(mapList[0]) >= 760:
                for x in range(len(mapList)):
                    if len(mapList[x]) > 20:
                        data = writeJSON(mapCount, mapList[x], data)
                        writeTerrain(mapCount)
                        mapCount += 1
                mapList = [""] * 9

    data = data[:-1] + "\n]"
    finalizeEntry(data)

#Takes the mapNum and 'text' is the 24x24 map
def writeJSON(mapNum, text, data):
    entry = ''
    with open(os.path.join(os.getcwd(), 'json_header.txt'),
              "r") as json_header_file:
        entry += ''.join(json_header_file)

    entry += str(mapNum)

    with open(os.path.join(os.getcwd(), 'json_middle.txt'),
              "r") as json_middle_file:
        entry += ''.join(json_middle_file)

    entry += text

    with open(os.path.join(os.getcwd(), 'json_footer.txt'),
              "r") as json_footer_file:
        entry += ''.join(json_footer_file)

    data = data + entry
    return data

def finalizeEntry(data):
    with open(os.path.join(os.getcwd(), 'output', 'office_tower.json'),
              "w") as outfile:
        outfile.write(data)

def writeTerrain(mapNum):
    entry = ''
    with open(os.path.join(os.getcwd(), 'terrain_header.txt'),
              "r") as terrain_header_file:
        entry += ''.join(terrain_header_file)

    entry += str(mapNum)

    with open(os.path.join(os.getcwd(), 'terrain_footer.txt'),
              "r") as terrain_footer_file:
        entry += ''.join(terrain_footer_file)

    with open(os.path.join(os.getcwd(), 'output',
                           'overmap_terrain_entry.json'),
              "a") as outfile:
        outfile.write(entry)

if __name__ == "__main__":
    main()

