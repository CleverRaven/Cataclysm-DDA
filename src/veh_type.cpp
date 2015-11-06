#include "veh_type.h"
#include "vehicle.h"
#include "game.h"
#include "debug.h"
#include "item_group.h"
#include "json.h"
#include "translations.h"
#include "color.h"
#include "itype.h"
#include "vehicle_group.h"

#include <unordered_map>
#include <unordered_set>

std::unordered_map<vproto_id, vehicle_prototype> vtypes;

// GENERAL GUIDELINES
// To determine mount position for parts (dx, dy), check this scheme:
//         orthogonal dir left: (Y-)
//                ^
//  back: X-   -------> forward dir: X+
//                v
//         orthogonal dir right (Y+)
//
// i.e, if you want to add a part to the back from the center of vehicle,
// use dx = -1, dy = 0;
// for the part 1 tile forward and two tiles left from the center of vehicle,
// use dx = 1, dy = -2.
//
// Internal parts should be added after external on the same mount point, i.e:
//  part {"x": 0, "y": 1, "part": "seat"},       // put a seat (it's external)
//  part {"x": 0, "y": 1, "part": "controls"},   // put controls for driver here
//  part {"x": 0, "y": 1, "seatbelt"}   // also, put a seatbelt here
// To determine, what parts can be external, and what can not, check
// vehicle_parts.json
// If you use wrong config, installation of part will fail

static const std::unordered_map<std::string, vpart_bitflags> vpart_bitflag_map = {
    { "ARMOR", VPFLAG_ARMOR },
    { "EVENTURN", VPFLAG_EVENTURN },
    { "ODDTURN", VPFLAG_ODDTURN },
    { "CONE_LIGHT", VPFLAG_CONE_LIGHT },
    { "CIRCLE_LIGHT", VPFLAG_CIRCLE_LIGHT },
    { "BOARDABLE", VPFLAG_BOARDABLE },
    { "AISLE", VPFLAG_AISLE },
    { "CONTROLS", VPFLAG_CONTROLS },
    { "OBSTACLE", VPFLAG_OBSTACLE },
    { "OPAQUE", VPFLAG_OPAQUE },
    { "OPENABLE", VPFLAG_OPENABLE },
    { "SEATBELT", VPFLAG_SEATBELT },
    { "WHEEL", VPFLAG_WHEEL },
    { "FLOATS", VPFLAG_FLOATS },
    { "DOME_LIGHT", VPFLAG_DOME_LIGHT },
    { "AISLE_LIGHT", VPFLAG_AISLE_LIGHT },
    { "ATOMIC_LIGHT", VPFLAG_ATOMIC_LIGHT },
    { "ALTERNATOR", VPFLAG_ALTERNATOR },
    { "ENGINE", VPFLAG_ENGINE },
    { "FRIDGE", VPFLAG_FRIDGE },
    { "FUEL_TANK", VPFLAG_FUEL_TANK },
    { "LIGHT", VPFLAG_LIGHT },
    { "WINDOW", VPFLAG_WINDOW },
    { "CURTAIN", VPFLAG_CURTAIN },
    { "CARGO", VPFLAG_CARGO },
    { "INTERNAL", VPFLAG_INTERNAL },
    { "SOLAR_PANEL", VPFLAG_SOLAR_PANEL },
    { "VARIABLE_SIZE", VPFLAG_VARIABLE_SIZE },
    { "VPFLAG_TRACK", VPFLAG_TRACK },
    { "RECHARGE", VPFLAG_RECHARGE },
    { "VISION", VPFLAG_EXTENDS_VISION }
};

std::map<vpart_str_id, vpart_info> vehicle_part_types;
// Contains pointer into the vehicle_part_types map. It is an implicit mapping of int ids
// to the matching vpart_info object. To store the object only once, it is in the map and only
// linked to. Pointers here are always valid.
std::vector<const vpart_info*> vehicle_part_int_types;

template<>
const vpart_str_id string_id<vpart_info>::NULL_ID( "null" );

template<>
const vpart_info &int_id<vpart_info>::obj() const
{
    if( static_cast<size_t>( _id ) >= vehicle_part_int_types.size() ) {
        debugmsg( "invalid vehicle part id %d", _id );
        static const vpart_info dummy{};
        return dummy;
    }
    return *vehicle_part_int_types[_id];
}

template<>
const string_id<vpart_info> &int_id<vpart_info>::id() const
{
    return obj().id;
}

