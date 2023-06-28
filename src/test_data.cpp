#include "test_data.h"

#include "flexbuffer_json.h"

std::set<itype_id> test_data::known_bad;
std::map<vproto_id, std::vector<double>> test_data::drag_data;
std::map<vproto_id, efficiency_data> test_data::eff_data;
std::map<itype_id, double> test_data::expected_dps;
std::map<spawn_type, std::vector<container_spawn_test_data>> test_data::container_spawn_data;

void efficiency_data::deserialize( const JsonObject &jo )
{
    jo.read( "forward", forward );
    jo.read( "reverse", reverse );
}

void container_spawn_test_data::deserialize( const JsonObject &jo )
{
    jo.read( "given", given );
    jo.read( "expected", expected_amount );

    if( jo.has_member( "group" ) ) {
        jo.read( "group", group );
    }
    if( jo.has_member( "recipe" ) ) {
        jo.read( "recipe", recipe );
    }
    if( jo.has_member( "vehicle" ) ) {
        jo.read( "vehicle", vehicle );
    }
    if( jo.has_member( "profession" ) ) {
        jo.read( "profession", profession );
    }
    if( jo.has_member( "item" ) ) {
        jo.read( "item", item );
    }
    if( jo.has_member( "charges" ) ) {
        jo.read( "charges", charges );
    }
}

void test_data::load( const JsonObject &jo )
{
    // It's probably not necessary, but these are set up to
    // extend existing data instead of overwrite it.
    if( jo.has_array( "known_bad" ) ) {
        std::set<itype_id> new_known_bad;
        jo.read( "known_bad", new_known_bad );
        known_bad.insert( new_known_bad.begin(), new_known_bad.end() );
    }

    if( jo.has_object( "drag_data" ) ) {
        std::map<vproto_id, std::vector<double>> new_drag_data;
        jo.read( "drag_data", new_drag_data );
        drag_data.insert( new_drag_data.begin(), new_drag_data.end() );
    }

    if( jo.has_object( "efficiency_data" ) ) {
        std::map<vproto_id, efficiency_data> new_efficiency_data;
        jo.read( "efficiency_data", new_efficiency_data );
        eff_data.insert( new_efficiency_data.begin(), new_efficiency_data.end() );
    }

    if( jo.has_object( "expected_dps" ) ) {
        std::map<itype_id, double> new_expected_dps;
        jo.read( "expected_dps", new_expected_dps );
        expected_dps.insert( new_expected_dps.begin(), new_expected_dps.end() );
    }

    if( jo.has_object( "spawn_data" ) )  {
        JsonObject spawn_jo = jo.get_object( "spawn_data" );
        if( spawn_jo.has_array( "group" ) ) {
            std::vector<container_spawn_test_data> test_groups;
            spawn_jo.read( "group", test_groups );
            container_spawn_data[spawn_type::item_group].insert(
                container_spawn_data[spawn_type::item_group].end(), test_groups.begin(), test_groups.end() );
        }
        if( spawn_jo.has_array( "recipe" ) ) {
            std::vector<container_spawn_test_data> test_recipes;
            spawn_jo.read( "recipe", test_recipes );
            container_spawn_data[spawn_type::recipe].insert( container_spawn_data[spawn_type::recipe].end(),
                    test_recipes.begin(), test_recipes.end() );
        }
        if( spawn_jo.has_array( "vehicle" ) ) {
            std::vector<container_spawn_test_data> test_vehicles;
            spawn_jo.read( "vehicle", test_vehicles );
            container_spawn_data[spawn_type::vehicle].insert( container_spawn_data[spawn_type::vehicle].end(),
                    test_vehicles.begin(), test_vehicles.end() );
        }
        if( spawn_jo.has_array( "profession" ) ) {
            std::vector<container_spawn_test_data> test_professions;
            spawn_jo.read( "profession", test_professions );
            container_spawn_data[spawn_type::profession].insert(
                container_spawn_data[spawn_type::profession].end(), test_professions.begin(),
                test_professions.end() );
        }
        if( spawn_jo.has_array( "map" ) ) {
            std::vector<container_spawn_test_data> test_map;
            spawn_jo.read( "map", test_map );
            container_spawn_data[spawn_type::map].insert( container_spawn_data[spawn_type::map].end(),
                    test_map.begin(), test_map.end() );
        }
    }
}
