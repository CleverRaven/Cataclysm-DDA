#pragma once
#ifndef CATA_SRC_FUNGAL_EFFECTS_H
#define CATA_SRC_FUNGAL_EFFECTS_H

struct tripoint;
class map;
class game;
class Creature;

class fungal_effects
{
    private:
        // Dependency injection to try to be less global
        game &gm;
        map &m;
    public:
        fungal_effects( game &g, map &mp );
        fungal_effects( const fungal_effects & ) = delete;
        fungal_effects( fungal_effects && ) = delete;

        void marlossify( const tripoint &p );
        /** Makes spores at p. source is used for kill counting */
        void create_spores( const tripoint &p, Creature *origin = nullptr );
        void fungalize( const tripoint &p, Creature *origin = nullptr, double spore_chance = 0.0 );

        void spread_fungus( const tripoint &p );
        void spread_fungus_one_tile( const tripoint &p, int growth );
};

#endif // CATA_SRC_FUNGAL_EFFECTS_H
