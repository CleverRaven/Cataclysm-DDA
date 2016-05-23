# Internals

## UID

The `uid` class provides a 64-bit unsigned integer identifier which is unique amongst all non-null instances of a class within a world. Unlike a C++ pointer
it can be serialized to JSON.

It's value can never be directly accessed however it can be compared to another
`uid` of the same type. If a class is copied then the duplicate is allocated the next available identifier for that type whereas moving an instance leaves the `uid` unchanged.

### Adding a class UID

The implementation supports a maximum of 256 possible types of `uid` with the known types enumerated in `uid.h`. There should only be one `uid_factory` for each type and this will usually be found within the `game` class.

#### Define the UID

```
# uid.h

enum {
   ...
   UID_VEHICLE = 2
}

using vehicle_uid = uid<UID_VEHICLE>
```

```
# uid.cpp

enum {
    template <>
    uid_factory<UID_ITEM> *uid_factory<UID_ITEM>::instance = nullptr;
}
```

#### Define one instance of the factory

```
# game.h
class game {
    ...
    uid_factory<UID_VEHICLE> vehicle_uid_factory;
}
```

```
# savegame.cpp

void game::serialize_master(std::ostream &fout) {
    ...
    json.member( "vehicle_uid", vehicle_uid_factory );
}

void game::unserialize_master(std::istream &fin) {
    ...
    jsin.read( vehicle_uid_factory );
}
```

#### Connect UID to factory in class constructor

```
#vehicle.h

class vehicle {
    ....
    vehicle_uid uid_;

   vehicle::vehicle( ... ) {
       uid_ = g->vehicle_uid_factory.assign();
   }
}
```

#### Serialize the UID

```
# savegame_json.cpp

void vehicle::serialize( JsonOut &js ) const {
    js.start_object();
    ....

    js.member( "uid", uid_ );
}

void vehicle::deserialize( JsonIn &js )
{
    JsonObject obj = jsin.get_object();
    ...

    obj.read( "uid", uid_ );
    if( !uid_.valid() ) {
        /** handle legacy saves without uids */
        uid_ = g->vehicle_uid_factory.assign();
    }  
}
```
