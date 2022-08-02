#pragma once
#ifndef CATA_SRC_TALKER_AVATAR_H
#define CATA_SRC_TALKER_AVATAR_H

#include <iosfwd>
#include <vector>

#include "talker_character.h"
#include "type_id.h"

class avatar;
class talker;
class translation;

/*
 * Talker wrapper class for avatar.
 */
class talker_avatar: public talker_character
{
    public:
        explicit talker_avatar( avatar *new_me );
        ~talker_avatar() override = default;

        // mandatory functions for starting a dialogue
        std::vector<std::string> get_topics( bool ) override;
        int parse_mod( const std::string &attribute, int factor ) const override;
        int trial_chance_mod( const std::string &trial_type ) const override;

        // for training NPCs (reverse of talker_npc)
        std::vector<skill_id> skills_offered_to( const talker &student ) const override;

        // inventory and such
        bool buy_monster( talker &seller, const mtype_id &mtype, int cost,
                          int count, bool pacified, const translation &name ) override;
};
#endif // CATA_SRC_TALKER_AVATAR_H

