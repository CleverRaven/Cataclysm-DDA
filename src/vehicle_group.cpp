#include "vehicle_group.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>

#include "debug.h"
#include "json.h"
#include "map.h"
#include "memory_fast.h"
#include "point.h"
#include "translations.h"
#include "vehicle.h"
#include "vpart_position.h"

using vplacement_id = string_id<VehiclePlacement>;

std::unordered_map<vgroup_id, VehicleGroup> vgroups;
std::unordered_map<vplacement_id, VehiclePlacement> vplacements;
std::unordered_map<vspawn_id, VehicleSpawn> vspawns;

/** @relates string_id */
template<>
const VehicleGroup &string_id<VehicleGroup>::obj() const
{
    const auto iter = vgroups.find( *this );
    if( iter == vgroups.end() ) {
        debugmsg( "invalid vehicle group id %s", c_str() );
        static const VehicleGroup dummy{};
        return dummy;
    }
    return iter->second;
}

point VehicleLocation::pick_point() const
{
    return point( x.get(), y.get() );
}

/** @relates string_id */
template<>
bool string_id<VehicleGroup>::is_valid() const
{
    return vgroups.count( *this ) > 0;
}

template<>
const VehiclePlacement &string_id<VehiclePlacement>::obj() const
{
    const auto iter = vplacements.find( *this );
    if( iter == vplacements.end() ) {
        debugmsg( "invalid vehicle placement id %s", c_str() );
        static const VehiclePlacement dummy{};
        return dummy;
    }
    return iter->second;
}

template<>
bool string_id<VehiclePlacement>::is_valid() const
{
    return vplacements.count( *this ) > 0;
}

void VehicleGroup::load( const JsonObject &jo )
{
    VehicleGroup &group = vgroups[vgroup_id( jo.get_string( "id" ) )];

    for( JsonArray pair : jo.get_array( "vehicles" ) ) {
        group.add_vehicle( vproto_id( pair.get_string( 0 ) ), pair.get_int( 1 ) );
    }
}

void VehicleGroup::reset()
{
    vgroups.clear();
}

VehicleFacings::VehicleFacings( const JsonObject &jo, const std::string &key )
{
    if( jo.has_array( key ) ) {
        for( const int i : jo.get_array( key ) ) {
            values.push_back( i );
        }
    } else {
        values.push_back( jo.get_int( key ) );
    }
}

void VehiclePlacement::load( const JsonObject &jo )
{
    VehiclePlacement &placement = vplacements[vplacement_id( jo.get_string( "id" ) )];

    for( JsonObject jloc : jo.get_array( "locations" ) ) {
        placement.add( jmapgen_int( jloc, "x" ), jmapgen_int( jloc, "y" ),
                       VehicleFacings( jloc, "facing" ) );
    }
}

void VehiclePlacement::reset()
{
    vplacements.clear();
}

const VehicleLocation *VehiclePlacement::pick() const
{
    if( const auto chosen = random_entry_opt( locations ) ) {
        return &chosen->get();
    }
    debugmsg( "vehicleplacement has no locations" );
    return nullptr;
}

VehicleFunction_json::VehicleFunction_json( const JsonObject &jo )
    : vehicle( jo.get_string( "vehicle" ) ),
      number( jo, "number" ),
      fuel( jo.get_int( "fuel" ) ),
      status( jo.get_int( "status" ) )
{
    if( jo.has_string( "placement" ) ) {
        placement = jo.get_string( "placement" );
    } else {
        VehicleFacings facings( jo, "facing" );
        location.emplace( jmapgen_int( jo, "x" ), jmapgen_int( jo, "y" ), facings );
    }
}

void VehicleFunction_json::apply( map &m, const std::string &terrain_name ) const
{
    for( auto i = number.get(); i > 0; i-- ) {
        if( !location ) {
            const size_t replace = placement.find( "%t" );
            const VehicleLocation *loc = vplacement_id( replace != std::string::npos ?
                                         placement.substr( 0, replace ) + terrain_name +
                                         placement.substr( replace + 2 ) :
                                         placement ).obj().pick();

            if( !loc ) {
                debugmsg( "vehiclefunction_json: unable to get location to place vehicle." );
                return;
            }
            m.add_vehicle( vehicle, loc->pick_point(), loc->pick_facing(), fuel, status );
        } else {
            m.add_vehicle( vehicle, location->pick_point(), location->pick_facing(), fuel, status );
        }
    }
}

template<>
const VehicleSpawn &string_id<VehicleSpawn>::obj() const
{
    const auto iter = vspawns.find( *this );
    if( iter == vspawns.end() ) {
        debugmsg( "invalid vehicle spawn id %s", c_str() );
        static const VehicleSpawn dummy{};
        return dummy;
    }
    return iter->second;
}

template<>
bool string_id<VehicleSpawn>::is_valid() const
{
    return vspawns.count( *this ) > 0;
}