template<>
int_id<vpart_info> string_id<vpart_info>::id() const
{
    const auto iter = vehicle_part_types.find( *this );
    if( iter == vehicle_part_types.end() ) {
        debugmsg( "invalid vehicle part id %s", c_str() );
        return vpart_id();
    }
    return iter->second.loadid;
}

template<>
const vpart_info &string_id<vpart_info>::obj() const
{
    return id().obj();
}

template<>
bool string_id<vpart_info>::is_valid() const
{
    return vehicle_part_types.count( *this ) > 0;
}

template<>
int_id<vpart_info>::int_id( const string_id<vpart_info> &id )
: _id( id.id() )
{
}

// Note on the 'symbol' flag in vehicle parts -
// the following symbols will be translated:
// y, u, n, b to NW, NE, SE, SW lines correspondingly
// h, j, c to horizontal, vertical, cross correspondingly
/**
 * Reads in a vehicle part from a JsonObject.
 */
void vpart_info::load( JsonObject &jo )
{
    vpart_info next_part;

    next_part.id = vpart_str_id( jo.get_string( "id" ) );
    next_part.name = _(jo.get_string("name").c_str());
    next_part.sym = jo.get_string("symbol")[0];
    next_part.color = color_from_string(jo.get_string("color"));
    next_part.sym_broken = jo.get_string("broken_symbol")[0];
    next_part.color_broken = color_from_string(jo.get_string("broken_color"));
    next_part.dmg_mod = jo.has_member("damage_modifier") ? jo.get_int("damage_modifier") : 100;
    next_part.durability = jo.get_int("durability");
    next_part.power = jo.get_int("power", 0);
    next_part.epower = jo.get_int("epower", 0);
    next_part.folded_volume = jo.get_int("folded_volume", 0);
    next_part.range = jo.get_int( "range", 12 );

    //Handle the par1 union as best we can by accepting any ONE of its elements
    int element_count = (jo.has_member("par1") ? 1 : 0)
                        + (jo.has_member("size") ? 1 : 0)
                        + (jo.has_member("wheel_width") ? 1 : 0)
                        + (jo.has_member("bonus") ? 1 : 0);

    if(element_count == 0) {
        //If not specified, assume 0
        next_part.par1 = 0;
    } else if(element_count == 1) {
        if(jo.has_member("par1")) {
            next_part.par1 = jo.get_int("par1");
        } else if(jo.has_member("size")) {
            next_part.par1 = jo.get_int("size");
        } else if(jo.has_member("wheel_width")) {
            next_part.par1 = jo.get_int("wheel_width");
        } else { //bonus
            next_part.par1 = jo.get_int("bonus");
        }
    } else {
        //Too many
        debugmsg("Error parsing vehicle part '%s': \
               Use AT MOST one of: par1, power, size, wheel_width, bonus",
                 next_part.name.c_str());
        //Keep going to produce more messages if other parts are wrong
        next_part.par1 = 0;
    }
    next_part.fuel_type = jo.get_string( "fuel_type", "null" );
    next_part.item = jo.get_string("item");
    next_part.difficulty = jo.get_int("difficulty");
    next_part.location = jo.has_member("location") ? jo.get_string("location") : "";

    JsonArray jarr = jo.get_array("flags");
    while (jarr.has_more()) {
        next_part.set_flag( jarr.next_string() );
    }

    if( jo.has_member( "breaks_into" ) ) {
        JsonIn& stream = *jo.get_raw( "breaks_into" );
        next_part.breaks_into_group = item_group::load_item_group( stream, "collection" );
    } else {
        next_part.breaks_into_group = "EMPTY_GROUP";
    }

    //Calculate and cache z-ordering based off of location
    // list_order is used when inspecting the vehicle
    if(next_part.location == "on_roof") {
        next_part.z_order = 9;
        next_part.list_order = 3;
    } else if(next_part.location == "on_cargo") {
        next_part.z_order = 8;
        next_part.list_order = 6;
    } else if(next_part.location == "center") {
        next_part.z_order = 7;
        next_part.list_order = 7;
    } else if(next_part.location == "under") {
        //Have wheels show up over frames
        next_part.z_order = 6;
        next_part.list_order = 10;
    } else if(next_part.location == "structure") {
        next_part.z_order = 5;
        next_part.list_order = 1;
    } else if(next_part.location == "engine_block") {
        //Should be hidden by frames
        next_part.z_order = 4;
        next_part.list_order = 8 ;
    } else if (next_part.location == "on_battery_mount"){
        //Should be hidden by frames
        next_part.z_order = 3;
        next_part.list_order = 10;
    } else if(next_part.location == "fuel_source") {
        //Should be hidden by frames
        next_part.z_order = 3;
        next_part.list_order = 9;
    } else if(next_part.location == "roof") {
        //Shouldn't be displayed
        next_part.z_order = -1;
        next_part.list_order = 4;
    } else if(next_part.location == "armor") {
        //Shouldn't be displayed (the color is used, but not the symbol)
        next_part.z_order = -2;
        next_part.list_order = 2;
    } else {
        //Everything else
        next_part.z_order = 0;
        next_part.list_order = 5;
    }

    auto const iter = vehicle_part_types.find( next_part.id );
    if( iter != vehicle_part_types.end() ) {
        // Entry in the map already exists, so the pointer in the vector is already correct
        // and does not need to be changed, only the int-id needs to be taken from the old entry.
        next_part.loadid = iter->second.loadid;
        iter->second = next_part;
    } else {
        // The entry is new, "generate" a new int-id and link the new entry from the vector.
        next_part.loadid = vpart_id( vehicle_part_int_types.size() );
        vpart_info &new_entry = vehicle_part_types[next_part.id];
        new_entry = next_part;
        vehicle_part_int_types.push_back( &new_entry );
    }
}

