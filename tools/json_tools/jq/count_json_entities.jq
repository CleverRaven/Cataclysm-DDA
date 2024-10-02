# Output counts of the different json types.

# Need -s to unify all the different files into one stream of objects.
# The -c outputs each object on one line which makes later processing easier.
# jq -s -c -f tools/json_tools/jq/count_json_entities.jq $(find data/json -name "*.json" | grep -v obsolete)

flatten |
[ .[] | objects | { type } ] |
group_by( .type ) |
.[] | { type:.[0].type,count:[ (.[].id | tostring) ] | length }
