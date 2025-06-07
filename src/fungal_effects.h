#pragma once
#ifndef CATA_SRC_FUNGAL_EFFECTS_H
#define CATA_SRC_FUNGAL_EFFECTS_H

#include "coords_fwd.h"

class Creature;

class fungal_effects
{
    public:
        void marlossify( const tripoint_bub_ms &p );
        /** Makes spores at p. source is used for kill counting */
        void create_spores( const tripoint_bub_ms &p, Creature *origin = nullptr );
        void fungalize( const tripoint_bub_ms &p, Creature *origin = nullptr, double spore_chance = 0.0 );

        void spread_fungus( const tripoint_bub_ms &p );
        void spread_fungus_one_tile( const tripoint_bub_ms &p, int growth );
};

#endif // CATA_SRC_FUNGAL_EFFECTS_H
