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

static void extract_effect_float( JsonObject &j, std::string mod_type, std::pair<float, float> &mod_value, std::string backup)
{
    if(j.has_member(mod_type)) {
        JsonArray jsarr = j.get_array(mod_type);
        mod_value.first = jsarr.get_float(0);
        if (jsarr.size() >= 2) {
            mod_value.second = jsarr.get_float(1);
        } else {
            mod_value.second = mod_value.first;
        }
    } else if (j.has_member(backup)) {
        JsonArray jsarr = j.get_array(backup);
        mod_value.first = jsarr.get_float(0);
        if (jsarr.size() >= 2) {
            mod_value.second = jsarr.get_float(1);
        } else {
            mod_value.second = mod_value.first;
        }
    }
}
static void extract_effect_int( JsonObject &j, std::string mod_type, std::pair<int, int> &mod_value, std::string backup)
{
    if(j.has_member(mod_type)) {
        JsonArray jsarr = j.get_array(mod_type);
        mod_value.first = jsarr.get_int(0);
        if (jsarr.size() >= 2) {
            mod_value.second = jsarr.get_int(1);
        } else {
            mod_value.second = mod_value.first;
        }
    } else if(j.has_member(backup)) {
        JsonArray jsarr = j.get_array(backup);
        mod_value.first = jsarr.get_int(0);
        if (jsarr.size() >= 2) {
            mod_value.second = jsarr.get_int(1);
        } else {
            mod_value.second = mod_value.first;
        }
    }
}

