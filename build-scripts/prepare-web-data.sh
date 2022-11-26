rm -rf web_bundle

DATA_DIR=web_bundle/data/
mkdir -p $DATA_DIR

cp data/fontdata.json $DATA_DIR
cp -R data/core $DATA_DIR
cp -R data/font $DATA_DIR
cp -R data/json $DATA_DIR
cp -R data/mods $DATA_DIR
cp -R data/names $DATA_DIR
cp -R data/raw $DATA_DIR
cp -R data/motd $DATA_DIR
cp -R data/credits $DATA_DIR
cp -R data/title $DATA_DIR
cp -R data/help $DATA_DIR

cp -R gfx web_bundle/

# Remove .DS_Store files.
find web_bundle -name ".DS_Store" -type f -exec rm {} \;

# Remove old non-ttf font
rm web_bundle/data/font/terminus.fon

# Remove many non-default tilesets.
echo "Removing non-default tilesets..."
rm -rf web_bundle/gfx/MshockXotto+
rm -rf web_bundle/gfx/ChestHole16Tileset
rm -rf web_bundle/gfx/ChibiUltica
rm -rf web_bundle/gfx/Cuteclysm
rm -rf web_bundle/gfx/BrownLikeBears
rm -rf web_bundle/gfx/HoderTileset
rm -rf web_bundle/gfx/NeoDaysTileset
rm -rf web_bundle/gfx/RetroDaysTileset

# Remove obsolete mods.
echo "Removing obsolete mods..."
for MOD_DIR in web_bundle/data/mods/*/ ; do
    if cat "$MOD_DIR/modinfo.json" | jq '.[] | select(.type == "MOD_INFO") | .obsolete == true' | grep -q 'true'; then
        echo "$MOD_DIR is obsolete... Removing..."
        rm -rf $MOD_DIR
    fi
done