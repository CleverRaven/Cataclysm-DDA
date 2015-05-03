#ifndef VEHICLE_GROUP_H
#define VEHICLE_GROUP_H

#include "json.h"
#include <string>
#include <memory>
#include "weighted_list.h"

typedef std::string Vehicle_tag;

struct Vehicle_spawn_data
{
    Vehicle_spawn_data(const std::string &type) : type(type) {}

    std::string type; //vehicle type id
};

class Vehicle_Group {

    public:
        void add_vehicle_entry(const std::string &type, const int &probability);
        const Vehicle_spawn_data* pick();

    private:
        weighted_int_list<Vehicle_spawn_data> m_vehicles;
};

class Vehicle_Factory {

    public:
        /**
         * Callback for the init system (@ref DynamicDataLoader), loads a vehicle group definitions.
         * @param jsobj The json object to load from.
         * @throw std::string if the json object contains invalid data.
         */
        void load_vehicle_group(JsonObject &jo);

    private:
        typedef std::map<Vehicle_tag, Vehicle_Group> GroupMap;
        GroupMap m_groups;

};

extern std::unique_ptr<Vehicle_Factory> vehicle_controller;

#endif
