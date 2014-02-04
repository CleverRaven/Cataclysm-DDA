#ifndef _EFFECT_H_
#define _EFFECT_H_

#include "pldata.h"
#include "creature.h"
#include "json.h"


class Creature;
class effect_type;

extern std::map<std::string, effect_type> effect_types;

class effect_type
{
        friend void load_effect_type(JsonObject &jo);
        friend class effect;
    public:
        effect_type();
        effect_type(const effect_type &rhs);


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

class effect : public JsonSerializer, public JsonDeserializer
{
    public:
        effect();
        effect(effect_type *eff_type, int dur);
        effect(const effect &rhs);
        effect &operator=(const effect &rhs);

        std::string disp_name();

        effect_type *get_effect_type();
        void do_effect(Creature &t); // applies the disease's effects

        int get_duration();
        void set_duration(int dur);
        void mod_duration(int dur);

        int get_intensity();
        void set_intensity(int dur);

        efftype_id get_id() {
            return eff_type->id;
        }

        using JsonSerializer::serialize;
        void serialize(JsonOut &json) const {
            json.start_object();
            json.member("eff_type", eff_type != NULL ? eff_type->id : "");
            json.member("duration", duration);
            json.member("intensity", intensity);
            json.end_object();
        }
        using JsonDeserializer::deserialize;
        void deserialize(JsonIn &jsin) {
            JsonObject jo = jsin.get_object();
            eff_type = &effect_types[jo.get_string("eff_type")];
            duration = jo.get_int("duration");
            intensity = jo.get_int("intensity");
        }

    protected:
        effect_type *eff_type;
        int duration;
        int intensity;

};

void load_effect_type(JsonObject &jo);
void reset_effect_types();
#endif
