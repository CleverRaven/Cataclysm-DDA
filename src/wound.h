#ifndef _WOUND_H_
#define _WOUND_H_

#include "pldata.h"
#include "creature.h"
#include "json.h"

class wound_effect_type;
class Creature;

extern std::map<std::string, wound_effect_type> wound_effect_types;

struct wound_effect_info {
    efftype_id effect_placed;
    bool targeted_effect;
    int effect_duration;
    bool effect_perm;
    int effect_intensity;
    float bleed;

    effect_morph_info() : effect_placed(""), targeted_effect(false), effect_duration(0),
                        effect_perm(false), effect_intensity(0), bleed(0) {};
    bool load(JsonObject &jsobj, std::string member);
};

class wound_effect_type
{
        friend void load_wound_effect_type(JsonObject &jo);
        friend class wound;
    public:
        wound_effect_type();
        wound_effect_type(const wound_effect_type &rhs);

        wefftype_id id;

        wefftype_id get_wound_id();

        std::string get_name_mod(bool triggered = false);
        std::string get_desc(bool triggered = false);

        int get_trig_delay();
        int get_base_trig_chance();
        int get_trig_max();
        int get_heal_delay();
        int get_base_heal_chance();
        int get_heal_mod();

    protected:
        std::string name_mod;
        std::string name_mod_trig;
        std::string desc;
        std::string desc_trig;
        int trig_delay;
        int base_trig_chance;
        int trig_max;
        int heal_delay;
        int base_heal_chance;
        int heal_mod;

        wound_effect_info base_mods;
        wound_effect_info sev_scale_mods;
}

class wound : public JsonSerializer, public JsonDeserializer
{
    public:
        wound();
        wound(body_part target, int nside, int sev = 1, int nheal_mod = 0);
        wound(const wound &rhs);
        wound &operator=(const wound &rhs);
        
        int get_duration();
        void mod_duration(int dur);
        void set_duration(int dur);
        
        int get_heal_mod();
        void mod_heal_mod(int mod);
        void set_heal_mod(int mod);
        
        int get_severity();
        void mod_severity(int sev);
        void set_severity(int sev);
        
        body_part get_bp();
        int get_side();
        
        using JsonSerializer::serialize;
        void serialize(JsonOut &json) const {
            json.start_object();
            json.member("duration", duration);
            json.member("heal_mod", heal_mod);
            json.member("severity", severity);
            json.member("bp", (int)bp);
            json.member("side", side);
            json.end_object();
        }
        using JsonDeserializer::deserialize;
        void deserialize(JsonIn &jsin) {
            JsonObject jo = jsin.get_object();
            duration = jo.get_int("duration");
            heal_mod = jo.get_int("heal_mod");
            severity = jo.get_int("severity");
            bp = (body_part)jo.get_int("bp");
            side = jo.get_int("side");
        }
    protected:
        int duration;
        std::vector<wefftype_id> wound_effects;
        std::map<wefftype_id, bool> wound_trigs;
        std::map<wefftype_id, int> wound_trig_counts;
        int heal_mod;
        int severity;
        body_part bp;
        int side;
}

void load_wound_effect_type(JsonObject &jo);
void reset_wound_effect_types();

#endif