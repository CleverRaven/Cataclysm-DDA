#include "effect.h"
#include "rng.h"
#include "output.h"
#include "player.h"
#include <map>
#include <sstream>

std::map<std::string, effect_type> effect_types;

void weed_msg(player *p) {
    int howhigh = p->get_effect_dur("weed_high");
    int smarts = p->get_int();
    if(howhigh > 125 && one_in(7)) {
        int msg = rng(0,5);
        switch(msg) {
        case 0: // Freakazoid
            p->add_msg_if_player(_("The scariest thing in the world would be... if all the air in the world turned to WOOD!"));
            return;
        case 1: // Simpsons
            p->add_msg_if_player(_("Could Jesus microwave a burrito so hot, that he himself couldn't eat it?"));
            p->hunger += 2;
            return;
        case 2:
            if(smarts > 8) { // Timothy Leary
                p->add_msg_if_player(_("Science is all metaphor."));
            } else if(smarts < 3){ // It's Always Sunny in Phildelphia
                p->add_msg_if_player(_("Science is a liar sometimes."));
            } else { // Durr
                p->add_msg_if_player(_("Science is... wait, what was I talking about again?"));
            }
            return;
        case 3: // Dazed and Confused
            p->add_msg_if_player(_("Behind every good man there is a woman, and that woman was Martha Washington, man."));
            if(one_in(2)) {
                p->add_msg_if_player(_("Every day, George would come home, and she would have a big fat bowl waiting for him when he came in the door, man."));
                if(one_in(2)) {
                    p->add_msg_if_player(_("She was a hip, hip, hip lady, man."));
                }
            }
            return;
        case 4:
            if(p->has_amount("money_bundle", 1)) { // Half Baked
                p->add_msg_if_player(_("You ever see the back of a twenty dollar bill... on weed?"));
                if(one_in(2)) {
                    p->add_msg_if_player(_("Oh, there's some crazy shit, man. There's a dude in the bushes. Has he got a gun? I dunno!"));
                    if(one_in(3)) {
                        p->add_msg_if_player(_("RED TEAM GO, RED TEAM GO!"));
                    }
                }
            } else if(p->has_amount("holybook_bible", 1)) {
                p->add_msg_if_player(_("You have a sudden urge to flip your bible open to Genesis 1:29..."));
            } else { // Big Lebowski
                p->add_msg_if_player(_("That rug really tied the room together..."));
            }
            return;
        case 5:
            p->add_msg_if_player(_("I used to do drugs...  I still do, but I used to, too."));
        default:
            return;
        }
    } else if(howhigh > 100 && one_in(5)) {
        int msg = rng(0, 5);
        switch(msg) {
        case 0: // Bob Marley
            p->add_msg_if_player(_("The herb reveals you to yourself."));
            return;
        case 1: // Freakazoid
            p->add_msg_if_player(_("Okay, like, the scariest thing in the world would be... if like you went to grab something and it wasn't there!"));
            return;
        case 2: // Simpsons
            p->add_msg_if_player(_("They call them fingers, but I never see them fing."));
            if(smarts > 2 && one_in(2)) {
                p->add_msg_if_player(_("... oh, there they go."));
            }
            return;
        case 3: // Bill Hicks
            p->add_msg_if_player(_("You suddenly realize that all matter is merely energy condensed to a slow vibration, and we are all one consciousness experiencing itself subjectively."));
            return;
        case 4: // Steve Martin
            p->add_msg_if_player(_("I usually only smoke in the late evening."));
            if(one_in(4)) {
                p->add_msg_if_player(_("Oh, occasionally the early evening, but usually the late evening, or the mid-evening."));
            }
            if(one_in(4)) {
                p->add_msg_if_player(_("Just the early evening, mid-evening and late evening."));
            }
            if(one_in(4)) {
                p->add_msg_if_player(_("Occasionally, early afternoon, early mid-afternoon, or perhaps the late mid-afternoon."));
            }
            if(one_in(4)) {
                p->add_msg_if_player(_("Oh, sometimes the early-mid-late-early-morning."));
            }
            if(smarts > 2) {
                p->add_msg_if_player(_("...But never at dusk."));
            }
            return;
        case 5:
        default:
            return;
        }
    } else if(howhigh > 50 && one_in(3)) {
        int msg = rng(0, 5);
        switch(msg) {
        case 0: // Cheech and Chong
            p->add_msg_if_player(_("Dave's not here, man."));
            return;
        case 1: // Real Life
            p->add_msg_if_player(_("Man, a cheeseburger sounds SO awesome right now."));
            p->hunger += 4;
            if(p->has_trait("VEGETARIAN")) {
               p->add_msg_if_player(_("Eh... maybe not."));
            } else if(p->has_trait("LACTOSE")) {
                p->add_msg_if_player(_("I guess, maybe, without the cheese... yeah."));
            }
            return;
        case 2: // Dazed and Confused
            p->add_msg_if_player( _("Walkin' down the hall, by myself, smokin' a j with fifty elves."));
            return;
        case 3: // Half Baked
            p->add_msg_if_player(_("That weed was the shiz-nittlebam snip-snap-sack."));
            return;
        case 4:
            weed_msg(p); // re-roll
        case 5:
        default:
            return;
        }
    }
}

