#ifndef MATTACK_ACTORS_H
#define MATTACK_ACTORS_H

#include "mtype.h"

class JsonObject;
class monster;

class leap_actor : public mattack_actor
{
    public:
        float max_range;
        // Jump has to be at least this tiles long
        float min_range;
        // Don't leap without a hostile target creature
        bool allow_no_target;
        int move_cost;
        // Range below which we don't consider jumping at all
        float min_consider_range;
        // Don't jump if distance to target is more than this
        float max_consider_range;

        leap_actor() { }
        ~leap_actor() { }

        void load( JsonObject &jo );
        bool call( monster & ) const override;
        virtual mattack_actor *clone() const override;
};

class bite_actor : public mattack_actor
{
    public:
        // Maximum damage (and possible tags) from the attack
        damage_instance damage_max_instance;
        // Minimum multiplier on damage above (rolled per attack)
        float min_mul;
        // Maximum multiplier on damage above (also per attack)
        float max_mul;
        // Cost in moves (for attacker)
        int move_cost;
        // If set, the attack will use a different accuracy from mon's
        // regular melee attack.
        int accuracy;
        // one_in( this - damage dealt ) chance of getting infected
        // ie. the higher is this, the lower chance of infection
        int no_infection_chance;

        bite_actor();
        ~bite_actor() { }

        void load( JsonObject &jo );
        bool call( monster & ) const override;
        virtual mattack_actor *clone() const override;
};

#endif
