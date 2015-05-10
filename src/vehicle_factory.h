#ifndef VEHICLE_GROUP_H
#define VEHICLE_GROUP_H

#include "json.h"
#include "mapgen.h"
#include <string>
#include <memory>
#include "weighted_list.h"

typedef std::string Vehicle_tag;

/**
 * This class is used to group vehicles together into groups in much the same was as
 *  item groups work.
 */
class VehicleGroup {

    public:
        inline void add_vehicle(const std::string &type, const int &probability) {
            vehicles.add(type, probability);
        }

        inline const std::string* pick() const {
            return vehicles.pick();
        }

    private:
        weighted_int_list<std::string> vehicles;
};

/**
 * The location and facing data needed to place a vehicle onto the map.
 */
 struct Vehicle_Facings {
    Vehicle_Facings(JsonObject &jo, std::string key);

    int pick() const;
    std::vector<int> values;
};

struct Vehicle_Location {
    Vehicle_Location(const jmapgen_int &x, const jmapgen_int &y, const Vehicle_Facings &facings) : x(x), y(y), facings(facings) {}
    int pick_facing() const;

    jmapgen_int x;
    jmapgen_int y;
    Vehicle_Facings facings;
};

/**
 * A list of vehicle locations which are valid for spawning new vehicles.
 */
struct Vehicle_Placement {
    void add(const jmapgen_int &x, const jmapgen_int &y, const Vehicle_Facings &facings);
    const Vehicle_Location* pick() const;

    typedef std::vector<Vehicle_Location> vehicle_locations;
    vehicle_locations locations;
};

/**
 * These classes are used to wrap around a set of vehicle spawning functions. There are
 * currently two methods that can be used to spawn a vehicle. The first is by using a
 * c++ function. The second is by using data loaded from json.
 */

class Vehicle_Function {
public:
    virtual ~Vehicle_Function() { }
    virtual void apply(map* m, std::string terrainid) = 0;
};

typedef void (*vehicle_gen_pointer)(map *m, std::string terrainid);
class Vehicle_Function_builtin : public Vehicle_Function {
public:
    Vehicle_Function_builtin(const vehicle_gen_pointer &func) : func(func) {}
    ~Vehicle_Function_builtin() { }

    void apply(map* m, std::string terrainid) override;

private:
    vehicle_gen_pointer func;
};

class Vehicle_Function_json : public Vehicle_Function {
public:
    Vehicle_Function_json(JsonObject &jo);
    ~Vehicle_Function_json() { }

    void apply(map* m, std::string terrain_name) override;

    std::string vehicle;
    jmapgen_int number;
    int fuel;
    int status;

    std::string placement;
    std::unique_ptr<Vehicle_Location> location;
};

/**
 * This class handles a weighted list of different spawn functions, allowing a single
 * vehicle_spawn to have multiple possibilities.
 */
struct Vehicle_SpawnType {
    std::string description;
    std::shared_ptr<Vehicle_Function> func;
};

class Vehicle_Spawn {
public:
    void add(const double &weight, const std::string &description, const std::shared_ptr<Vehicle_Function> &func);
    const Vehicle_SpawnType* pick() const;

private:
    weighted_float_list<Vehicle_SpawnType> types;
};

/**
 * Vehicle placment management class.
 *
 * You usually use the single global instance @ref vehicle_controller.
 */
class Vehicle_Factory {

    public:
        /**
         * Callback for the init system (@ref DynamicDataLoader), loads a vehicle group definitions.
         * @param jsobj The json object to load from.
         * @throw std::string if the json object contains invalid data.
         */
        void load_vehicle_group(JsonObject &jo);

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

        const Vehicle_Placement* get_placement(const std::string &id) const;
        void vehicle_spawn(std::string spawn_id, map* m, std::string terrain_name);

        void add_vehicle(map* m, const std::string &vehicle_id, const int x, const int y, const int facing, const int fuel, const int status, const bool mergewrecks = true);

    private:
        // builtin functions
        static void builtin_no_vehicles(map* m, std::string terrainid);
        static void builtin_jackknifed_semi(map* m, std::string terrainid);

        typedef std::map<Vehicle_tag, VehicleGroup> GroupMap;
        GroupMap groups;

        typedef std::map<std::string, Vehicle_Placement> PlacementMap;
        PlacementMap placements;

        typedef std::map<std::string, Vehicle_Spawn> VehicleSpawnsMap;
        VehicleSpawnsMap spawns;

        typedef std::map<std::string, std::shared_ptr<Vehicle_Function>> FunctionMap;
        FunctionMap builtin_functions {
            { "no_vehicles", std::make_shared<Vehicle_Function_builtin>(builtin_no_vehicles) },
            { "jack-knifed_semi", std::make_shared<Vehicle_Function_builtin>(builtin_jackknifed_semi) }
        };
};

extern std::unique_ptr<Vehicle_Factory> vehicle_controller;

#endif