static void extract_effect( JsonObject &j, std::unordered_map<std::tuple<std::string, bool, std::string, std::string>, double> &data,
                std::string mod_type, std::string data_key, std::string type_key, std::string arg_key)
{
    double val = 0;
    double reduced_val = 0;
    if (j.has_member(mod_type)) {
        JsonArray jsarr = j.get_array(mod_type);
        val = jsarr.get_float(0);
        // If a second value exists use it, else reduced_val = val.
        if (jsarr.size() >= 2) {
            reduced_val = jsarr.get_float(1);
        } else {
            reduced_val = val;
        }
    }
    // Store values if they aren't zero.
    if (val != 0) {
        data[std::make_tuple(data_key, false, type_key, arg_key)] = val;
    }
    if (reduced_val != 0) {
        data[std::make_tuple(data_key, true, type_key, arg_key)] = val;
    }
}

bool effect_type::load_mod_data(JsonObject &jsobj, std::string member) {
    if (jsobj.has_object(member)) {
        JsonObject j = jsobj.get_object(member);
        
        // Stats first
        //                          json field                  type key    arg key
        extract_effect(j, mod_data, "str_mod",          member, "STR",      "min");
        extract_effect(j, mod_data, "dex_mod",          member, "DEX",      "min");
        extract_effect(j, mod_data, "per_mod",          member, "PER",      "min");
        extract_effect(j, mod_data, "int_mod",          member, "INT",      "min");
        extract_effect(j, mod_data, "speed_mod",        member, "SPEED",    "min");
        
        // Then pain
        extract_effect(j, mod_data, "pain_amount",      member, "PAIN",     "amount");
        extract_effect(j, mod_data, "pain_min",         member, "PAIN",     "min");
        extract_effect(j, mod_data, "pain_max",         member, "PAIN",     "max");
        extract_effect(j, mod_data, "pain_max_val",     member, "PAIN",     "max_val");
        extract_effect(j, mod_data, "pain_chance",      member, "PAIN",     "chance_top");
        extract_effect(j, mod_data, "pain_chance_bot",  member, "PAIN",     "chance_bot");
        extract_effect(j, mod_data, "pain_tick",        member, "PAIN",     "tick");
        
        // Then hurt
        extract_effect(j, mod_data, "hurt_amount",      member, "HURT",     "amount");
        extract_effect(j, mod_data, "hurt_min",         member, "HURT",     "min");
        extract_effect(j, mod_data, "hurt_max",         member, "HURT",     "max");
        extract_effect(j, mod_data, "hurt_chance",      member, "HURT",     "chance_top");
        extract_effect(j, mod_data, "hurt_chance_bot",  member, "HURT",     "chance_bot");
        extract_effect(j, mod_data, "hurt_tick",        member, "HURT",     "tick");
        
        // Then sleep
        extract_effect(j, mod_data, "sleep_amount",     member, "SLEEP",    "amount");
        extract_effect(j, mod_data, "sleep_min",        member, "SLEEP",    "min");
        extract_effect(j, mod_data, "sleep_max",        member, "SLEEP",    "max");
        extract_effect(j, mod_data, "sleep_chance",     member, "SLEEP",    "chance_top");
        extract_effect(j, mod_data, "sleep_chance_bot", member, "SLEEP",    "chance_bot");
        extract_effect(j, mod_data, "sleep_tick",       member, "SLEEP",    "tick");
        
        // Then pkill
        extract_effect(j, mod_data, "pkill_amount",     member, "PKILL",    "amount");
        extract_effect(j, mod_data, "pkill_min",        member, "PKILL",    "min");
        extract_effect(j, mod_data, "pkill_max",        member, "PKILL",    "max");
        extract_effect(j, mod_data, "pkill_max_val",    member, "PKILL",    "max_val");
        extract_effect(j, mod_data, "pkill_chance",     member, "PKILL",    "chance_top");
        extract_effect(j, mod_data, "pkill_chance_bot", member, "PKILL",    "chance_bot");
        extract_effect(j, mod_data, "pkill_tick",       member, "PKILL",    "tick");
        
        // Then stim
        extract_effect(j, mod_data, "stim_amount",      member, "STIM",     "amount");
        extract_effect(j, mod_data, "stim_min",         member, "STIM",     "min");
        extract_effect(j, mod_data, "stim_max",         member, "STIM",     "max");
        extract_effect(j, mod_data, "stim_min_val",     member, "STIM",     "min_val");
        extract_effect(j, mod_data, "stim_max_val",     member, "STIM",     "max_val");
        extract_effect(j, mod_data, "stim_chance",      member, "STIM",     "chance_top");
        extract_effect(j, mod_data, "stim_chance_bot",  member, "STIM",     "chance_bot");
        extract_effect(j, mod_data, "stim_tick",        member, "STIM",     "tick");
        
        // Then health
        extract_effect(j, mod_data, "health_amount",    member, "HEALTH",   "amount");
        extract_effect(j, mod_data, "health_min",       member, "HEALTH",   "min");
        extract_effect(j, mod_data, "health_max",       member, "HEALTH",   "max");
        extract_effect(j, mod_data, "health_min_val",   member, "HEALTH",   "min_val");
        extract_effect(j, mod_data, "health_max_val",   member, "HEALTH",   "max_val");
        extract_effect(j, mod_data, "health_chance",    member, "HEALTH",   "chance_top");
        extract_effect(j, mod_data, "health_chance_bot",member, "HEALTH",   "chance_bot");
        extract_effect(j, mod_data, "health_tick",      member, "HEALTH",   "tick");
        
        // Then health mod
        extract_effect(j, mod_data, "h_mod_amount",     member, "H_MOD",    "amount");
        extract_effect(j, mod_data, "h_mod_min",        member, "H_MOD",    "min");
        extract_effect(j, mod_data, "h_mod_max",        member, "H_MOD",    "max");
        extract_effect(j, mod_data, "h_mod_min_val",    member, "H_MOD",    "min_val");
        extract_effect(j, mod_data, "h_mod_max_val",    member, "H_MOD",    "max_val");
        extract_effect(j, mod_data, "h_mod_chance",     member, "H_MOD",    "chance_top");
        extract_effect(j, mod_data, "h_mod_chance_bot", member, "H_MOD",    "chance_bot");
        extract_effect(j, mod_data, "h_mod_tick",       member, "H_MOD",    "tick");
        
        // Then radiation
        extract_effect(j, mod_data, "rad_amount",       member, "RAD",      "amount");
        extract_effect(j, mod_data, "rad_min",          member, "RAD",      "min");
        extract_effect(j, mod_data, "rad_max",          member, "RAD",      "max");
        extract_effect(j, mod_data, "rad_max_val",      member, "RAD",      "max_val");
        extract_effect(j, mod_data, "rad_chance",       member, "RAD",      "chance_top");
        extract_effect(j, mod_data, "rad_chance_bot",   member, "RAD",      "chance_bot");
        extract_effect(j, mod_data, "rad_tick",         member, "RAD",      "tick");
        
        // Then hunger
        extract_effect(j, mod_data, "hunger_amount",    member, "HUNGER",   "amount");
        extract_effect(j, mod_data, "hunger_min",       member, "HUNGER",   "min");
        extract_effect(j, mod_data, "hunger_max",       member, "HUNGER",   "max");
        extract_effect(j, mod_data, "hunger_min_val",   member, "HUNGER",   "min_val");
        extract_effect(j, mod_data, "hunger_max_val",   member, "HUNGER",   "max_val");
        extract_effect(j, mod_data, "hunger_chance",    member, "HUNGER",   "chance_top");
        extract_effect(j, mod_data, "hunger_chance_bot",member, "HUNGER",   "chance_bot");
        extract_effect(j, mod_data, "hunger_tick",      member, "HUNGER",   "tick");
        
        // Then thirst
        extract_effect(j, mod_data, "thirst_amount",    member, "THIRST",   "amount");
        extract_effect(j, mod_data, "thirst_min",       member, "THIRST",   "min");
        extract_effect(j, mod_data, "thirst_max",       member, "THIRST",   "max");
        extract_effect(j, mod_data, "thirst_min_val",   member, "THIRST",   "min_val");
        extract_effect(j, mod_data, "thirst_max_val",   member, "THIRST",   "max_val");
        extract_effect(j, mod_data, "thirst_chance",    member, "THIRST",   "chance_top");
        extract_effect(j, mod_data, "thirst_chance_bot",member, "THIRST",   "chance_bot");
        extract_effect(j, mod_data, "thirst_tick",      member, "THIRST",   "tick");
        
        // Then fatigue
        extract_effect(j, mod_data, "fatigue_amount",    member, "FATIGUE",  "amount");
        extract_effect(j, mod_data, "fatigue_min",       member, "FATIGUE",  "min");
        extract_effect(j, mod_data, "fatigue_max",       member, "FATIGUE",  "max");
        extract_effect(j, mod_data, "fatigue_min_val",   member, "FATIGUE",  "min_val");
        extract_effect(j, mod_data, "fatigue_max_val",   member, "FATIGUE",  "max_val");
        extract_effect(j, mod_data, "fatigue_chance",    member, "FATIGUE",  "chance_top");
        extract_effect(j, mod_data, "fatigue_chance_bot",member, "FATIGUE",  "chance_bot");
        extract_effect(j, mod_data, "fatigue_tick",      member, "FATIGUE",  "tick");
        
        // Then coughing
        extract_effect(j, mod_data, "cough_chance",     member, "COUGH",    "chance_top");
        extract_effect(j, mod_data, "cough_chance_bot", member, "COUGH",    "chance_bot");
        extract_effect(j, mod_data, "cough_tick",       member, "COUGH",    "tick");
        
        // Then vomiting
        extract_effect(j, mod_data, "vomit_chance",     member, "VOMIT",    "chance_top");
        extract_effect(j, mod_data, "vomit_chance_bot", member, "VOMIT",    "chance_bot");
        extract_effect(j, mod_data, "vomit_tick",       member, "VOMIT",    "tick");
        
        return true;
    } else {
        return false;
    }
}

