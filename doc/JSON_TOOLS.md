# JSON tools

In the `tools/json_tools/` folder you will find several scripts for working with JSON data, written in
[python](https://python.org) and [jq](https://stedolan.github.io/jq/).

## Keys and Values

The `keys.py` and `values.py` scripts are related, with similar usage:

- `keys.py` counts how many times each key (field) occurs throughout all matching JSON data.  You
might use this to learn how many items have "weight" and/or "volume", or what fields are used most
often by "overmap_special" types, for example.

- `values.py` counts how many times each value occurs in a given key (field).  Use this if you want
to know what different "volume" or "weight" values items have or to look up all "id" values from a
set of matching items, for instance.

Run either script with `-h` to see help on their command-line options.  Both scripts output JSON
text by default, but you can pass the `--human` option for output in a more human-readable format.

You can run these scripts from the root `Cataclysm-DDA` directory using a relative path:

```console
$ tools/json_tools/keys.py -h
$ tools/json_tools/keys.py --human type=TOOL
$ tools/json_tools/values.py --key material type=TOOL
```

Or, `cd` into the `tools/json_tools` directory and execute the Python scripts in
that directory - it will still work the same:

```console
$ cd tools/json_tools

$ ./keys.py -h
$ ./keys.py --human type=TOOL
$ ./values.py --key material type=TOOL
```

## Examples

Here is `keys.py` showing how many `TOOL` types have each key defined (abbreviated for simplicity,
since the full list is rather long).  Out of 752 tools, all of them have `type` and `name` defined,
but only some of them include `pocket_data` or `longest_side`:

```console
$ tools/json_tools/keys.py --human type=TOOL

Count of keys
(Data from 752 out of 25050 blobs)
-------------
type         : 752
name         : 752
weight       : 656
pocket_data  : 143
looks_like   : 101
techniques   : 92
longest_side : 56
...
```

Here is `values.py` printing the number of `TOOL` type items having each `material` value.  The
`MISSING` column tells how many tools have no `material` defined in their JSON:

```console
$ tools/json_tools/values.py --key material --human type=TOOL

Count of values from field 'material'
(Data from 752 out of 25050 blobs)
-------------
steel    : 295
plastic  : 245
MISSING  : 105
aluminum : 73
wood     : 70
glass    : 35
...
```

Show values of "longest_side" defined for `TOOL` type items in the `weapon` category, including how
many items are *missing* an explicit value for "longest_side":

```console
$ tools/json_tools/values.py --key longest_side type=TOOL category=weapon

{"50 cm": 10, "40 cm", 1, "MISSING": 95, "20 cm": 2, "100 cm": 6, ...}
```

Pipe output to `lister.py` to get a distinct list of alphabetized keys from a JSON string.  For
example, to list all `type` values alone, without counting how many of each:

```console
$ tools/json_tools/values.py --key type | tools/json_tools/lister.py

["AMMO", "ARMOR", "BATTERY", "BIONIC_ITEM", "BOOK", "COMESTIBLE", ...]
```

The `keys.py` output will use `parent.child` dotted notation for objects nested within one another.
For example, the "name" value is often an object including a pluralized form, and items with
"pocket_data" typically contain a nested list of objects in their JSON. The dot indicates keys
within those objects:

```console
$ tools/json_tools/keys.py type=ARMOR | tools/json_tools/lister.py

{ ..., "pocket_data.max_contains_volume", "pocket_data.max_item_length", "pocket_data.pocket_type", ... }
```

These dotted keys can be passed as the `-k` or `--key` argument of `values.py`, to inspect values
within those nested objects or lists of objects in the JSON data. For example, to look at the values
of "max_item_length" within the "pocket_data" objects of `ARMOR` types:

```console
$ tools/json_tools/values.py -k pocket_data.max_item_length type=ARMOR

{ "25 cm": 1, "140 cm": 1, "16 cm": 1, "50 cm": 5, "MISSING": 785, ... }
```


## Examples and use cases

| Desired keys | Command-line
| ---          | ---
| of `material` types | `keys.py type=material`
| of `overmap_special` types | `keys.py type=overmap_special`
| of `weapons` category | `keys.py category=weapons`
| of `weapons` with `wood` material | `keys.py category=weapons material=wood`

| Desired info | Command-line
| ---          | ---
| All `type` values in all JSON data | `values.py -k type`
| All `category` values in all JSON data | `values.py -k category`
| IDs of items with `material` including `kevlar` | `values.py -k id material=kevlar`
| IDs of all `MAGAZINE` types | `values.py -k id type=MAGAZINE`
| Materials of all `GUN` items | `values.py -k material type=GUN`
| Materials of all `TOOL` items | `values.py -k material type=TOOL`
| Encumbrances of `ARMOR` made of `leather` | `values.py -k encumbrance type=ARMOR material=leather`
| All `mutation` type `category` values | `values.py -k category type=mutation`
| Addiction types of `COMESTIBLE` items | `values.py -k addiction_type type=COMESTIBLE`
| Valid targets of `SPELL` types | `values.py -k valid_targets type=SPELL`
| Monster conditions in `monstergroup` types | `values.py -k monsters.conditions type=monstergroup`
