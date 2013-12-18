#include "effect.h"
#include "rng.h"
#include "output.h"
#include <map>
#include <iostream>

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

effect::effect() :
    eff_type(NULL),
    duration(0),
    intensity(0)
{ }

effect::effect(effect_type *peff_type, int dur) :
    eff_type(peff_type),
    duration(dur),
    intensity(1)
{ }

effect::effect(const effect &rhs) :
    eff_type(rhs.eff_type),
    duration(rhs.duration),
    intensity(rhs.intensity)
{ }

effect &effect::operator=(const effect &rhs)
{
    if (this == &rhs) {
        return *this;    // No self-assignment
    }

    eff_type = rhs.eff_type;
    duration = rhs.duration;
    intensity = rhs.intensity;

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

int effect::get_intensity()
{
    return intensity;
}
void effect::set_intensity(int nintensity)
{
    intensity = nintensity;
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

    new_etype.apply_message = jo.get_string("apply_message", "");
    new_etype.remove_message = jo.get_string("remove_message", "");
    new_etype.apply_memorial_log = jo.get_string("apply_memorial_log", "");
    new_etype.remove_memorial_log = jo.get_string("remove_memorial_log", "");

    effect_types[new_etype.id] = new_etype;
}