effect_type::effect_type() {}
effect_type::effect_type(const effect_type &) {}

effect_rating effect_type::get_rating() const
{
    return rating;
}

bool effect_type::use_name_ints() const
{
    return ((size_t)max_intensity <= name.size());
}

bool effect_type::use_desc_ints(bool reduced) const
{
    if (reduced) {
        return ((size_t)max_intensity <= reduced_desc.size());
    } else {
        return ((size_t)max_intensity <= desc.size());
    }
}

game_message_type effect_type::gain_game_message_type() const
{
    switch(rating) {
    case e_good:
        return m_good;
    case e_bad:
        return m_bad;
    case e_neutral:
        return m_neutral;
    case e_mixed:
        return m_mixed;
    default:
        return m_neutral;  // Should never happen
    }
}
game_message_type effect_type::lose_game_message_type() const
{
    switch(rating) {
    case e_good:
        return m_bad;
    case e_bad:
        return m_good;
    case e_neutral:
        return m_neutral;
    case e_mixed:
        return m_mixed;
    default:
        return m_neutral;  // Should never happen
    }
}
std::string effect_type::get_apply_message() const
{
    return apply_message;
}
std::string effect_type::get_apply_memorial_log() const
{
    return apply_memorial_log;
}
std::string effect_type::get_remove_message() const
{
    return remove_message;
}
std::string effect_type::get_remove_memorial_log() const
{
    return remove_memorial_log;
}
bool effect_type::get_main_parts() const
{
    return main_parts_only;
}
bool effect_type::load_miss_msgs(JsonObject &jo, std::string member)
{
    if (jo.has_array(member)) {
        JsonArray outer = jo.get_array(member);
        while (outer.has_more()) {
            JsonArray inner = outer.next_array();
            miss_msgs.push_back(std::make_pair(inner.get_string(0), inner.get_int(1)));
        }
        return true;
    }
    return false;
}
bool effect_type::load_decay_msgs(JsonObject &jo, std::string member)
{
    if (jo.has_array(member)) {
        JsonArray outer = jo.get_array(member);
        while (outer.has_more()) {
            JsonArray inner = outer.next_array();
            std::string msg = inner.get_string(0);
            std::string r = inner.get_string(1);
            game_message_type rate = m_neutral;
            if(r == "good") {
                rate = m_good;
            } else if(r == "neutral" ) {
                rate = m_neutral;
            } else if(r == "bad" ) {
                rate = m_bad;
            } else if(r == "mixed" ) {
                rate = m_mixed;
            } else {
                rate = m_neutral;
            }
            decay_msgs.push_back(std::make_pair(msg, rate));
        }
        return true;
    }
    return false;
}

