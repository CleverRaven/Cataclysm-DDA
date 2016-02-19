#ifndef NPC_FAVOR_H
#define NPC_FAVOR_H

#include "json.h"

enum npc_favor_type {
    FAVOR_NULL,
    FAVOR_GENERAL, // We owe you... a favor?
    FAVOR_CASH, // We owe cash (or goods of equivalent value)
    FAVOR_ITEM, // We owe a specific item
    FAVOR_TRAINING,// We owe skill or style training
    NUM_FAVOR_TYPES
};

struct npc_favor : public JsonSerializer, public JsonDeserializer {
    npc_favor_type type;
    int value;
    itype_id item_id;
    const Skill *skill;

    npc_favor() {
        type = FAVOR_NULL;
        value = 0;
        item_id = "null";
        skill = NULL;
    };

    using JsonSerializer::serialize;
    void serialize( JsonOut &jsout ) const override;
    using JsonDeserializer::deserialize;
    void deserialize( JsonIn &jsin ) override;
};

#endif