void vpart_info::set_flag( const std::string &flag )
{
    flags.insert( flag );
    const auto iter = vpart_bitflag_map.find( flag );
    if( iter != vpart_bitflag_map.end() ) {
        bitflags.set( iter->second );
    }
}

void vpart_info::check()
{
    for( auto &part_ptr : vehicle_part_int_types ) {
        auto &part = *part_ptr;
        if( !item_group::group_is_defined( part.breaks_into_group ) ) {
            debugmsg( "Vehicle part %s breaks into non-existent item group %s.",
                      part.id.c_str(), part.breaks_into_group.c_str() );
        }
        if( part.has_flag( "FOLDABLE" ) && part.folded_volume == 0 ) {
            debugmsg("Error: folded part %s has a volume of 0!", part.name.c_str());
        }
        if( part.has_flag( VPFLAG_FUEL_TANK ) && !item::type_is_defined( part.fuel_type ) ) {
            debugmsg( "vehicle part %s is a fuel tank, but has invalid fuel type %s (not a valid item id)", part.id.c_str(), part.fuel_type.c_str() );
        }
        // For now, ignore invalid item ids, later add a check and assume here they are valid.
        if( part.has_flag( "TURRET" ) && item::type_is_defined( part.item ) ) {
            if( !item::find_type( part.item )->gun ) {
                debugmsg( "vehicle part %s has the TURRET flag, but is not made from a gun item", part.id.c_str(), part.item.c_str() );
            }
        }
    }
}

void vpart_info::reset()
{
    vehicle_part_types.clear();
    vehicle_part_int_types.clear();
}

const std::vector<const vpart_info*> &vpart_info::get_all()
{
    return vehicle_part_int_types;
}

template<>
const vehicle_prototype &string_id<vehicle_prototype>::obj() const
{
    const auto iter = vtypes.find( *this );
    if( iter == vtypes.end() ) {
        debugmsg( "invalid vehicle prototype id %s", c_str() );
        static const vehicle_prototype dummy = {
            "",
            std::vector<std::pair<point, vpart_str_id>>{},
            std::vector<vehicle_item_spawn>{},
            nullptr
        };
        return dummy;
    }
    return iter->second;
}

template<>
bool string_id<vehicle_prototype>::is_valid() const
{
    return vtypes.count( *this ) > 0;
}

/**
 *Caches a vehicle definition from a JsonObject to be loaded after itypes is initialized.
 */
