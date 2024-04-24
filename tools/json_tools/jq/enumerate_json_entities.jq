# Output list of all json entities in target files, grouped by entity type and sorted.

# Need -s to unify all the different files into one stream of objects.
# jq -s -f tools/json_tools/jq/enumerate_json_entities.jq $(find data/json -name "*.json" | grep -v obsolete)

flatten |
map( select( .type? != "migration" and has( "id" ) ) ) |
group_by( .type ) |
.[] |
{ type:.[0].type,id:[ (.[].id? | tostring), (.[].ident? | tostring) ] }
