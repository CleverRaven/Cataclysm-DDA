#include "test_data.h"

#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "pocket_type.h"  // IWYU pragma: keep // need full type to read from json

// Define the static class varaibles
std::set<itype_id> test_data::legacy_to_hit;
std::set<itype_id> test_data::known_bad;
std::vector<pulp_test_data> test_data::pulp_test;
std::vector<std::regex> test_data::overmap_terrain_coverage_whitelist;
std::map<vproto_id, std::vector<double>> test_data::drag_data;
std::map<vproto_id, efficiency_data> test_data::eff_data;
std::map<itype_id, double> test_data::expected_dps;
std::map<spawn_type, std::vector<container_spawn_test_data>> test_data::container_spawn_data;
std::map<std::string, pocket_mod_test_data> test_data::pocket_mod_data;
std::map<std::string, npc_boarding_test_data> test_data::npc_boarding_data;
std::vector<bash_test_set> test_data::bash_tests;
std::map<std::string, item_demographic_test_data> test_data::item_demographics;

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

void pocket_mod_test_data::deserialize( const JsonObject &jo )
{
    jo.read( "base_item", base_item );
    jo.read( "mod_item", mod_item );
    jo.read( "expected_pockets", expected_pockets );
}

void npc_boarding_test_data::deserialize( const JsonObject &jo )
{
    jo.read( "vehicle", veh_prototype );
    jo.read( "player_pos", player_pos );
    jo.read( "npc_pos", npc_pos );
    jo.read( "npc_target", npc_target );
}

void bash_test_loadout::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "strength", strength );
    mandatory( jo, false, "expected_ability", expected_smash_ability );
    optional( jo, false, "worn", worn );
    optional( jo, false, "wielded", wielded, std::nullopt );
}

void single_bash_test::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "id", id );
    mandatory( jo, false, "loadout", loadout );
    optional( jo, false, "furn_tries", furn_tries );
    optional( jo, false, "ter_tries", ter_tries );
}

void bash_test_set::deserialize( const JsonObject &jo )
{
    optional( jo, false, "furn", tested_furn );
    optional( jo, false, "ter", tested_ter );
    mandatory( jo, false, "tests", tests );
}

void item_demographic_test_data::deserialize( const JsonObject &jo )
{
    if( !jo.has_member( "type" ) ) {
        for( const JsonMember &member : jo ) {
            std::string name = member.name();
            itype_id itm_id = itype_id( name );
            int item_weight = member.get_int();
            item_weights[itm_id] = item_weight;
        }
        return;
    }
    std::string type;
    jo.read( "type", type );
    groups[type].first = jo.get_int( "weight" );
    JsonObject demo_list = jo.get_object( "items" );
    for( const JsonMember &member : demo_list ) {
        std::string name = member.name();
        itype_id itm_id = itype_id( name );
        int item_weight = member.get_int();
        item_weights[itm_id] = item_weight;
        groups[type].second[itm_id] = item_weight;
    }
}

void test_data::load( const JsonObject &jo )
{
    // It's probably not necessary, but these are set up to
    // extend existing data instead of overwrite it.
    if( jo.has_array( "legacy_to_hit" ) ) {
        std::set<itype_id> new_legacy_to_hit;
        jo.read( "legacy_to_hit", new_legacy_to_hit );
        legacy_to_hit.insert( new_legacy_to_hit.begin(), new_legacy_to_hit.end() );
    }

    if( jo.has_array( "known_bad" ) ) {
        std::set<itype_id> new_known_bad;
        jo.read( "known_bad", new_known_bad );
        known_bad.insert( new_known_bad.begin(), new_known_bad.end() );
    }

    if( jo.has_array( "pulp_testing_data" ) ) {
        for( JsonObject job : jo.get_array( "pulp_testing_data" ) ) {

            std::string name;
            int pulp_time = 0;
            std::vector<itype_id> items;
            std::map<skill_id, int> skills;
            bool profs;
            mtype_id corpse;

            job.read( "name", name );
            job.read( "pulp_time", pulp_time );
            job.read( "items", items );
            profs = job.get_bool( "proficiencies", false );
            job.read( "corpse", corpse );

            if( job.has_array( "skills" ) ) {
                for( JsonObject job_skills : job.get_array( "skills" ) ) {
                    skills.emplace( job_skills.get_string( "skill" ), job_skills.get_int( "level" ) );
                }
            }

            pulp_test.push_back( { name, pulp_time, items, skills, profs, corpse } );
        }
    }

    if( jo.has_array( "overmap_terrain_coverage_whitelist" ) ) {
        std::vector<std::string> new_overmap_terrain_coverage_whitelist;
        jo.read( "overmap_terrain_coverage_whitelist", new_overmap_terrain_coverage_whitelist );
        for( const std::string &o : new_overmap_terrain_coverage_whitelist ) {
            overmap_terrain_coverage_whitelist.emplace_back( o );
        }
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

    if( jo.has_object( "bash_test" ) ) {
        bash_test_set loaded;
        optional( jo, false, "bash_test", loaded );
        bash_tests.push_back( loaded );
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

    if( jo.has_object( "pocket_mod_data" ) ) {
        std::map<std::string, pocket_mod_test_data> new_pocket_mod_data;
        jo.read( "pocket_mod_data", new_pocket_mod_data );
        pocket_mod_data.insert( new_pocket_mod_data.begin(), new_pocket_mod_data.end() );
    }

    if( jo.has_object( "npc_boarding_data" ) ) {
        std::map<std::string, npc_boarding_test_data> new_boarding_data;
        jo.read( "npc_boarding_data", new_boarding_data );
        npc_boarding_data.insert( new_boarding_data.begin(), new_boarding_data.end() );
    }

    if( jo.has_object( "item_demographics" ) ) {
        item_demographic_test_data data;
        std::string category;
        const JsonObject demo_obj = jo.get_object( "item_demographics" );
        demo_obj.read( "category", category );
        demo_obj.read( "tests", data.tests );
        demo_obj.read( "ignored_items", data.ignored_items );
        JsonArray demo_list = demo_obj.get_array( "items" );
        while( demo_list.has_more() ) {
            demo_list.read_next( data );
        }
        // This does not support merging, so only add one instance of each category.
        item_demographics[category] = data;
    }
}