std::string effect::disp_name() const
{
    // End result should look like "name (l. arm)" or "name [intensity] (l. arm)"
    std::stringstream ret;
    if (eff_type->use_name_ints()) {
        ret << _(eff_type->name[intensity - 1].c_str());
    } else {
        ret << _(eff_type->name[0].c_str());
        if (intensity > 1) {
            ret << " [" << intensity << "]";
        }
    }
    if (bp != num_bp) {
        ret << " (" << body_part_name(bp).c_str() << ")";
    }
    return ret.str();
}

// Used in disp_desc()
struct desc_freq {
    double chance;
    int val;
    std::string pos_string;
    std::string neg_string;
    
    desc_freq(double c, int v, std::string pos, std::string neg) : chance(c), val(v), pos_string(pos), neg_string(neg) {};
};

std::string effect::disp_desc(bool reduced) const
{
    std::stringstream ret;
    // First print stat changes, adding + if value is positive
    int tmp = get_avg_mod("STR", reduced);
    if (tmp > 0) {
        ret << string_format(_("Strength +%d;  "), tmp);
    } else if (tmp < 0) {
        ret << string_format(_("Strength %d;  "), tmp);
    }
    tmp = get_avg_mod("DEX", reduced);
    if (tmp > 0) {
        ret << string_format(_("Dexterity +%d;  "), tmp);
    } else if (tmp < 0) {
        ret << string_format(_("Dexterity %d;  "), tmp);
    }
    tmp = get_avg_mod("PER", reduced);
    if (tmp > 0) {
        ret << string_format(_("Perception +%d;  "), tmp);
    } else if (tmp < 0) {
        ret << string_format(_("Perception %d;  "), tmp);
    }
    tmp = get_avg_mod("INT", reduced);
    if (tmp > 0) {
        ret << string_format(_("Intelligence +%d;  "), tmp);
    } else if (tmp < 0) {
        ret << string_format(_("Intelligence %d;  "), tmp);
    }
    tmp = get_avg_mod("SPEED", reduced);
    if (tmp > 0) {
        ret << string_format(_("Speed +%d;  "), tmp);
    } else if (tmp < 0) {
        ret << string_format(_("Speed %d;  "), tmp);
    }
    // Newline if necessary
    if (ret.str() != "") {
        ret << "\n";
    }
    
    // Then print pain/damage/coughing/vomiting, we don't display pkill, health, or radiation
    std::vector<std::string> constant;
    std::vector<std::string> frequent;
    std::vector<std::string> uncommon;
    std::vector<std::string> rare;
    std::vector<desc_freq> values;
    // Add various desc_freq structs to values. If more effects wish to be placed in the descriptions this is the
    // place to add them.
    int val = 0;
    val = get_avg_mod("PAIN", reduced);
    values.push_back(desc_freq(get_percentage("PAIN", val, reduced), val, _("pain"), _("pain")));
    val = get_avg_mod("HURT", reduced);
    values.push_back(desc_freq(get_percentage("HURT", val, reduced), val, _("damage"), _("damage")));
    val = get_avg_mod("THIRST", reduced);
    values.push_back(desc_freq(get_percentage("THIRST", val, reduced), val, _("thirst"), _("quench")));
    val = get_avg_mod("HUNGER", reduced);
    values.push_back(desc_freq(get_percentage("HUNGER", val, reduced), val, _("hunger"), _("sate")));
    val = get_avg_mod("FATIGUE", reduced);
    values.push_back(desc_freq(get_percentage("FATIGUE", val, reduced), val, _("fatigue"), _("rest")));
    val = get_avg_mod("COUGH", reduced);
    values.push_back(desc_freq(get_percentage("COUGH", val, reduced), val, _("coughing"), _("coughing")));
    val = get_avg_mod("VOMIT", reduced);
    values.push_back(desc_freq(get_percentage("VOMIT", val, reduced), val, _("vomiting"), _("vomiting")));
    val = get_avg_mod("SLEEP", reduced);
    values.push_back(desc_freq(get_percentage("SLEEP", val, reduced), val, _("blackouts"), _("blackouts")));

    for (auto &i : values) {
        if (i.val > 0) {
            // +50% chance, every other step
            if (i.chance >= 50.0) {
                constant.push_back(i.pos_string);
            // +1% chance, every 100 steps
            } else if (i.chance >= 1.0) {
                frequent.push_back(i.pos_string);
            // +.4% chance, every 250 steps
            } else if (i.chance >= .4) {
                uncommon.push_back(i.pos_string);
            // <.4% chance
            } else if (i.chance > 0) {
                rare.push_back(i.pos_string);
            }
        } else if (i.val < 0) {
            // +50% chance, every other step
            if (i.chance >= 50.0) {
                constant.push_back(i.neg_string);
            // +1% chance, every 100 steps
            } else if (i.chance >= 1.0) {
                frequent.push_back(i.neg_string);
            // +.4% chance, every 250 steps
            } else if (i.chance >= .4) {
                uncommon.push_back(i.neg_string);
            // <.4% chance
            } else if (i.chance > 0) {
                rare.push_back(i.neg_string);
            }
        }
    }
    if (constant.size() > 0) {
        ret << _("Const: ");
        for (size_t i = 0; i < constant.size(); i++) {
            if (i == 0) {
                // No comma on the first one
                ret << constant[i];
            } else {
                ret << ", " << constant[i];
            }
        }
        ret << " ";
    }
    if (frequent.size() > 0) {
        ret << _("Freq: ");
        for (size_t i = 0; i < frequent.size(); i++) {
            if (i == 0) {
                // No comma on the first one
                ret << frequent[i];
            } else {
                ret << ", " << frequent[i];
            }
        }
        ret << " ";
    }
    if (uncommon.size() > 0) {
        ret << _("Unfreq: ");
        for (size_t i = 0; i < uncommon.size(); i++) {
            if (i == 0) {
                // No comma on the first one
                ret << uncommon[i];
            } else {
                ret << ", " << uncommon[i];
            }
        }
        ret << " ";
    }
    if (rare.size() > 0) {
        ret << _("Rare: ");
        for (size_t i = 0; i < rare.size(); i++) {
            if (i == 0) {
                // No comma on the first one
                ret << rare[i];
            } else {
                ret << ", " << rare[i];
            }
        }
        // No space needed at the end
    }
    
    // Newline if necessary
    if (ret.str() != "") {
        ret << "\n";
    }
    
    // Then print the effect description
    if (use_part_descs()) {
        //~ Used for effect descriptions, "Your (body_part_name) (effect description)"
        ret << string_format(_("Your %s "), body_part_name(bp).c_str());
    }
    std::string tmp_str;
    if (eff_type->use_desc_ints(reduced)) {
        if (reduced) {
            tmp_str = eff_type->reduced_desc[intensity - 1];
        } else {
            tmp_str = eff_type->desc[intensity - 1];
        }
    } else {
        if (reduced) {
            tmp_str = eff_type->reduced_desc[0];
        } else {
            tmp_str = eff_type->desc[0];
        }
    }
    ret << _(tmp_str.c_str());

    return ret.str();
}

