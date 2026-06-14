#pragma once
#ifndef CATA_SRC_TALKER_AVATAR_H
#define CATA_SRC_TALKER_AVATAR_H

#include <memory>
#include <string>

#include "avatar.h"
#include "talker.h"
#include "talker_character.h"
#include "type_id.h"

/*
 * Talker wrapper class for avatar.
 */
class talker_avatar_const: public const_talker_cloner<talker_avatar_const, talker_character_const>
{
    public:
        talker_avatar_const( const talker_avatar_const & ) = default;
        talker_avatar_const( talker_avatar_const && ) = delete;
        talker_avatar_const &operator=( const talker_avatar_const & ) = default;
        talker_avatar_const &operator=( talker_avatar_const && ) = delete;
        explicit talker_avatar_const( avatar const *new_me )
            : talker_character_const{ new_me }, me_chr( new_me ) {};
        ~talker_avatar_const() override = default;

        // mandatory functions for starting a dialogue
        std::vector<std::string> get_topics( bool ) const override;
        int parse_mod( const std::string &attribute, int factor ) const override;
        int trial_chance_mod( const std::string &trial_type ) const override;
        int get_daily_calories( int, std::string const & ) const override;

    private:
        avatar const *me_chr{};
};

class talker_avatar: virtual public talker_avatar_const,
    public talker_cloner<talker_avatar, talker_character>
{
    public:
        talker_avatar( const talker_avatar & ) = default;
        talker_avatar( talker_avatar && ) = delete;
        talker_avatar &operator=( const talker_avatar & ) = default;
        talker_avatar &operator=( talker_avatar && ) = delete;
        explicit talker_avatar( avatar *new_me )
            : talker_character_const( new_me ), talker_avatar_const( new_me ),
              talker_character( new_me ), me_chr( new_me ) {};
        ~talker_avatar() override = default;

        bool buy_monster( talker &seller, const mtype_id &mtype, int cost,
                          int count, bool pacified, const translation &name ) override;

    private:
        avatar *me_chr{};
};
#endif // CATA_SRC_TALKER_AVATAR_H

