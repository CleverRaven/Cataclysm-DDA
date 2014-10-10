#include "effect.h"
#include "rng.h"
#include "output.h"
#include <map>
#include <sstream>

std::map<std::string, effect_type> effect_types;

bool effect_mod_info::load(JsonObject &jsobj, std::string member) {
    if jsobj.has_object(member) {
        JsonObject j = jsobj.get_object(member);
        if(j.has_member("str_mod")) {
            JsonArray jsarr = j.get_array("str_mod");
            str_mod.first = jsarr.get_float(0);
            if (jsarr.size() >= 2) {
                str_mod.second = jsarr.get_float(1);
            } else {
                str_mod.second = str_mod.first;
            }
        }
        if(j.has_member("dex_mod")) {
            JsonArray jsarr = j.get_array("dex_mod");
            dex_mod.first = jsarr.get_float(0);
            if (jsarr.size() >= 2) {
                dex_mod.second = jsarr.get_float(1);
            } else {
                dex_mod.second = dex_mod.first;
            }
        }
        if(j.has_member("per_mod")) {
            JsonArray jsarr = j.get_array("per_mod");
            per_mod.first = jsarr.get_float(0);
            if (jsarr.size() >= 2) {
                per_mod.second = jsarr.get_float(1);
            } else {
                per_mod.second = per_mod.first;
            }
        }
        if(j.has_member("int_mod")) {
            JsonArray jsarr = j.get_array("int_mod");
            int_mod.first = jsarr.get_float(0);
            if (jsarr.size() >= 2) {
                int_mod.second = jsarr.get_float(1);
            } else {
                int_mod.second = int_mod.first;
            }
        }
        if(j.has_member("speed_mod")) {
            JsonArray jsarr = j.get_array("speed_mod");
            speed_mod.first = jsarr.get_float(0);
            if (jsarr.size() >= 2) {
                speed_mod.second = jsarr.get_float(1);
            } else {
                speed_mod.second = speed_mod.first;
            }
        }
        if(j.has_member("pain_amount")) {
            JsonArray jsarr = j.get_array("pain_amount");
            pain_amount.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                pain_amount.second = jsarr.get_int(1);
            } else {
                pain_amount.second = pain_amount.first;
            }
        }
        if(j.has_member("pain_min")) {
            JsonArray jsarr = j.get_array("pain_min");
            pain_min.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                pain_min.second = jsarr.get_int(1);
            } else {
                pain_min.second = pain_min.first;
            }
        }
        if(j.has_member("pain_max")) {
            JsonArray jsarr = j.get_array("pain_max");
            pain_max.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                pain_max.second = jsarr.get_int(1);
            } else {
                pain_max.second = pain_max.first;
            }
        }
        if(j.has_member("pain_max_val")) {
            JsonArray jsarr = j.get_array("pain_max_val");
            pain_max_val.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                pain_max_val.second = jsarr.get_int(1);
            } else {
                pain_max_val.second = pain_max_val.first;
            }
        }
        if(j.has_member("pain_chance")) {
            JsonArray jsarr = j.get_array("pain_chance");
            pain_chance_top.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                pain_chance_top.second = jsarr.get_int(1);
            } else {
                pain_chance_top.second = pain_chance_top.first;
            }
        }
        if(j.has_member("pain_chance_bot")) {
            JsonArray jsarr = j.get_array("pain_chance_bot");
            pain_chance_bot.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                pain_chance_bot.second = jsarr.get_int(1);
            } else {
                pain_chance_bot.second = pain_chance_bot.first;
            }
        }
        if(j.has_member("pain_tick")) {
            JsonArray jsarr = j.get_array("pain_tick");
            pain_tick.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                pain_tick.second = jsarr.get_int(1);
            } else {
                pain_tick.second = pain_tick.first;
            }
        }
        if(j.has_member("hurt_amount")) {
            JsonArray jsarr = j.get_array("hurt_amount");
            hurt_amount.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                hurt_amount.second = jsarr.get_int(1);
            } else {
                hurt_amount.second = hurt_amount.first;
            }
        }
        if(j.has_member("hurt_min")) {
            JsonArray jsarr = j.get_array("hurt_min");
            hurt_min.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                hurt_min.second = jsarr.get_int(1);
            } else {
                hurt_min.second = hurt_min.first;
            }
        }
        if(j.has_member("hurt_max")) {
            JsonArray jsarr = j.get_array("hurt_max");
            hurt_max.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                hurt_max.second = jsarr.get_int(1);
            } else {
                hurt_max.second = hurt_max.first;
            }
        }
        if(j.has_member("hurt_chance")) {
            JsonArray jsarr = j.get_array("hurt_chance");
            hurt_chance_top.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                hurt_chance_top.second = jsarr.get_int(1);
            } else {
                hurt_chance_top.second = hurt_chance_top.first;
            }
        }
        if(j.has_member("hurt_chance_bot")) {
            JsonArray jsarr = j.get_array("hurt_chance_bot");
            hurt_chance_bot.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                hurt_chance_bot.second = jsarr.get_int(1);
            } else {
                hurt_chance_bot.second = hurt_chance_bot.first;
            }
        }
        if(j.has_member("hurt_tick")) {
            JsonArray jsarr = j.get_array("hurt_tick");
            hurt_tick.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                hurt_tick.second = jsarr.get_int(1);
            } else {
                hurt_tick.second = hurt_tick.first;
            }
        }
        if(j.has_member("pkill_amount")) {
            JsonArray jsarr = j.get_array("pkill_amount");
            pkill_amount.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                pkill_amount.second = jsarr.get_int(1);
            } else {
                pkill_amount.second = pkill_amount.first;
            }
        }
        if(j.has_member("pkill_min")) {
            JsonArray jsarr = j.get_array("pkill_min");
            pkill_min.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                pkill_min.second = jsarr.get_int(1);
            } else {
                pkill_min.second = pkill_min.first;
            }
        }
        if(j.has_member("pkill_max")) {
            JsonArray jsarr = j.get_array("pkill_max");
            pkill_max.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                pkill_max.second = jsarr.get_int(1);
            } else {
                pkill_max.second = pkill_max.first;
            }
        }
        if(j.has_member("pkill_max_val")) {
            JsonArray jsarr = j.get_array("pkill_max_val");
            pkill_max_val.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                pkill_max_val.second = jsarr.get_int(1);
            } else {
                pkill_max_val.second = pkill_max_val.first;
            }
        }
        if(j.has_member("pkill_chance")) {
            JsonArray jsarr = j.get_array("pkill_chance");
            pkill_chance_top.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                pkill_chance_top.second = jsarr.get_int(1);
            } else {
                pkill_chance_top.second = pkill_chance_top.first;
            }
        }
        if(j.has_member("pkill_chance_bot")) {
            JsonArray jsarr = j.get_array("pkill_chance_bot");
            pkill_chance_bot.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                pkill_chance_bot.second = jsarr.get_int(1);
            } else {
                pkill_chance_bot = pkill_chance_bot.first;
            }
        }
        if(j.has_member("pkill_tick")) {
            JsonArray jsarr = j.get_array("pkill_tick");
            pkill_tick.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                pkill_tick.second = jsarr.get_int(1);
            } else {
                pkill_tick.second = pkill_tick.first;
            }
        }
        if(j.has_member("cough_chance")) {
            JsonArray jsarr = j.get_array("cough_chance");
            cough_chance_top.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                cough_chance_top.second = jsarr.get_int(1);
            } else {
                cough_chance_top.second = cough_chance_top.first;
            }
        }
        if(j.has_member("cough_chance_bot")) {
            JsonArray jsarr = j.get_array("cough_chance_bot");
            cough_chance_bot.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                cough_chance_bot.second = jsarr.get_int(1);
            } else {
                cough_chance_bot.second = cough_chance_bot.first;
            }
        }
        if(j.has_member("cough_tick")) {
            JsonArray jsarr = j.get_array("cough_tick");
            cough_tick.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                cough_tick.second = jsarr.get_int(1);
            } else {
                cough_tick.second = cough_tick.first;
            }
        }
        if(j.has_member("vomit_chance")) {
            JsonArray jsarr = j.get_array("vomit_chance");
            vomit_chance_top.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                vomit_chance_top.second = jsarr.get_int(1);
            } else {
                vomit_chance_top.second = vomit_chance_top.first;
            }
        }
        if(j.has_member("vomit_chance_bot")) {
            JsonArray jsarr = j.get_array("vomit_chance_bot");
            vomit_chance_bot.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                vomit_chance_bot.second = jsarr.get_int(1);
            } else {
                vomit_chance_bot.second = vomit_chance_bot.first;
            }
        }
        if(j.has_member("vomit_tick")) {
            JsonArray jsarr = j.get_array("vomit_tick");
            vomit_tick.first = jsarr.get_int(0);
            if (jsarr.size() >= 2) {
                vomit_tick.second = jsarr.get_int(1);
            } else {
                vomit_tick.second = vomit_tick.first;
            }
        }
    }
}