void effect::decay(std::vector<std::string> &rem_ids, std::vector<body_part> &rem_bps, unsigned int turn, bool player)
{
    // Decay duration if not permanent
    if (!is_permanent()) {
        duration -= 1;
        add_msg( m_debug, "ID: %s, Duration %d", get_id().c_str(), duration );
    }
    // Store current intensity for comparison later
    int tmp_int = intensity;
    
    // Fix bad intensities
    if (intensity < 1) {
        add_msg( m_debug, "Bad intensity, ID: %s", get_id().c_str() );
        intensity = 1;
    } else if (intensity > 1) {
        // Decay intensity if necessary
        if (eff_type->int_decay_tick != 0 && turn % eff_type->int_decay_tick == 0) {
            intensity += eff_type->int_decay_step;
        }
    }
    // Force intensity if it is duration based
    if (eff_type->int_dur_factor != 0) {
        // + 1 here so that the lowest is intensity 1, not 0
        intensity = (duration / eff_type->int_dur_factor) + 1;
    }
    // Bound intensity to [1, max_intensity]
    if (intensity > eff_type->max_intensity) {
        intensity = eff_type->max_intensity;
    }
    // Display decay message if available
    if (player && tmp_int > intensity && (intensity - 1) < int(eff_type->decay_msgs.size())) {
        // -1 because intensity = 1 is the first message
        add_msg(eff_type->decay_msgs[intensity - 1].second, eff_type->decay_msgs[intensity - 1].first.c_str());
    }
    
    // Add to removal list if duration is <= 0
    if (duration <= 0) {
        rem_ids.push_back(get_id());
        rem_bps.push_back(bp);
    }
}

bool effect::use_part_descs() const
{
    return eff_type->part_descs;
}

int effect::get_duration() const
{
    return duration;
}
int effect::get_max_duration() const
{
    return eff_type->max_duration;
}
void effect::set_duration(int dur)
{
    duration = dur;
    // Cap to max_duration if it exists
    if (eff_type->max_duration > 0 && duration > eff_type->max_duration) {
        duration = eff_type->max_duration;
    }
}
void effect::mod_duration(int dur)
{
    duration += dur;
    // Cap to max_duration if it exists
    if (eff_type->max_duration > 0 && duration > eff_type->max_duration) {
        duration = eff_type->max_duration;
    }
}
void effect::mult_duration(double dur)
{
    duration *= dur;
    // Cap to max_duration if it exists
    if (eff_type->max_duration > 0 && duration > eff_type->max_duration) {
        duration = eff_type->max_duration;
    }
}

