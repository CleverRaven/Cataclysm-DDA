#!/bin/bash

mkdir ../data/process
mv ../data/mods ../data/process
mv ../data/json ../data/process
cd json_tools

python3 legacy_item_unpack_nested.py ../../data/process ammo_data AMMO
python3 legacy_item_unpack_nested.py ../../data/process armor_data ARMOR
python3 legacy_item_unpack_nested.py ../../data/process pet_armor_data PET_ARMOR
python3 legacy_item_unpack_nested.py ../../data/process book_data BOOK
python3 legacy_item_unpack_nested.py ../../data/process gun_data GUN
python3 legacy_item_unpack_nested.py ../../data/process gunmod_data GUNMOD
python3 legacy_item_unpack_nested.py ../../data/process seed_data SEED
python3 legacy_item_unpack_nested.py ../../data/process brewable BREWABLE
python3 legacy_item_unpack_nested.py ../../data/process compostable COMPOSTABLE
python3 legacy_item_unpack_nested.py ../../data/process milling MILLING
python3 legacy_item_unpack_nested.py ../../data/process relic_data ARTIFACT

python3 legacy_item_rename_type.py ../../data/process TOOL
python3 legacy_item_rename_type.py ../../data/process ARMOR
python3 legacy_item_rename_type.py ../../data/process TOOL_ARMOR
python3 legacy_item_rename_type.py ../../data/process GUN
python3 legacy_item_rename_type.py ../../data/process GUNMOD
python3 legacy_item_rename_type.py ../../data/process AMMO
python3 legacy_item_rename_type.py ../../data/process MAGAZINE
python3 legacy_item_rename_type.py ../../data/process COMESTIBLE
python3 legacy_item_rename_type.py ../../data/process BOOK
python3 legacy_item_rename_type.py ../../data/process PET_ARMOR
python3 legacy_item_rename_type.py ../../data/process BIONIC_ITEM
python3 legacy_item_rename_type.py ../../data/process TOOLMOD
python3 legacy_item_rename_type.py ../../data/process ENGINE
python3 legacy_item_rename_type.py ../../data/process WHEEL
python3 legacy_item_rename_type.py ../../data/process GENERIC

cd ../..

mv data/process/mods data
mv data/process/json data