from inspect import getsourcefile
from os.path import dirname
import os, gzip
dir_path = dirname(getsourcefile(lambda:0))
overmaps_path = os.path.join( dir_path + "\overmaps" )
template_path = os.path.join( dir_path + "\overmap_template.txt" )
template_file = open( template_path, "r" )
template_first_line = template_file.readline()
template_second_line = template_file.readline()

x = 0
for y in range( -20, 20):
    output_file_string = template_first_line + str(y) + template_second_line
    output_file = str.encode( output_file_string )
    output_filename = "overmap_0_" + str(y) + ".omap.gz"
    output_path = os.path.join( overmaps_path, output_filename )
    f = gzip.open( output_path, 'wb')
    f.write(output_file)
    f.close()