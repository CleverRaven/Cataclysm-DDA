#pragma once
#ifndef NPC_DESTINATIONS_CLASS_H
#define NPC_DESTINATIONS_CLASS_H

#include <vector>

#include "string_id.h"
#include "overmap.h"

class JsonObject;

class npc_destination;
using npc_destination_id = string_id<npc_destination>;

class npc_destination
{
    private:
        std::vector<string_id<oter_type_t>> terrains;

    public:
        npc_destination_id id;

        bool was_loaded = false;

        npc_destination();

        npc_destination( std::string npc_destination_id );

        std::string get_random_dest();

        void load( JsonObject &jo, const std::string &src );

        static void load_npc_destination( JsonObject &jo, const std::string &src );

        static const std::vector<npc_destination> &get_all();

        static void reset_npc_destinations();

        static void check_consistency();
};

#endif
