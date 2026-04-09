#pragma once
#ifndef CATA_SRC_CHARACTER_ORACLE_H
#define CATA_SRC_CHARACTER_ORACLE_H

#include <string_view>

#include "behavior_oracle.h"

class Character;

namespace behavior
{
enum class status_t : char;

class character_oracle_t : public oracle_t
{
    public:
        explicit character_oracle_t( const Character *subject ) {
            this->subject = subject;
        }
        /**
         * Predicates used by AI to determine goals.
         */
        status_t needs_warmth_badly( std::string_view ) const;
        status_t needs_water_badly( std::string_view ) const;
        status_t needs_food_badly( std::string_view ) const;
        status_t can_wear_warmer_clothes( std::string_view ) const;
        status_t can_make_fire( std::string_view ) const;
        status_t can_take_shelter( std::string_view ) const;
        status_t has_water( std::string_view ) const;
        status_t has_food( std::string_view ) const;
        status_t can_obtain_food( std::string_view ) const;
        status_t can_obtain_water( std::string_view ) const;
        status_t can_obtain_warmth( std::string_view ) const;
        status_t needs_sleep_badly( std::string_view ) const;
        status_t can_sleep( std::string_view ) const;
        /**
         * Top-level decision predicates (NPC-specific via dynamic_cast).
         */
        status_t in_danger( std::string_view ) const;
        status_t should_flee( std::string_view ) const;
        status_t has_target( std::string_view ) const;
        status_t has_sound_alerts( std::string_view ) const;
        status_t displaced_from_post( std::string_view ) const;
        status_t on_shift( std::string_view ) const;
        status_t npc_is_following( std::string_view ) const;
        status_t npc_has_goto_order( std::string_view ) const;
        status_t has_camp_job( std::string_view ) const;
        status_t is_away_from_camp( std::string_view ) const;
        status_t is_camp_idle( std::string_view ) const;
        /**
         * Score predicates for utility strategy (return 0-1 urgency).
         */
        float thirst_urgency( std::string_view ) const;
        float hunger_urgency( std::string_view ) const;
        float warmth_urgency( std::string_view ) const;
        float sleepiness_urgency( std::string_view ) const;
        float duty_urgency( std::string_view ) const;
        float npc_following_urgency( std::string_view ) const;
        float npc_goto_order_urgency( std::string_view ) const;
        float camp_work_urgency( std::string_view ) const;
        float return_to_camp_urgency( std::string_view ) const;
        float free_time_urgency( std::string_view ) const;
    private:
        const Character *subject;
};

} //namespace behavior
#endif // CATA_SRC_CHARACTER_ORACLE_H
