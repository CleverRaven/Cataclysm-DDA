# Output lists of regional terrain and furniture defined in regional settings

# To see all of the regional settings, run the following:
# jq -f tools/json_tools/jq/regional_terrain.jq data/**.json

# To diff two different entries (most useful for the base game settings and a mod), run the following (adjust paths to relevant files):
# diff <(jq -f tools/json_tools/jq/regional_terrain.jq data/json/regional_map_settings.json) <(jq -f tools/json_tools/jq/regional_terrain.jq data/mods/rural_biome/rural_regional_map_settings.json)

.[]
| select(.type | test("region_settings"))?
| {id: .id, file: input_filename, terrain: [(.region_terrain_and_furniture["terrain"] | to_entries[] | .key )], furniture: [(.region_terrain_and_furniture["furniture"] | to_entries[] | .key )]}
