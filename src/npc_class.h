#ifndef NPC_CLASS_H
#define NPC_CLASS_H

#include <vector>
#include <map>
#include <array>

#include "string_id.h"

class npc_class;
using npc_class_id = string_id<npc_class>;
class JsonObject;

class npc_class
{
    private:
        std::string name;

    public:
        npc_class_id id;
        bool was_loaded;

        npc_class();

        const std::string &get_name() const;

        void load( JsonObject &jo );

        static const npc_class_id &from_legacy_int( int i );

        static const npc_class_id &random_common();

        static void load_npc_class( JsonObject &jo );

        static void reset_npc_classes();

        static void check_consistency();
};

// @todo Get rid of that
extern npc_class_id NC_NONE;
extern npc_class_id NC_EVAC_SHOPKEEP;
extern npc_class_id NC_SHOPKEEP;
extern npc_class_id NC_HACKER;
extern npc_class_id NC_DOCTOR;
extern npc_class_id NC_TRADER;
extern npc_class_id NC_NINJA;
extern npc_class_id NC_COWBOY;
extern npc_class_id NC_SCIENTIST;
extern npc_class_id NC_BOUNTY_HUNTER;
extern npc_class_id NC_THUG;
extern npc_class_id NC_SCAVENGER;
extern npc_class_id NC_ARSONIST;
extern npc_class_id NC_HUNTER;
extern npc_class_id NC_SOLDIER;
extern npc_class_id NC_BARTENDER;
extern npc_class_id NC_JUNK_SHOPKEEP;

#endif
