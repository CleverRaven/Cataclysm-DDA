#pragma once
#ifndef NPC_FAVOR_H
#define NPC_FAVOR_H

#include "string_id.h"

#include <string>

using itype_id = std::string;
class Skill;
using skill_id = string_id<Skill>;
class JsonIn;
class JsonOut;

enum npc_favor_type {
    FAVOR_NULL,
    FAVOR_GENERAL, // We owe you... a favor?
    FAVOR_CASH, // We owe cash (or goods of equivalent value)
    FAVOR_ITEM, // We owe a specific item
    FAVOR_TRAINING,// We owe skill or style training
    NUM_FAVOR_TYPES
};

struct npc_favor {
    npc_favor_type type;
    int value;
    itype_id item_id;
    skill_id skill;

    npc_favor() {
        type = FAVOR_NULL;
        value = 0;
        item_id = "null";
        skill = skill_id::NULL_ID();
    };

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
};

#endif
