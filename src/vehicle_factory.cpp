#include "vehicle_factory.h"
#include "debug.h"
#include "translations.h"

std::unique_ptr<VehicleFactory> vehicle_controller( new VehicleFactory() );

VehicleFacings::VehicleFacings(JsonObject &jo, const std::string &key)
{
    if(jo.has_array(key)) {
        JsonArray jpos = jo.get_array(key);

        while (jpos.has_more()) {
            values.push_back(jpos.next_int());
        }
    }
    else {
        values.push_back(jo.get_int(key));
    }
}

const VehicleLocation* VehiclePlacement::pick() const {
    if(locations.size() == 0) {
        debugmsg("vehicleplacement has no locations");
        return NULL;
    }

    return &(locations[rng(0, locations.size()-1)]);
}

VehicleFunction_json::VehicleFunction_json(JsonObject &jo)
    : vehicle(jo.get_string("vehicle")),
    number(jo, "number"),
    fuel(jo.get_int("fuel")),
    status(jo.get_int("status"))
{
    if(jo.has_string("placement")) {
        placement = jo.get_string("placement");
    }
    else {
        //location = std::make_unique<Vehicle_Location>(jmapgen_int(jo, "x"), jmapgen_int(jo, "y"), facings);
        // that would be better, but it won't exist until c++14, so for now we do this:
        VehicleFacings facings(jo, "facing");
        location.reset(new VehicleLocation(jmapgen_int(jo, "x"), jmapgen_int(jo, "y"), facings));
    }
}

void VehicleFunction_json::apply(map& m, const std::string &terrain_name) const
{
    for(auto i = number.get(); i > 0; i--) {
        if(! location) {
            size_t replace = placement.find("%t");
            const VehicleLocation* loc = vehicle_controller->pick_location(
                replace != std::string::npos ? placement.substr(0,replace) + terrain_name + placement.substr(replace+2) : placement);

            if(! loc) {
                debugmsg("vehiclefunction_json: unable to get location to place vehicle.");
                return;
            }
            vehicle_controller->add_vehicle(m, vehicle, loc->pick_point(), loc->pick_facing(), fuel, status);
        }
        else {
            vehicle_controller->add_vehicle(m, vehicle, location->pick_point(), location->pick_facing(), fuel, status);
        }
    }
}

void VehicleFactory::vehicle_spawn(map& m, const std::string &spawn_id, const std::string &terrain_name)
{
    spawns[spawn_id].pick()->apply(m, terrain_name);
}

vehicle* VehicleFactory::add_vehicle(map& m, const std::string &vehicle_id, const point &p, const int facing, const int fuel, const int status, const bool mergewrecks)
{
    return m.add_vehicle(groups.count(vehicle_id) > 0 ? vproto_id(groups[vehicle_id].pick()) : vproto_id(vehicle_id),
        p.x, p.y, facing, fuel, status, mergewrecks);
}

const VehicleLocation* VehicleFactory::pick_location(const std::string &placement_id) const
{
    if(placements.count(placement_id) == 0) {
        debugmsg("vehiclefactory unknown placement %s", placement_id.c_str());
        return NULL;
    }

    return placements.find(placement_id)->second.pick();
}

void VehicleFactory::load_vehicle_group(JsonObject &jo)
{
    const Vehicle_tag group_id = jo.get_string("id");
    JsonArray vehicles = jo.get_array("vehicles");

    while (vehicles.has_more()) {
        JsonArray pair = vehicles.next_array();
        groups[group_id].add_vehicle(pair.get_string(0), pair.get_int(1));
    }
}

void VehicleFactory::load_vehicle_placement(JsonObject &jo)
{
    const std::string placement_id = jo.get_string("id");
    VehiclePlacement &placement = placements[placement_id];

    JsonArray locations = jo.get_array("locations");
    while (locations.has_more()) {
        JsonObject jloc = locations.next_object();

        placement.add(jmapgen_int(jloc, "x"), jmapgen_int(jloc, "y"), VehicleFacings(jloc, "facing"));
    }
}

void VehicleFactory::load_vehicle_spawn(JsonObject &jo)
{
    const std::string spawn_id = jo.get_string("id");
    VehicleSpawn spawn;

    JsonArray types = jo.get_array("spawn_types");

    while (types.has_more()) {
        JsonObject type = types.next_object();

        if(type.has_object("vehicle_json")) {
            JsonObject vjo = type.get_object("vehicle_json");
            spawn.add(type.get_float("weight"), std::make_shared<VehicleFunction_json>(vjo));
        }
        else if(type.has_string("vehicle_function")) {
            if(builtin_functions.count(type.get_string("vehicle_function")) == 0) {
                type.throw_error("load_vehicle_spawn: unable to find builtin function", "vehicle_function");
            }

            spawn.add(type.get_float("weight"), std::make_shared<VehicleFunction_builtin>(
                builtin_functions[type.get_string("vehicle_function")]));
        }
        else {
            type.throw_error("load_vehicle_spawn: missing required vehicle_json (object) or vehicle_function (string).");
        }
    }

    spawns[spawn_id] = spawn;
}

void VehicleFactory::builtin_no_vehicles(map&, const std::string&)
{}

void VehicleFactory::builtin_jackknifed_semi(map& m, const std::string &terrainid)
{
    const VehicleLocation* loc = vehicle_controller->pick_location(terrainid+"_semi");
    if(! loc) {
        debugmsg("builtin_jackknifed_semi unable to get location to place vehicle. placement %s", (terrainid+"_semi").c_str());
        return;
    }

    int facing = loc->pick_facing();
    point semi_p = loc->pick_point();
    point trailer_p;

    if(facing == 0) {
        trailer_p.x = semi_p.x + 4;
        trailer_p.y = semi_p.y - 10;
    } else if(facing == 90) {
        trailer_p.x = semi_p.x + 12;
        trailer_p.y = semi_p.y + 1;
    } else if(facing == 180) {
        trailer_p.x = semi_p.x - 4;
        trailer_p.y = semi_p.y + 10;
    } else {
        trailer_p.x = semi_p.x - 12;
        trailer_p.y = semi_p.y - 1;
    }

    vehicle_controller->add_vehicle(m, "semi_truck", semi_p, (facing + 135) % 360, -1, 1);
    vehicle_controller->add_vehicle(m, "truck_trailer", trailer_p, (facing + 90) % 360, -1, 1);
}

void VehicleFactory::builtin_pileup(map& m, const std::string&)
{
    vehicle *last_added_car = NULL;
    int num_cars = rng(18, 22);

    for(int i = 0; i < num_cars; i++) {
        const VehicleLocation* loc = vehicle_controller->pick_location("pileup");
        if(! loc) {
            debugmsg("builtin_pileup unable to get location to place vehicle.");
            return;
        }

        last_added_car = vehicle_controller->add_vehicle(m, "city_pileup", loc->pick_point(),
            loc->pick_facing(), -1, 1);
    }

    if (last_added_car != NULL) {
        last_added_car->name = _("pile-up");
    }
}

void VehicleFactory::builtin_policepileup(map& m, const std::string&)
{
    vehicle *last_added_car = NULL;
    int num_cars = rng(18, 22);

    for(int i = 0; i < num_cars; i++) {
        const VehicleLocation* loc = vehicle_controller->pick_location("pileup");
        if(! loc) {
            debugmsg("builtin_policepileup unable to get location to place vehicle.");
            return;
        }

        last_added_car = vehicle_controller->add_vehicle(m, "policecar", loc->pick_point(),
            loc->pick_facing(), -1, 1);
    }

    if (last_added_car != NULL) {
        last_added_car->name = _("policecar pile-up");
    }
}
