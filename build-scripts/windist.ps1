if (Test-path bindist) {
  rm -Force -Recurse bindist
}

mkdir bindist
cp cataclysm-tiles.exe bindist/cataclysm-tiles.exe
cp cataclysm-tiles.stripped.pdb bindist/cataclysm-tiles.pdb
cp tools/format/json_formatter.exe bindist/json_formatter.exe

mkdir bindist/lang
cp -r lang/mo bindist/lang

$extras = "data", "doc", "gfx", "LICENSE.txt", "LICENSE-OFL-Terminus-Font.txt", "README.md", "VERSION.txt"
ForEach ($extra in $extras) {
	cp -r $extra bindist
}
Compress-Archive -Force -Path bindist/* -DestinationPath "cataclysmdda-0.I.zip"
