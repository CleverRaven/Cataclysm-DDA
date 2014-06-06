#include "effect.h"
#include "rng.h"
#include "output.h"
#include <map>
#include <sstream>

std::map<std::string, effect_type> effect_types;

effect_type::effect_type() {}
effect_type::effect_type(const effect_type &) {}

std::string effect_type::get_name()
{
    return name;
}
std::string effect_type::get_desc()
{
    return desc;
}
effect_rating effect_type::get_rating()
{
    return rating;
}
game_message_type effect_type::gain_game_message_type()
{
    switch(rating) {
        case e_good: return m_good;
        case e_bad: return m_bad;
        case e_neutral: return m_neutral;
        case e_mixed: return m_mixed;
        default: return m_neutral;  // should never happen
    }
}
game_message_type effect_type::lose_game_message_type()
{
    switch(rating) {
        case e_good: return m_bad;
        case e_bad: return m_good;
        case e_neutral: return m_neutral;
        case e_mixed: return m_mixed;
        default: return m_neutral;  // should never happen
    }
}
std::string effect_type::get_apply_message()
{
    return apply_message;
}
std::string effect_type::get_apply_memorial_log()
{
    return apply_memorial_log;
}
std::string effect_type::get_remove_message()
{
    return remove_message;
}
std::string effect_type::get_remove_memorial_log()
{
    return remove_memorial_log;
}
int effect_type::get_max_intensity()
{
    return max_intensity;
}

effect::effect() :
    eff_type(NULL),
    duration(0),
    intensity(0),
    permanent(false)
{ }

effect::effect(effect_type *peff_type, int dur, int nintensity, bool perm) :
    eff_type(peff_type),
    duration(dur),
    intensity(nintensity),
    permanent(perm)
{ }

effect::effect(const effect &rhs) : JsonSerializer(), JsonDeserializer(),
    eff_type(rhs.eff_type),
    duration(rhs.duration),
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
    intensity = rhs.intensity;
    permanent = rhs.permanent;

    return *this;
}

std::string effect::disp_name()
{
    std::stringstream ret;
    ret << eff_type->get_name();
    if (intensity > 1) {
        ret << " [" << intensity << "]";
    }
    return ret.str();
}
void effect::do_effect(Creature &)
{
    return;
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
        if(r == "good") { new_etype.rating = e_good; }
        else if(r == "neutral" ) { new_etype.rating = e_neutral; }
        else if(r == "bad" ) { new_etype.rating = e_bad; }
        else if(r == "mixed" ) { new_etype.rating = e_mixed; }
        else { new_etype.rating = e_neutral; }
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
