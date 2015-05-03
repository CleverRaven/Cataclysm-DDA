#include "vehicle_factory.h"
#include "debug.h"

std::unique_ptr<Vehicle_Factory> vehicle_controller( new Vehicle_Factory() );

void Vehicle_Group::add_vehicle_entry(const std::string &type, const int &probability)
{
    m_vehicles.add({type}, probability);
}

const Vehicle_spawn_data* Vehicle_Group::pick() const
{
    return m_vehicles.pick();
}

void Vehicle_Factory::load_vehicle_group(JsonObject &jo)
{
    const Vehicle_tag group_id = jo.get_string("id");
    JsonArray vehicles = jo.get_array("vehicles");

    while (vehicles.has_more()) {
        JsonArray pair = vehicles.next_array();
        Vehicle_Group &group = m_groups[group_id];
        group.add_vehicle_entry(pair.get_string(0), pair.get_int(1));
    }
}
