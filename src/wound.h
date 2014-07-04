#ifndef _WOUND_H_
#define _WOUND_H_

#include "pldata.h"
#include "creature.h"
#include "json.h"

class wound_eff_type;
class Creature;

extern std::map<std::string, wound_eff_type> wound_eff_types;

struct wound_effect_info {
    int effect_duration;
    int effect_intensity;
    float heal_mod_100K;
    float bleed_min;
    float bleed_max;
    float hurt_min;
    float hurt_max;
    float pain_min;
    float pain_max;
    float str_mod;
    float dex_mod;
    float per_mod;
    float int_mod;
    int speed_mod;

    wound_effect_info() :  effect_duration(0), effect_intensity(0), heal_mod_100K(0), bleed_min(0),
                            bleed_max(0), hurt_min(0), hurt_max(0), pain_min(0), pain_max(0),
                            str_mod(0), dex_mod(0), per_mod(0), int_mod(0), speed_mod(0){};
    bool load(JsonObject &jsobj, std::string member);
};

struct wound_trigger_info {
    int trig_delay;
    int trig_chance_100K;
    float health_mult;
    int chance_min;
    int chance_max;
    bool rem_effect;
    bool heal_wound;
    std::string trig_message;
    
    wefftype_id weff_added;

    wound_effect_info flat_mods;
    wound_effect_info sev_mods;

    wound_trigger_info() : trig_delay(0), trig_chance_100K(0), health_mult(0), chance_min(0),
                            chance_max(0), rem_effect(false), heal_wound(false), trig_message(""),
                            weff_added(""), flat_mods(), sev_mods() {};
    bool load(JsonObject &jsobj, std::string member);
};

class wound_eff_type
{
        friend void load_wound_eff_type(JsonObject &jo);
        friend class wound;
    public:
        wound_eff_type();

        wefftype_id id;

        std::string get_name_mod();
        std::string get_desc();
        
        int get_trig_str_mod(int sev);
        int get_trig_dex_mod(int sev);
        int get_trig_per_mod(int sev);
        int get_trig_int_mod(int sev);
        int get_trig_speed_mod(int sev);
        
        int get_trig_bleed(int sev);
        int get_trig_pain(int sev);
        int get_trig_hurt(int sev);
        
        int get_trig_heal_mod(int sev);
        
        int get_trig_effect_dur(int sev);
        int get_trig_effect_int(int sev);
        
        int get_effect_dur(int sev);
        int get_effect_int(int sev);
        
        efftype_id get_effect_placed();
        bool get_targeted_effect();
        bool get_effect_perm();
        bool get_targeted_harm();
        bool get_heal_wound();
        std::string get_weff_added();
        bool get_rem_effect();
        
        int get_trig_delay();
        int get_trig_chance(int health);

    protected:
        std::string name_mod;
        std::string desc;
        
        efftype_id effect_placed;
        bool targeted_effect;
        bool effect_perm;
        bool targeted_harm;

        wound_effect_info base_mods;
        wound_effect_info sev_scale_mods;
        
        wound_trigger_info trigger_info;
};

class wound : public JsonSerializer, public JsonDeserializer
{
    public:
        wound();
        wound(body_part target, int nside, int sev = 1,
                std::vector<wefftype_id> wound_effs = std::vector<wefftype_id>());
        wound(const wound &rhs);
        wound &operator=(const wound &rhs);
        
        bool add_wound_effect(wefftype_id eff);
        bool remove_wound_effect(wefftype_id eff);
        
        bool has_wound_effect(wefftype_id eff);
        
        int get_age();
        void mod_age(int dur);
        void set_age(int dur);
        
        int get_base_heal_mod();
        int get_base_str_mod();
        int get_base_dex_mod();
        int get_base_per_mod();
        int get_base_int_mod();
        int get_base_speed_mod();
        
        void mod_base_heal_mod(int mod);
        void mod_base_str_mod(int mod);
        void mod_base_dex_mod(int mod);
        void mod_base_per_mod(int mod);
        void mod_base_int_mod(int mod);
        void mod_base_speed_mod(int mod);
        
        void set_base_heal_mod(int mod);
        void set_base_str_mod(int mod);
        void set_base_dex_mod(int mod);
        void set_base_per_mod(int mod);
        void set_base_int_mod(int mod);
        void set_base_speed_mod(int mod);
        
        int get_severity();
        void mod_severity(int sev);
        void set_severity(int sev);
        
        body_part get_bp();
        int get_side();
        
        int get_str_mod();
        int get_dex_mod();
        int get_per_mod();
        int get_int_mod();
        int get_speed_mod();
        
        int get_bleed();
        int get_pain();
        int get_hurt();
        
        int get_heal_chance_100K();
        
        std::vector<wefftype_id> wound_effects;
        
        /** Handles wound trigger effects, returns true if the wound is removed */
        bool roll_trigs(player &p);
        
        using JsonSerializer::serialize;
        void serialize(JsonOut &json) const {
            json.start_object();
            json.member("bp", (int)bp);
            json.member("side", side);
            json.member("severity", severity);
            json.member("wound_effects", wound_effects);
            json.member("base_heal_mod", base_heal_mod);
            json.member("base_str_mod", base_str_mod);
            json.member("base_dex_mod", base_dex_mod);
            json.member("base_per_mod", base_per_mod);
            json.member("base_int_mod", base_int_mod);
            json.member("base_speed_mod", base_speed_mod);
            json.member("age", age);
            json.end_object();
        }
        using JsonDeserializer::deserialize;
        void deserialize(JsonIn &jsin) {
            JsonObject jo = jsin.get_object();
            bp = (body_part)jo.get_int("bp");
            side = jo.get_int("side");
            severity = jo.get_int("severity");

            std::vector<wefftype_id> wound_effects;
            JsonArray jarr = jo.get_array("wound_effects");
            while (jarr.has_more()) {
                wound_effects.push_back(jarr.next_string());
            }
            
            base_heal_mod = jo.get_int("base_heal_mod");
            base_str_mod = jo.get_int("base_str_mod");
            base_dex_mod = jo.get_int("base_dex_mod");
            base_per_mod = jo.get_int("base_per_mod");
            base_int_mod = jo.get_int("base_int_mod");
            base_speed_mod = jo.get_int("base_speed_mod");
            age = jo.get_int("age");
        }
    protected:
        body_part bp;
        int side;
        int severity;
        int base_heal_mod;
        int base_str_mod;
        int base_dex_mod;
        int base_per_mod;
        int base_int_mod;
        int base_speed_mod;
        int age;
};

void load_wound_eff_type(JsonObject &jo);
void reset_wound_eff_types();

#endif