body_part effect::get_bp() const
{
    return bp;
}
void effect::set_bp(body_part part)
{
    bp = part;
}

bool effect::is_permanent() const
{
    return permanent;
}
void effect::pause_effect()
{
    permanent = true;
}
void effect::unpause_effect()
{
    permanent = false;
}

int effect::get_intensity() const
{
    return intensity;
}
int effect::get_max_intensity() const
{
    return eff_type->max_intensity;
}
void effect::set_intensity(int nintensity)
{
    intensity = nintensity;
    // Cap to [1, max_intensity]
    if (intensity > eff_type->max_intensity) {
        intensity = eff_type->max_intensity;
    } else if (intensity < 1) {
        intensity = 1;
    }
}
void effect::mod_intensity(int nintensity)
{
    intensity += nintensity;
    // Cap to [1, max_intensity]
    if (intensity > eff_type->max_intensity) {
        intensity = eff_type->max_intensity;
    } else if (intensity < 1) {
        intensity = 1;
    }
}

std::string effect::get_resist_trait() const
{
    return eff_type->resist_trait;
}
std::string effect::get_resist_effect() const
{
    return eff_type->resist_effect;
}
std::string effect::get_removes_effect() const
{
    return eff_type->removes_effect;
}

int effect::get_mod(std::string arg, bool reduced) const
{
    auto &mod_data = eff_type->mod_data;
    double min = 0;
    double max = 0;
    // Get the minimum total
    auto found = mod_data.find(std::make_tuple("base_mods", reduced, arg, "min"));
    if (found != mod_data.end()) {
        min += found->second;
    }
    found = mod_data.find(std::make_tuple("scaling_mods", reduced, arg, "min"));
    if (found != mod_data.end()) {
        min += found->second * (intensity - 1);
    }
    // Get the maximum total
    found = mod_data.find(std::make_tuple("base_mods", reduced, arg, "max"));
    if (found != mod_data.end()) {
        max += found->second;
    }
    found = mod_data.find(std::make_tuple("scaling_mods", reduced, arg, "max"));
    if (found != mod_data.end()) {
        max += found->second * (intensity - 1);
    }
    if (int(max) != 0) {
        // Return a random value between [min, max]
        return int(rng(min, max));
    } else {
        // Else return the minimum value
        return min;
    }
}

int effect::get_avg_mod(std::string arg, bool reduced) const
{
    auto &mod_data = eff_type->mod_data;
    double min = 0;
    double max = 0;
    // Get the minimum total
    auto found = mod_data.find(std::make_tuple("base_mods", reduced, arg, "min"));
    if (found != mod_data.end()) {
        min += found->second;
    }
    found = mod_data.find(std::make_tuple("scaling_mods", reduced, arg, "min"));
    if (found != mod_data.end()) {
        min += found->second * (intensity - 1);
    }
    // Get the maximum total
    found = mod_data.find(std::make_tuple("base_mods", reduced, arg, "max"));
    if (found != mod_data.end()) {
        max += found->second;
    }
    found = mod_data.find(std::make_tuple("scaling_mods", reduced, arg, "max"));
    if (found != mod_data.end()) {
        max += found->second * (intensity - 1);
    }
    if (int(max) != 0) {
        // Return an average of min and max
        return int((min + max) / 2);
    } else {
        // Else return the minimum value
        return min;
    }
}

int effect::get_amount(std::string arg, bool reduced) const
{
    auto &mod_data = eff_type->mod_data;
    double ret = 0;
    auto found = mod_data.find(std::make_tuple("base_mods", reduced, arg, "amount"));
    if (found != mod_data.end()) {
        ret += found->second;
    }
    found = mod_data.find(std::make_tuple("scaling_mods", reduced, arg, "amount"));
    if (found != mod_data.end()) {
        ret += found->second * (intensity - 1);
    }
    return int(ret);
}

int effect::get_min_val(std::string arg, bool reduced) const
{
    auto &mod_data = eff_type->mod_data;
    double ret = 0;
    auto found = mod_data.find(std::make_tuple("base_mods", reduced, arg, "min_val"));
    if (found != mod_data.end()) {
        ret += found->second;
    }
    found = mod_data.find(std::make_tuple("scaling_mods", reduced, arg, "min_val"));
    if (found != mod_data.end()) {
        ret += found->second * (intensity - 1);
    }
    return int(ret);
}

int effect::get_max_val(std::string arg, bool reduced) const
{
    auto &mod_data = eff_type->mod_data;
    double ret = 0;
    auto found = mod_data.find(std::make_tuple("base_mods", reduced, arg, "max_val"));
    if (found != mod_data.end()) {
        ret += found->second;
    }
    found = mod_data.find(std::make_tuple("scaling_mods", reduced, arg, "max_val"));
    if (found != mod_data.end()) {
        ret += found->second * (intensity - 1);
    }
    return int(ret);
}

bool effect::get_sizing(std::string arg) const
{
    if (arg == "PAIN") {
        return eff_type->pain_sizing;
    } else if ("HURT") {
        return eff_type->hurt_sizing;
    }
    return false;
}

