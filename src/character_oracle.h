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
    private:
        const Character *subject;
};

} //namespace behavior
#endif // CATA_SRC_CHARACTER_ORACLE_H
