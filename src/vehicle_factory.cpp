#include "vehicle_factory.h"
#include "debug.h"

std::unique_ptr<Vehicle_Factory> vehicle_controller( new Vehicle_Factory() );

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

void Vehicle_Function_builtin::apply(map* m, std::string terrainid)
{
    func(m, terrainid);
}

Vehicle_Function_json::Vehicle_Function_json(JsonObject &jo)
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
        std::unique_ptr<VehicleLocation> ploc(new VehicleLocation(jmapgen_int(jo, "x"), jmapgen_int(jo, "y"), facings));
        location = std::move(ploc);
    }
}

void Vehicle_Function_json::apply(map* m, std::string terrain_name)
{
    if(! location) {
        size_t replace = placement.find("%t");
        const VehicleLocation &loc = vehicle_controller->get_placement(
            replace >= 0 ? placement.substr(0,replace) + terrain_name + placement.substr(replace+2) : placement)->pick();

        vehicle_controller->add_vehicle(m, vehicle, loc.x.get(), loc.y.get(), loc.pick_facing(), fuel, status);
    }
    else {
        vehicle_controller->add_vehicle(m, vehicle, location->x.get(), location->y.get(), location->pick_facing(), fuel, status);
    }
}

void Vehicle_Spawn::add(const double &weight, const std::string &description, const std::shared_ptr<Vehicle_Function> &func)
{
    types.add({description, func}, weight);
}

const Vehicle_SpawnType* Vehicle_Spawn::pick() const
{
    return types.pick();
}

void Vehicle_Factory::load_vehicle_group(JsonObject &jo)
{
    const Vehicle_tag group_id = jo.get_string("id");
    JsonArray vehicles = jo.get_array("vehicles");

    while (vehicles.has_more()) {
        JsonArray pair = vehicles.next_array();
        groups[group_id].add_vehicle(pair.get_string(0), pair.get_int(1));
    }
}

void Vehicle_Factory::load_vehicle_placement(JsonObject &jo)
{
    const std::string placement_id = jo.get_string("id");
    VehiclePlacement &placement = placements[placement_id];

    JsonArray locations = jo.get_array("locations");
    while (locations.has_more()) {
        JsonObject jloc = locations.next_object();

        placement.add(jmapgen_int(jloc, "x"), jmapgen_int(jloc, "y"), VehicleFacings(jloc, "facing"));
    }
}

void Vehicle_Factory::load_vehicle_spawn(JsonObject &jo)
{
    const std::string spawn_id = jo.get_string("id");
    Vehicle_Spawn spawn;

    JsonArray types = jo.get_array("spawn_types");

    while (types.has_more()) {
        JsonObject type = types.next_object();
        std::shared_ptr<Vehicle_Function> func;

        if(type.has_object("vehicle_json")) {
            JsonObject vjo = type.get_object("vehicle_json");
            func = std::make_shared<Vehicle_Function_json>(vjo);
        }
        else if(type.has_string("vehicle_function")) {
            func = builtin_functions[type.get_string("vehicle_function")];
            if(! func) {
                debugmsg("load_vehicle_spawn: unable to find builtin function %s", type.get_string("vehicle_function").c_str());
                continue;
            }
        }
        else {
            debugmsg("load_vehicle_spawn: missing required vehicle_json (object) or vehicle_function (string).");
            continue;
        }

        spawn.add(type.get_float("weight"), type.get_string("description"), func);
    }

    spawns[spawn_id] = spawn;
}

const VehiclePlacement* Vehicle_Factory::get_placement(const std::string &id) const
{
    if(placements.count(id) == 0) {
        debugmsg("vehicle_factory unknown placement %s", id.c_str());
        return NULL;
    }

    return &(placements.find(id)->second);
}

void Vehicle_Factory::vehicle_spawn(std::string spawn_id, map* m, std::string terrain_name)
{
    spawns[spawn_id].pick()->func->apply(m, terrain_name);
}

void Vehicle_Factory::add_vehicle(map* m, const std::string &vehicle_id, const int x, const int y, const int facing, const int fuel, const int status, const bool mergewrecks)
{
    m->add_vehicle(groups.count(vehicle_id) > 0 ? *(groups[vehicle_id].pick()) : vehicle_id,
        x, y, facing, fuel, status, mergewrecks);
}

void Vehicle_Factory::builtin_no_vehicles(map*, std::string)
{}

void Vehicle_Factory::builtin_jackknifed_semi(map* m, std::string terrainid)
{
    const VehiclePlacement* placement = vehicle_controller->get_placement(terrainid+"_semi");
    if(! placement) {
        debugmsg("builtin_jackknifed_semi cannot find placement %s", (terrainid+"_semi").c_str());
        return;
    }

    const VehicleLocation &location = placement->pick();

    int facing = location.pick_facing();
    int semi_x = location.x.get();
    int semi_y = location.y.get();
    int trailer_x = 0;
    int trailer_y = 0;

    if(facing == 0) {
        trailer_x = semi_x + 4;
        trailer_y = semi_y - 10;
    } else if(facing == 90) {
        trailer_x = semi_x + 12;
        trailer_y = semi_y + 1;
    } else if(facing == 180) {
        trailer_x = semi_x - 4;
        trailer_y = semi_y + 10;
    } else {
        trailer_x = semi_x - 12;
        trailer_y = semi_y - 1;
    }

    vehicle_controller->add_vehicle(m, "semi_truck", semi_x, semi_y, (facing + 135) % 360, -1, 1);
    vehicle_controller->add_vehicle(m, "truck_trailer", trailer_x, trailer_y, (facing + 90) % 360, -1, 1);
}
