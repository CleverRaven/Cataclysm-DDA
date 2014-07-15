# This program runs well with PYTHON 2.5 but may not
# run without an error with PYTHON 2.2
#
# I (acidia) just made this as a quick utility to change a character map in drawing.txt
#into a json that the game can process.  You'll have to modify the paths in this
#folder unless you decide to put all the files in 'c:\building-utility' and create
#a folder called 'output' for it to drop the finished result in.  This is in no
#way polished, it just saved me a huge amount of time creating 9x9 buildings so
#I figured someone else might be able to use it.
#

def main():
    mapCount = 1
    file_path = 'C:\\building-utility\\drawing.txt'
    file = open(file_path,"r")
    data = '[\n'
    mapList = ["","","","","","","","",""]
    for line in file:
        lineCT = 0
        while len(line) > 23:
            if len(mapList[lineCT]) >= 725: 
                mapList[lineCT] = mapList[lineCT] + '				"' + line[:24] + '"\n'
            else:
                mapList[lineCT] = mapList[lineCT] + '				"' + line[:24] + '",\n'
            line = line[24:]
            lineCT += 1
        if len(mapList[0]) >= 760: 
            for x in range(len(mapList)):
                if len(mapList[x]) > 20:
                    data = writeJSON(mapCount,mapList[x],data)
                    writeTerrain(mapCount)
                    mapCount += 1
            mapList = ["","","","","","","","",""]         
    file.close()
    data = data[:-1] + "\n]"
    finalizeEntry(data)

#Takes the mapNum and 'text' is the 24x24 map
def writeJSON(mapNum, text, data):
    entry = ''
    file_path = 'C:\\building-utility\\json_header.txt'
    file = open(file_path,"r")
    for line in file:
        entry = entry + line
    file.close

    entry = entry + str(mapNum)
    
    file_path = 'C:\\building-utility\\json_middle.txt'
    file = open(file_path,"r")
    for line in file:
        entry = entry + line
    file.close
    
    entry = entry + text
    
    file_path = 'C:\\building-utility\\json_footer.txt'
    file = open(file_path,"r")
    for line in file:
        entry = entry + line
    file.close

    data = data + entry
    return data

def finalizeEntry(data):
    file_path = 'C:\\building-utility\\output\\office_tower.json'
    file = open(file_path,"w")
    file.write(data)
    file.close()
    
def writeTerrain(mapNum):
    entry = ''
    file_path = 'C:\\building-utility\\terrain_header.txt'
    file = open(file_path,"r")
    for line in file:
        entry = entry + line
    file.close

    entry = entry + str(mapNum)

    file_path = 'C:\\building-utility\\terrain_footer.txt'
    file = open(file_path,"r")
    for line in file:
        entry = entry + line
    file.close
    
    file_path = 'C:\\building-utility\\output\\overmap_terrain_entry.json'
    file = open(file_path,"a")
    file.write(entry)
    file.close()

main()

