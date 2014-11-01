#ifndef EFFECT_H
#define EFFECT_H

#include "pldata.h"
#include "json.h"
#include "messages.h"

class effect_type;
class Creature;

extern std::map<std::string, effect_type> effect_types;

void weed_msg(player *p);

enum effect_rating {
    e_good, // the effect is good for the one who has it.
    e_neutral,  // there is no effect or the effect is very nominal. This is the default.
    e_bad,      // the effect is bad for the one who has it
    e_mixed     // the effect has good and bad parts to the one who has it
};

struct effect_mod_info {
    /** All pairs are unmodified,reduced */
    std::pair<float, float> str_mod;
    std::pair<float, float> dex_mod;
    std::pair<float, float> per_mod;
    std::pair<float, float> int_mod;
    std::pair<int, int> speed_mod;
    
    std::pair<int, int> pain_amount;
    std::pair<int, int> pain_min;
    std::pair<int, int> pain_max;
    std::pair<int, int> pain_max_val;
    std::pair<int, int> pain_chance_top;
    std::pair<int, int> pain_chance_bot;
    std::pair<int, int> pain_tick;

    std::pair<int, int> hurt_amount;
    std::pair<int, int> hurt_min;
    std::pair<int, int> hurt_max;
    std::pair<int, int> hurt_chance_top;
    std::pair<int, int> hurt_chance_bot;
    std::pair<int, int> hurt_tick;

    std::pair<int, int> sleep_amount;
    std::pair<int, int> sleep_min;
    std::pair<int, int> sleep_max;
    std::pair<int, int> sleep_chance_top;
    std::pair<int, int> sleep_chance_bot;
    std::pair<int, int> sleep_tick;
    
    std::pair<int, int> pkill_amount;
    std::pair<int, int> pkill_min;
    std::pair<int, int> pkill_max;
    std::pair<int, int> pkill_max_val;
    std::pair<int, int> pkill_chance_top;
    std::pair<int, int> pkill_chance_bot;
    std::pair<int, int> pkill_tick;
    
    std::pair<int, int> stim_amount;
    std::pair<int, int> stim_min;
    std::pair<int, int> stim_max;
    std::pair<int, int> stim_min_val;
    std::pair<int, int> stim_max_val;
    std::pair<int, int> stim_chance_top;
    std::pair<int, int> stim_chance_bot;
    std::pair<int, int> stim_tick;
    
    std::pair<int, int> health_amount;
    std::pair<int, int> health_min;
    std::pair<int, int> health_max;
    std::pair<int, int> health_min_val;
    std::pair<int, int> health_max_val;
    std::pair<int, int> health_chance_top;
    std::pair<int, int> health_chance_bot;
    std::pair<int, int> health_tick;
    
    std::pair<int, int> h_mod_amount;
    std::pair<int, int> h_mod_min;
    std::pair<int, int> h_mod_max;
    std::pair<int, int> h_mod_min_val;
    std::pair<int, int> h_mod_max_val;
    std::pair<int, int> h_mod_chance_top;
    std::pair<int, int> h_mod_chance_bot;
    std::pair<int, int> h_mod_tick;
    
    std::pair<int, int> rad_amount;
    std::pair<int, int> rad_min;
    std::pair<int, int> rad_max;
    std::pair<int, int> rad_max_val;
    std::pair<int, int> rad_chance_top;
    std::pair<int, int> rad_chance_bot;
    std::pair<int, int> rad_tick;
    
    std::pair<int, int> hunger_amount;
    std::pair<int, int> hunger_min;
    std::pair<int, int> hunger_max;
    std::pair<int, int> hunger_min_val;
    std::pair<int, int> hunger_max_val;
    std::pair<int, int> hunger_chance_top;
    std::pair<int, int> hunger_chance_bot;
    std::pair<int, int> hunger_tick;
    
    std::pair<int, int> thirst_amount;
    std::pair<int, int> thirst_min;
    std::pair<int, int> thirst_max;
    std::pair<int, int> thirst_min_val;
    std::pair<int, int> thirst_max_val;
    std::pair<int, int> thirst_chance_top;
    std::pair<int, int> thirst_chance_bot;
    std::pair<int, int> thirst_tick;
    
    std::pair<int, int> fatigue_amount;
    std::pair<int, int> fatigue_min;
    std::pair<int, int> fatigue_max;
    std::pair<int, int> fatigue_min_val;
    std::pair<int, int> fatigue_max_val;
    std::pair<int, int> fatigue_chance_top;
    std::pair<int, int> fatigue_chance_bot;
    std::pair<int, int> fatigue_tick;

    std::pair<int, int> cough_chance_top;
    std::pair<int, int> cough_chance_bot;
    std::pair<int, int> cough_tick;
    
    std::pair<int, int> vomit_chance_top;
    std::pair<int, int> vomit_chance_bot;
    std::pair<int, int> vomit_tick;
    
