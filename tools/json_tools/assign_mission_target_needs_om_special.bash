#!/usr/bin/env bash
set -euo pipefail

function Q {
	find ./data/ \
		-not -path ./data/fontdata.json \
		-not -path ./data/mods/replacements.json \
		-name '*.json' \
		-exec jq "${@}" {} +
}

SPECIAL_OF_TERRAIN="$(mktemp --suffix -special-of-terrain.json )"
Q 'def skip(to_skip): select(all(. != to_skip; .));
    if type=="array" then .[] else . end
	| select(type=="object")
	| select(.type=="overmap_special")
	| .id as $id
	| .overmaps[].overmap
    | select(. != null)
	| sub("_(north|south|east|west)$"; "")
	# Skip common terrains to prevent false positives
	| skip("forest", "forest_thick", "forest_water", "field", "road_ew")
	| [., $id]' \
| jq --null-input \
	'reduce inputs as [$k, $v] ({}; .[$k] += [$v])
	| map_values(unique)' \
> "$SPECIAL_OF_TERRAIN"

MISSING_OM_SPECIAL="$(mktemp --suffix -missing-om-special.json )"
Q --slurpfile special_of_terrain "$SPECIAL_OF_TERRAIN" \
   	'if type=="array" then .[] else . end
	| select(type=="object")
   	| select(.type=="mission_definition")
   	| .id as $id
   	| .start?.assign_mission_target?
   	| select(.om_terrain)
   	| .XXX_NEEDS_SPECIAL=$special_of_terrain[][.om_terrain]
   	| select(.XXX_NEEDS_SPECIAL)
	| select(all(.om_special != .XXX_NEEDS_SPECIAL[]; .))
   	| {id: $id, om_special, XXX_NEEDS_SPECIAL}' \
| jq --slurp unique > "$MISSING_OM_SPECIAL"

if jq --exit-status '. != []' < "$MISSING_OM_SPECIAL" > /dev/null; then
	echo "The following missions lack .start.assign_mission_target.om_special
See https://github.com/CleverRaven/Cataclysm-DDA/blob/master/doc/MISSIONS_JSON.md#assign_mission_target
> If the om_terrain is part of an overmap special, it's essential to specify
> the om_special value as well--otherwise, the game will not know how to spawn
> the entire special." >&2
	cat "$MISSING_OM_SPECIAL"
	exit 1
fi
