from urllib.request import Request, urlopen
import json
from zipfile import ZipFile
import os
import argparse

parser = argparse.ArgumentParser()

parser.add_argument("-o", "--output_folder", help="Folder where the tilesets are extracted")

args = parser.parse_args()


req = Request('https://api.github.com/repos/I-am-Erk/CDDA-Tilesets/releases/latest')

req.add_header('Accept','application/vnd.github.v3+json')

content = urlopen(req)

data = json.loads(content.read().decode(content.info().get_param('charset') or 'utf-8'))

for asset in data['assets']:
    file_url =asset['browser_download_url']
    file_name = asset['name']
    print('Downloading '+file_name)
    file_path = args.output_folder+'\\'+file_name
    tileset = open(file_path, 'wb')
    tileset.write(urlopen(file_url).read())
    tileset.close()
    with ZipFile(file_path, 'r') as zip_ref:
        zip_ref.extractall(args.output_folder)
    os.remove(file_path)