    bool load(JsonObject &jsobj, std::string member);
};

class effect_type
{
        friend void load_effect_type(JsonObject &jo);
        friend class effect;
    public:
        effect_type();
        effect_type(const effect_type &rhs);

        efftype_id id;

        effect_rating get_rating() const;
        
        bool use_name_ints() const;
        bool use_desc_ints() const;

        // Appropriate game_message_type when effect is optained.
        game_message_type gain_game_message_type() const;
        // Appropriate game_message_type when effect is lost.
        game_message_type lose_game_message_type() const;

        std::string get_apply_message() const;
        std::string get_apply_memorial_log() const;
        std::string get_remove_message() const;
        std::string get_remove_memorial_log() const;

        bool get_main_parts() const;
    
        bool load_miss_msgs(JsonObject &jsobj, std::string member);
        bool load_decay_msgs(JsonObject &jsobj, std::string member);

    protected:
        int max_intensity;
        int max_duration;
        
        int dur_add_perc;
        int int_add_val;
        
        int int_decay_step;
        int int_decay_tick;
        int int_dur_factor;
        
        bool main_parts_only;
        
        std::string resist_trait;
        std::string resist_effect;
        std::string removes_effect;
        
        std::vector<std::pair<std::string, int>> miss_msgs;
        
        bool pain_sizing;
        bool hurt_sizing;
        bool harmful_cough;
        // Once addictions are JSON-ized it should be trivial to convert this to a 
        // "generic" addiction reduces value
        bool pkill_addict_reduces;

        std::vector<std::string> name;
        std::string speed_mod_name;
        std::vector<std::string> desc;
        std::vector<std::string> reduced_desc;
        bool part_descs;
        
        std::vector<std::pair<std::string, game_message_type>> decay_msgs;

        effect_rating rating;

        std::string apply_message;
        std::string apply_memorial_log;
        std::string remove_message;
        std::string remove_memorial_log;
        
        effect_mod_info base_mods;
        effect_mod_info scaling_mods;
};

class effect : public JsonSerializer, public JsonDeserializer
{
    public:
        effect() :
            eff_type(NULL),
            duration(0),
            bp(num_bp),
            permanent(false),
            intensity(1)
        { }
        effect(effect_type *peff_type, int dur, body_part part, bool perm, int nintensity) :
            eff_type(peff_type),
            duration(dur),
            bp(part),
            permanent(perm),
            intensity(nintensity)
        { }
        effect(const effect &) = default;
        effect &operator=(const effect &) = default;

        std::string disp_name() const;
        std::string disp_desc(bool reduced = false) const;
        bool use_part_descs() const;

        effect_type *get_effect_type() const;
        
        void decay(std::vector<std::string> &rem_ids, std::vector<body_part> &rem_bps, unsigned int turn, bool player);

        int get_duration() const;
        int get_max_duration() const;
        void set_duration(int dur);
        void mod_duration(int dur);
        void mult_duration(double dur);
        
        body_part get_bp() const;
        void set_bp(body_part part);

        bool is_permanent() const;
        void pause_effect();
        void unpause_effect();

        int get_intensity() const;
        int get_max_intensity() const;
        void set_intensity(int nintensity);
        void mod_intensity(int nintensity);
        
        std::string get_resist_trait() const;
        std::string get_resist_effect() const;
        std::string get_removes_effect() const;
        
        int get_mod(std::string arg, bool reduced = false) const;
        int get_amount(std::string arg, bool reduced = false) const;
        int get_min_val(std::string arg, bool reduced = false) const;
        int get_max_val(std::string arg, bool reduced = false) const;
        bool get_sizing(std::string arg) const;
        void get_activation_vals(std::string arg, bool reduced, effect_mod_info base, effect_mod_info scale, 
                                double &tick, double &top_base, double &top_scale, double &bot_base,
                                double &bot_scale) const;
        /** returns the approximate percentage chance of activating, used for descriptions */
        double get_percentage(std::string arg, bool reduced = false) const;
        /** mod modifies x in x_in_y or one_in(x) */
        bool activated(unsigned int turn, std::string arg, bool reduced = false, double mod = 1) const;
        
        double get_addict_mod(std::string arg, int addict_level) const;
        bool get_harmful_cough() const;
        int get_dur_add_perc() const;
        int get_int_add_val() const;
        
        std::vector<std::pair<std::string, int>> get_miss_msgs() const;
        
        std::string get_speed_name() const;

        efftype_id get_id() const
        {
            return eff_type->id;
        }

        using JsonSerializer::serialize;
        void serialize(JsonOut &json) const;
        using JsonDeserializer::deserialize;
        void deserialize(JsonIn &jsin);

    protected:
        effect_type *eff_type;
        int duration;
        body_part bp;
        bool permanent;
        int intensity;

};

void load_effect_type(JsonObject &jo);
void reset_effect_types();

#endif
