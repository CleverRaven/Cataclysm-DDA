#ifndef VEHICLE_GROUP_H
#define VEHICLE_GROUP_H

#include "json.h"
#include "mapgen.h"
#include <string>
#include <memory>
#include "weighted_list.h"

/**
 * This class is used to group vehicles together into groups in much the same way as
 *  item groups work.
 */
class VehicleGroup {
public:
    VehicleGroup() : vehicles() {}

    void add_vehicle(const vproto_id &type, const int &probability) {
        vehicles.add(type, probability);
    }

    const vproto_id &pick() const {
        return *(vehicles.pick());
    }

    static void load( JsonObject &jo );

private:
    weighted_int_list<vproto_id> vehicles;
};

using vgroup_id = string_id<VehicleGroup>;

/**
 * The location and facing data needed to place a vehicle onto the map.
 */
 struct VehicleFacings {
    VehicleFacings(JsonObject &jo, const std::string &key);

    int pick() const {
        return values[rng(0, values.size()-1)];
    }

    std::vector<int> values;
};

struct VehicleLocation {
    VehicleLocation(const jmapgen_int &x, const jmapgen_int &y, const VehicleFacings &facings)
        : x(x), y(y), facings(facings) {}

    int pick_facing() const {
        return facings.pick();
    }

    point pick_point() const {
        return point(x.get(), y.get());
    }

    jmapgen_int x;
    jmapgen_int y;
    VehicleFacings facings;
};

/**
 * A list of vehicle locations which are valid for spawning new vehicles.
 */
struct VehiclePlacement {
    void add(const jmapgen_int &x, const jmapgen_int &y, const VehicleFacings &facings) {
        locations.emplace_back(x, y, facings);
    }

    const VehicleLocation* pick() const;

    typedef std::vector<VehicleLocation> LocationMap;
    LocationMap locations;
};

/**
 * These classes are used to wrap around a set of vehicle spawning functions. There are
 * currently two methods that can be used to spawn a vehicle. The first is by using a
 * c++ function. The second is by using data loaded from json.
 */

class VehicleFunction {
public:
    virtual ~VehicleFunction() { }
    virtual void apply(map& m, const std::string &terrainid) const = 0;
};

typedef void (*vehicle_gen_pointer)(map &m, const std::string &terrainid);

class VehicleFunction_builtin : public VehicleFunction {
public:
    VehicleFunction_builtin(const vehicle_gen_pointer &func) : func(func) {}
    ~VehicleFunction_builtin() { }

    void apply(map& m, const std::string &terrainid) const override {
        func(m, terrainid);
    }

private:
    vehicle_gen_pointer func;
};

class VehicleFunction_json : public VehicleFunction {
public:
    VehicleFunction_json(JsonObject &jo);
    ~VehicleFunction_json() { }

    void apply(map& m, const std::string &terrain_name) const override;

private:
    vgroup_id vehicle;
    jmapgen_int number;
    int fuel;
    int status;

    std::string placement;
    std::unique_ptr<VehicleLocation> location;
};

/**
 * This class handles a weighted list of different spawn functions, allowing a single
 * vehicle_spawn to have multiple possibilities.
 */
class VehicleSpawn {
public:
    void add(const double &weight, const std::shared_ptr<VehicleFunction> &func) {
        types.add(func, weight);
    }

    const VehicleFunction* pick() const {
        return types.pick()->get();
    }

private:
    weighted_float_list<std::shared_ptr<VehicleFunction>> types;
};

/**
 * Vehicle placment management class.
 *
 * You usually use the single global instance @ref vehicle_controller.
 */
class VehicleFactory {
public:

    /**
     * This will invoke the given vehicle spawn on the map.
     * @param spawn_id The id of the vehicle spawn to invoke.
     * @param m The map on which to add the vehicle.
     */
    void vehicle_spawn(map& m, const std::string &spawn_id, const std::string &terrain_name);

    /**
     * This will randomly select one of the locations from a vehicle placement
     * @param placement_id The id of the placement from which to select a location.
     * @return either null, if there is no placement with that id or that placement has no locations. Else one of the locations of that placement chosen at random.
     */
    const VehicleLocation* pick_location(const std::string &placement_id) const;

    /**
     * Callback for the init system (@ref DynamicDataLoader), loads a vehicle placement definitions.
     * @param jsobj The json object to load from.
     * @throw std::string if the json object contains invalid data.
     */
    void load_vehicle_placement(JsonObject &jo);

    /**
     * Callback for the init system (@ref DynamicDataLoader), loads a vehicle spawn definitions.
     * @param jsobj The json object to load from.
     * @throw std::string if the json object contains invalid data.
     */
    void load_vehicle_spawn(JsonObject &jo);

private:
    // builtin functions
    static void builtin_no_vehicles(map& m, const std::string &terrainid);
    static void builtin_jackknifed_semi(map& m, const std::string &terrainid);
    static void builtin_pileup(map& m, const std::string &terrainid);
    static void builtin_policepileup(map& m, const std::string &terrainid);

    typedef std::unordered_map<std::string, VehiclePlacement> PlacementMap;
    PlacementMap placements;

    typedef std::unordered_map<std::string, VehicleSpawn> VehicleSpawnsMap;
    VehicleSpawnsMap spawns;

    typedef std::unordered_map<std::string, vehicle_gen_pointer> FunctionMap;
    FunctionMap builtin_functions {
        { "no_vehicles", builtin_no_vehicles },
        { "jack-knifed_semi", builtin_jackknifed_semi },
        { "vehicle_pileup", builtin_pileup },
        { "policecar_pileup", builtin_policepileup }
    };
};

extern std::unique_ptr<VehicleFactory> vehicle_controller;
extern std::unordered_map<vgroup_id, VehicleGroup> vgroups;

#endif
