#include "effect.h"
#include "rng.h"
#include "output.h"
#include <map>
#include <sstream>

std::map<std::string, effect_type> effect_types;

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
        extract_effect_int(j, "pkill_chance", pkill_chance_bot, "");
        extract_effect_int(j, "pkill_tick", pkill_tick, "");
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
int effect_type::get_max_intensity() const
{
    return max_intensity;
}
bool effect_type::get_main_parts() const
{
    return main_parts_only;
}

effect::effect() :
    eff_type(NULL),
    duration(0),
    bp(num_bp),
    permanent(false),
    intensity(0)
{ }

effect::effect(effect_type *peff_type, int dur, body_part part, int nintensity, bool perm) :
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
    
    if (ret.str() != "") {
        ret << "\n";
    }
    if (use_part_descs()) {
        // Used for effect descriptions, "Your body_part_name is ..."
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

void decay(std::vector<std::string> &rem_ids, std::vector<body_part> &rem_bps, unsigned int turn)
{
    if (!is_permanent()) {
        duration -= 1;
        add_msg( m_debug, "ID: %s, Duration %d", get_id(), duration )
    }
    if (intensity < 1) {
        add_msg( m_debug, "Bad intensity, ID: %s", get_id() )
        intensity = 1;
    } else if (intensity > 1) {
        if (eff_type->int_decay_tick != 0 && turn % eff_type->int_decay_tick == 0) {
            intensity += eff_type->int_decay_step;
        }
    }
    if (intensity > eff_type->max_intensity) {
        intensity = eff_type->max_intensity;
    }
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
void effect::set_duration(int dur)
{
    duration = dur;
}
void effect::mod_duration(int dur)
{
    duration += dur;
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
    return eff_type->get_max_intensity();
}
void effect::set_intensity(int nintensity)
{
    intensity = nintensity;
}
void effect::mod_intensity(int nintensity)
{
    intensity += nintensity;
}

std::string effect::get_resist_trait()
{
    return eff_type->resist_trait;
}

int effect::get_mod(std::string arg, bool reduced)
{
    float ret = 0;
    if (!reduced) {
        if (arg == "STR") {
            ret += eff_type->base_mods.str_mod.first;
            ret += eff_type->scaling_mods.str_mod.first * intensity;
        } else if (arg == "DEX") {
            ret += eff_type->base_mods.dex_mod.first;
            ret += eff_type->scaling_mods.dex_mod.first * intensity;
        } else if (arg == "PER") {
            ret += eff_type->base_mods.per_mod.first;
            ret += eff_type->scaling_mods.per_mod.first * intensity;
        } else if (arg == "INT") {
            ret += eff_type->base_mods.int_mod.first;
            ret += eff_type->scaling_mods.int_mod.first * intensity;
        } else if (arg == "SPEED") {
            ret += eff_type->base_mods.speed_mod.first;
            ret += eff_type->scaling_mods.speed_mod.first * intensity;
        } else if (arg == "PAIN") {
            ret += rng(eff_type->base_mods.pain_min.first + eff_type->scaling_mods.pain_min.first * intensity,
                    eff_type->base_mods.pain_max.first + eff_type->scaling_mods.pain_max.first * intensity);
        } else if (arg == "HURT") {
            ret += rng(eff_type->base_mods.hurt_min.first + eff_type->scaling_mods.hurt_min.first * intensity,
                    eff_type->base_mods.hurt_max.first + eff_type->scaling_mods.hurt_max.first * intensity);
        } else if (arg == "PKILL") {
            ret += rng(eff_type->base_mods.pkill_min.first + eff_type->scaling_mods.pkill_min.first * intensity,
                    eff_type->base_mods.pkill_max.first + eff_type->scaling_mods.pkill_max.first * intensity);
        }
    } else {
        if (arg == "STR") {
            ret += eff_type->base_mods.str_mod.second;
            ret += eff_type->scaling_mods.str_mod.second * intensity;
        } else if (arg == "DEX") {
            ret += eff_type->base_mods.dex_mod.second;
            ret += eff_type->scaling_mods.dex_mod.second * intensity;
        } else if (arg == "PER") {
            ret += eff_type->base_mods.per_mod.second;
            ret += eff_type->scaling_mods.per_mod.second * intensity;
        } else if (arg == "INT") {
            ret += eff_type->base_mods.int_mod.second;
            ret += eff_type->scaling_mods.int_mod.second * intensity;
        } else if (arg == "SPEED") {
            ret += eff_type->base_mods.speed_mod.second;
            ret += eff_type->scaling_mods.speed_mod.second * intensity;
        } else if (arg == "PAIN") {
            ret += rng(eff_type->base_mods.pain_min.second + eff_type->scaling_mods.pain_min.second * intensity,
                    eff_type->base_mods.pain_max.second + eff_type->scaling_mods.pain_max.second * intensity);
        } else if (arg == "HURT") {
            ret += rng(eff_type->base_mods.hurt_min.second + eff_type->scaling_mods.hurt_min.second * intensity,
                    eff_type->base_mods.hurt_max.second + eff_type->scaling_mods.hurt_max.second * intensity);
        } else if (arg == "PKILL") {
            ret += rng(eff_type->base_mods.pkill_min.second + eff_type->scaling_mods.pkill_min.second * intensity,
                    eff_type->base_mods.pkill_max.second + eff_type->scaling_mods.pkill_max.second * intensity);
        }
    }
    return int(ret);
}

int effect::get_amount(std::string arg, bool reduced)
{
    float ret = 0;
    if (!reduced) {
        if (arg == "PAIN") {
            ret += eff_type->base_mods.pain_amount.first;
            ret += eff_type->scaling_mods.pain_amount.first * intensity;
        } else if (arg == "HURT") {
            ret += eff_type->base_mods.hurt_amount.first;
            ret += eff_type->scaling_mods.hurt_amount.first * intensity;
        } else if (arg == "PKILL") {
            ret += eff_type->base_mods.pkill_amount.first;
            ret += eff_type->scaling_mods.pkill_amount.first * intensity;
        }
    } else {
        if (arg == "PAIN") {
            ret += eff_type->base_mods.pain_amount.second;
            ret += eff_type->scaling_mods.pain_amount.second * intensity;
        } else if (arg == "HURT") {
            ret += eff_type->base_mods.hurt_amount.second;
            ret += eff_type->scaling_mods.hurt_amount.second * intensity;
        } else if (arg == "PKILL") {
            ret += eff_type->base_mods.pkill_amount.second;
            ret += eff_type->scaling_mods.pkill_amount.second * intensity;
        }
    }
    return int(ret);
}

int effect::get_max_val(std::string arg, bool reduced)
{
    float ret = 0;
    if (!reduced) {
        if (arg == "PAIN") {
            ret += eff_type->base_mods.pain_max_val.first;
            ret += eff_type->scaling_mods.pain_max_val.first * intensity;
        } else if (arg == "PKILL") {
            ret += eff_type->base_mods.pkill_max_val.first;
            ret += eff_type->scaling_mods.pkill_max_val.first * intensity;
        }
    } else {
        if (arg == "PAIN") {
            ret += eff_type->base_mods.pain_max_val.second;
            ret += eff_type->scaling_mods.pain_max_val.second * intensity;
        } else if (arg == "PKILL") {
            ret += eff_type->base_mods.pkill_max_val.second;
            ret += eff_type->scaling_mods.pkill_max_val.second * intensity;
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

bool effect::activated(unsigned int turn, std::string arg, bool reduced, double mod)
{
    int tick = 0;
    int bot_base = 0;
    int bot_scale = 0;
    int top = 0;
    if (!reduced) {
        if (arg == "PAIN") {
            tick = eff_type->base_mods.pain_tick.first + eff_type->scaling_mods.pain_tick.first * intensity;
            bot_base = eff_type->base_mods.pain_chance_bot.first;
            bot_scale = eff_type->scaling_mods.pain_chance_bot.first * intensity;
            top = eff_type->base_mods.pain_chance_top.first + eff_type->scaling_mods.pain_chance_top.first * intensity;
        } else if (arg == "HURT") {
            tick = eff_type->base_mods.hurt_tick.first + eff_type->scaling_mods.hurt_tick.first * intensity;
            bot_base = eff_type->base_mods.hurt_chance_bot.first;
            bot_scale = eff_type->scaling_mods.hurt_chance_bot.first * intensity;
            top = eff_type->base_mods.hurt_chance_top.first + eff_type->scaling_mods.hurt_chance_top.first * intensity;
        } else if (arg == "PKILL") {
            tick = eff_type->base_mods.pkill_tick.first + eff_type->scaling_mods.pkill_tick.first * intensity;
            bot_base = eff_type->base_mods.pkill_chance_bot.first;
            bot_scale = eff_type->scaling_mods.pkill_chance_bot.first * intensity;
            top = eff_type->base_mods.pkill_chance_top.first + eff_type->scaling_mods.pkill_chance_top.first * intensity;
        } else if (arg == "COUGH") {
            tick = eff_type->base_mods.cough_tick.first + eff_type->scaling_mods.cough_tick.first * intensity;
            bot_base = eff_type->base_mods.cough_chance_bot.first;
            bot_scale = eff_type->scaling_mods.cough_chance_bot.first * intensity;
            top = eff_type->base_mods.cough_chance_top.first + eff_type->scaling_mods.cough_chance_top.first * intensity;
        } else if (arg == "VOMIT") {
            tick = eff_type->base_mods.vomit_tick.first + eff_type->scaling_mods.vomit_tick.first * intensity;
            bot_base = eff_type->base_mods.vomit_chance_bot.first;
            bot_scale = eff_type->scaling_mods.vomit_chance_bot.first * intensity;
            top = eff_type->base_mods.vomit_chance_top.first + eff_type->scaling_mods.vomit_chance_top.first * intensity;
        }
    } else {
        if (arg == "PAIN") {
            tick = eff_type->base_mods.pain_tick.second + eff_type->scaling_mods.pain_tick.second * intensity;
            bot_base = eff_type->base_mods.pain_chance_bot.second;
            bot_scale = eff_type->scaling_mods.pain_chance_bot.second * intensity;
            top = eff_type->base_mods.pain_chance_top.second + eff_type->scaling_mods.pain_chance_top.second * intensity;
        } else if (arg == "HURT") {
            tick = eff_type->base_mods.hurt_tick.second + eff_type->scaling_mods.hurt_tick.second * intensity;
            bot_base = eff_type->base_mods.hurt_chance_bot.second;
            bot_scale = eff_type->scaling_mods.hurt_chance_bot.second * intensity;
            top = eff_type->base_mods.hurt_chance_top.second + eff_type->scaling_mods.hurt_chance_top.second * intensity;
        } else if (arg == "PKILL") {
            tick = eff_type->base_mods.pkill_tick.second + eff_type->scaling_mods.pkill_tick.second * intensity;
            bot_base = eff_type->base_mods.pkill_chance_bot.second;
            bot_scale = eff_type->scaling_mods.pkill_chance_bot.second * intensity;
            top = eff_type->base_mods.pkill_chance_top.second + eff_type->scaling_mods.pkill_chance_top.second * intensity;
        } else if (arg == "COUGH") {
            tick = eff_type->base_mods.cough_tick.second + eff_type->scaling_mods.cough_tick.second * intensity;
            bot_base = eff_type->base_mods.cough_chance_bot.second;
            bot_scale = eff_type->scaling_mods.cough_chance_bot.second * intensity;
            top = eff_type->base_mods.cough_chance_top.second + eff_type->scaling_mods.cough_chance_top.second * intensity;
        } else if (arg == "VOMIT") {
            tick = eff_type->base_mods.vomit_tick.second + eff_type->scaling_mods.vomit_tick.second * intensity;
            bot_base = eff_type->base_mods.vomit_chance_bot.second;
            bot_scale = eff_type->scaling_mods.vomit_chance_bot.second * intensity;
            top = eff_type->base_mods.vomit_chance_top.second + eff_type->scaling_mods.vomit_chance_top.second * intensity;
        }
    }
    if (bot_base == 0 && bot_scale == 0) {
        return false;
    }

    if(tick <= 0 || turn % tick == 0) {
        if(bot_base + bot_scale > 0) {
            return x_in_y((bot_base + bot_scale) * mod, top);
        } else {
            return one_in(top * mod);
        }
    }
    return false;
}

double effect::get_addict_mod(std::string arg, int addict_level)
{
    // TODO: convert this to JSON id's and values once we have JSON'ed addictions
    if (arg == "PKILL") {
        return 1 / (addict_level * 2);
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

    new_etype.permanent = jo.get_bool("permanent", false);
    
    new_etype.max_intensity = jo.get_int("max_intensity", 1);
    new_etype.dur_add_perc = jo.get_int("dur_add_perc", 100);
    new_etype.int_add_val = jo.get_int("int_add_val", 1);
    new_etype.int_decay_step = jo.get_int("int_decay_step", 0);
    new_etype.int_decay_tick = jo.get_int("int_decay_tick", 0);
    
    new_etype.main_parts_only = jo.get_bool("main_parts_only", false);
    new_etype.pkill_addict_reduces = jo.get_bool("pkill_addict_reduces", false);
    
    new_etype.pain_sizing = jo.get_bool("pain_sizing", false);
    new_etype.hurt_sizing = jo.get_bool("pain_sizing", false);
    new_etype.harmful_cough = jo.get_bool("harmful_cough", false);
    
    new_etype.base_mods.load(jo, "base_mods");
    new_etype.scaling_mods.load(jo, "scaling_mods");

    effect_types[new_etype.id] = new_etype;
}

void reset_effect_types()
{
    effect_types.clear();
}
