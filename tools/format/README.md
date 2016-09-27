# Overview

A linting tool for JSON data similar to astyle in just over 100 lines of
idiomatic perl. Configurable without knowledge of perl and requiring no
external dependencies beyond the p5-JSON module

# Usage

`format.pl [-qc] [file...]`

* `-q` quiet, suppress output of formatted JSON data
* `-c` check, set non-zero exit code if input data was not canonical

## Example usages

Parse a file and output JSON in canonical format

`format.pl examples/vehicle_part.json > output.json`

Check a file is valid JSON and has no unknown fields

`format.pl -q examples/vehicle_part.json`

Check all files in directory are valid JSON in canonical format

`format.pl -cq examples/* && echo OK`

# Configuration

The provided unit tests should be run after making any changes to the ruleset.

## Contexts and rules

As a file is parsed each JSON **node** is assigned a **context** based upon its
relative position in the hierarchy. Contexts are matched against a series of
**rules** which control which **nodes** are valid.

Given the following example data:
```
{
  "id": "bottle_plastic"
  "type": "CONTAINER",
  "name": "plastic bottle"
}
```

The following **contexts** are parsed:
```
CONTAINER:id
CONTAINER:type
CONTAINER:name
```

The configuration file `format.conf` contains all of the **rules**. The first
matching **rule** applies and a **context** without any matching rule is an
error. For almost all nodes their **contexts** and **rules** will be identical:

```
# This ruleset matches a CONTAINER object
# Comments and blank lines are ignored

CONTAINER:id
CONTAINER:type
CONTAINER:name # trailing comments are also supported
```

Each **rule** is case sensitive and anchored to the right of the **context** so
`CONTAINER:id` is also matched by `:id` but not by `container:id`

## Nesting

JSON data will probably contain nested **nodes**:
```
{
  "id": "flashlight",
  "type": "TOOL",
  "name": "flashlight",
  "materials": [ "steel", "plastic" ],
  "ammo": "battery",
  "use_action": {
    "type": "transform",
    "target": "flashlight_on"
  }
}
```

When constructing the **context** nested objects use their key whereas array
elements (which are otherwise anonymous) use the `@` symbol:
```
TOOL:id
TOOL:type
TOOL:name
TOOL:materials
TOOL:materials:@
TOOL:ammo
TOOL:use_action
TOOL:use_action:type
TOOL:use_action:target
```

## Advanced matching

Rules are PCRE-compatible regular expressions. This allows us to write more
concise rulesets. The below handles both the plastic bottle and flashlight:

```
# Match GENERIC fields which all items have
:id
:type
:name
:materials
:materials:@

# Only TOOL and GUN has ammo
(TOOL|GUN):ammo

# Support TOOL items
TOOL:use_action
TOOL:use_action:type
TOOL:use_action:target
```

As the ordering of rules controls the sorting of keys within a *node* you
should avoid constructs such as `CONTAINER:(id|name)` as these result in an
ambiguous sorting order.

## Output flags

### Wrapping

By default each node results in a new line in the output:
```
{
  "id": "flashlight",
  "materials": [
    "steel",
    "plastic"
   ]
}
```

Each **rule** may specify one or more **flags** that change how a node is
formatted. A flag is specified by the `=FLAG` syntax:

```
:id
:type
:name
:materials=NOWRAP
:materials=@
:flags=ARRAY,NOWRAP
```

The `NOWRAP` flag results in the following output:
```
{
  "id": "flashlight",
  "materials": [ "steel", "plastic" ]
}
```

The `WRAP` flag prevents concatenation of a solitary element to a single line:
```
{
  "modes": [
    [ "DEFAULT", "semi-auto", 1 ]
  ]
}
```

### Canonical arrays

Some **nodes** can have a single value or an array of elements:
```
"flags": "RELOAD_ONE"
"flags": [ "RELOAD_ONE" ]
"flags": [ "RELOAD_ONE", "RELOAD_EJECT" ]
```

The `ARRAY` flag encloses single values in an array ensuring canonical output:
```
"flags": [ "RELOAD_ONE" ]
"flags": [ "RELOAD_ONE", "RELOAD_EJECT" ]
```

## Specifiers

The following **nodes** have the same initial *context* but should differ in
their sorting and handling. For example `transform` should not accept `trap`:

```
"use_action": {
   "type": "transform",
   "target": "flashlight_on"
}
```

```
"use_action": {
   "type": "place_trap",
   "trap": "tr_rollmat"
}
```

This can be handled via **specifiers** which are available for any **node** that
has a `type` subfield:

```
use_action:type
use_action<transform>:target
use_action<place_trap>:trap
```

Specifiers can be exploited to provide special handling for only certain types:

```
# Note the ordering of rules is significant here
use_action:type<transform>=NOWRAP
use_action:type

use_action:type<transform>:target
use_action:type<tplace_trap>:trap
```

Produces the output:
```
"use_action": { "type": "transform", "target": "flashlight_on" }

"use_action": {
   "type": "place_trap",
   "trap": "tr_rollmat"
}
```

## Cookbook

## Multiple nested arrays and wrapping

```
:magazines
:magazines:@=NOWRAP
:magazines:@:@=NOWRAP
```

Note only the first level of elements starts a new line:
```
"magazines": [
  [ "223", [ "stanag30", "stanag10", "survivor223mag" ] ],
  [ "22", [ "ruger1022bigmag", "ruger1022mag" ] ],
  [ "308", [ "g3mag", "g3bigmag" ] ]
]
```

## Nodes optionally accepting arrays

```
:use_action(:@)?:type
:use_action(:@)?<sewing_kit>:item_action_type
:use_action(:@)?<cauterize>:flame
```

Note that `use_action` could either have a direct child or an array:

```
{
  "id": "sewing_kit",
  "use_action": {
     "type": "repair_item",
     "item_action_type": "repair_fabric"
  }
}
```

```
{
  "id": "soldering_iron",
  "use_action": [
    {
      "type": "repair_item",
      "item_action_type" : "repair_metal",
    },
    {
      "type": "cauterize",
      "flame": false
    }
  }
}
```

## Variable object keys

```
:use_action:charges_needed
:use_action:charges_needed:.*    # naive
:use_action:charges_needed<>:\w+ # more specific
```

For objects with variable keys both work but the more specific is preferred:

```
"use_action": {
  "type": "consume_drug",
  "charges_needed": { "fire": 1 },
  "tools_needed": { "apparatus": -1 }
}
```
