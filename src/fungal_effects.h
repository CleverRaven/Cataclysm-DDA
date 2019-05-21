#pragma once
#ifndef FUNGAL_EFFECTS_H
#define FUNGAL_EFFECTS_H

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

        bool marlossify( const tripoint &p );
        /** Makes spores at p. source is used for kill counting */
        void create_spores( const tripoint &p, Creature *origin = nullptr );
        void fungalize( const tripoint &p, Creature *origin = nullptr, double spore_chance = 0.0 );

        bool spread_fungus( const tripoint &p );
};

#endif