effect_type::effect_type() {}
effect_type::effect_type(const effect_type &) {}

effect_rating effect_type::get_rating() const
{
    return rating;
}

bool effect_type::use_name_ints()
{
    return ((size_t)max_intensity <= name.size());
}

bool effect_type::use_desc_ints()
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

effect::effect() :
    eff_type(NULL),
    duration(0),
    bp(num_bp),
    intensity(0),
    permanent(false)
{ }

effect::effect(effect_type *peff_type, int dur, body_part part, int nintensity, bool perm) :
    eff_type(peff_type),
    duration(dur),
    bp(part),
    intensity(nintensity),
    permanent(perm)
{ }

effect::effect(const effect &rhs) : JsonSerializer(), JsonDeserializer(),
    eff_type(rhs.eff_type),
    duration(rhs.duration),
    bp(rhs.bp),
    intensity(rhs.intensity),
    permanent(rhs.permanent)
{ }

effect &effect::operator=(const effect &rhs)
{
    if (this == &rhs) {
        return *this;    // No self-assignment
    }

    eff_type = rhs.eff_type;
    duration = rhs.duration;
    bp = rhs.bp;
    intensity = rhs.intensity;
    permanent = rhs.permanent;

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
    int tmp = get_str_mod(reduced);
    if (tmp > 0) {
        ret << string_format(_("Strength +%d;  "), tmp);
    } else if (tmp < 0) {
        ret << string_format(_("Strength %d;  "), tmp);
    }
    tmp = get_dex_mod(reduced);
    if (tmp > 0) {
        ret << string_format(_("Dexterity +%d;  "), tmp);
    } else if (tmp < 0) {
        ret << string_format(_("Dexterity %d;  "), tmp);
    }
    tmp = get_per_mod(reduced);
    if (tmp > 0) {
        ret << string_format(_("Perception +%d;  "), tmp);
    } else if (tmp < 0) {
        ret << string_format(_("Perception %d;  "), tmp);
    }
    tmp = get_int_mod(reduced);
    if (tmp > 0) {
        ret << string_format(_("Intelligence +%d;  "), tmp);
    } else if (tmp < 0) {
        ret << string_format(_("Intelligence %d;  "), tmp);
    }
    tmp = get_speed_mod(reduced);
    if (tmp > 0) {
        ret << string_format(_("Speed +%d;  "), tmp);
    } else if (tmp < 0) {
        ret << string_format(_("Speed %d;  "), tmp);
    }
    
    if (ret.str() != "") {
        ret << "\n";
    }
    if (eff_type->use_part_descs()) {
        // Used for effect descriptions, "Your body_part_name is ..."
        ret << _("Your ") << body_part_name(bp).c_str() << " ";
    }
    std::string tmp;
    if (eff_type->use_desc_ints()) {
        if (reduced) {
            tmp = eff_type->reduced_desc[intensity - 1];
        } else {
            tmp = eff_type->desc[intensity - 1];
        }
    } else {
        if (reduced) {
            tmp = eff_type->reduced_desc[0];
        } else {
            tmp = eff_type->desc[0];
        }
    }
    ret << _(tmp.c_str());

    return ret.str();
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

int effect::get_mod(std::string arg, bool reduced)
{
    float ret = 0;
    if (!reduced) {
        switch (arg) {
        case "STR":
            ret += eff_type->base_mods.str_mod.first;
            ret += eff_type->scaling_mods.str_mod.first * intensity;
            break;
        case "DEX":
            ret += eff_type->base_mods.dex_mod.first;
            ret += eff_type->scaling_mods.dex_mod.first * intensity;
            break;
        case "PER":
            ret += eff_type->base_mods.per_mod.first;
            ret += eff_type->scaling_mods.per_mod.first * intensity;
            break;
        case "INT":
            ret += eff_type->base_mods.int_mod.first;
            ret += eff_type->scaling_mods.int_mod.first * intensity;
            break;
        case "SPEED":
            ret += eff_type->base_mods.speed_mod.first;
            ret += eff_type->scaling_mods.speed_mod.first * intensity;
            break;
        case "PAIN":
            ret += rng(eff_type->base_mods.pain_min.first + eff_type->scaling_mods.pain_min.first * intensity,
                    eff_type->base_mods.pain_max.first + eff_type->scaling_mods.pain_max.first * intensity);
            break;
        case "HURT":
            ret += rng(eff_type->base_mods.hurt_min.first + eff_type->scaling_mods.hurt_min.first * intensity,
                    eff_type->base_mods.hurt_max.first + eff_type->scaling_mods.hurt_max.first * intensity);
            break;
        case "PKILL":
            ret += rng(eff_type->base_mods.pkill_min.first + eff_type->scaling_mods.pkill_min.first * intensity,
                    eff_type->base_mods.pkill_max.first + eff_type->scaling_mods.pkill_max.first * intensity);
            break;
        }
    } else {
        switch (arg) {
        case "STR":
            ret += eff_type->base_mods.str_mod.second;
            ret += eff_type->scaling_mods.str_mod.second * intensity;
            break;
        case "DEX":
            ret += eff_type->base_mods.dex_mod.second;
            ret += eff_type->scaling_mods.dex_mod.second * intensity;
            break;
        case "PER":
            ret += eff_type->base_mods.per_mod.second;
            ret += eff_type->scaling_mods.per_mod.second * intensity;
            break;
        case "INT":
            ret += eff_type->base_mods.int_mod.second;
            ret += eff_type->scaling_mods.int_mod.second * intensity;
            break;
        case "SPEED":
            ret += eff_type->base_mods.speed_mod.second;
            ret += eff_type->scaling_mods.speed_mod.second * intensity;
            break;
        case "PAIN":
            ret += rng(eff_type->base_mods.pain_min.second + eff_type->scaling_mods.pain_min.second * intensity,
                    eff_type->base_mods.pain_max.second + eff_type->scaling_mods.pain_max.second * intensity);
            break;
        case "HURT":
            ret += rng(eff_type->base_mods.hurt_min.second + eff_type->scaling_mods.hurt_min.second * intensity,
                    eff_type->base_mods.hurt_max.second + eff_type->scaling_mods.hurt_max.second * intensity);
            break;
        case "PKILL":
            ret += rng(eff_type->base_mods.pkill_min.second + eff_type->scaling_mods.pkill_min.second * intensity,
                    eff_type->base_mods.pkill_max.second + eff_type->scaling_mods.pkill_max.second * intensity);
            break;
        }
    }
    return int(ret);
}

int effect::get_amount(std::string arg, bool reduced)
{
    float ret = 0;
    if (!reduced) {
        switch (arg) {
        case "PAIN":
            ret += eff_type->base_mods.pain_amount.first;
            ret += eff_type->scaling_mods.pain_amount.first * intensity;
            break;
        case "HURT":
            ret += eff_type->base_mods.hurt_amount.first;
            ret += eff_type->scaling_mods.hurt_amount.first * intensity;
            break;
        case "PKILL":
            ret += eff_type->base_mods.pkill_amount.first;
            ret += eff_type->scaling_mods.pkill_amount.first * intensity;
            break;
        }
    } else {
        switch (arg) {
        case "PAIN":
            ret += eff_type->base_mods.pain_amount.second;
            ret += eff_type->scaling_mods.pain_amount.second * intensity;
            break;
        case "HURT":
            ret += eff_type->base_mods.hurt_amount.second;
            ret += eff_type->scaling_mods.hurt_amount.second * intensity;
            break;
        case "PKILL":
            ret += eff_type->base_mods.pkill_amount.second;
            ret += eff_type->scaling_mods.pkill_amount.second * intensity;
            break;
        }
    }
}

int effect::get_max_val(std::string arg, bool reduced)
{
    float ret = 0;
    if (!reduced) {
        switch (arg) {
        case "PAIN":
            ret += eff_type->base_mods.pain_max_val.first;
            ret += eff_type->scaling_mods.pain_max_val.first * intensity;
            break;
        case "PKILL":
            ret += eff_type->base_mods.pkill_max_val.first;
            ret += eff_type->scaling_mods.pkill_max_val.first * intensity;
            break;
        }
    } else {
        switch (arg) {
        case "PAIN":
            ret += eff_type->base_mods.pain_max_val.second;
            ret += eff_type->scaling_mods.pain_max_val.second * intensity;
            break;
        case "PKILL":
            ret += eff_type->base_mods.pkill_max_val.second;
            ret += eff_type->scaling_mods.pkill_max_val.second * intensity;
            break;
        }
    
    }
}

bool effect::activated(unsigned int turn, std::string arg, bool reduced)
{
    int tmp;
    int tmp2;
    if (!reduced) {
        switch (arg) {
        case "PAIN":
            tmp = eff_type->base_mods.pain_tick.first + eff_type->scaling_mods.pain_tick.first * intensity;
            if(tmp <= 0 || turn % tmp == 0) {
                tmp = eff_type->base_mods.pain_chance_bot.first + eff_type->scaling_mods.pain_chance_bot.first * intensity;
                tmp2 = eff_type->base_mods.pain_chance_top.first + eff_type->scaling_mods.pain_chance_top.first * intensity;
                if(tmp > 0) {
                    return x_in_y(tmp, tmp2);
                } else {
                    return one_in(tmp2);
                }
            }
        case "HURT":
            tmp = eff_type->base_mods.hurt_tick.first + eff_type->scaling_mods.hurt_tick.first * intensity;
            if(tmp <= 0 || turn % tmp == 0) {
                tmp = eff_type->base_mods.hurt_chance_bot.first + eff_type->scaling_mods.hurt_chance_bot.first * intensity;
                tmp2 = eff_type->base_mods.hurt_chance_top.first + eff_type->scaling_mods.hurt_chance_top.first * intensity;
                if(tmp > 0) {
                    return x_in_y(tmp, tmp2);
                } else {
                    return one_in(tmp2);
                }
            }
        case "PKILL":
            tmp = eff_type->base_mods.pkill_tick.first + eff_type->scaling_mods.pkill_tick.first * intensity;
            if(tmp <= 0 || turn % tmp == 0) {
                tmp = eff_type->base_mods.pkill_chance_bot.first + eff_type->scaling_mods.pkill_chance_bot.first * intensity;
                tmp2 = eff_type->base_mods.pkill_chance_top.first + eff_type->scaling_mods.pkill_chance_top.first * intensity;
                if(tmp > 0) {
                    return x_in_y(tmp, tmp2);
                } else {
                    return one_in(tmp2);
                }
            }
        case "COUGH":
            tmp = eff_type->base_mods.cough_tick.first + eff_type->scaling_mods.cough_tick.first * intensity;
            if(tmp <= 0 || turn % tmp == 0) {
                tmp = eff_type->base_mods.cough_chance_bot.first + eff_type->scaling_mods.cough_chance_bot.first * intensity;
                tmp2 = eff_type->base_mods.cough_chance_top.first + eff_type->scaling_mods.cough_chance_top.first * intensity;
                if(tmp > 0) {
                    return x_in_y(tmp, tmp2);
                } else {
                    return one_in(tmp2);
                }
            }
        case "VOMIT":
            tmp = eff_type->base_mods.vomit_tick.first + eff_type->scaling_mods.vomit_tick.first * intensity;
            if(tmp <= 0 || turn % tmp == 0) {
                tmp = eff_type->base_mods.vomit_chance_bot.first + eff_type->scaling_mods.vomit_chance_bot.first * intensity;
                tmp2 = eff_type->base_mods.vomit_chance_top.first + eff_type->scaling_mods.vomit_chance_top.first * intensity;
                if(tmp > 0) {
                    return x_in_y(tmp, tmp2);
                } else {
                    return one_in(tmp2);
                }
            }
        }
    } else {
        switch (arg) {
        case "PAIN":
            tmp = eff_type->base_mods.pain_tick.second + eff_type->scaling_mods.pain_tick.second * intensity;
            if(tmp <= 0 || turn % tmp == 0) {
                tmp = eff_type->base_mods.pain_chance_bot.second + eff_type->scaling_mods.pain_chance_bot.second * intensity;
                tmp2 = eff_type->base_mods.pain_chance_top.second + eff_type->scaling_mods.pain_chance_top.second * intensity;
                if(tmp > 0) {
                    return x_in_y(tmp, tmp2);
                } else {
                    return one_in(tmp2);
                }
            }
        case "HURT":
            tmp = eff_type->base_mods.hurt_tick.second + eff_type->scaling_mods.hurt_tick.second * intensity;
            if(tmp <= 0 || turn % tmp == 0) {
                tmp = eff_type->base_mods.hurt_chance_bot.second + eff_type->scaling_mods.hurt_chance_bot.second * intensity;
                tmp2 = eff_type->base_mods.hurt_chance_top.second + eff_type->scaling_mods.hurt_chance_top.second * intensity;
                if(tmp > 0) {
                    return x_in_y(tmp, tmp2);
                } else {
                    return one_in(tmp2);
                }
            }
        case "PKILL":
            tmp = eff_type->base_mods.pkill_tick.second + eff_type->scaling_mods.pkill_tick.second * intensity;
            if(tmp <= 0 || turn % tmp == 0) {
                tmp = eff_type->base_mods.pkill_chance_bot.second + eff_type->scaling_mods.pkill_chance_bot.second * intensity;
                tmp2 = eff_type->base_mods.pkill_chance_top.second + eff_type->scaling_mods.pkill_chance_top.second * intensity;
                if(tmp > 0) {
                    return x_in_y(tmp, tmp2);
                } else {
                    return one_in(tmp2);
                }
            }
        case "COUGH":
            tmp = eff_type->base_mods.cough_tick.second + eff_type->scaling_mods.cough_tick.second * intensity;
            if(tmp <= 0 || turn % tmp == 0) {
                tmp = eff_type->base_mods.cough_chance_bot.second + eff_type->scaling_mods.cough_chance_bot.second * intensity;
                tmp2 = eff_type->base_mods.cough_chance_top.second + eff_type->scaling_mods.cough_chance_top.second * intensity;
                if(tmp > 0) {
                    return x_in_y(tmp, tmp2);
                } else {
                    return one_in(tmp2);
                }
            }
        case "VOMIT":
            tmp = eff_type->base_mods.vomit_tick.second + eff_type->scaling_mods.vomit_tick.second * intensity;
            if(tmp <= 0 || turn % tmp == 0) {
                tmp = eff_type->base_mods.vomit_chance_bot.second + eff_type->scaling_mods.vomit_chance_bot.second * intensity;
                tmp2 = eff_type->base_mods.vomit_chance_top.second + eff_type->scaling_mods.vomit_chance_top.second * intensity;
                if(tmp > 0) {
                    return x_in_y(tmp, tmp2);
                } else {
                    return one_in(tmp2);
                }
            }
        }
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

    new_etype.name = jo.get_string("name", "");
    new_etype.desc = jo.get_string("desc", "");
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

    new_etype.max_intensity = jo.get_int("max_intensity", 1);

    effect_types[new_etype.id] = new_etype;
}

void reset_effect_types()
{
    effect_types.clear();
}
