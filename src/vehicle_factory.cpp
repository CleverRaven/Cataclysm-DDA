#include "vehicle_factory.h"
#include "debug.h"

std::unique_ptr<Vehicle_Factory> vehicle_controller( new Vehicle_Factory() );

void Vehicle_Group::add_vehicle_entry(const std::string &type, const int &probability)
{
    vehicles.add({type}, probability);
}

const Vehicle_group_choice* Vehicle_Group::pick() const
{
    return vehicles.pick();
}

void Vehicle_Placement::add(const jmapgen_int &x, const jmapgen_int &y, const vehicle_facings &facings)
{
    locations.emplace_back(x, y, facings);
}

const Vehicle_Location* Vehicle_Placement::pick() const
{
    return &(locations[rng(0, locations.size()-1)]);
}

void Vehicle_Factory::load_vehicle_group(JsonObject &jo)
{
    const Vehicle_tag group_id = jo.get_string("id");
    JsonArray vehicles = jo.get_array("vehicles");

    while (vehicles.has_more()) {
        JsonArray pair = vehicles.next_array();
        Vehicle_Group &group = groups[group_id];
        group.add_vehicle_entry(pair.get_string(0), pair.get_int(1));
    }
}

void Vehicle_Factory::load_vehicle_placement(JsonObject &jo)
{
    const std::string placement_id = jo.get_string("id");
    Vehicle_Placement &placement = placements[placement_id];

    JsonArray locations = jo.get_array("locations");
    while (locations.has_more()) {
        JsonObject jloc = locations.next_object();

        vehicle_facings facings;
        if(jloc.has_array("facing")) {
            JsonArray jpos = jloc.get_array("facing");

            while (jpos.has_more()) {
                facings.push_back(jpos.next_int());
            }
        }
        else {
            facings.push_back(jloc.get_int("facing"));
        }

        placement.add(jmapgen_int(jloc, "x"), jmapgen_int(jloc, "y"), facings);
    }
}
