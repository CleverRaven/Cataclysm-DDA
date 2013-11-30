#ifndef _EFFECT_H_
#define _EFFECT_H_

#include "pldata.h"
#include "creature.h"
#include "json.h"

class game;
class creature;

class effect_type
{
    friend void load_effect_type(JsonObject& jo);
    friend class effect;
    public:
        effect_type();
        effect_type(const effect_type & rhs);


        efftype_id id;

        /*
        bool is_permanent();

        int get_max_intensity();
        */
        std::string get_name();
        std::string get_desc();

        std::string get_apply_message();
        std::string get_apply_memorial_log();
        std::string get_remove_message();
        std::string get_remove_memorial_log();

    protected:
        int max_intensity;
        bool permanent;

        std::string name;
        std::string desc;

        std::string apply_message;
        std::string apply_memorial_log;
        std::string remove_message;
        std::string remove_memorial_log;
};

class effect
{
    public:
        effect();
        effect(effect_type* eff_type, int dur);
        effect(const effect & rhs);
        effect& operator=(const effect & rhs);

        effect_type* get_effect_type();
        void do_effect(game* g, creature& t); // applies the disease's effects

        int get_duration();
        void set_duration(int dur);
        void mod_duration(int dur);

        efftype_id get_id() {
          return eff_type->id;
        }

    protected:
        effect_type* eff_type;
        int duration;
        int intensity;
};

extern std::map<std::string, effect_type> effect_types;

void load_effect_type(JsonObject& jo);
#endif
