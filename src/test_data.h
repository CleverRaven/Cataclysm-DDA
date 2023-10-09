#pragma once
#ifndef CATA_TEST_DATA_H
#define CATA_TEST_DATA_H

#include <map>
#include <set>
#include <vector>

#include "point.h"
#include "type_id.h"

class JsonObject;

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

struct npc_boarding_test_data {
    vproto_id veh_prototype;
    tripoint player_pos;
    tripoint npc_pos;
    tripoint npc_target;

    void deserialize( const JsonObject &jo );
};

class test_data
{
    public:
        // todo: remove when all known bad items got fixed
        static std::set<itype_id> known_bad;
        static std::map<vproto_id, std::vector<double>> drag_data;
        static std::map<vproto_id, efficiency_data> eff_data;
        static std::map<itype_id, double> expected_dps;
        static std::map<spawn_type, std::vector<container_spawn_test_data>> container_spawn_data;
        static std::map<std::string, npc_boarding_test_data> npc_boarding_data;

        static void load( const JsonObject &jo );
};

#endif // CATA_TEST_DATA_H
