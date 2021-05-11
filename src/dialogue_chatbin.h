#pragma once
#ifndef CATA_SRC_DIALOGUE_CHATBIN_H
#define CATA_SRC_DIALOGUE_CHATBIN_H

#include <iosfwd>
#include <vector>

#include "type_id.h"

class JsonIn;
class JsonOut;
class mission;

struct dialogue_chatbin {
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
     * Missions that the talker can give out. All missions in this vector should be unassigned,
     * when given out, they should be moved to @ref missions_assigned.
     */
    std::vector<mission *> missions;
    /**
     * Mission that have been assigned by this dialogue to a player character.
     */
    std::vector<mission *> missions_assigned;
    /**
     * The mission (if any) that we talk about right now. Can be null. Should be one of the
     * missions in @ref missions or @ref missions_assigned.
     */
    mission *mission_selected = nullptr;
    /**
     * The skill this dialogue offers to train.
     */
    skill_id skill = skill_id();
    /**
     * The martial art style this dialogue offers to train.
     */
    matype_id style = matype_id();
    /**
     * The spell this dialogue offers to train
     */
    spell_id dialogue_spell;
    /**
     * The proficiency this dialogue offers to train
     */
    proficiency_id proficiency;
    void store_chosen_training( const skill_id &c_skill, const matype_id &c_style,
                                const spell_id &c_spell, const proficiency_id &c_proficiency );
    void clear_training();
    std::string first_topic = "TALK_NONE";

    dialogue_chatbin() = default;

    void clear_all();
    void serialize( JsonOut &json ) const;
    void deserialize( JsonIn &jsin );
};

#endif // CATA_SRC_DIALOGUE_CHATBIN_H
