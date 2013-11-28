#ifndef _CREATURE_H_
#define _CREATURE_H_

#include "pldata.h"
#include "skill.h"
#include "bionics.h"
#include "json.h"
#include <string>


class game;

class creature
{
    public:
        // TODO: fill these in
        creature();
        creature(const creature & rhs);

        virtual std::string disp_name() = 0; // displayname for creature
        virtual std::string skin_name() = 0; // name of outer layer, e.g. "armor plates"

        virtual int dodge_roll(game* g) = 0;

        virtual int hit_creature(game *g, creature &t, bool allow_grab) = 0; // Returns a damage

        virtual int hit (game *g, body_part bphurt, int side,
                int dam, int cut) = 0;

        virtual bool block_hit(game *g, creature &t, body_part &bp_hit, int &side,
            int &bash_dam, int &cut_dam, int &stab_dam) = 0;

        virtual bool is_on_ground() = 0;
        virtual bool has_weapon() = 0;

        // xpos and ypos, because posx/posy are used as public variables in
        // player.cpp and therefore referenced everywhere
        virtual int xpos() = 0;
        virtual int ypos() = 0;

        // these also differ between player and monster (player takes
        // body_part) but we will use this for now
        virtual int armor_bash() = 0;
        virtual int armor_cut() = 0;
};

#endif



