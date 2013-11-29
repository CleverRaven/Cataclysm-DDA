#ifndef _EFFECT_H_
#define _EFFECT_H_

#include "pldata.h"
#include "creature.h"
#include "json.h"

class game;
class creature;

class effect_type
{
    public:
        effect_type();
        effect_type(const effect_type & rhs);


        efftype_id id;

        std::string apply_message;
        std::string apply_memorial_log;
        std::string remove_message;
        std::string remove_memorial_log;
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

        efftype_id get_id() {
          return eff_type->id;
        }

    protected:
        effect_type* eff_type;
        int duration;
        int intensity;
};

void load_effect_type(JsonObject& jo);

extern std::map<std::string, effect_type> effect_types;

#endif
