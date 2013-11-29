#ifndef _EFFECT_H_
#define _EFFECT_H_

#include "creature.h"

class game;
class creature;

class effect_type
{
    public:
        effect_type();
        effect_type(const effect_type & rhs);


        /*
        bool is_permanent();

        int get_max_intensity();
        */

    protected:
        int max_intensity;
        bool permanent;
};

class effect
{
    public:
        effect();
        effect(effect_type* eff_type, int dur);
        effect(const effect & rhs);

        effect_type* get_effect_type();
        void do_effects(game* g, creature& t); // applies the disease's effects

    protected:
        effect_type* eff_type;
        int duration;
        int intensity;
};

#endif
