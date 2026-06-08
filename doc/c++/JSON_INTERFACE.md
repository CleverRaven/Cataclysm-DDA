# The JSON Interface

The purpose of this document is to explain the JSON interface that is used by the game, and how to interact with it.
It covers what it is used for, how to use it, and best practices in using it.

Unlike many other programs Cataclysm: DDA uses a large amount (~half a million lines) of hand-written and read JSON.
While there is JSON that is only generated and read by the game itself, the amount of hand-written JSON means that care must be given to how the JSON is structured to facilitate development.
<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->

- [The JSON Interface](#the-json-interface)
  - [What is JSON?](#what-is-json)
  - [What the Interface Does](#what-the-interface-does)
  - [The JSON API](#the-json-api)
    - ["Types" of JSON data](#types-of-json-data)
    - [The Basics](#the-basics)
    - [Mapping to "canonical" structures, `serialize` and `deserialize`](#mapping-to-canonical-structures-serialize-and-deserialize)
- [Loading _game data_](#loading-game-data)
  - [JSON Inheritance, or `copy-from`, `extend`, `delete`, `proportional`, and `relative`](#json-inheritance-or-copy-from-extend-delete-proportional-and-relative)
    - [`copy-from`](#copy-from)
    - [`extend` and `delete`](#extend-and-delete)
    - [`relative`](#relative)
    - [`proportional`](#proportional)
  - [`optional` and `mandatory`](#optional-and-mandatory)
  - [Readers for `optional` and `mandatory`](#readers-for-optional-and-mandatory)
    - [Reader support for `extend`, `delete`, `relative`, and `proportional`](#reader-support-for-extend-delete-relative-and-proportional)
    - [`relative` Containers Support](#relative-containers-support)
    - [Custom JSON Formats with Readers](#custom-json-formats-with-readers)
    - [Readers with Container Values](#readers-with-container-values)
  - [Supporting `relative`, `proportional`, `extend`, and `delete` without Readers](#supporting-relative-proportional-extend-and-delete-without-readers)
  - [Loading Whole Types of *game data*](#loading-whole-types-of-game-data)
    - [`generic_factory`](#generic_factory)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

## What is JSON?

JSON is a structured data format that the game uses to dynamically load and store information.
The specifications of JSON are explained at [json.org](json.org), but here is a brief explanation.

JSON consists of what is termed a "value".
There are seven possible types of values - *string*, *number*, *object*, *array*, `true`, `false`, and `null`.
*Object*s and *array*s are values that can contain other values.
*String*s and *number*s are values that represent strings of characters and numerical values respectively.
`true`, `false` and `null` (without quotes!) are literal values.

Because *object*s and *array*s can contain other values, they form the building blocks of JSON.
An *array* is an ordered list of values separated by commas and enclosed by brackets `[ value1, value2 ]`.
An *object* consists of key (*string*) and  value (any type) pairs.
The start of an *object* is denoted by a `{` symbol, the end by a `}` symbol, and keys are separated from values by a `:` symbol.
Key/value pairs are separated by commas `{ "key1": value1, "key2": value2 }`.

Considering a game data JSON file, it can be understood as a value (an *array*) containing other values (*object*s), which themselves contain other values in various forms.
```json
[
  {
    "id": "foo",
    "type": "A",
    "count": 3
  },
  {
    "id": "baz",
    "type": "B",
    "bounds": [ 2, 3 ],
    "limited": true
  }
]
```

## What the Interface Does

The role of the interface is to map or translate between JSON structures and C++ structures.
To help build an understanding of what this looks like, the table below shows some example translations.

| C++ Value | JSON Value
| --------- | ----------
| `int`, `float`, numeric types | *number*
| `bool` | `true` or `false`
| `std::string` | *string*
| `std::optional<int>` | *number* or `null`
| `std::pair<std::string, int>` | `[ "string", number ]`
| `std::vector<int>` | `[ number0, number1, ..., numberN ]`
| `std::set<std::string>` | `[ "string0", ..., "stringN" ]`
| `std::map<std::string, int>` | `{ "key0": number0, ..., "keyN": numberN }` or `[ [ "key0", number0 ], ..., [ "keyN", numberN ] ]`
| `class tripoint { int x; int y; int z; };` | `[ numberX, numberY, numberZ ]` or `{ "x": numberX, "y": numberY, "z": numberZ }`
| `units::energy` | `"quantity unit"`, e.g. `"42 kJ"` a *string*. A *number* could be used, but is harder to understand
| `std::map<tripoint, int>` | `[ [ [ numberX0, numberY0, numberZ0 ], number0 ], ..., [ [ numberXN, numberYN, numberZN ], numberN ] ]`
| `class foo { std::string id; std::pair<int, int> limits; bool bounded; };` | `{ "id": "string", "limits": [ number0, number1 ], "bounded": false }`
| `class baz { int weight; std::vector<foo> options; };` | `{ "weight": number0, "options": [ { "id": "string", "limits": [ number0, number1 ], "bounded": false } ] }`

There are some important things to note here.

#### 1. Some C++ types have "canonical" JSON structures

For numeric, boolean, or string types this is fairly straightforward, they translate to *number*s, `true`/`false`, or *string*s.

Arrays (C-style or `std::`), `std::vector`s and sequence containers in general translate to *array*s, with the values inside corresponding to the data.
In fact, all containers with the expection of maps are best implemented as *array*s.
Maps more naiively translate to *object*s, but because the keys of *object*s must be *string*s, it may not be possible to use an *object*.

The naiive translation of a `class` or `struct` would also be to an *object*, but...

#### 2. There are multiple valid representations...

...and so the best option depends on what it is being used for.
For types (primitives, containers, etc) with a straightforward translation, that is almost always best.
But for other types, often user-defined `class`es or `struct`s which have many possible implementations, some consideration may be required to find a "canonical" implementation, or many formats may be used.

While the table above lists two formats for `tripoint`, within JSON they are almost always represented in the *array* form.
The *object* form requires more space, and the `[x, y, z]` form is easily understood from mathematical notation `(x, y, z)`.

The `units::` types exist to leverage C++ type features and provide safety in mathematical operations involving them.
As they are just a wrapper over an underlying numeric value, they could be represented as just a *number*.
However, a *string* containing both the numeric value and a unit is much easier to understand and works to extend the type safety to the JSON interface.

#### 3. Complex structures are formed by composition of values

_This feels obvious. Is it helpful to have?_

The complex structure `baz` can be transformed to JSON by transforming its members recursively.
The class `baz` is an object, with members `weight` and `options`.
`options` is an array, then the values in the array are defined by type, `foo`.
`foo` transforms into an object with three members, and so on.

```cpp
class foo
{
    std::string id;
    std::pair<int, int> limits;
    bool bounded;
};

class baz
{
    int weight;
    std::vector<foo> options;
}
```

## The JSON API

This section explains the JSON read/write API and covers some basic usage of it.
It does not cover the transformation to/from the C++ classes representing JSON values and files, only how to use the C++ classes to read/write JSON data.

### "Types" of JSON data

There are effectively three forms of JSON interaction that a programer will need to deal with.

#### 1. Reading _game data_

_Game data_ is the human-written portion of the data, and is used to define the content and some of the functionality of the game.
Because it is human-written, there are [a variety of features](../JSON/JSON_INHERITANCE.md) built on top of (and into) the JSON to make it easier to modify, maintain, and read.
The functioning and implementation of these features will be covered [later in this document](#loading-game-data), but these make reading _game data_ subject to different considerations than other forms of data.

#### 2. Reading _save data_

_Save data_ is the program-written JSON data.
It is termed _save data_ because the majority of this data is for user saves, but other data is read in this mode, such as game options.
This data is not human-written and so does not have extra features to support.
However, because it is supporting user saves and other generated data, it needs to support old formats and migrating data to new formats.

#### 3. Writing _save data_

This is the most straightforward form of JSON interaction, and requires few additional notes.

### The Basics

The JSON read API is implemented primarily in `flexbuffer_json.h` and `flexbuffer_json-inl.h`.
The interface is easy to use, and uses a handful of classes.

| Class | Purpose
| ----- | -------
| `JsonValue` | This represents a JSON value, as described [above](#what-is-json). This can be transformed into any of the other classes, a `std::string`, a `bool`, or a variety of numeric types.
| `JsonArray` | This represents an *array* of JSON values. The `JsonValue`s within can be iterated over, accessed by index, or incrementally consumed.
| `JsonObject` | This represents an *object*. The values within can be accessed by key, or all the key/value pairs can be iterated over as `JsonMember`s.
| `JsonMember` | This is a special form of `JsonValue` that represents both the key and value of a JSON object. It behaves as a `JsonValue` representing the value, but provides access to the key.

All of these classes provide functions to test the types of values (either that they represent or by index/key), and read values into C++ types.
Each of these provides a `read` member function that is passed a reference to a value and will read that value from JSON if the type is understood by the API.
The next part covers how to make the API understand a particular type.

The JSON write API is provided in `json.h`, and provides a simple stream interface.
It provides functions to start and close objects and arrays, as well as writing members and values.
Values can be written by passing then to a `write` function, if they are of a type that the API understands, as will be covered in the next part.

### Mapping to "canonical" structures, `serialize` and `deserialize`

While the `write` and `read` functions of the API understand some structures natively, some structures may need to specify what JSON structure they will generate.
This is done with `serialize` and `deserialize` functions.
If a type `T` defines a `T::serialize( JsonOut & )`  member function, or there is a `serialize( const T &, JsonOut & )` function, that function will be called when trying to write a value of type `T`.
Similarly, if there is a `T::deserialize( const JsonType & )` or `deserialize( const T &, const JsonType & )` function, that will be called to read a value of type `T`.
`JsonType` can be `JsonValue`, `JsonObject`, or `JsonArray`.

The example below demonstrates how this works.

```cpp
class foo
{
    std::string id;
    std::pair<int, int> limits;
    bool bounded;

    void deserialize( const JsonObject &jo );
    void serialize( JsonOut &jsout ) const;
};

class baz
{
    int weight;
    std::vector<foo> options;

    void deserialize( const JsonObject &jo );
    void serialize( JsonOut &jsout ) const;
};

void foo::serialize( JsonOut &jsout )
{
    jsout.start_object();
    jsout.member( "id", id );
    jsout.member( "limits", limits );
    if( bounded ) {
        jsout.member( "bounded", bounded );
    }
    jsout.end_object();
}

void baz::serialize( JsonOut &jsout )
{
    jsout.start_object();
    jsout.member( "weight", weight );
    jsout.member( "options", options );
    jsout.end_object();
}

void foo::deserialize( const JsonObject &jo )
{
    jo.read( "id", id );
    jo.read( "limits", limits );
    if( jo.has_member( "bounded" ) ) {
        jo.read( "bounded", bounded );
    } else {
        bounded = false;
    }
}

void baz::deserialize( const JsonObject &jo )
{
    jo.read( "weight", weight );
    jo.read( "options", options );
}
```

This is the most basic way to read JSON values, and is not recommended for _game data_, which is covered [later](#loading-game-data).
Because all the types involved in this have `serialize`/`deserialize` functions or are natively understood, it is very easy to read and write the data from JSON.
The example below shows the `serialize` and `deserialize` functions of `baz` if `foo` does not have these functions.

```cpp

void baz::serialize( JsonOut &jsout )
{
    jsout.start_object();
    jsout.member( "weight", weight );
    jsout.member( "options" );
    jsout.start_array();
    for( const foo &f : options ) {
        jsout.start_object();
        jsout.member( "id", f.id );
        jsout.member( "limits", f.limits );
        if( f.bounded ) {
            jsout.member( "bounded", f.bounded );
        }
        jsout.end_object();
    }
    jsout.end_array();
    jsout.end_object();
}

void baz::deserialize( const JsonObject &jo )
{
    jo.read( "weight", weight );
    for( const JsonObject &subjo : jo.get_array( "options" ) ) {
        foo tmp;
        subjo.read( "id", tmp.id );
        subjo.read( "limits", tmp.limits );
        if( subjo.has_member( "bounded" ) ) {
            subjo.read( "bounded", tmp.bounded );
        } else {
            tmp.bounded = false;
        }
        options.push_back( tmp );
    }
}
```

This just consists of inlining the serialize and deserialize functions for `foo`, with some extra boilerplate.
This also needs to be repeated for everying that deserializes `foo`, and for this reason it is highly recommended to implement `serialize` and `deserialize` functions for types that need to be read or written.

# Loading _game data_

The basics are sufficient for _save data_, but for game data, as it is manually read and written, there are more sophisticated tools.

## JSON Inheritance, or `copy-from`, `extend`, `delete`, `proportional`, and `relative`

The primary way _game data_ differs from _save data_ is that game data utilizes [JSON inheritance infrastructure](../JSON/JSON_INHERITANCE.md).
This allows defining JSON entities as copies of other entities, and then modifying the data on top of that.

### `copy-from`

`copy-from` is the special JSON key used to mark that an entity should copy from a "parent" entity of the same type with the id provided (`"copy-from": "foo"`).
In the engine, this is implemented by
1. initializing the C++ structure for the entity with the C++ structure for the entity it copies from
2. Running the loading function for the new entity, skipping missing values (thus preserving them from the parent)

As you may be able to guess from how looking at how _save data_ is deserialized, skipping missing values requires some special support.
However, before [we cover that](#optional-and-mandatory), we will cover the other features the inheritance infrastructure provides.

### `extend` and `delete`

`extend` and `delete` allow adding and removing elements from containers in the C++ structure, without needing to re-specify the entire contents.
A common use-case for this is to add or remove flags from an entity.

This is an opt-in feature - it requires some extra work to support for a field, but often very little.
For full details, see the sections on [readers](#readers-for-optional-and-mandatory) and [without readers](#supporting-relative-proportional-extend-and-delete-without-readers).

### `relative`

`relative` is used to provide additive adjustment to a value.
For example, increasing the weight of an object by 45 g, or increasing damage by 5 points.

The functioning of `relative` is straightforward for simple numerical types, but it is supported by more types and can be supported for any type.
Any type that supports the `+=` operation with values of the same type will automatically support relative.
For full details, see the sections on [readers](#reader-support-for-extend-delete-relative-and-proportional) and [without readers](#supporting-relative-proportional-extend-and-delete-without-readers).

### `proportional`

`proportional` is used to provide scalar adjustment to a value.
For example, halving the weight of an object, or increasing damage by 25%.

The functioning of `proportional` is straightforward for simple numerical types, but it is supported by more types and can be supported for any type.
Any type that supports the `*=` operation with a `float` type will automatically support proportional.
For full details, see the sections on [readers](#reader-support-for-extend-delete-relative-and-proportional) and [without readers](#supporting-relative-proportional-extend-and-delete-without-readers).

## `optional` and `mandatory`

The primary tools to enable these features are the functions `optional` and `mandatory`.
These are defined in `generic_factory.h`, but are heavily templated to provide all the necessary features, so a quick rundown of how they work is provided here.

`optional` is a function with five or six parameters, and is used to read optional data - data that has a default value if not specified.
Importantly, the default value for optional data **must be in the call to optional**, not initialized in the header or some other location (other than `copy-from`).

The two signatures for `optional<T>` are
1. `optional( const JsonObject &jo, bool was_loaded, std::string_view name, T &member, const T &default = T() )`
2. `optional( const JsonObject &jo, bool was_loaded, std::string_view name, T &member, const ReaderType &reader, const T &default = T() )`

`mandatory` is a function with four or five parameters, and is used to read data that must be specified.
If the key for the field being loaded is missing, the game will not complete loading.

The two signatures for `mandatory<T>` are
1. `mandatory( const JsonObject &jo, bool was_loaded, std::string_view name, T &member )`
1. `mandatory( const JsonObject &jo, bool was_loaded, std::string_view name, T &member, const ReaderType &reader )`

The parameters are as follows:
- `jo` is the `JsonObject` that the field is being loaded from.
- `was_loaded` is used to support [skipping members](#copy-from) for `copy-from`. If the entity being loaded does not have a `bool was_loaded` field in its definition, it should always be `false`. If it is false and the field is not defined in the loaded `JsonObject`, the field being loaded will be set to the default value.
- `member` is the variable being loaded.
- `reader` defines a reader object for loading the field. This is a powerful feature and allows support of many JSON formats, among other things. The version of these functions without a `reader` does not support `extend` and `delete`.
- `default` is the default value of the variable being loaded. If not specified, it will use `T()`.

Ideally, the loading function for game data will just be a series of calls to `mandatory` or `optional`.
Re-using the example from [above](#mapping-to-canonical-structures-serialize-and-deserialize), but if `baz` was _game data_ instead:
```cpp
class foo
{
    std::string id;
    std::pair<int, int> limits;
    bool bounded;

    void deserialize( const JsonObject &jo );
};

class baz
{
    string_id<baz> id;
    bool was_loaded = false;

    int weight;
    std::vector<foo> options;

    void load( const JsonObject &jo, const std::string & );
};

void foo::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "id", id );
    mandatory( jo, false, "limits", limits );
    optional( jo, false, "bounded", bounded, false );
}

void baz::load( const JsonObject &jo, const std::string & )
{
    optional( jo, was_loaded, "weight", weight, 1 );
    mandatory( jo, was_loaded, "options", options );
}
```

You will note there are some differences to `baz`.
These are covered in the section on [generic_factory](#loading-whole-types), but do not change the fundamentals of loading.

## Readers for `optional` and `mandatory`

Unless a reader is used, `optional` and `mandatory` will simply use the `read` function [described above](#mapping-to-canonical-structures-serialize-and-deserialize) to load the given variable.
This means that it only supports the "canonical" forms and does not know anything about the structure of the data being read.

When using a reader, `optional` or `mandatory` will detect when a container is being loaded.
If it detects a container is being used (there is a `handler<C>` implementation in `generic_factory.h`), then it will operate on the contents of the container, instead of the whole type.

There are a variety of pre-existing readers, implemented mostly in `generic_factory.h`.
A reader is a `class` or `struct` that implements a `T get_next( const JsonValue &jv ) const` function, and inherits from `generic_typed_reader<reader_type>`.
These are the minimum requirements, but there are more features.
An example reader is implemented below.

```cpp
struct foo_reader : generic_typed_reader<foo_reader>
{
    foo get_next( const JsonValue &jv ) const;
};
```

To use a reader, you need to create an instance of that type, and pass it as an argument to `optional` or `mandatory`.
This shows how the `baz::load` example above would be modified to use a reader.
Because a reader is now being used, when `options` is loaded, it will detect it is being loaded into a container and enable using `extend`/`delete`.
If you just need a reader to enable `extend`/`delete`, consider using `json_read_reader<T>`, which just reads the type as it would otherwise be read, but enables the container detection feature of `optional`/`mandatory`.

```cpp
void baz::load( const JsonObject &jo, const std::string & )
{
    optional( jo, was_loaded, "weight", weight, 1 );
    mandatory( jo, was_loaded, "options", options, foo_reader{} );
}
```

### Reader support for `extend`, `delete`, `relative`, and `proportional`

The support of these features is dependent on whether or not a container is being used.

If a container is being used, the readers natively support `extend` and `delete`.
`relative` is supported for [only certain containers](#relative-containers-support), and `proportional` is not supported at all, but support could easily be implemented for the same containers.

If a container is not being used, `extend` and `delete` are supported if `T` has a `handle_extend` or `handle_delete` function, or the reader implements `do_extend` or `do_delete`.
Similarly, `do_relative` can be implemented in the reader to perform `relative`, or is natively supported if `T` supports `operator::+=` with the return type of the reader's `get_next`.
Support for `proportional` is provided if `T::handle_proportional( const JsonValue & )` is implemented or `T` can perform a `*=` operation against a `double` value.
There is no equivalent `do_proportional` function.

The `do_extend`, `do_delete`, and `do_relative` functions all have the same three arguments.
1. `jo`, the `extend`/`delete`/`relative` `JsonObject`
2. `name`, the key being modified
3. `member`, the variable of type `T` being modified

The functions will always be called if they exist, regardless of if the key is present in the object.
They return `true` if a modification has occurred and `false` if it has not.

An example of the use of a reader to handle extend/delete follows.

```cpp
struct capability
{
    std::string tag;
    int amount = 1;
    bool visible = true;
    bool harmful = false;

    void deserialize( const JsonObject &jo );
};

// Just a wrapper of a vector, so there's no handler<>
// Have a better reason in a real implementation :)
struct capabilities
{
    std::vector<capability> list;
};

struct foo
{
    bool was_loaded;
    string_id<foo> id;

    capabilities list;

    void load( const JsonObject &jo, const std::string & );
};

struct capability_reader : generic_typed_reader<capability_reader>
{
    capabilities get_next( const JsonValue &jv ) const {
        capabilities ret;
        jv.read( ret.list );
        return ret;
    }

    bool do_extend( const JsonObject &jo, const std::string_view name, capabilities &member ) const {
        if( jo.has_member( name ) ) {
            return false;
        }
        JsonValue jv = jo.get_value( name );
        // adding multiple, array format
        if( jv.test_array() ) {
            JsonArray ja = jv.get_array();
            for( const JsonValue &sv : ja ) {
                capability tmp;
                tmp.deserialize( sv.get_object() );
                member.list.push_back( tmp );
            }
            return true;
        }
        // adding only one
        JsonObject jo = jv.get_object();
        capability tmp;
        tmp.deserialize( jo );
        member.list.push_back( tmp );
        return true;
    }

    // delete just deletes by tag
    bool do_extend( const JsonObject &jo, const std::string_view name, capabilities &member ) const {
        if( jo.has_member( name ) ) {
            return false;
        }
        JsonValue jv = jo.get_value( name );
        // Ideally check if removal actually occurs
        // not done here to keep the example simple
        if( jv.test_array() ) {
            JsonArray ja = jv.get_array();
            for( const JsonValue &sv : ja ) {
                std::string tag = sv.get_string();
                std::erase( std::remove_if( member.list.begin(),
                                            member.list.end(),
                                            [&tag](const capability &c) { return c.tag == tag; } ),
                            member.list.end() );
            }
            return true;
        }
        std::string tag = jv.get_string();
        std::erase( std::remove_if( member.list.begin(),
                                    member.list.end(),
                                    [&tag](const capability &c) { return c.tag == tag; } ),
                    member.list.end() );
        return true;
    }
};

void foo::load( const JsonObject &jo, const std::string & )
{
    mandatory( jo, was_loaded, "capbilities", list, capability_reader{} );
}
```

#### Disabling Strict Checks
The `weakpoints` class uses `extend`/`delete` on an object that does not use `copy-from`.
This requires disabling some safety checks, which can be done on a per reader basis by adding the field `static constexpr bool check_extend_delete_copy_from = false;`.

### `relative` Containers Support
`relative` only supports certain containers.
Specifically, "keyed" containers (`std::map`, and `std::unordered_map`) are supported, because they can use the key to find the value to modify.
The modification is then performed by use of `+=` on the value.

### Custom JSON Formats with Readers

Because `get_next` takes in a `JsonValue`, it allows you to define custom JSON formats for the data being loaded.
If I wanted to load a `tripoint` with a constant z value, and so only require the x and y values, a reader would enable doing this.
Further, if I want to support a variety of formats, such as `{ "x": X, "y": Y }`, `[ X, Y ]`, `NUMBER_FOR_X_AND_Y`, this is possible.
An example showing how this would be done is below.

```cpp
struct my_tripoint_reader : generic_typed_reader<my_tripoint_reader>
{
    int z;

    explicit my_tripoint_reader( int z_ ) : z( z_ ) {}

    tripoint get_next( const JsonValue &jv ) const {
        tripoint ret;
        ret.z = z;
        if( jv.test_object() ) {
            JsonObject jo = jv.get_object();
            jo.read( "x", ret.x );
            jo.read( "y", ret.y );
        } else if( jv.test_array() ) {
            JsonArray ja = jv.get_array();
            if( ja.size() != 2 ) {
                ja.throw_error( "Expected array of length 2" );
            }
            ret.x = ja.get_int( 0 );
            ret.y = ja.get_int( 1 );
        } else if( jv.test_int() ) {
            int xy = jv.get_int();
            ret.x = xy;
            ret.y = xy;
        } else {
            jv.throw_error( "Invalid point format" );
        }
        return ret;
    }
};

void examp::load( const JsonObject &jo, const std::string & )
{
    optional( jo, was_loaded, "points", points, my_tripoint_reader{2} );
}
```

### Readers with Container Values

One difficulty with reading container values is determining what to pass to the `get_next` function of a reader.
For single values in containers (e.g. a vector), omitting the container structure is permitted (e.g. `[ 1 ]` or `1` is acceptable).
This is handled by testing if the value for the key is an *array*, and passing the contents to `get_next` one by one if it is, otherwise just passing the value directly to `get_next`.
However, some readers may want to read the elements of *object*s, such as those for `map`s or weighted lists.
By overwriting the `read_objects` property of `generic_typed_reader`, the behavior can be changed to pass the key/values (`JsonMember`s) of *object*s, the values of *array*s, and the value only if it is neither of those to `get_next`.
An example is provided below.

```cpp
struct my_reader : generic_typed_reader<my_reader>
{
    static constexpr bool read_objects = true;

    std::pair<std::string, int> get_next( const JsonValue &jv ) const {
        // "a": 1
        if( jv.is_member() ) {
            const JsonMember &jm = dynamic_cast<const JsonMember &>( jv );
            return std::make_pair( jm.name(), jm.get_int() );
        }
        // [ "a", 1 ]
        // If this fails it's an invalid format, will throw an error for us
        JsonArray ja = jv.get_array();
        // TODO: check size of ja
        return std::make_pair( ja.get_string( 0 ), ja.get_int( 1 ) );
    }
}
```

## Supporting `relative`, `proportional`, `extend`, and `delete` without Readers

If readers are not used, `relative`, `proportional`, `extend`, and `delete` can still be used.

`extend` and `delete` are supported if the type `T` being loaded has `T::handle_extend( const JsonValue & )` or `T::handle_delete( const JsonValue & )` function respectively.
The `JsonValue` argument is the value of the key passed to `optional` or `mandatory` in the `extend` or `delete` blocks.

`relative` is enabled if the type `T` being loaded supports a `+=` operation against its own type.

`proportional` is enabled if the type `T` being loaded supports a `*=` operation against a floating point value or `T::handle_proportional( const JsonValue & )` is implemented.
The `JsonValue` argument is the value of the key passed to `optional` or `mandatory` in the `proportional`block.

## Loading Whole Types of *game data*

So far this document has covered how to load individual pieces of *game data*.
This section covers how to load types of data, such as `"ITEM"`s or `"profession"`s.

A "flyweight" pattern is used for these types of data.
Each *object* in JSON is loaded with an `"id"`, and this is used within the game engine to refer to the loaded data, which is mutable after loading.
If any entity in the game has dynamic state which is modified after loading, it generally uses the loaded data to define a "type", which the dynamic object points to with an id.
An example of this is `itype` (item type), which is what `"type": "ITEM"` loads into, and `item`, which represents an item in the game world, and points to its `itype`.
Because professions have no dynamic data, they can simply use the loaded type.

### `generic_factory`

`generic_factory` is the class that is used to load these types.

The comments in `generic_factory.h` explain how to use it.