double effect::get_percentage(std::string arg, int val, bool reduced) const
{
    auto &mod_data = eff_type->mod_data;
    auto found_top_base = mod_data.find(std::make_tuple("base_mods", reduced, arg, "chance_top"));
    auto found_top_scale = mod_data.find(std::make_tuple("scaling_mods", reduced, arg, "chance_top"));
    // Convert to int or 0
    int top_base = 0;
    int top_scale = 0;
    if (found_top_base != mod_data.end()) {
        top_base = found_top_base->second;
    }
    if (found_top_scale != mod_data.end()) {
        top_scale = found_top_scale->second * (intensity - 1);
    }
    // Check chances if value is 0 (so we can check valueless effects like vomiting)
    // Else a nonzero value overrides a 0 chance for default purposes
    if (val == 0) {
        // If both top values <= 0 then it should never trigger
        if (top_base <= 0 && top_scale <= 0) {
            return 0;
        }
        // It will also never trigger if top_base + top_scale <= 0
        if (top_base + top_scale <= 0) {
            return 0;
        }
    }
    
    // We only need to calculate these if we haven't already returned
    int bot_base = 0;
    int bot_scale = 0;
    int tick = 0;
    auto found_bot_base = mod_data.find(std::make_tuple("base_mods", reduced, arg, "chance_bot"));
    auto found_bot_scale = mod_data.find(std::make_tuple("scaling_mods", reduced, arg, "chance_bot"));
    auto found_tick_base = mod_data.find(std::make_tuple("base_mods", reduced, arg, "tick"));
    auto found_tick_scale = mod_data.find(std::make_tuple("scaling_mods", reduced, arg, "tick"));
    if (found_bot_base != mod_data.end()) {
        bot_base = found_bot_base->second;
    }
    if (found_bot_scale != mod_data.end()) {
        bot_scale = found_bot_scale->second * (intensity - 1);
    }
    if (found_tick_base != mod_data.end()) {
        tick += found_tick_base->second;
    }
    if (found_bot_scale != mod_data.end()) {
        tick += found_tick_scale->second * (intensity - 1);
    }
    // Tick is the exception where tick = 0 means tick = 1
    if (tick == 0) {
        tick = 1;
    }

    double ret = 0;
    // If both bot values are zero the formula is one_in(top), else the formula is x_in_y(top, bot)
    if(bot_base != 0 && bot_scale != 0) {
        if (bot_base + bot_scale == 0) {
            // Special crash avoidance case, in most effect fields 0 = "nothing happens"
            // so assume false here for consistency
            ret = 0;
        } else {
            // Cast to double here to allow for partial percentages
            ret = 100 * double(top_base + top_scale) / double(bot_base + bot_scale);
        }
    } else {
        // Cast to double here to allow for partial percentages
        ret = 100 / double(top_base + top_scale);
    }
    // Divide by ticks between rolls
    if (tick > 1) {
        ret = ret / tick;
    }
    return ret;
}

bool effect::activated(unsigned int turn, std::string arg, int val, bool reduced, double mod) const
{
    auto &mod_data = eff_type->mod_data;
    auto found_top_base = mod_data.find(std::make_tuple("base_mods", reduced, arg, "chance_top"));
    auto found_top_scale = mod_data.find(std::make_tuple("scaling_mods", reduced, arg, "chance_top"));
    // Convert to int or 0
    int top_base = 0;
    int top_scale = 0;
    if (found_top_base != mod_data.end()) {
        top_base = found_top_base->second;
    }
    if (found_top_scale != mod_data.end()) {
        top_scale = found_top_scale->second * (intensity - 1);
    }
    // Check chances if value is 0 (so we can check valueless effects like vomiting)
    // Else a nonzero value overrides a 0 chance for default purposes
    if (val == 0) {
        // If both top values <= 0 then it should never trigger
        if (top_base <= 0 && top_scale <= 0) {
            return false;
        }
        // It will also never trigger if top_base + top_scale <= 0
        if (top_base + top_scale <= 0) {
            return false;
        }
    }
    
    // We only need to calculate these if we haven't already returned
    int bot_base = 0;
    int bot_scale = 0;
    int tick = 0;
    auto found_bot_base = mod_data.find(std::make_tuple("base_mods", reduced, arg, "chance_bot"));
    auto found_bot_scale = mod_data.find(std::make_tuple("scaling_mods", reduced, arg, "chance_bot"));
    auto found_tick_base = mod_data.find(std::make_tuple("base_mods", reduced, arg, "tick"));
    auto found_tick_scale = mod_data.find(std::make_tuple("scaling_mods", reduced, arg, "tick"));
    if (found_bot_base != mod_data.end()) {
        bot_base = found_bot_base->second;
    }
    if (found_bot_scale != mod_data.end()) {
        bot_scale = found_bot_scale->second * (intensity - 1);
    }
    if (found_tick_base != mod_data.end()) {
        tick += found_tick_base->second;
    }
    if (found_bot_scale != mod_data.end()) {
        tick += found_tick_scale->second * (intensity - 1);
    }
    // Tick is the exception where tick = 0 means tick = 1
    if (tick == 0) {
        tick = 1;
    }

    // Check if tick allows for triggering. If both bot values are zero the formula is 
    // x_in_y(1, top) i.e. one_in(top), else the formula is x_in_y(top, bot),
    // mod multiplies the overall percentage chances
    
    // has to be an && here to avoid undefined behavior of turn % 0
    if(tick > 0 && turn % tick == 0) {
        if(bot_base != 0 && bot_scale != 0) {
            if (bot_base + bot_scale == 0) {
                // Special crash avoidance case, in most effect fields 0 = "nothing happens"
                // so assume false here for consistency
                return false;
            } else {
                return x_in_y((top_base + top_scale) * mod, (bot_base + bot_scale));
            }
        } else {
            return x_in_y(mod, top_base + top_scale);
        }
    }
    return false;
}

