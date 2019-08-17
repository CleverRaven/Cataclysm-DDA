#pragma once
#ifndef VEHICLE_GROUP_H
#define VEHICLE_GROUP_H

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

#include "mapgen.h"
#include "optional.h"
#include "rng.h"
#include "string_id.h"
#include "type_id.h"
#include "weighted_list.h"

class JsonObject;
class map;
class VehicleSpawn;
class VehicleGroup;

using vspawn_id = string_id<VehicleSpawn>;
struct point;

extern std::unordered_map<vgroup_id, VehicleGroup> vgroups;

/**
 * This class is used to group vehicles together into groups in much the same way as
 *  item groups work.
 */
class VehicleGroup
{
    public:
        VehicleGroup() {}

        void add_vehicle( const vproto_id &type, const int &probability ) {
            vehicles.add( type, probability );
        }

        const vproto_id &pick() const {
            return *vehicles.pick();
        }

        static void load( JsonObject &jo );
        static void reset();

    private:
        weighted_int_list<vproto_id> vehicles;
};

/**
 * The location and facing data needed to place a vehicle onto the map.
 */
struct VehicleFacings {
    VehicleFacings( JsonObject &jo, const std::string &key );

    int pick() const {
        return random_entry( values );
    }

    std::vector<int> values;
};

struct VehicleLocation {
    VehicleLocation( const jmapgen_int &x, const jmapgen_int &y, const VehicleFacings &facings )
        : x( x ), y( y ), facings( facings ) {}

    int pick_facing() const {
        return facings.pick();
    }

    point pick_point() const;

    jmapgen_int x;
    jmapgen_int y;
    VehicleFacings facings;
};

/**
 * A list of vehicle locations which are valid for spawning new vehicles.
 */
struct VehiclePlacement {
    VehiclePlacement() {}

    void add( const jmapgen_int &x, const jmapgen_int &y, const VehicleFacings &facings ) {
        locations.emplace_back( x, y, facings );
    }

    const VehicleLocation *pick() const;
    static void load( JsonObject &jo );

    using LocationMap = std::vector<VehicleLocation>;
    LocationMap locations;
};

/**
 * These classes are used to wrap around a set of vehicle spawning functions. There are
 * currently two methods that can be used to spawn a vehicle. The first is by using a
 * c++ function. The second is by using data loaded from json.
 */

class VehicleFunction
{
    public:
        virtual ~VehicleFunction() = default;
        virtual void apply( map &m, const std::string &terrainid ) const = 0;
};

using vehicle_gen_pointer = void ( * )( map &, const std::string & );

class VehicleFunction_builtin : public VehicleFunction
{
    public:
        VehicleFunction_builtin( const vehicle_gen_pointer &func ) : func( func ) {}
        ~VehicleFunction_builtin() override = default;

        /**
         * This will invoke the vehicle spawning function on the map.
         * @param m The map on which to add the vehicle.
         * @param terrainid The name of the terrain being spawned on.
         */
        void apply( map &m, const std::string &terrainid ) const override {
            func( m, terrainid );
        }

    private:
        vehicle_gen_pointer func;
};

class VehicleFunction_json : public VehicleFunction
{
    public:
        VehicleFunction_json( JsonObject &jo );
        ~VehicleFunction_json() override = default;

        /**
         * This will invoke the vehicle spawning function on the map.
         * @param m The map on which to add the vehicle.
         * @param terrain_name The name of the terrain being spawned on. This is ignored by the json handler.
         */
        void apply( map &m, const std::string &terrain_name ) const override;

    private:
        vgroup_id vehicle;
        jmapgen_int number;
        int fuel;
        int status;

        std::string placement;
        cata::optional<VehicleLocation> location;
};

/**
 * This class handles a weighted list of different spawn functions, allowing a single
 * vehicle_spawn to have multiple possibilities.
 */
class VehicleSpawn
{
    public:
        VehicleSpawn() {}

        void add( const double &weight, const std::shared_ptr<VehicleFunction> &func ) {
            types.add( func, weight );
        }

        /**
         * This will invoke the vehicle spawn on the map.
         * @param m The map on which to add the vehicle.
         * @param terrain_name The name of the terrain being spawned on.
         */
        void apply( map &m, const std::string &terrain_name ) const;

        /**
         * A static helper function. This will invoke the supplied vehicle spawn on the map.
         * @param id The spawnid to apply
         * @param m The map on which to add the vehicle.
         * @param terrain_name The name of the terrain being spawned on.
         */
        static void apply( const vspawn_id &id, map &m, const std::string &terrain_name );

        static void load( JsonObject &jo );

    private:
        weighted_float_list<std::shared_ptr<VehicleFunction>> types;

        using FunctionMap = std::unordered_map<std::string, vehicle_gen_pointer>;
        static FunctionMap builtin_functions;
};

#endif
