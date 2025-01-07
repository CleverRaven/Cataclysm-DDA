# Output list of all json entities in target files, grouped by entity type and sorted.

# Need -s to unify all the different files into one stream of objects.
# jq -s -f tools/json_tools/jq/enumerate_json_entities.jq $(find data/json -name "*.json" | grep -v obsolete)

# This filters all entries down to { type: foo, id: bar } while mapping
# a bunch of different fields to the id. That's the big section in square brackets.
# Then it groups by type and outputs groups as { type: foo, id: [ bar, baz, bix ] }
# Some entities will have multiple entries because they have more than one of the id fields we're looking for.

flatten |
[
  .[] |
  {
    type:.type,
    id: (
      .id?,
      .result?,
      .text?,
      .update_mapgen_id?,
      .nested_mapgen_id?,
      .id_suffix?,
      .tuple?,
      .monsters?,
      .om_terrain?,
      .messages?,
      .speaker?
    )
  } |
  select( .id != null )
]|
group_by(.type) | .[] |
{ type:.[0].type,id:[ (.[].id? | tostring) ] }