double effect::get_addict_mod(std::string arg, int addict_level) const
{
    // TODO: convert this to JSON id's and values once we have JSON'ed addictions
    if (arg == "PKILL") {
        if (eff_type->pkill_addict_reduces) {
            return 1 / std::max(addict_level * 2, 1);
        } else {
            return 1;
        }
    } else {
        return 1;
    }
}

bool effect::get_harmful_cough() const
{
    return eff_type->harmful_cough;
}
int effect::get_dur_add_perc() const
{
    return eff_type->dur_add_perc;
}
int effect::get_int_add_val() const
{
    return eff_type->int_add_val;
}

std::vector<std::pair<std::string, int>> effect::get_miss_msgs() const
{
    return eff_type->miss_msgs;
}
std::string effect::get_speed_name() const
{
    // USes the speed_mod_name if one exists, else defaults to the first entry in "name".
    if (eff_type->speed_mod_name == "") {
        return eff_type->name[0];
    } else {
        return eff_type->speed_mod_name;
    }
}

effect_type *effect::get_effect_type() const
{
    return eff_type;
}

void load_effect_type(JsonObject &jo)
{
    effect_type new_etype;
    new_etype.id = jo.get_string("id");

    if(jo.has_member("name")) {
        JsonArray jsarr = jo.get_array("name");
        while (jsarr.has_more()) {
            new_etype.name.push_back(_(jsarr.next_string().c_str()));
        }
    } else {
        new_etype.name.push_back("");
    }
    new_etype.speed_mod_name = jo.get_string("speed_name", "");

    if(jo.has_member("desc")) {
        JsonArray jsarr = jo.get_array("desc");
        while (jsarr.has_more()) {
            new_etype.desc.push_back(_(jsarr.next_string().c_str()));
        }
    } else {
        new_etype.desc.push_back("");
    }
    if(jo.has_member("reduced_desc")) {
        JsonArray jsarr = jo.get_array("reduced_desc");
        while (jsarr.has_more()) {
            new_etype.reduced_desc.push_back(_(jsarr.next_string().c_str()));
        }
    } else if (jo.has_member("desc")) {
        JsonArray jsarr = jo.get_array("desc");
        while (jsarr.has_more()) {
            new_etype.reduced_desc.push_back(_(jsarr.next_string().c_str()));
        }
    } else {
        new_etype.reduced_desc.push_back("");
    }
    new_etype.part_descs = jo.get_bool("part_descs", false);

    if(jo.has_member("rating")) {
        std::string r = jo.get_string("rating");
        if(r == "good") {
            new_etype.rating = e_good;
        } else if(r == "neutral" ) {
            new_etype.rating = e_neutral;
        } else if(r == "bad" ) {
            new_etype.rating = e_bad;
        } else if(r == "mixed" ) {
            new_etype.rating = e_mixed;
        } else {
            new_etype.rating = e_neutral;
        }
    } else {
        new_etype.rating = e_neutral;
    }
    new_etype.apply_message = jo.get_string("apply_message", "");
    new_etype.remove_message = jo.get_string("remove_message", "");
    new_etype.apply_memorial_log = jo.get_string("apply_memorial_log", "");
    new_etype.remove_memorial_log = jo.get_string("remove_memorial_log", "");
    
    new_etype.resist_trait = jo.get_string("resist_trait", "");
    new_etype.resist_effect = jo.get_string("resist_effect", "");
    new_etype.removes_effect = jo.get_string("removes_effect", "");
    
    new_etype.max_intensity = jo.get_int("max_intensity", 1);
    new_etype.max_duration = jo.get_int("max_duration", 0);
    new_etype.dur_add_perc = jo.get_int("dur_add_perc", 100);
    new_etype.int_add_val = jo.get_int("int_add_val", 0);
    new_etype.int_decay_step = jo.get_int("int_decay_step", -1);
    new_etype.int_decay_tick = jo.get_int("int_decay_tick", 0);
    new_etype.int_dur_factor = jo.get_int("int_dur_factor", 0);
    
    new_etype.load_miss_msgs(jo, "miss_messages");
    new_etype.load_decay_msgs(jo, "decay_messages");
    
    new_etype.main_parts_only = jo.get_bool("main_parts_only", false);
    new_etype.pkill_addict_reduces = jo.get_bool("pkill_addict_reduces", false);
    
    new_etype.pain_sizing = jo.get_bool("pain_sizing", false);
    new_etype.hurt_sizing = jo.get_bool("hurt_sizing", false);
    new_etype.harmful_cough = jo.get_bool("harmful_cough", false);
    
    new_etype.load_mod_data(jo, "base_mods");
    new_etype.load_mod_data(jo, "scaling_mods");

    effect_types[new_etype.id] = new_etype;
}

void reset_effect_types()
{
    effect_types.clear();
}

void effect::serialize(JsonOut &json) const
{
    json.start_object();
    json.member("eff_type", eff_type != NULL ? eff_type->id : "");
    json.member("duration", duration);
    json.member("bp", (int)bp);
    json.member("permanent", permanent);
    json.member("intensity", intensity);
    json.end_object();
}
void effect::deserialize(JsonIn &jsin)
{
    JsonObject jo = jsin.get_object();
    eff_type = &effect_types[jo.get_string("eff_type")];
    duration = jo.get_int("duration");
    bp = (body_part)jo.get_int("bp");
    permanent = jo.get_bool("permanent");
    intensity = jo.get_int("intensity");
}
