# Output stats for armor, including resistance calculations

# jq -f tools/json_tools/jq/armor_stats.jq --argfile mat data/json/materials.json data/json/items/*.json

# load bash and cut resist data from the material array
( $mat | map ( { (.ident):.bash_resist } ) | add ) as $br
|
( $mat | map ( { (.ident):.cut_resist } ) | add ) as $cr
# now, $br and $cr are objects mapping materials to bash/cut resist values
# like this {plastic:2, paper:1, ...}
|

# for each item in the input array, which should be an item json file
map(
    if (.type=="ARMOR" or .type=="TOOL_ARMOR") then
    # output an object with these attributes copied/calculated from the input
    {
        id,
        name,
        weight,
        warmth,
        envprot:.environmental_protection,
        coverage,
        covers:.covers | join(", "),
        # calculate bash_resist hopefully matching item::bash_resist()
        bash_resist:
            (
                0.5
                +
                1.5
                *
                .material_thickness
                *
                # add up the bash resists for the materials
                ( .material | map($br[.]) | add )
                / 
                # divide by the number of non-null materials
                ( .material | map(select(.!="null")) | length )
            ) |
            # round down
            floor,
        # same as above, with $cr this time
        cut_resist:
            (
                0.5
                +
                1.5
                *
                .material_thickness
                *
                ( .material | map($cr[.]) | add )
                /
                ( .material | map(select(.!="null")) | length )
            )
            |
            floor
    } else empty end
)