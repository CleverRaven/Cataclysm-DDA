#pragma once
#ifndef CATA_SRC_EFFECT_SOURCE_H
#define CATA_SRC_EFFECT_SOURCE_H

#include "optional.h"
#include "cata_utility.h"
#include "character_id.h"
#include "type_id.h"

class faction;
class monster;
class Character;
class Creature;

// This class stores the source of an effect; e.g. if something
// has bleed effect on it then this can try and tell you who/what caused it
class effect_source
{
    public:
        effect_source() = default;
        explicit effect_source( const monster *mon );
        explicit effect_source( const faction *fac );
        explicit effect_source( const Character *character );
        explicit effect_source( const Creature *creature );

        // const static member provided for empty sources so
        // unassigned sources can more easily be found
        static effect_source empty();

        cata::optional<character_id> get_character_id() const;
        cata::optional<faction_id> get_faction_id() const;
        cata::optional<mfaction_id> get_mfaction_id() const;

        // This attempts to resolve the creature that caused the effect
        // Currently only supports resolving player and NPC characters
        // TODO: figure out if monsters should be resolve-able
        Creature *resolve_creature() const;

        void serialize( JsonOut & ) const;
        void deserialize( JsonIn & );

    private:
        cata::optional<character_id> character = character_id();
        cata::optional<faction_id> fac = faction_id();
        cata::optional<mfaction_id> mfac = mfaction_id();
};

#endif // CATA_SRC_EFFECT_SOURCE_H
