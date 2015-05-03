#ifndef VEHICLE_GROUP_H
#define VEHICLE_GROUP_H

#include "json.h"
#include "mapgen.h"
#include <string>
#include <memory>
#include "weighted_list.h"

typedef std::string Vehicle_tag;

struct Vehicle_spawn_data {
    Vehicle_spawn_data(const std::string &type) : type(type) {}

    std::string type; //vehicle type id
};

class Vehicle_Group {

    public:
        void add_vehicle_entry(const std::string &type, const int &probability);
        const Vehicle_spawn_data* pick() const;

    private:
        weighted_int_list<Vehicle_spawn_data> vehicles;
};

typedef std::vector<int> vehicle_facings;
struct Vehicle_Location {
    Vehicle_Location(const jmapgen_int &x, const jmapgen_int &y, const vehicle_facings &facings) : x(x), y(y), facings(facings) {}

    jmapgen_int x;
    jmapgen_int y;
    vehicle_facings facings;
};

struct Vehicle_Placement {
    void add(const jmapgen_int &x, const jmapgen_int &y, const vehicle_facings &facings);
    const Vehicle_Location* pick() const;

    typedef std::vector<Vehicle_Location> vehicle_locations;
    vehicle_locations locations;
};

class Vehicle_Factory {

    public:
        /**
         * Callback for the init system (@ref DynamicDataLoader), loads a vehicle group definitions.
         * @param jsobj The json object to load from.
         * @throw std::string if the json object contains invalid data.
         */
        void load_vehicle_group(JsonObject &jo);

        /**
         * Callback for the init system (@ref DynamicDataLoader), loads a vehicle group definitions.
         * @param jsobj The json object to load from.
         * @throw std::string if the json object contains invalid data.
         */
        void load_vehicle_placement(JsonObject &jo);

    private:
        typedef std::map<Vehicle_tag, Vehicle_Group> GroupMap;
        GroupMap groups;

        typedef std::map<std::string, Vehicle_Placement> PlacementMap;
        PlacementMap placements;
};

extern std::unique_ptr<Vehicle_Factory> vehicle_controller;

#endif