bool effect_mod_info::load(JsonObject &jsobj, std::string member) {
    if (jsobj.has_object(member)) {
        JsonObject j = jsobj.get_object(member);
        extract_effect_float(j, "str_mod", str_mod, "");
        extract_effect_float(j, "dex_mod", dex_mod, "");
        extract_effect_float(j, "per_mod", per_mod, "");
        extract_effect_float(j, "int_mod", int_mod, "");
        extract_effect_int(j, "speed_mod", speed_mod, "");
        
        extract_effect_int(j, "pain_amount", pain_amount, "");
        extract_effect_int(j, "pain_min", pain_min, "");
        extract_effect_int(j, "pain_max", pain_max, "pain_min");
        extract_effect_int(j, "pain_max_val", pain_max_val, "");
        extract_effect_int(j, "pain_chance", pain_chance_top, "");
        extract_effect_int(j, "pain_chance_bot", pain_chance_bot, "");
        extract_effect_int(j, "pain_tick", pain_tick, "");
        
        extract_effect_int(j, "hurt_amount", hurt_amount, "");
        extract_effect_int(j, "hurt_min", hurt_min, "");
        extract_effect_int(j, "hurt_max", hurt_max, "hurt_min");
        extract_effect_int(j, "hurt_chance", hurt_chance_top, "");
        extract_effect_int(j, "hurt_chance_bot", hurt_chance_bot, "");
        extract_effect_int(j, "hurt_tick", hurt_tick, "");
        
        extract_effect_int(j, "pkill_amount", pkill_amount, "");
        extract_effect_int(j, "pkill_min", pkill_min, "");
        extract_effect_int(j, "pkill_max", pkill_max, "pkill_min");
        extract_effect_int(j, "pkill_max_val", pkill_max_val, "");
        extract_effect_int(j, "pkill_chance", pkill_chance_top, "");
        extract_effect_int(j, "pkill_chance_bot", pkill_chance_bot, "");
        extract_effect_int(j, "pkill_tick", pkill_tick, "");
        
        extract_effect_int(j, "stim_amount", stim_amount, "");
        extract_effect_int(j, "stim_min", stim_min, "");
        extract_effect_int(j, "stim_max", stim_max, "stim_min");
        extract_effect_int(j, "stim_min_val", stim_min_val, "");
        extract_effect_int(j, "stim_max_val", stim_max_val, "");
        extract_effect_int(j, "stim_chance", stim_chance_top, "");
        extract_effect_int(j, "stim_chance_bot", stim_chance_bot, "");
        extract_effect_int(j, "stim_tick", stim_tick, "");
        
        extract_effect_int(j, "health_amount", health_amount, "");
        extract_effect_int(j, "health_min", health_min, "");
        extract_effect_int(j, "health_max", health_max, "health_min");
        extract_effect_int(j, "health_min_val", health_min_val, "");
        extract_effect_int(j, "health_max_val", health_max_val, "");
        extract_effect_int(j, "health_chance", health_chance_top, "");
        extract_effect_int(j, "health_chance_bot", health_chance_bot, "");
        extract_effect_int(j, "health_tick", health_tick, "");
        
        extract_effect_int(j, "h_mod_amount", h_mod_amount, "");
        extract_effect_int(j, "h_mod_min", h_mod_min, "");
        extract_effect_int(j, "h_mod_max", h_mod_max, "h_mod_min");
        extract_effect_int(j, "h_mod_min_val", h_mod_min_val, "");
        extract_effect_int(j, "h_mod_max_val", h_mod_max_val, "");
        extract_effect_int(j, "h_mod_chance", h_mod_chance_top, "");
        extract_effect_int(j, "h_mod_chance_bot", h_mod_chance_bot, "");
        extract_effect_int(j, "h_mod_tick", h_mod_tick, "");
        
        extract_effect_int(j, "rad_amount", rad_amount, "");
        extract_effect_int(j, "rad_min", rad_min, "");
        extract_effect_int(j, "rad_max", rad_max, "rad_min");
        extract_effect_int(j, "rad_max_val", rad_max_val, "");
        extract_effect_int(j, "rad_chance", rad_chance_top, "");
        extract_effect_int(j, "rad_chance_bot", rad_chance_bot, "");
        extract_effect_int(j, "rad_tick", rad_tick, "");
        
        extract_effect_int(j, "hunger_amount", hunger_amount, "");
        extract_effect_int(j, "hunger_min", hunger_min, "");
        extract_effect_int(j, "hunger_max", hunger_max, "hunger_min");
        extract_effect_int(j, "hunger_min_val", hunger_min_val, "");
        extract_effect_int(j, "hunger_max_val", hunger_max_val, "");
        extract_effect_int(j, "hunger_chance", hunger_chance_top, "");
        extract_effect_int(j, "hunger_chance_bot", hunger_chance_bot, "");
        extract_effect_int(j, "hunger_tick", hunger_tick, "");
        
        extract_effect_int(j, "thirst_amount", thirst_amount, "");
        extract_effect_int(j, "thirst_min", thirst_min, "");
        extract_effect_int(j, "thirst_max", thirst_max, "thirst_min");
        extract_effect_int(j, "thirst_min_val", thirst_min_val, "");
        extract_effect_int(j, "thirst_max_val", thirst_max_val, "");
        extract_effect_int(j, "thirst_chance", thirst_chance_top, "");
        extract_effect_int(j, "thirst_chance_bot", thirst_chance_bot, "");
        extract_effect_int(j, "thirst_tick", thirst_tick, "");   
        
        extract_effect_int(j, "fatigue_amount", fatigue_amount, "");
        extract_effect_int(j, "fatigue_min", fatigue_min, "");
        extract_effect_int(j, "fatigue_max", fatigue_max, "fatigue_min");
        extract_effect_int(j, "fatigue_min_val", fatigue_min_val, "");
        extract_effect_int(j, "fatigue_max_val", fatigue_max_val, "");
        extract_effect_int(j, "fatigue_chance", fatigue_chance_top, "");
        extract_effect_int(j, "fatigue_chance_bot", fatigue_chance_bot, "");
        extract_effect_int(j, "fatigue_tick", fatigue_tick, "");
        
        extract_effect_int(j, "cough_chance", cough_chance_top, "");
        extract_effect_int(j, "cough_chance_bot", cough_chance_bot, "");
        extract_effect_int(j, "cough_tick", cough_tick, "");
        
        extract_effect_int(j, "vomit_chance", vomit_chance_top, "");
        extract_effect_int(j, "vomit_chance_bot", vomit_chance_bot, "");
        extract_effect_int(j, "vomit_tick", vomit_tick, "");
        
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

bool effect_type::use_desc_ints() const
{
    return ((size_t)max_intensity <= desc.size());
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
        return m_neutral;  // should never happen
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
        return m_neutral;  // should never happen
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
    if (jo.has_object(member)) {
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
    if (jo.has_object(member)) {
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

effect::effect() :
    eff_type(NULL),
    duration(0),
    bp(num_bp),
    permanent(false),
    intensity(0)
{ }

effect::effect(effect_type *peff_type, int dur, body_part part, bool perm, int nintensity) :
    eff_type(peff_type),
    duration(dur),
    bp(part),
    permanent(perm),
    intensity(nintensity)
{ }

effect::effect(const effect &rhs) : JsonSerializer(), JsonDeserializer(),
    eff_type(rhs.eff_type),
    duration(rhs.duration),
    bp(rhs.bp),
    permanent(rhs.permanent),
    intensity(rhs.intensity)
{ }

effect &effect::operator=(const effect &rhs)
{
    if (this == &rhs) {
        return *this;    // No self-assignment
    }

    eff_type = rhs.eff_type;
    duration = rhs.duration;
    bp = rhs.bp;
    permanent = rhs.permanent;
    intensity = rhs.intensity;

    return *this;
}

std::string effect::disp_name()
{
    // End result should look like "name [L. Arm]"
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

std::string effect::disp_desc(bool reduced)
{
    std::stringstream ret;
    // First print stat changes
    int tmp = get_mod("STR", reduced);
    if (tmp > 0) {
        ret << string_format(_("Strength +%d;  "), tmp);
    } else if (tmp < 0) {
        ret << string_format(_("Strength %d;  "), tmp);
    }
    tmp = get_mod("DEX", reduced);
    if (tmp > 0) {
        ret << string_format(_("Dexterity +%d;  "), tmp);
    } else if (tmp < 0) {
        ret << string_format(_("Dexterity %d;  "), tmp);
    }
    tmp = get_mod("PER", reduced);
    if (tmp > 0) {
        ret << string_format(_("Perception +%d;  "), tmp);
    } else if (tmp < 0) {
        ret << string_format(_("Perception %d;  "), tmp);
    }
    tmp = get_mod("INT", reduced);
    if (tmp > 0) {
        ret << string_format(_("Intelligence +%d;  "), tmp);
    } else if (tmp < 0) {
        ret << string_format(_("Intelligence %d;  "), tmp);
    }
    tmp = get_mod("SPEED", reduced);
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
    double chances[8] = {get_percentage("PAIN", reduced), get_percentage("HURT", reduced),
                            get_percentage("THIRST", reduced), get_percentage("HUNGER", reduced),
                            get_percentage("FATIGUE", reduced), get_percentage("COUGH", reduced),
                            get_percentage("VOMIT", reduced), get_percentage("SLEEP", reduced)};
    int vals[8] = {get_mod("PAIN", reduced), get_mod("HURT", reduced), get_mod("THIRST", reduced),
                    get_mod("HUNGER", reduced), get_mod("FATIGUE", reduced),
                    get_mod("COUGH", reduced), get_mod("VOMIT", reduced)};
    std::string pos_strings[8] = {_("pain"),_("damage"),_("thirst"),_("hunger"),_("fatigue"),
                                _("coughing"),_("vomiting"),_("blackouts")};
    std::string neg_strings[8] = {_("pain"),_("damage"),_("quench"),_("sate"),_("rest"),
                                _("coughing"),_("vomiting"),_("blackouts")};
    for (int i = 0; i < 4; i++) {
        if (vals[i] > 0) {
            // +50% chance
            if (chances[i] >= 50) {
                constant.push_back(pos_strings[i]);
            // +1% chance
            } else if (chances[i] >= 1) {
                constant.push_back(pos_strings[i]);
            // +.4% chance
            } else if (chances[i] >= .4) {
                uncommon.push_back(pos_strings[i]);
            // <.4% chance
            } else if (chances[i] != 0) {
                rare.push_back(pos_strings[i]);
            }
        } else if (vals[i] < 0) {
            // +50% chance
            if (chances[i] >= 50) {
                constant.push_back(neg_strings[i]);
            // +1% chance
            } else if (chances[i] >= 1) {
                constant.push_back(neg_strings[i]);
            // +.4% chance
            } else if (chances[i] >= .4) {
                uncommon.push_back(neg_strings[i]);
            // <.4% chance
            } else if (chances[i] != 0) {
                rare.push_back(neg_strings[i]);
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
        //~ Used for effect descriptions, "Your (body_part_name) ..."
        ret << _("Your ") << body_part_name(bp).c_str() << " ";
    }
    std::string tmp_str;
    if (eff_type->use_desc_ints()) {
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
    if (player && tmp_int > intensity && eff_type->decay_msgs.size() > (size_t)intensity) {
        // -1 because intensity = 1 is the first message
        add_msg(eff_type->decay_msgs[intensity - 1].second, eff_type->decay_msgs[intensity - 1].first.c_str());
    }
    
    // Add to removal list if duration is <= 0
    if (duration <= 0) {
        rem_ids.push_back(get_id());
        rem_bps.push_back(bp);
    }
}

bool effect::use_part_descs()
{
    return eff_type->part_descs;
}

int effect::get_duration()
{
    return duration;
}
int effect::get_max_duration()
{
    return eff_type->max_duration;
}
void effect::set_duration(int dur)
{
    duration = dur;
}
void effect::mod_duration(int dur)
{
    duration += dur;
}
void effect::mult_duration(double dur)
{
    duration *= dur;
}

body_part effect::get_bp()
{
    return bp;
}
void effect::set_bp(body_part part)
{
    bp = part;
}

bool effect::is_permanent()
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

int effect::get_intensity()
{
    return intensity;
}
int effect::get_max_intensity()
{
    return eff_type->max_intensity;
}
void effect::set_intensity(int nintensity)
{
    intensity = nintensity;
    if (intensity > eff_type->max_intensity) {
        intensity = eff_type->max_intensity;
    } else if (intensity < 1) {
        intensity = 1;
    }
}
void effect::mod_intensity(int nintensity)
{
    intensity += nintensity;
    if (intensity > eff_type->max_intensity) {
        intensity = eff_type->max_intensity;
    } else if (intensity < 1) {
        intensity = 1;
    }
}

std::string effect::get_resist_trait()
{
    return eff_type->resist_trait;
}
std::string effect::get_resist_effect()
{
    return eff_type->resist_effect;
}
std::string effect::get_removes_effect()
{
    return eff_type->removes_effect;
}

int effect::get_mod(std::string arg, bool reduced)
{
    float ret = 0;
    auto &base = eff_type->base_mods;
    auto &scale = eff_type->scaling_mods;
    if (!reduced) {
        if (arg == "STR") {
            ret += base.str_mod.first;
            ret += scale.str_mod.first * (intensity - 1);
        } else if (arg == "DEX") {
            ret += base.dex_mod.first;
            ret += scale.dex_mod.first * (intensity - 1);
        } else if (arg == "PER") {
            ret += base.per_mod.first;
            ret += scale.per_mod.first * (intensity - 1);
        } else if (arg == "INT") {
            ret += base.int_mod.first;
            ret += scale.int_mod.first * (intensity - 1);
        } else if (arg == "SPEED") {
            ret += base.speed_mod.first;
            ret += scale.speed_mod.first * (intensity - 1);
        } else if (arg == "PAIN") {
            ret += rng(base.pain_min.first + scale.pain_min.first * (intensity - 1),
                    base.pain_max.first + scale.pain_max.first * (intensity - 1));
        } else if (arg == "HURT") {
            ret += rng(base.hurt_min.first + scale.hurt_min.first * (intensity - 1),
                    base.hurt_max.first + scale.hurt_max.first * (intensity - 1));
        } else if (arg == "SLEEP") {
            ret += rng(base.hurt_min.first + scale.sleep_min.first * (intensity - 1),
                    base.hurt_max.first + scale.sleep_max.first * (intensity - 1));
        } else if (arg == "PKILL") {
            ret += rng(base.pkill_min.first + scale.pkill_min.first * (intensity - 1),
                    base.pkill_max.first + scale.pkill_max.first * (intensity - 1));
        } else if (arg == "STIM") {
            ret += rng(base.stim_min.first + scale.stim_min.first * (intensity - 1),
                    base.stim_max.first + scale.stim_max.first * (intensity - 1));
        } else if (arg == "RAD") {
            ret += rng(base.rad_min.first + scale.rad_min.first * (intensity - 1),
                    base.rad_max.first + scale.rad_max.first * (intensity - 1));
        } else if (arg == "HUNGER") {
            ret += rng(base.hunger_min.first + scale.hunger_min.first * (intensity - 1),
                    base.hunger_max.first + scale.hunger_max.first * (intensity - 1));
        } else if (arg == "THIRST") {
            ret += rng(base.thirst_min.first + scale.thirst_min.first * (intensity - 1),
                    base.thirst_max.first + scale.thirst_max.first * (intensity - 1));
        } else if (arg == "FATIGUE") {
            ret += rng(base.fatigue_min.first + scale.fatigue_min.first * (intensity - 1),
                    base.fatigue_max.first + scale.fatigue_max.first * (intensity - 1));
        } else if (arg == "HEALTH") {
            ret += rng(base.health_min.first + scale.health_min.first * (intensity - 1),
                    base.health_max.first + scale.health_max.first * (intensity - 1));
        } else if (arg == "H_MOD") {
            ret += rng(base.h_mod_min.first + scale.h_mod_min.first * (intensity - 1),
                    base.h_mod_max.first + scale.h_mod_max.first * (intensity - 1));
        }
    } else {
        if (arg == "STR") {
            ret += base.str_mod.second;
            ret += scale.str_mod.second * (intensity - 1);
        } else if (arg == "DEX") {
            ret += base.dex_mod.second;
            ret += scale.dex_mod.second * (intensity - 1);
        } else if (arg == "PER") {
            ret += base.per_mod.second;
            ret += scale.per_mod.second * (intensity - 1);
        } else if (arg == "INT") {
            ret += base.int_mod.second;
            ret += scale.int_mod.second * (intensity - 1);
        } else if (arg == "SPEED") {
            ret += base.speed_mod.second;
            ret += scale.speed_mod.second * (intensity - 1);
        } else if (arg == "PAIN") {
            ret += rng(base.pain_min.second + scale.pain_min.second * (intensity - 1),
                    base.pain_max.second + scale.pain_max.second * (intensity - 1));
        } else if (arg == "HURT") {
            ret += rng(base.hurt_min.second + scale.hurt_min.second * (intensity - 1),
                    base.hurt_max.second + scale.hurt_max.second * (intensity - 1));
        } else if (arg == "SLEEP") {
            ret += rng(base.hurt_min.second + scale.sleep_min.second * (intensity - 1),
                    base.hurt_max.second + scale.sleep_max.second * (intensity - 1));
        } else if (arg == "PKILL") {
            ret += rng(base.pkill_min.second + scale.pkill_min.second * (intensity - 1),
                    base.pkill_max.second + scale.pkill_max.second * (intensity - 1));
        } else if (arg == "STIM") {
            ret += rng(base.stim_min.second + scale.stim_min.second * (intensity - 1),
                    base.stim_max.second + scale.stim_max.second * (intensity - 1));
        } else if (arg == "RAD") {
            ret += rng(base.rad_min.second + scale.rad_min.second * (intensity - 1),
                    base.rad_max.second + scale.rad_max.second * (intensity - 1));
        } else if (arg == "HUNGER") {
            ret += rng(base.hunger_min.second + scale.hunger_min.second * (intensity - 1),
                    base.hunger_max.second + scale.hunger_max.second * (intensity - 1));
        } else if (arg == "THIRST") {
            ret += rng(base.thirst_min.second + scale.thirst_min.second * (intensity - 1),
                    base.thirst_max.second + scale.thirst_max.second * (intensity - 1));
        } else if (arg == "FATIGUE") {
            ret += rng(base.fatigue_min.second + scale.fatigue_min.second * (intensity - 1),
                    base.fatigue_max.second + scale.fatigue_max.second * (intensity - 1));
        } else if (arg == "HEALTH") {
            ret += rng(base.health_min.second + scale.health_min.second * (intensity - 1),
                    base.health_max.second + scale.health_max.second * (intensity - 1));
        } else if (arg == "H_MOD") {
            ret += rng(base.h_mod_min.second + scale.h_mod_min.second * (intensity - 1),
                    base.h_mod_max.second + scale.h_mod_max.second * (intensity - 1));
        }
    }
    return int(ret);
}

int effect::get_amount(std::string arg, bool reduced)
{
    float ret = 0;
    auto &base = eff_type->base_mods;
    auto &scale = eff_type->scaling_mods;
    if (!reduced) {
        if (arg == "PAIN") {
            ret += base.pain_amount.first;
            ret += scale.pain_amount.first * (intensity - 1);
        } else if (arg == "HURT") {
            ret += base.hurt_amount.first;
            ret += scale.hurt_amount.first * (intensity - 1);
        } else if (arg == "SLEEP") {
            ret += base.sleep_amount.first;
            ret += scale.sleep_amount.first * (intensity - 1);
        } else if (arg == "PKILL") {
            ret += base.pkill_amount.first;
            ret += scale.pkill_amount.first * (intensity - 1);
        } else if (arg == "STIM") {
            ret += base.stim_amount.first;
            ret += scale.stim_amount.first * (intensity - 1);
        } else if (arg == "RAD") {
            ret += base.rad_amount.first;
            ret += scale.rad_amount.first * (intensity - 1);
        } else if (arg == "HUNGER") {
            ret += base.hunger_amount.first;
            ret += scale.hunger_amount.first * (intensity - 1);
        } else if (arg == "THIRST") {
            ret += base.thirst_amount.first;
            ret += scale.thirst_amount.first * (intensity - 1);
        } else if (arg == "FATIGUE") {
            ret += base.fatigue_amount.first;
            ret += scale.fatigue_amount.first * (intensity - 1);
        } else if (arg == "HEALTH") {
            ret += base.health_amount.first;
            ret += scale.health_amount.first * (intensity - 1);
        } else if (arg == "H_MOD") {
            ret += base.h_mod_amount.first;
            ret += scale.h_mod_amount.first * (intensity - 1);
        }
    } else {
        if (arg == "PAIN") {
            ret += base.pain_amount.second;
            ret += scale.pain_amount.second * (intensity - 1);
        } else if (arg == "HURT") {
            ret += base.hurt_amount.second;
            ret += scale.hurt_amount.second * (intensity - 1);
        } else if (arg == "SLEEP") {
            ret += base.sleep_amount.second;
            ret += scale.sleep_amount.second * (intensity - 1);
        } else if (arg == "PKILL") {
            ret += base.pkill_amount.second;
            ret += scale.pkill_amount.second * (intensity - 1);
        } else if (arg == "STIM") {
            ret += base.stim_amount.second;
            ret += scale.stim_amount.second * (intensity - 1);
        } else if (arg == "RAD") {
            ret += base.rad_amount.second;
            ret += scale.rad_amount.second * (intensity - 1);
        } else if (arg == "HUNGER") {
            ret += base.hunger_amount.second;
            ret += scale.hunger_amount.second * (intensity - 1);
        } else if (arg == "THIRST") {
            ret += base.thirst_amount.second;
            ret += scale.thirst_amount.second * (intensity - 1);
        } else if (arg == "FATIGUE") {
            ret += base.fatigue_amount.second;
            ret += scale.fatigue_amount.second * (intensity - 1);
        } else if (arg == "HEALTH") {
            ret += base.health_amount.second;
            ret += scale.health_amount.second * (intensity - 1);
        } else if (arg == "H_MOD") {
            ret += base.h_mod_amount.second;
            ret += scale.h_mod_amount.second * (intensity - 1);
        }
    }
    return int(ret);
}

int effect::get_min_val(std::string arg, bool reduced)
{
    float ret = 0;
    auto &base = eff_type->base_mods;
    auto &scale = eff_type->scaling_mods;
    if (!reduced) {
        if (arg == "STIM") {
            ret += base.stim_min_val.first;
            ret += scale.stim_min_val.first * (intensity - 1);
        } else if (arg == "HUNGER") {
            ret += base.hunger_min_val.first;
            ret += scale.hunger_min_val.first * (intensity - 1);
        } else if (arg == "THIRST") {
            ret += base.thirst_min_val.first;
            ret += scale.thirst_min_val.first * (intensity - 1);
        } else if (arg == "FATIGUE") {
            ret += base.fatigue_min_val.first;
            ret += scale.fatigue_min_val.first * (intensity - 1);
        } else if (arg == "HEALTH") {
            ret += base.health_min_val.first;
            ret += scale.health_min_val.first * (intensity - 1);
        } else if (arg == "H_MOD") {
            ret += base.h_mod_min_val.first;
            ret += scale.h_mod_min_val.first * (intensity - 1);
        }
    } else {
        if (arg == "STIM") {
            ret += base.stim_min_val.second;
            ret += scale.stim_min_val.second * (intensity - 1);
        } else if (arg == "HUNGER") {
            ret += base.hunger_min_val.second;
            ret += scale.hunger_min_val.second * (intensity - 1);
        } else if (arg == "THIRST") {
            ret += base.thirst_min_val.second;
            ret += scale.thirst_min_val.second * (intensity - 1);
        } else if (arg == "FATIGUE") {
            ret += base.fatigue_min_val.second;
            ret += scale.fatigue_min_val.second * (intensity - 1);
        } else if (arg == "HEALTH") {
            ret += base.health_min_val.second;
            ret += scale.health_min_val.second * (intensity - 1);
        } else if (arg == "H_MOD") {
            ret += base.h_mod_min_val.second;
            ret += scale.h_mod_min_val.second * (intensity - 1);
        }
    
    }
    return int(ret);
}

int effect::get_max_val(std::string arg, bool reduced)
{
    float ret = 0;
    auto &base = eff_type->base_mods;
    auto &scale = eff_type->scaling_mods;
    if (!reduced) {
        if (arg == "PAIN") {
            ret += base.pain_max_val.first;
            ret += scale.pain_max_val.first * (intensity - 1);
        } else if (arg == "PKILL") {
            ret += base.pkill_max_val.first;
            ret += scale.pkill_max_val.first * (intensity - 1);
        } else if (arg == "STIM") {
            ret += base.stim_max_val.first;
            ret += scale.stim_max_val.first * (intensity - 1);
        } else if (arg == "RAD") {
            ret += base.rad_max_val.first;
            ret += scale.rad_max_val.first * (intensity - 1);
        } else if (arg == "HUNGER") {
            ret += base.hunger_max_val.first;
            ret += scale.hunger_max_val.first * (intensity - 1);
        } else if (arg == "THIRST") {
            ret += base.thirst_max_val.first;
            ret += scale.thirst_max_val.first * (intensity - 1);
        } else if (arg == "FATIGUE") {
            ret += base.fatigue_max_val.first;
            ret += scale.fatigue_max_val.first * (intensity - 1);
        } else if (arg == "HEALTH") {
            ret += base.health_max_val.first;
            ret += scale.health_max_val.first * (intensity - 1);
        } else if (arg == "H_MOD") {
            ret += base.h_mod_max_val.first;
            ret += scale.h_mod_max_val.first * (intensity - 1);
        }
    } else {
        if (arg == "PAIN") {
            ret += base.pain_max_val.second;
            ret += scale.pain_max_val.second * (intensity - 1);
        } else if (arg == "PKILL") {
            ret += base.pkill_max_val.second;
            ret += scale.pkill_max_val.second * (intensity - 1);
        } else if (arg == "STIM") {
            ret += base.stim_max_val.second;
            ret += scale.stim_max_val.second * (intensity - 1);
        } else if (arg == "RAD") {
            ret += base.rad_max_val.second;
            ret += scale.rad_max_val.second * (intensity - 1);
        } else if (arg == "HUNGER") {
            ret += base.hunger_max_val.second;
            ret += scale.hunger_max_val.second * (intensity - 1);
        } else if (arg == "THIRST") {
            ret += base.thirst_max_val.second;
            ret += scale.thirst_max_val.second * (intensity - 1);
        } else if (arg == "FATIGUE") {
            ret += base.fatigue_max_val.second;
            ret += scale.fatigue_max_val.second * (intensity - 1);
        } else if (arg == "HEALTH") {
            ret += base.health_max_val.second;
            ret += scale.health_max_val.second * (intensity - 1);
        } else if (arg == "H_MOD") {
            ret += base.h_mod_max_val.second;
            ret += scale.h_mod_max_val.second * (intensity - 1);
        }
    
    }
    return int(ret);
}

bool effect::get_sizing(std::string arg)
{
    if (arg == "PAIN") {
        return eff_type->pain_sizing;
    } else if ("HURT") {
        return eff_type->hurt_sizing;
    }
    return false;
}

void effect::get_activation_vals(std::string arg, bool reduced, effect_mod_info base, effect_mod_info scale, 
                                double &tick, double &top_base, double &top_scale, double &bot_base,
                                double &bot_scale)
{
    // Get the tick, top, and bottom values for specific argument type
    if (!reduced) {
        if (arg == "PAIN") {
            tick = base.pain_tick.first + scale.pain_tick.first * (intensity - 1);
            top_base = base.pain_chance_top.first;
            top_scale = scale.pain_chance_top.first * (intensity - 1);
            bot_base = base.pain_chance_bot.first;
            bot_scale = scale.pain_chance_bot.first * (intensity - 1);
        } else if (arg == "HURT") {
            tick = base.hurt_tick.first + scale.hurt_tick.first * (intensity - 1);
            top_base = base.hurt_chance_top.first;
            top_scale = scale.hurt_chance_top.first * (intensity - 1);
            bot_base = base.hurt_chance_bot.first;
            bot_scale = scale.hurt_chance_bot.first * (intensity - 1);
        } else if (arg == "SLEEP") {
            tick = base.sleep_tick.first + scale.sleep_tick.first * (intensity - 1);
            top_base = base.sleep_chance_top.first;
            top_scale = scale.sleep_chance_top.first * (intensity - 1);
            bot_base = base.sleep_chance_bot.first;
            bot_scale = scale.sleep_chance_bot.first * (intensity - 1);
        } else if (arg == "PKILL") {
            tick = base.pkill_tick.first + scale.pkill_tick.first * (intensity - 1);
            top_base = base.pkill_chance_top.first;
            top_scale = scale.pkill_chance_top.first * (intensity - 1);
            bot_base = base.pkill_chance_bot.first;
            bot_scale = scale.pkill_chance_bot.first * (intensity - 1);
        } else if (arg == "STIM") {
            tick = base.stim_tick.first + scale.stim_tick.first * (intensity - 1);
            top_base = base.stim_chance_top.first;
            top_scale = scale.stim_chance_top.first * (intensity - 1);
            bot_base = base.stim_chance_bot.first;
            bot_scale = scale.stim_chance_bot.first * (intensity - 1);
        } else if (arg == "COUGH") {
            tick = base.cough_tick.first + scale.cough_tick.first * (intensity - 1);
            top_base = base.cough_chance_top.first;
            top_scale = scale.cough_chance_top.first * (intensity - 1);
            bot_base = base.cough_chance_bot.first;
            bot_scale = scale.cough_chance_bot.first * (intensity - 1);
        } else if (arg == "VOMIT") {
            tick = base.vomit_tick.first + scale.vomit_tick.first * (intensity - 1);
            top_base = base.vomit_chance_top.first;
            top_scale = scale.vomit_chance_top.first * (intensity - 1);
            bot_base = base.vomit_chance_bot.first;
            bot_scale = scale.vomit_chance_bot.first * (intensity - 1);
        } else if (arg == "RAD") {
            tick = base.rad_tick.first + scale.rad_tick.first * (intensity - 1);
            top_base = base.rad_chance_top.first;
            top_scale = scale.rad_chance_top.first * (intensity - 1);
            bot_base = base.rad_chance_bot.first;
            bot_scale = scale.rad_chance_bot.first * (intensity - 1);
        } else if (arg == "HUNGER") {
            tick = base.hunger_tick.first + scale.hunger_tick.first * (intensity - 1);
            top_base = base.hunger_chance_top.first;
            top_scale = scale.hunger_chance_top.first * (intensity - 1);
            bot_base = base.hunger_chance_bot.first;
            bot_scale = scale.hunger_chance_bot.first * (intensity - 1);
        } else if (arg == "THIRST") {
            tick = base.thirst_tick.first + scale.thirst_tick.first * (intensity - 1);
            top_base = base.thirst_chance_top.first;
            top_scale = scale.thirst_chance_top.first * (intensity - 1);
            bot_base = base.thirst_chance_bot.first;
            bot_scale = scale.thirst_chance_bot.first * (intensity - 1);
        } else if (arg == "FATIGUE") {
            tick = base.fatigue_tick.first + scale.fatigue_tick.first * (intensity - 1);
            top_base = base.fatigue_chance_top.first;
            top_scale = scale.fatigue_chance_top.first * (intensity - 1);
            bot_base = base.fatigue_chance_bot.first;
            bot_scale = scale.fatigue_chance_bot.first * (intensity - 1);
        } else if (arg == "HEALTH") {
            tick = base.health_tick.first + scale.health_tick.first * (intensity - 1);
            top_base = base.health_chance_top.first;
            top_scale = scale.health_chance_top.first * (intensity - 1);
            bot_base = base.health_chance_bot.first;
            bot_scale = scale.health_chance_bot.first * (intensity - 1);
        } else if (arg == "H_MOD") {
            tick = base.h_mod_tick.first + scale.h_mod_tick.first * (intensity - 1);
            top_base = base.h_mod_chance_top.first;
            top_scale = scale.h_mod_chance_top.first * (intensity - 1);
            bot_base = base.h_mod_chance_bot.first;
            bot_scale = scale.h_mod_chance_bot.first * (intensity - 1);
        }
    } else {
        if (arg == "PAIN") {
            tick = base.pain_tick.second + scale.pain_tick.second * (intensity - 1);
            top_base = base.pain_chance_top.second;
            top_scale = scale.pain_chance_top.second * (intensity - 1);
            bot_base = base.pain_chance_bot.second;
            bot_scale = scale.pain_chance_bot.second * (intensity - 1);
        } else if (arg == "HURT") {
            tick = base.hurt_tick.second + scale.hurt_tick.second * (intensity - 1);
            top_base = base.hurt_chance_top.second;
            top_scale = scale.hurt_chance_top.second * (intensity - 1);
            bot_base = base.hurt_chance_bot.second;
            bot_scale = scale.hurt_chance_bot.second * (intensity - 1);
        } else if (arg == "SLEEP") {
            tick = base.sleep_tick.second + scale.sleep_tick.second * (intensity - 1);
            top_base = base.sleep_chance_top.second;
            top_scale = scale.sleep_chance_top.second * (intensity - 1);
            bot_base = base.sleep_chance_bot.second;
            bot_scale = scale.sleep_chance_bot.second * (intensity - 1);
        } else if (arg == "PKILL") {
            tick = base.pkill_tick.second + scale.pkill_tick.second * (intensity - 1);
            top_base = base.pkill_chance_top.second;
            top_scale = scale.pkill_chance_top.second * (intensity - 1);
            bot_base = base.pkill_chance_bot.second;
            bot_scale = scale.pkill_chance_bot.second * (intensity - 1);
        } else if (arg == "STIM") {
            tick = base.stim_tick.second + scale.stim_tick.second * (intensity - 1);
            top_base = base.stim_chance_top.second;
            top_scale = scale.stim_chance_top.second * (intensity - 1);
            bot_base = base.stim_chance_bot.second;
            bot_scale = scale.stim_chance_bot.second * (intensity - 1);
        } else if (arg == "COUGH") {
            tick = base.cough_tick.second + scale.cough_tick.second * (intensity - 1);
            top_base = base.cough_chance_top.second;
            top_scale = scale.cough_chance_top.second * (intensity - 1);
            bot_base = base.cough_chance_bot.second;
            bot_scale = scale.cough_chance_bot.second * (intensity - 1);
        } else if (arg == "VOMIT") {
            tick = base.vomit_tick.second + scale.vomit_tick.second * (intensity - 1);
            top_base = base.vomit_chance_top.second;
            top_scale = scale.vomit_chance_top.second * (intensity - 1);
            bot_base = base.vomit_chance_bot.second;
            bot_scale = scale.vomit_chance_bot.second * (intensity - 1);
        } else if (arg == "RAD") {
            tick = base.rad_tick.second + scale.rad_tick.second * (intensity - 1);
            top_base = base.rad_chance_top.second;
            top_scale = scale.rad_chance_top.second * (intensity - 1);
            bot_base = base.rad_chance_bot.second;
            bot_scale = scale.rad_chance_bot.second * (intensity - 1);
        } else if (arg == "HUNGER") {
            tick = base.hunger_tick.second + scale.hunger_tick.second * (intensity - 1);
            top_base = base.hunger_chance_top.second;
            top_scale = scale.hunger_chance_top.second * (intensity - 1);
            bot_base = base.hunger_chance_bot.second;
            bot_scale = scale.hunger_chance_bot.second * (intensity - 1);
        } else if (arg == "THIRST") {
            tick = base.thirst_tick.second + scale.thirst_tick.second * (intensity - 1);
            top_base = base.thirst_chance_top.second;
            top_scale = scale.thirst_chance_top.second * (intensity - 1);
            bot_base = base.thirst_chance_bot.second;
            bot_scale = scale.thirst_chance_bot.second * (intensity - 1);
        } else if (arg == "FATIGUE") {
            tick = base.fatigue_tick.second + scale.fatigue_tick.second * (intensity - 1);
            top_base = base.fatigue_chance_top.second;
            top_scale = scale.fatigue_chance_top.second * (intensity - 1);
            bot_base = base.fatigue_chance_bot.second;
            bot_scale = scale.fatigue_chance_bot.second * (intensity - 1);
        } else if (arg == "HEALTH") {
            tick = base.health_tick.second + scale.health_tick.second * (intensity - 1);
            top_base = base.health_chance_top.second;
            top_scale = scale.health_chance_top.second * (intensity - 1);
            bot_base = base.health_chance_bot.second;
            bot_scale = scale.health_chance_bot.second * (intensity - 1);
        } else if (arg == "H_MOD") {
            tick = base.h_mod_tick.second + scale.h_mod_tick.second * (intensity - 1);
            top_base = base.h_mod_chance_top.second;
            top_scale = scale.h_mod_chance_top.second * (intensity - 1);
            bot_base = base.h_mod_chance_bot.second;
            bot_scale = scale.h_mod_chance_bot.second * (intensity - 1);
        }
    }
}

double effect::get_percentage(std::string arg, bool reduced)
{
    double tick = 0;
    double top_base = 0;
    double top_scale = 0;
    double bot_base = 0;
    double bot_scale = 0;
    auto &base = eff_type->base_mods;
    auto &scale = eff_type->scaling_mods;
    get_activation_vals(arg, reduced, base, scale, tick, top_base, top_scale, bot_base, bot_scale);
    // If both top values = 0 then it should never trigger
    if (top_base <= 0 && top_scale <= 0) {
        return 0;
    }
    // It will also never trigger if top_base + top_scale <= 0
    if (top_base + top_scale <= 0) {
        return 0;
    }

    double ret = 0;
    // If both bot values are zero the formula is one_in(top), else the formula is x_in_y(top, bot)
    if(bot_base != 0 && bot_scale != 0) {
        if (bot_base + bot_scale == 0) {
            // Special crash avoidance case, assume bot = 1 which means x_in_y(top, bot) = true
            ret = 1;
        } else {
            ret = (top_base + top_scale) / (bot_base + bot_scale);
        }
    } else {
        ret = 1 / (top_base + top_scale);
    }
    // Divide by ticks between rolls
    if (tick > 1) {
        ret = ret / tick;
    }
    // Multiply by 100 to convert to percentages
    ret *= 100;
    return ret;
}

bool effect::activated(unsigned int turn, std::string arg, bool reduced, double mod)
{
    double dtick = 0;
    double dtop_base = 0;
    double dtop_scale = 0;
    double dbot_base = 0;
    double dbot_scale = 0;
    auto &base = eff_type->base_mods;
    auto &scale = eff_type->scaling_mods;
    get_activation_vals(arg, reduced, base, scale, dtick, dtop_base, dtop_scale, dbot_base, dbot_scale);
    // Convert to ints
    int tick = dtick;
    int top_base = dtop_base;
    int top_scale = dtop_scale;
    int bot_base = dbot_base;
    int bot_scale = dbot_scale;
    // If both top values <= 0 then it should never trigger
    if (top_base <= 0 && top_scale <= 0) {
        return false;
    }
    // It will also never trigger if top_base + top_scale <= 0
    if (top_base + top_scale <= 0) {
        return false;
    }

    // Else check if tick allows for triggering. If both bot values are zero the formula is 
    // x_in_y(1, top) i.e. one_in(top), else the formula is x_in_y(top, bot),
    // mod multiplies the overall percentage chances
    if(tick <= 0 || turn % tick == 0) {
        if(bot_base != 0 && bot_scale != 0) {
            if (bot_base + bot_scale == 0) {
                // Special crash avoidance case, assume bot = 1 which means x_in_y(top, bot) = true
                return true;
            } else {
                return x_in_y((top_base + top_scale) * mod, (bot_base + bot_scale));
            }
        } else {
            return x_in_y(mod, top_base + top_scale);
        }
    }
    return false;
}

double effect::get_addict_mod(std::string arg, int addict_level)
{
    // TODO: convert this to JSON id's and values once we have JSON'ed addictions
    if (arg == "PKILL") {
        if (pkill_addict_reduces) {
            return 1 / (addict_level * 2);
        } else {
            return 1;
        }
    } else {
        return 1;
    }
    return 1;
}

bool effect::get_harmful_cough()
{
    return eff_type->harmful_cough;
}
int effect::get_dur_add_perc()
{
    return eff_type->dur_add_perc;
}
int effect::get_int_add_val()
{
    return eff_type->int_add_val;
}

std::vector<std::pair<std::string, int>> effect::get_miss_msgs()
{
    return eff_type->miss_msgs;
}
std::string effect::get_speed_name()
{
    if (eff_type->speed_mod_name == "") {
        return eff_type->name[0];
    } else {
        return eff_type->speed_mod_name;
    }
}

effect_type *effect::get_effect_type()
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

    new_etype.permanent = jo.get_bool("permanent", false);
    
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
    
    new_etype.base_mods.load(jo, "base_mods");
    new_etype.scaling_mods.load(jo, "scaling_mods");

    effect_types[new_etype.id] = new_etype;
}

void reset_effect_types()
{
    effect_types.clear();
}
