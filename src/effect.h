#ifndef _EFFECT_H_
#define _EFFECT_H_

#include "pldata.h"
#include "creature.h"
#include "json.h"


class effect_type;
class Creature;

extern std::map<std::string, effect_type> effect_types;

class effect_type
{
        friend void load_effect_type(JsonObject &jo);
        friend class effect;
    public:
        effect_type();
        effect_type(const effect_type &rhs);

        efftype_id id;

        std::string get_name(int intensity = 0);
        std::string get_desc(int intensity = 0);
        bool use_name_intensities();
        bool use_desc_intensities();

        std::string get_apply_message();
        std::string get_apply_memorial_log();
        std::string get_remove_message();
        std::string get_remove_memorial_log();

        int get_max_intensity();
        int get_decay_rate();
        int get_additive();
        int main_parts();

        /** True if health alters disease duration */
        bool health_mods();

    protected:
        bool permanent;
        int max_intensity;
        int decay_rate;
        int additive;
        bool main_parts_only;
        bool health_affects;

        std::vector<std::string> name;
        std::vector<std::string> desc;

        std::string apply_message;
        std::string apply_memorial_log;
        std::string remove_message;
        std::string remove_memorial_log;
};

class effect : public JsonSerializer, public JsonDeserializer
{
    public:
        effect();
        effect(effect_type *eff_type, int dur, bool perm = false, int nintensity = 1,
                body_part target = num_bp, int nside = -1);
        effect(const effect &rhs);
        effect &operator=(const effect &rhs);

        std::string disp_name();
        std::string disp_desc();

        effect_type *get_effect_type();
        void do_effect(Creature &t); // applies the disease's effects

        int get_duration();
        void set_duration(int dur);
        void mod_duration(int dur);

        bool is_permanent();
        void pause_effect();
        void unpause_effect();

        int get_intensity();
        int get_max_intensity();

        void decay(int health_val);

        void set_intensity(int nintensity);
        void mod_intensity(int nintensity);

        body_part get_bp();
        int get_side();

        efftype_id get_id() {
            return eff_type->id;
        }

        using JsonSerializer::serialize;
        void serialize(JsonOut &json) const {
            json.start_object();
            json.member("eff_type", eff_type != NULL ? eff_type->id : "");
            json.member("duration", duration);
            json.member("permanent", permanent);
            json.member("intensity", intensity);
            json.member("bp", (int)bp);
            json.member("side", side);
            json.end_object();
        }
        using JsonDeserializer::deserialize;
        void deserialize(JsonIn &jsin) {
            JsonObject jo = jsin.get_object();
            eff_type = &effect_types[jo.get_string("eff_type")];
            duration = jo.get_int("duration");
            permanent = jo.get_bool("permanent");
            intensity = jo.get_int("intensity");
            bp = (body_part)jo.get_int("bp");
            side = jo.get_int("side");
        }

    protected:
        effect_type *eff_type;
        int duration;
        bool permanent;
        int intensity;
        body_part bp;
        int side;

};

void load_effect_type(JsonObject &jo);
void reset_effect_types();
#endif
