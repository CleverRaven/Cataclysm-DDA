#pragma once
#ifndef CATA_SRC_CHARACTER_ORACLE_H
#define CATA_SRC_CHARACTER_ORACLE_H

#include "behavior.h"
#include "behavior_oracle.h"

class Character;

namespace behavior
{

class character_oracle_t : public oracle_t
{
    public:
        character_oracle_t( const Character *subject ) {
            this->subject = subject;
        }
        /**
         * Predicates used by AI to determine goals.
         */
        status_t needs_warmth_badly() const;
        status_t needs_water_badly() const;
        status_t needs_food_badly() const;
        status_t can_wear_warmer_clothes() const;
        status_t can_make_fire() const;
        status_t can_take_shelter() const;
        status_t has_water() const;
        status_t has_food() const;
    private:
        const Character *subject;
};

} //namespace behavior
#endif // CATA_SRC_CHARACTER_ORACLE_H
