# JSON Inheritance
To reduce duplication in the JSON data it is possible for some types to inherit from an existing type.

## Examples
In the following condensed example ```556``` ammo is derived from ```223``` ammo via ```copy-from```:
```
"id": "556",
"copy-from": "223",
"type": "AMMO",
"name": "5.56 NATO M855A1",
"description": "5.56x45mm ammunition with a 62gr FMJ bullet...",
"price": 3500,
"relative": {
    "damage": -2,
    "pierce": 4,
},
"extend": { "effects": [ "NEVER_MISFIRES" ] }
```
The following rules apply to the above example:

* Missing fields have the same value as the parent

* Fields explicitly specified replace those of the parent type. The above example replaces ```name```, ```description``` and ```price```.

* Numeric values may be specified ```relative``` to the parent. For example ```556``` has less ```damage``` but more ```pierce``` than ```223``` and will maintain this relationship if the definition for ```223``` is changed.

* Flags can be added via ```extend```. For example ```556``` is military ammo and gains the ```NEVER_MISFIRES``` ammo effect. Any existing flags specified from ```223``` are preserved.

* The entry you copied from must be of the same ```type``` as the item you added or changed (not all types are supported, see 'support' below)

Reloaded ammo is derived from the factory equivalent but with a 10% penalty to ```damage``` and ```dispersion``` and a chance to misfire:

```
"id": "reloaded_556",
"copy-from": "556",
"type": "AMMO",
"name": "reloaded 5.56 NATO",
"proportional": {
    "damage": 0.9,
    "dispersion": 1.1
},
"extend": { "effects": [ "RECYCLED" ] },
"delete": { "effects": [ "NEVER_MISFIRES" ] }
```
The following additional rules apply to the above example:

Chained inheritance is possible; for example ```reloaded_556``` inherits from ```556``` which is itself derived from ```223```

Numeric values may be specified ```proportional``` to the parent by via a decimal factor where ```0.5``` is 50% and ```2.0``` is 200%.

Flags can be deleted via ```delete```. It is not an error if the deleted flag does not exist in the parent.

It is possible to define an ```abstract``` type that exists only for other types to inherit from and cannot itself be used in game. In the following condensed example ```magazine_belt``` provides values common to all implemented ammo belts:
```
"abstract": "magazine_belt",
"type": "MAGAZINE",
"name": "Ammo belt",
"description": "An ammo belt consisting of metal linkages which disintegrate upon firing.",
"rigid": false,
"armor_data": {
    "covers": [ "TORSO" ],
    ...
},
"flags": [ "MAG_BELT", "MAG_DESTROY" ]
```
The following additional rules apply to the above example:

Missing mandatory fields do not result in errors as the ```abstract``` type is discarded after JSON loading completes

Missing optional fields are set to the usual defaults for that type

## Support
The following types currently support inheritance:
```
GENERIC
AMMO
GUN
GUNMOD
MAGAZINE
TOOL (but not TOOL_ARMOR)
COMESTIBLE
BOOK
ENGINE
MATERIAL
```

To find out if a types supports copy-from, you need to know if it has implemented generic_factory. To find out if this is the case, do the following:
* Open [init.cpp](https://github.com/CleverRaven/Cataclysm-DDA/tree/master/src/init.cpp)
* Find the line that mentions your type, for example `add( "gate", &gates::load );`
* Copy the load function, in this case it would be *gates::load*
* Use this in [the search bar on github](https://github.com/CleverRaven/Cataclysm-DDA/search?q=%22gates%3A%3Aload%22&unscoped_q=%22gates%3A%3Aload%22&type=Code) to find the file that contains *gates::load* (Note, you cannot search for ":" in file finder.  The search will simply ignore this symbol.)
* In the search results you find [gates.cpp](https://github.com/CleverRaven/Cataclysm-DDA/tree/master/src/gates.cpp). open it.
* In gates.cpp, find the generic_factory line, it looks like this: `generic_factory<gate_data> gates_data( "gate type", "handle", "other_handles" );`
* Since the generic_factory line is present, you can now conclude that it supports copy-from. 
* If you don't find generic_factoy present, it does not support copy-from, as is the case for type vitamin (repeat the above steps and find that [vitamin.cpp](https://github.com/CleverRaven/Cataclysm-DDA/tree/master/src/vitamin.cpp) does not contain generic_factoy)
