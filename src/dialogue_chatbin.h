#pragma once
#ifndef CATA_SRC_DIALOGUE_CHATBIN_H
#define CATA_SRC_DIALOGUE_CHATBIN_H

#include <string>
#include <vector>
#include "string_id.h"
#include "type_id.h"

class JsonIn;
class JsonOut;
class mission;

struct npc_chatbin {
    /**
     * Add a new mission to the available missions (@ref missions). For compatibility it silently
     * ignores null pointers passed to it.
     */
    void add_new_mission( mission *miss );
    /**
     * Check that assigned missions are still assigned if not move them back to the
     * unassigned vector. This is called directly before talking.
     */
    void check_missions();
    /**
     * Missions that the NPC can give out. All missions in this vector should be unassigned,
     * when given out, they should be moved to @ref missions_assigned.
     */
    std::vector<mission *> missions;
    /**
     * Mission that have been assigned by this NPC to a player character.
     */
    std::vector<mission *> missions_assigned;
    /**
     * The mission (if any) that we talk about right now. Can be null. Should be one of the
     * missions in @ref missions or @ref missions_assigned.
     */
    mission *mission_selected = nullptr;
    /**
     * The skill this NPC offers to train.
     */
    skill_id skill = skill_id::NULL_ID();
    /**
     * The martial art style this NPC offers to train.
     */
    matype_id style;
    /**
     * The spell this NPC offers to train
     */
    spell_id dialogue_spell;
    std::string first_topic = "TALK_NONE";

    npc_chatbin() = default;

    void serialize( JsonOut &json ) const;
    void deserialize( JsonIn &jsin );
};

#endif // CATA_SRC_DIALOGUE_CHATBIN_H
