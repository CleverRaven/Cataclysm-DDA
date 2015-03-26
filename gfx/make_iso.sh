#!/bin/bash

# creates tileset_iso/ based on tileset/
# currently only works with 24x24 tileset with tiles.png and fallback.png

if [ "$#" -ne 1 ]; then
    echo "USAGE: make_iso.sh [dirname]"
fi

rm -rf $1_iso
cp -av $1 $1_iso
cd $1_iso

sed -i '' \
    -e 's/"height": 24/"height": 32/' \
    -e 's/"width": 24/"width": 32, "iso":true/' \
    -e "s/$1/$1_iso/g" \
    -e 's/NAME: /NAME: iso_/' \
    -e 's/VIEW: /VIEW: iso_/' \
    tile_config.json tileset.txt

mkdir tiles
echo "extract tiles"
convert -crop 24x24 tiles.png +repage tiles/tiles-%04d.png

mkdir fallback
echo "extract fallback"
convert -crop 24x24 fallback.png +repage fallback/fallback-%04d.png

rm iso_tilelist
jq '."tiles-new"[0].tiles|map(select(.id[0:2]=="t_"))|map(select(.bg==null))|map(.fg)[]' tile_config.json >> iso_tilelist
jq '."tiles-new"[0].tiles|map(select(.id[0:2]=="t_"))|map(.bg)[]' tile_config.json >> iso_tilelist
jq '."tiles-new"[0].tiles|map(select(.id[0:2]=="t_"))|map(.additional_tiles)|map(select(.!=null))[]|map(select(.bg==null))|map(.fg)[]' tile_config.json >> iso_tilelist
jq '."tiles-new"[0].tiles|map(select(.id[0:2]=="t_"))|map(.additional_tiles)|map(select(.!=null))[]|map(.bg)[]' tile_config.json >> iso_tilelist
jq '."tiles-new"[0].tiles|map(select(.id[0:9]=="lighting_"))|map(select(.bg==null))|map(.fg)[]' tile_config.json >> iso_tilelist
jq '."tiles-new"[0].tiles|map(select(.id[0:9]=="lighting_"))|map(.bg)[]' tile_config.json >> iso_tilelist
jq '."tiles-new"[0].tiles|map(select(.id[0:3]=="vp_"))|map(.fg)[]' tile_config.json >> iso_tilelist
jq '."tiles-new"[0].tiles|map(select(.id[0:3]=="vp_"))|map(.bg)[]' tile_config.json >> iso_tilelist
sort -n iso_tilelist | uniq > iso_tilelist_uniq
sed -i '' -e 's/^...$/0&/' -e 's/^..$/00&/' -e 's/^.$/000&/' -e '/^null$/d' iso_tilelist_uniq

cd tiles
mkdir iso
for i in `cat ../iso_tilelist_uniq`
do
    echo rotate tiles-$i.png
    convert tiles-$i.png -background transparent -rotate 45 +repage -resize 100%x50% -crop 32x16+2+1 +repage -extent 32x32+0-16 iso/tiles-$i.png
    rm tiles-$i.png
done
mkdir upsize
for i in *.png
do
    echo upsize $i
    convert -background transparent $i -extent 32x32-4-8 +repage upsize/$i
    rm $i
done
mv iso/* .
mv upsize/* .
cd ..

echo "reassemble tiles"
montage -background transparent tiles/tiles-*.png -tile 16x -geometry +0+0 tiles.png
rm -rf tiles

cd fallback
mkdir upsize
for i in *.png
do
    echo upsize $i
    convert -background transparent $i -extent 32x32-4-8 +repage upsize/$i
    rm $i
done
mv upsize/* .
cd ..

echo "reassemble fallback"
montage -background transparent fallback/fallback-*.png -tile 16x -geometry +0+0 fallback.png
rm -rf fallback
