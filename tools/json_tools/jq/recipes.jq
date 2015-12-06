# Output summary of recipes, with skills/tools/components compacted

# jq -f tools/json_tools/jq/recipes.jq data/json/recipes/*.json

.[] | {
    name:.result,
    skill_used,
    skills_required:
        (if (.skills_required[0]|type)=="string" then
            {"\(.skills_required[0])":.skills_required[1]}
        else
            (.skills_required|map({"\(.[0])":.[1]}))|add
        end),
    tools:(.tools|map({"\(.[0][0])":.[0][1]}))|add,
    components:(.components|map({"\(.[0][0])":.[0][1]}))|add
}