void vehicle_prototype::load(JsonObject &jo)
{
    vehicle_prototype &vproto = vtypes[ vproto_id( jo.get_string( "id" ) ) ];
    // If there are already parts defined, this vehicle prototype overrides an existing one.
    // If the json contains a name, it means a completely new prototype (replacing the
    // original one), therefor the old data has to be cleared.
    // If the json does not contain a name (the prototype would have no name), it means appending
    // to the existing prototype (the parts are not cleared).
    if( !vproto.parts.empty() && jo.has_string( "name" ) ) {
        vproto =  vehicle_prototype(); 
    }
    if( vproto.parts.empty() ) {
        vproto.name = jo.get_string( "name" );
    }

    vgroups[vgroup_id(jo.get_string("id"))].add_vehicle(vproto_id(jo.get_string("id")), 100);

    JsonArray parts = jo.get_array("parts");
    while (parts.has_more()) {
        JsonObject part = parts.next_object();
        const point pxy( part.get_int("x"), part.get_int("y") );
        const vpart_str_id pid( part.get_string( "part" ) );
        vproto.parts.emplace_back( pxy, pid );
    }

    JsonArray items = jo.get_array("items");
    while(items.has_more()) {
        JsonObject spawn_info = items.next_object();
        vehicle_item_spawn next_spawn;
        next_spawn.pos.x = spawn_info.get_int("x");
        next_spawn.pos.y = spawn_info.get_int("y");
        next_spawn.chance = spawn_info.get_int("chance");
        if(next_spawn.chance <= 0 || next_spawn.chance > 100) {
            debugmsg("Invalid spawn chance in %s (%d, %d): %d%%",
                     vproto.name.c_str(), next_spawn.pos.x, next_spawn.pos.y, next_spawn.chance);
        }
        if(spawn_info.has_array("items")) {
            //Array of items that all spawn together (ie jack+tire)
            JsonArray item_group = spawn_info.get_array("items");
            while(item_group.has_more()) {
                next_spawn.item_ids.push_back(item_group.next_string());
            }
        } else if(spawn_info.has_string("items")) {
            //Treat single item as array
            next_spawn.item_ids.push_back(spawn_info.get_string("items"));
        }
        if(spawn_info.has_array("item_groups")) {
            //Pick from a group of items, just like map::place_items
            JsonArray item_group_names = spawn_info.get_array("item_groups");
            while(item_group_names.has_more()) {
                next_spawn.item_groups.push_back(item_group_names.next_string());
            }
        } else if(spawn_info.has_string("item_groups")) {
            next_spawn.item_groups.push_back(spawn_info.get_string("item_groups"));
        }
        vproto.item_spawns.push_back( std::move( next_spawn ) );
    }
}

void vehicle_prototype::reset()
{
    vtypes.clear();
}

/**
 *Works through cached vehicle definitions and creates vehicle objects from them.
 */
void vehicle_prototype::finalize()
{
    for( auto &vp : vtypes ) {
        std::unordered_set<point> cargo_spots;
        vehicle_prototype &proto = vp.second;
        const vproto_id &id = vp.first;

        // Calls the default constructor to create an empty vehicle. Calling the constructor with
        // the type as parameter would make it look up the type in the map and copy the
        // (non-existing) blueprint.
        proto.blueprint.reset( new vehicle() );
        vehicle &blueprint = *proto.blueprint;
        blueprint.type = id;
        blueprint.name = _(proto.name.c_str());

        for( auto &part : proto.parts ) {
            const point &p = part.first;
            const vpart_str_id &part_id = part.second;
            if( !part_id.is_valid() ) {
                debugmsg("unknown vehicle part %s in %s", part_id.c_str(), id.c_str());
                continue;
            }

            if(blueprint.install_part(p.x, p.y, part_id) < 0) {
                debugmsg("init_vehicles: '%s' part '%s'(%d) can't be installed to %d,%d",
                         blueprint.name.c_str(), part_id.c_str(),
                         blueprint.parts.size(), p.x, p.y);
            }
            if( part_id.obj().has_flag("CARGO") ) {
                cargo_spots.insert( p );
            }
        }

        for (auto &i : proto.item_spawns) {
            if( cargo_spots.count( i.pos ) == 0 ) {
                debugmsg("Invalid spawn location (no CARGO vpart) in %s (%d, %d): %d%%",
                         proto.name.c_str(), i.pos.x, i.pos.y, i.chance);
            }
            for (auto &j : i.item_ids) {
                if( !item::type_is_defined( j ) ) {
                    debugmsg("unknown item %s in spawn list of %s", j.c_str(), id.c_str());
                }
            }
            for (auto &j : i.item_groups) {
                if (!item_group::group_is_defined(j)) {
                    debugmsg("unknown item group %s in spawn list of %s", j.c_str(), id.c_str());
                }
            }
        }
        // Clear the parts vector as it is not needed anymore. Usage of swap guaranties that the
        // memory of the vector is really freed (instead of simply marking the vector as empty).
        std::remove_reference<decltype(proto.parts)>::type().swap( proto.parts );
    }
}

std::vector<vproto_id> vehicle_prototype::get_all()
{
    std::vector<vproto_id> result;
    for( auto & vp : vtypes ) {
        result.push_back( vp.first );
    }
    return result;
}
