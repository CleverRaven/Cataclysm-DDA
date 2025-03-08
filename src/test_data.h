#pragma once
#ifndef CATA_TEST_DATA_H
#define CATA_TEST_DATA_H

#include <cstdint>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "coordinates.h"
#include "point.h"
#include "type_id.h"

class Character;
class JsonObject;
enum class pocket_type : int;

struct pulp_test_data {
    std::string name;
    int expected_pulp_time;
    std::vector<itype_id> items;
    std::map<skill_id, int> skills;
    bool profs;
    mtype_id corpse;
};

struct efficiency_data {
    std::vector<int> forward;
    std::vector<int> reverse;

    void deserialize( const JsonObject &jo );
};

enum class spawn_type : int {
    none = 0,
    item_group,
    recipe,
    vehicle,
    profession,
    map
};

struct container_spawn_test_data {
    std::string given;
    item_group_id group;
    recipe_id recipe;
    vproto_id vehicle;
    profession_id profession;
    itype_id item;
    int charges = 0;
    int expected_amount;

    void deserialize( const JsonObject &jo );
};

struct pocket_mod_test_data {
    itype_id base_item;
    itype_id mod_item;
    std::map<pocket_type, std::vector<uint64_t>> expected_pockets;

    void deserialize( const JsonObject &jo );
};

struct npc_boarding_test_data {
    vproto_id veh_prototype;
    tripoint_bub_ms player_pos;
    tripoint_bub_ms npc_pos;
    tripoint_bub_ms npc_target;

    void deserialize( const JsonObject &jo );
};

// TODO: Support mounts
struct bash_test_loadout {
    int strength;
    int expected_smash_ability;
    std::vector<itype_id> worn;
    std::optional<itype_id> wielded;

    void apply( Character &guy ) const;
    void deserialize( const JsonObject &jo );
};

struct single_bash_test {
    std::string id;
    bash_test_loadout loadout;
    std::map<furn_id, std::pair<int, int>> furn_tries;
    std::map<ter_id, std::pair<int, int>> ter_tries;

    void deserialize( const JsonObject &jo );
};

struct bash_test_set {
    std::vector<furn_id> tested_furn;
    std::vector<ter_id> tested_ter;

    std::vector<single_bash_test> tests;
    void deserialize( const JsonObject &jo );
};

struct item_demographic_test_data {
    std::map<itype_id, int> item_weights;
    std::map<std::string, std::pair<int, std::map<itype_id, int>>> groups;
    std::set<std::string> tests; // NOLINT(cata-serialize)
    std::unordered_set<itype_id> ignored_items; // NOLINT(cata-serialize)

    void deserialize( const JsonObject &jo );
};

class test_data
{
    public:
        static std::set<itype_id> legacy_to_hit;
        // TODO: remove when all known bad items got fixed
        static std::set<itype_id> known_bad;
        static std::vector<pulp_test_data> pulp_test;
        static std::vector<std::regex> overmap_terrain_coverage_whitelist;
        static std::map<vproto_id, std::vector<double>> drag_data;
        static std::map<vproto_id, efficiency_data> eff_data;
        static std::map<itype_id, double> expected_dps;
        static std::map<spawn_type, std::vector<container_spawn_test_data>> container_spawn_data;
        static std::map<std::string, pocket_mod_test_data> pocket_mod_data;
        static std::map<std::string, npc_boarding_test_data> npc_boarding_data;
        static std::vector<bash_test_set> bash_tests;
        static std::map<std::string, item_demographic_test_data> item_demographics;

        static void load( const JsonObject &jo );
};

#endif // CATA_TEST_DATA_H