void VehicleSpawn::load( const JsonObject &jo )
{
    VehicleSpawn &spawn = vspawns[vspawn_id( jo.get_string( "id" ) )];
    for( JsonObject type : jo.get_array( "spawn_types" ) ) {
        if( type.has_object( "vehicle_json" ) ) {
            JsonObject vjo = type.get_object( "vehicle_json" );
            spawn.add( type.get_float( "weight" ), make_shared_fast<VehicleFunction_json>( vjo ) );
        } else if( type.has_string( "vehicle_function" ) ) {
            if( builtin_functions.count( type.get_string( "vehicle_function" ) ) == 0 ) {
                type.throw_error( "load_vehicle_spawn: unable to find builtin function", "vehicle_function" );
            }

            spawn.add( type.get_float( "weight" ), make_shared_fast<VehicleFunction_builtin>
                       ( builtin_functions[type.get_string( "vehicle_function" )] ) );
        } else {
            type.throw_error( "load_vehicle_spawn: missing required vehicle_json (object) or vehicle_function (string)." );
        }
    }
}

void VehicleSpawn::reset()
{
    vspawns.clear();
}

void VehicleSpawn::apply( map &m, const std::string &terrain_name ) const
{
    const shared_ptr_fast<VehicleFunction> *func = types.pick();
    if( func == nullptr ) {
        debugmsg( "unable to find valid function for vehicle spawn" );
    } else {
        ( *func )->apply( m, terrain_name );
    }
}

void VehicleSpawn::apply( const vspawn_id &id, map &m, const std::string &terrain_name )
{
    id.obj().apply( m, terrain_name );
}

namespace VehicleSpawnFunction
{

static void builtin_no_vehicles( map &, const std::string & )
{}

static void builtin_jackknifed_semi( map &m, const std::string &terrainid )
{
    const VehicleLocation *loc = vplacement_id( terrainid + "_semi" ).obj().pick();
    if( !loc ) {
        debugmsg( "builtin_jackknifed_semi unable to get location to place vehicle.  placement %s_semi",
                  terrainid );
        return;
    }

    const int facing = loc->pick_facing();
    const point semi_p = loc->pick_point();
    point trailer_p;

    if( facing == 0 ) {
        trailer_p.x = semi_p.x + 4;
        trailer_p.y = semi_p.y - 10;
    } else if( facing == 90 ) {
        trailer_p.x = semi_p.x + 12;
        trailer_p.y = semi_p.y + 1;
    } else if( facing == 180 ) {
        trailer_p.x = semi_p.x - 4;
        trailer_p.y = semi_p.y + 10;
    } else {
        trailer_p.x = semi_p.x - 12;
        trailer_p.y = semi_p.y - 1;
    }

    m.add_vehicle( vgroup_id( "semi_truck" ), semi_p, ( facing + 135 ) % 360, -1, 1 );
    m.add_vehicle( vgroup_id( "truck_trailer" ), trailer_p, ( facing + 90 ) % 360, -1, 1 );
}

static void builtin_pileup( map &m, const std::string &, const std::string &vg )
{
    vehicle *last_added_car = nullptr;
    const int num_cars = rng( 5, 12 );

    for( int i = 0; i < num_cars; i++ ) {
        const VehicleLocation *loc = vplacement_id( "pileup" ).obj().pick();
        if( !loc ) {
            debugmsg( "builtin_pileup unable to get location to place vehicle." );
            return;
        }

        last_added_car = m.add_vehicle( vgroup_id( vg ), loc->pick_point(),
                                        loc->pick_facing(), -1, 1 );
        if( last_added_car != nullptr ) {
            last_added_car->name = _( "pile-up" );
        } else {
            break;
        }
    }
}

static void builtin_citypileup( map &m, const std::string &t )
{
    builtin_pileup( m, t, "city_pileup" );
}

static void builtin_policepileup( map &m, const std::string &t )
{
    builtin_pileup( m, t, "police_pileup" );
}

static void builtin_parkinglot( map &m, const std::string & )
{
    for( int v = 0; v < rng( 1, 4 ); v++ ) {
        tripoint pos_p;
        pos_p.x = rng( 0, 1 ) * 15 + rng( 4, 5 );
        pos_p.y = rng( 0, 4 ) * 4 + rng( 2, 4 );
        pos_p.z = m.get_abs_sub().z;

        if( !m.veh_at( pos_p ) ) {
            m.add_vehicle( vgroup_id( "parkinglot" ), pos_p,
                           ( one_in( 2 ) ? 0 : 180 ) + one_in( 10 ) * rng( 0, 179 ), -1, -1 );
        }
    }
}

} // namespace VehicleSpawnFunction

VehicleSpawn::FunctionMap VehicleSpawn::builtin_functions = {
    { "no_vehicles", VehicleSpawnFunction::builtin_no_vehicles },
    { "jack-knifed_semi", VehicleSpawnFunction::builtin_jackknifed_semi },
    { "vehicle_pileup", VehicleSpawnFunction::builtin_citypileup },
    { "policecar_pileup", VehicleSpawnFunction::builtin_policepileup },
    { "parkinglot", VehicleSpawnFunction::builtin_parkinglot }
};
