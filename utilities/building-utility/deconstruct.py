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
    file_path = os.path.join(os.getcwd(), 'drawing.txt')
    infile = open(file_path,"r")
    data = '[\n'
    mapList = [""] * 9
    for line in infile:
        lineCT = 0
        while len(line) > 23:
            mapList[lineCT] += '\t\t\t\t' + line[:24] + '"'

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
    infile.close()
    data = data[:-1] + "\n]"
    finalizeEntry(data)

#Takes the mapNum and 'text' is the 24x24 map
def writeJSON(mapNum, text, data):
    entry = ''
    file_path = os.path.join(os.getcwd(), 'json_header.txt')
    json_header_file = open(file_path,"r")
    for line in json_header_file:
        entry = entry + line
    json_header_file.close()

    entry = entry + str(mapNum)

    file_path = os.path.join(os.getcwd(), 'json_middle.txt')
    json_middle_file = open(file_path,"r")
    for line in json_middle_file:
        entry = entry + line
    json_middle_file.close()

    entry = entry + text

    file_path = os.path.join(os.getcwd(), 'json_footer.txt')
    json_footer_file = open(file_path,"r")
    for line in json_footer_file:
        entry = entry + line
    json_footer_file.close()

    data = data + entry
    return data

def finalizeEntry(data):
    file_path = os.path.join(os.getcwd(), 'output', 'office_tower.json')
    outfile = open(file_path,"w")
    outfile.write(data)
    outfile.close()

def writeTerrain(mapNum):
    entry = ''
    file_path = os.path.join(os.getcwd(), 'terrain_header.txt')
    terrain_header_file = open(file_path,"r")
    for line in terrain_header_file:
        entry = entry + line
    terrain_header_file.close()

    entry = entry + str(mapNum)

    file_path = os.path.join(os.getcwd(), 'terrain_footer.txt')
    terrain_footer_file = open(file_path,"r")
    for line in terrain_footer_file:
        entry = entry + line
    terrain_footer_file.close()

    file_path = os.path.join(os.getcwd(), 'output',
                             'overmap_terrain_entry.json')
    outfile = open(file_path,"a")
    outfile.write(entry)
    outfile.close()

if __name__ == "__main__":
    main()

