### Changelog Summary

#### Diffstat
For the (absurd) count of lines changed, run
git diff --shortstat <version tag>...

For the number of commits, run
git log --oneline <version tag>... | wc

For the number of authors run
sort -u <(git log --format="format:%an %ae" 0.G...) | wc
This over-counts authors because people don't present a consistent email or name, feel free to try and deduplicate the list.  In practice it's not that bad.

For the number of NEW authors, run
diff -d -u <(sort -u <(git log --format="format:%an" 0.1...<old version>)) <(sort -u <(git log --format="format:%an" <old version>...)) | grep "^+" | wc
Again, this will over or under-count due to mismatches, feel free to generate the list of authors and try to de-duplicate them before running the diff.

#### Game Entity additions

This is the section of the changelog that looks like so:
New game entities (core): 10293
Items: 2396
    502 misc items, 45 books, 941 articles of clothing, 117 guns and gun related items,
    364 comestibles, 308 tools, 119 ammunition types

Generating this summary is a multi-step process.
My instructions are for linux, you can probably do similar things on windows.
First, make sure you have jq installed.

Check out the previous release branch (i.e. when I'm writing this it's 0.G-branch).
Run the commands:
jq -s -c -f tools/json_tools/jq/count_json_entities.jq $(find data/json -name "*.json" | grep -v obsolete) > entity_count_<version>
jq -s -c -f tools/json_tools/jq/count_json_entities.jq $(find data/mods -name "*.json" | grep -v obsolete) > entity_count_mods_<version>

Check out the current release branch (i.e. when I'm writing this it's 0.H).
Run the same commands, but name the files using the current version.
jq -s -c -f tools/json_tools/jq/count_json_entities.jq $(find data/json -name "*.json" | grep -v obsolete) > entity_count_<version>
jq -s -c -f tools/json_tools/jq/count_json_entities.jq $(find data/mods -name "*.json" | grep -v obsolete) > entity_count_mods_<version>

Run the commands
join -t ',' <( sort ../entity_count_<old_version> ) <( sort ../entity_count_<new_version> ) 2>&1 > ../entity_count_diff_<old_version>_to_<new_version>
join -t ',' <( sort ../entity_count_mods_<old_version> ) <( sort ../entity_count_mods_<new_version> ) 2>&1 > ../entity_count_mods_diff_<old_version>_to_<new_version>

These two files are the difference in entity counts between the two versions, but they need some massaging to look decent. Unfortunately the rest is manual.
Open the existing changelog and one of the files, copy the headers ( ITEMS, CRAFTING, ACHIEVEMENTS, etc) from an existing section to your WIP file (I tend to just do this in the entity count file).

For each line in the entity count file, move or copy that line under the matching header.  Most of the mappings will have been established by previous changelog summaries, but things are subject to change, the goal is presenting the change counts in a meaningful way, use the previous summaries as hints, not rules.

You also need to take the before and after counts and get the difference, edit the janky json output into something readable, and rename the identifiers to something nicer. It's also totally resonable to merge entries where it makes sense and drop entries with zero or negative additions.