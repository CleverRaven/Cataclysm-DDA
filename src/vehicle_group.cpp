#include "vehicle_group.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "coordinates.h"
#include "debug.h"
#include "flexbuffer_json.h"
#include "map.h"
#include "memory_fast.h"
#include "point.h"
#include "units.h"
#include "vpart_position.h"

using vplacement_id = string_id<VehiclePlacement>;

static const vgroup_id VehicleGroup_parkinglot( "parkinglot" );

std::unordered_map<vgroup_id, VehicleGroup> vgroups;
static std::unordered_map<vplacement_id, VehiclePlacement> vplacements;
static std::unordered_map<vspawn_id, VehicleSpawn> vspawns;

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

point_bub_ms VehicleLocation::pick_point() const
{
    return point_bub_ms( x.get(), y.get() );
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

std::vector<vproto_id> VehicleGroup::all_possible_results() const
{
    std::vector<vproto_id> result;
    result.reserve( vehicles.size() );
    for( const weighted_object<int, vproto_id> &wo : vehicles ) {
        result.push_back( wo.obj );
    }
    return result;
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

VehicleFacings::VehicleFacings( const JsonObject &jo, std::string_view key )
{
    if( jo.has_array( key ) ) {
        for( const int i : jo.get_array( key ) ) {
            values.push_back( units::from_degrees( i ) );
        }
    } else {
        values.push_back( units::from_degrees( jo.get_int( key ) ) );
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
    for( int i = number.get(); i > 0; i-- ) {
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
            const tripoint_bub_ms pos{ loc->pick_point(), m.get_abs_sub().z() };
            m.add_vehicle( vehicle->pick(), pos, loc->pick_facing(), fuel, status );
        } else {
            const tripoint_bub_ms pos{ location->pick_point(), m.get_abs_sub().z()};
            m.add_vehicle( vehicle->pick(), pos, location->pick_facing(), fuel, status );
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
                type.throw_error_at( "vehicle_function", "load_vehicle_spawn: unable to find builtin function" );
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

static void builtin_no_vehicles( map &, std::string_view )
{}

static void builtin_parkinglot( map &m, std::string_view )
{
    for( int v = 0; v < rng( 1, 4 ); v++ ) {
        tripoint_bub_ms pos_p;
        pos_p.x() = rng( 0, 1 ) * 15 + rng( 4, 5 );
        pos_p.y() = rng( 0, 4 ) * 4 + rng( 2, 4 );
        pos_p.z() = m.get_abs_sub().z();

        if( !m.veh_at( pos_p ) ) {
            units::angle facing;
            if( one_in( 10 ) ) {
                facing = random_direction();
            } else {
                facing = one_in( 2 ) ? 0_degrees : 180_degrees;
            }
            m.add_vehicle( VehicleGroup_parkinglot->pick(), pos_p, facing, -1, -1 );
        }
    }
}

} // namespace VehicleSpawnFunction

VehicleSpawn::FunctionMap VehicleSpawn::builtin_functions = {
    { "no_vehicles", VehicleSpawnFunction::builtin_no_vehicles },
    { "parkinglot", VehicleSpawnFunction::builtin_parkinglot }
};
