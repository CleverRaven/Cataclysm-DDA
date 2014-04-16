#include "effect.h"
#include "rng.h"
#include "bodypart.h"
#include "output.h"
#include <map>
#include <sstream>

std::map<std::string, effect_type> effect_types;

bool effect_mod_info::load(JsonObject &jsobj, std::string member) {
    if( jsobj.has_object(member) ) {
        JsonObject j = jsobj.get_object(member);
        str_mod = j.get_int("str_mod", 0);
        dex_mod = j.get_int("dex_mod", 0);
        per_mod = j.get_int("per_mod", 0);
        int_mod = j.get_int("int_mod", 0);
        speed_mod = j.get_int("speed_mod", 0);

        return true;
    } else {
        return false;
    }
}

effect_type::effect_type() {}
effect_type::effect_type(const effect_type &) {}

std::string effect_type::get_name(int intensity)
{
    if (use_name_intensities()) {
        return name[intensity - 1];
    } else {
        return name[0];
    }
}
std::string effect_type::get_desc(int intensity)
{
    if (use_desc_intensities()) {
        return desc[intensity - 1];
    } else {
        return desc[0];
    }
}
bool effect_type::use_name_intensities()
{
    return ((size_t)max_intensity <= name.size());
}
bool effect_type::use_desc_intensities()
{
    return ((size_t)max_intensity <= desc.size());
}
std::string effect_type::speed_name()
{
    if (speed_mod_name == "") {
        return get_name(1);
    } else {
        return speed_mod_name;
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
int effect_type::get_decay_rate()
{
    return decay_rate;
}
int effect_type::get_additive()
{
    return additive;
}
int effect_type::main_parts()
{
    return main_parts_only;
}
bool effect_type::health_mods()
{
    return health_affects;
}

effect::effect() :
    eff_type(NULL),
    duration(0),
    permanent(false),
    intensity(0),
    bp(num_bp),
    side(-1)
{ }

effect::effect(effect_type *peff_type, int dur, bool perm, int nintensity, body_part target,
                int nside) :
    eff_type(peff_type),
    duration(dur),
    permanent(perm),
    intensity(nintensity),
    bp(target),
    side(nside)
{ }

effect::effect(const effect &rhs) : JsonSerializer(), JsonDeserializer(),
    eff_type(rhs.eff_type),
    duration(rhs.duration),
    permanent(rhs.permanent),
    intensity(rhs.intensity),
    bp(rhs.bp),
    side(rhs.side)
{ }

effect &effect::operator=(const effect &rhs)
{
    if (this == &rhs) {
        return *this;    // No self-assignment
    }

    eff_type = rhs.eff_type;
    duration = rhs.duration;
    permanent = rhs.permanent;
    intensity = rhs.intensity;
    bp = rhs.bp;
    side = rhs.side;

    return *this;
}

std::string effect::disp_name()
{
    std::stringstream ret;
    ret << eff_type->get_name(intensity);
    if (!eff_type->use_name_intensities() && intensity > 1) {
        ret << " [" << intensity << "]";
    }
    if (bp != num_bp) {
        ret << " (" << body_part_name(bp, side).c_str() << ")";
    }
    return ret.str();
}
std::string effect::disp_desc()
{
    return eff_type->get_desc(intensity);
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
void effect::decay(int health_val)
{
    if (!permanent && duration > 0) {
        // Up to a 50% chance to lose one extra/not lose one from good/bad health
        if (eff_type->health_mods()) {
            if (health_val > 0 && x_in_y(health_val / 2, 100)) {
                duration -= 1;
            } else if (health_val < 0 && x_in_y(-health_val / 2, 100)) {
                duration += 1;
            }
        }
        duration -= 1;
    }
    if (eff_type->decay_rate > 0 && intensity > 1 && (duration % eff_type->decay_rate == 0)) {
        intensity -= 1;
    }
}

void effect::set_intensity(int nintensity)
{
    intensity = nintensity;
}
void effect::mod_intensity(int nintensity)
{
    intensity += nintensity;
}

body_part effect::get_bp()
{
    return bp;
}
int effect::get_side()
{
    return side;
}
int effect::get_str_mod()
{
    int ret = 0;
    ret += eff_type->base_mods.str_mod;
    ret += eff_type->scaling_mods.str_mod * intensity;
    return ret;
}
int effect::get_dex_mod()
{
    int ret = 0;
    ret += eff_type->base_mods.dex_mod;
    ret += eff_type->scaling_mods.dex_mod * intensity;
    return ret;
}
int effect::get_per_mod()
{
    int ret = 0;
    ret += eff_type->base_mods.per_mod;
    ret += eff_type->scaling_mods.per_mod * intensity;
    return ret;
}
int effect::get_int_mod()
{
    int ret = 0;
    ret += eff_type->base_mods.int_mod;
    ret += eff_type->scaling_mods.int_mod * intensity;
    return ret;
}
int effect::get_speed_boost()
{
    int ret = 0;
    ret += eff_type->base_mods.speed_mod;
    ret += eff_type->scaling_mods.speed_mod * intensity;
    return ret;
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
    if(jo.has_member("desc")) {
        JsonArray jsarr = jo.get_array("desc");
        while (jsarr.has_more()) {
            new_etype.desc.push_back(_(jsarr.next_string().c_str()));
        }
    } else {
        new_etype.desc.push_back("");
    }
    new_etype.speed_mod_name = jo.get_string("speed_name", "");

    new_etype.apply_message = jo.get_string("apply_message", "");
    new_etype.remove_message = jo.get_string("remove_message", "");
    new_etype.apply_memorial_log = jo.get_string("apply_memorial_log", "");
    new_etype.remove_memorial_log = jo.get_string("remove_memorial_log", "");

    new_etype.max_intensity = jo.get_int("max_intensity", 1);
    new_etype.decay_rate = jo.get_int("decay_rate", 0);
    new_etype.additive = jo.get_int("additive", 1);
    new_etype.main_parts_only = jo.get_bool("main_parts", false);

    new_etype.health_affects = jo.get_bool("health_affects", false);

    new_etype.base_mods.load(jo, "base_mods");
    new_etype.scaling_mods.load(jo, "scaling_mods");

    effect_types[new_etype.id] = new_etype;
}

void reset_effect_types()
{
    effect_types.clear();
}
