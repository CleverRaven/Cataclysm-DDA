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
        if(j.has_member("str_mod")) {
            JsonArray jsarr = j.get_array("str_mod");
                str_mod = jsarr.get_float(0);
            if (jsarr.size() == 2) {
                str_mod_reduced = jsarr.get_float(1);
            }
        }
        if(j.has_member("dex_mod")) {
            JsonArray jsarr = j.get_array("dex_mod");
                dex_mod = jsarr.get_float(0);
            if (jsarr.size() == 2) {
                dex_mod_reduced = jsarr.get_float(1);
            }
        }
        if(j.has_member("per_mod")) {
            JsonArray jsarr = j.get_array("per_mod");
                per_mod = jsarr.get_float(0);
            if (jsarr.size() == 2) {
                per_mod_reduced = jsarr.get_float(1);
            }
        }
        if(j.has_member("int_mod")) {
            JsonArray jsarr = j.get_array("int_mod");
                int_mod = jsarr.get_float(0);
            if (jsarr.size() == 2) {
                int_mod_reduced = jsarr.get_float(1);
            }
        }
        if(j.has_member("speed_mod")) {
            JsonArray jsarr = j.get_array("speed_mod");
                speed_mod = jsarr.get_int(0);
            if (jsarr.size() == 2) {
                speed_mod_reduced = jsarr.get_int(1);
            }
        }

        if(j.has_member("pain")) {
            JsonArray jsarr = j.get_array("pain");
                pain_min = jsarr.get_int(0);
            if (jsarr.size() == 2) {
                pain_max = jsarr.get_int(1);
            } else {
                pain_max = pain_min;
            }
        }
        if(j.has_member("pain_reduced")) {
            JsonArray jsarr = j.get_array("pain_reduced");
                pain_reduced_min = jsarr.get_int(0);
            if (jsarr.size() == 2) {
                pain_reduced_max = jsarr.get_int(1);
            } else {
                pain_reduced_max = pain_reduced_min;
            }
        }
        if(j.has_member("pain_chance")) {
            JsonArray jsarr = j.get_array("pain_chance");
                pain_chance = jsarr.get_int(0);
            if (jsarr.size() == 2) {
                pain_chance_reduced = jsarr.get_int(1);
            }
        }

        if(j.has_member("hurt")) {
            JsonArray jsarr = j.get_array("hurt");
                hurt_min = jsarr.get_int(0);
            if (jsarr.size() == 2) {
                hurt_max = jsarr.get_int(1);
            } else {
                hurt_max = hurt_min;
            }
        }
        if(j.has_member("hurt_reduced")) {
            JsonArray jsarr = j.get_array("hurt_reduced");
                hurt_reduced_min = jsarr.get_int(0);
            if (jsarr.size() == 2) {
                hurt_reduced_max = jsarr.get_int(1);
            } else {
                hurt_reduced_max = hurt_reduced_min;
            }
        }
        if(j.has_member("hurt_chance")) {
            JsonArray jsarr = j.get_array("hurt_chance");
                hurt_chance = jsarr.get_int(0);
            if (jsarr.size() == 2) {
                hurt_chance_reduced = jsarr.get_int(1);
            }
        }

        if(j.has_member("cough_chance")) {
            JsonArray jsarr = j.get_array("cough_chance");
                cough_chance = jsarr.get_int(0);
            if (jsarr.size() == 2) {
                cough_chance_reduced = jsarr.get_int(1);
            }
        }

        if(j.has_member("vomit_chance")) {
            JsonArray jsarr = j.get_array("vomit_chance");
                vomit_chance = jsarr.get_int(0);
            if (jsarr.size() == 2) {
                vomit_chance_reduced = jsarr.get_int(1);
            }
        }

        if(j.has_member("pkill_amount")) {
            JsonArray jsarr = j.get_array("pkill_amount");
                pkill_amount = jsarr.get_int(0);
            if (jsarr.size() == 2) {
                pkill_amount_reduced = jsarr.get_int(1);
            }
        }

        if(j.has_member("pkill_increment")) {
            JsonArray jsarr = j.get_array("pkill_increment");
                pkill_increment = jsarr.get_int(0);
            if (jsarr.size() == 2) {
                pkill_increment_reduced = jsarr.get_int(1);
            }
        }

        if(j.has_member("pkill_max")) {
            JsonArray jsarr = j.get_array("pkill_max");
                pkill_max = jsarr.get_int(0);
            if (jsarr.size() == 2) {
                pkill_max_reduced = jsarr.get_int(1);
            }
        }

        return true;
    } else {
        return false;
    }
}

bool effect_morph_info::load(JsonObject &jsobj, std::string member) {
    if( jsobj.has_object(member) ) {
        JsonObject j = jsobj.get_object(member);
        morph_id = j.get_string("morph_id");
        morph_duration = j.get_int("morph_duration", 1);

        morph_perm = j.get_int("morph_perm", false);
        morph_with_parts = j.get_bool("morph_with_parts", true);
        morph_with_intensity = j.get_bool("morph_with_intensity", false);
        morph_intensity = j.get_int("morph_intensity", 1);
        cancel_trait = j.get_string("cancel_trait", "");

        return true;
    } else {
        return false;
    }
}

effect_type::effect_type(){}
effect_type::effect_type(const effect_type &) {}

std::string effect_type::get_name(int intensity)
{
    if (use_name_intensities()) {
        return name[intensity - 1];
    } else {
        return name[0];
    }
}
std::string effect_type::get_desc(int intensity, bool reduced)
{
    if (use_desc_intensities()) {
        if (reduced) {
            return reduced_desc[intensity - 1];
        } else {
            return desc[intensity - 1];
        }
    } else {
        if (reduced) {
            return reduced_desc[0];
        } else {
            return desc[0];
        }
    }
}
bool effect_type::use_part_descs()
{
    return part_descs;
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
efftype_id effect_type::get_morph_id()
{
    return morph.morph_id;
}
bool effect_type::get_morph_with_parts()
{
    return morph.morph_with_parts;
}
bool effect_type::get_morph_with_intensity()
{
    // Any value in morph_intensity trumps morph_with_intensity
    if (morph.morph_intensity > 1) {
        return false;
    } else {
        return morph.morph_with_intensity;
    }
}
int effect_type::get_morph_duration()
{
    return morph.morph_duration;
}
bool effect_type::get_morph_perm()
{
    return morph.morph_perm;
}
int effect_type::get_morph_intensity()
{
    return morph.morph_intensity;
}
std::string effect_type::get_cancel_trait()
{
    return morph.cancel_trait;
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
        ret << "Your " << body_part_name(bp, side).c_str() << " ";
    }
    ret << eff_type->get_desc(intensity, reduced);
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
    if (eff_type->decay_rate != 0 && duration % eff_type->decay_rate == 0) {
        if (eff_type->decay_rate > 0) {
            intensity = std::max(intensity - 1, 1);
        } else {
            intensity = std::min(intensity + 1, eff_type->max_intensity);
        }
    }
}

void effect::set_intensity(int nintensity)
{
    intensity = nintensity;
    if (intensity > get_max_intensity()) {
        intensity = get_max_intensity();
    }
}
void effect::mod_intensity(int nintensity)
{
    intensity += nintensity;
    if (intensity > get_max_intensity()) {
        intensity = get_max_intensity();
    }
}

body_part effect::get_bp()
{
    return bp;
}
int effect::get_side()
{
    return side;
}

int effect::get_str_mod(bool reduced)
{
    float ret = 0;
    if (!reduced) {
        ret += eff_type->base_mods.str_mod;
        ret += eff_type->scaling_mods.str_mod * intensity;
    } else {
        ret += eff_type->base_mods.str_mod_reduced;
        ret += eff_type->scaling_mods.str_mod_reduced * intensity;
    }
    return int(ret);
}
int effect::get_dex_mod(bool reduced)
{
    float ret = 0;
    if (!reduced) {
        ret += eff_type->base_mods.dex_mod;
        ret += eff_type->scaling_mods.dex_mod * intensity;
    } else {
        ret += eff_type->base_mods.dex_mod_reduced;
        ret += eff_type->scaling_mods.dex_mod_reduced * intensity;
    }
    return int(ret);
}
int effect::get_per_mod(bool reduced)
{
    float ret = 0;
    if (!reduced) {
        ret += eff_type->base_mods.per_mod;
        ret += eff_type->scaling_mods.per_mod * intensity;
    } else {
        ret += eff_type->base_mods.per_mod_reduced;
        ret += eff_type->scaling_mods.per_mod_reduced * intensity;
    }
    return int(ret);
}
int effect::get_int_mod(bool reduced)
{
    float ret = 0;
    if (!reduced) {
        ret += eff_type->base_mods.int_mod;
        ret += eff_type->scaling_mods.int_mod * intensity;
    } else {
        ret += eff_type->base_mods.int_mod_reduced;
        ret += eff_type->scaling_mods.int_mod_reduced * intensity;
    }
    return int(ret);
}
int effect::get_speed_mod(bool reduced)
{
    float ret = 0;
    if (!reduced) {
        ret += eff_type->base_mods.speed_mod;
        ret += eff_type->scaling_mods.speed_mod * intensity;
    } else {
        ret += eff_type->base_mods.speed_mod_reduced;
        ret += eff_type->scaling_mods.speed_mod_reduced * intensity;
    }
    return int(ret) + hardcoded_speed_mod();
}
int effect::hardcoded_speed_mod()
{
    //TODO - Eventually empty this function into JSON based things
    std::string effect_id = eff_type->id;
    if (effect_id == "hot" || effect_id == "cold") {
        switch (get_bp()) {
            case bp_head:
                switch (get_intensity()) {
                    case 1 : return  -2;
                    case 2 : return  -5;
                    case 3 : return -20;
                }
            case bp_torso:
                switch (get_intensity()) {
                    case 1 : return  -2;
                    case 2 : return  -5;
                    case 3 : return -20;
                }
            default:
                return 0;
        }
    } else if (effect_id == "frostbite") {
        switch (get_bp()) {
            case bp_feet:
                switch (get_intensity()) {
                    case 2 :
                        return -4;
                    default:
                        break;
                }
            default:
                return 0;
        }
    }
    return 0;
}
int effect::get_pain(bool reduced)
{
    int ret = 0;
    if (!reduced) {
        ret += rng(eff_type->base_mods.pain_min, eff_type->base_mods.pain_max);
        ret += rng(eff_type->scaling_mods.pain_min, eff_type->scaling_mods.pain_max) * intensity;
    } else {
        ret += rng(eff_type->base_mods.pain_reduced_min, eff_type->base_mods.pain_reduced_max);
        ret += rng(eff_type->scaling_mods.pain_reduced_min,
                    eff_type->scaling_mods.pain_reduced_max) * intensity;
    }
    return ret;
}
int effect::get_pain_chance(bool reduced)
{
    int ret = 0;
    if (!reduced) {
        ret += eff_type->base_mods.pain_chance;
        ret += eff_type->scaling_mods.pain_chance * intensity;
    } else {
        ret += eff_type->base_mods.pain_chance_reduced;
        ret += eff_type->scaling_mods.pain_chance_reduced * intensity;
    }
    return ret;
}
bool effect::get_pain_sizing()
{
    return (eff_type->pain_sizing);
}
int effect::get_hurt(bool reduced)
{
    int ret = 0;
    if (!reduced) {
        ret += rng(eff_type->base_mods.hurt_min, eff_type->base_mods.hurt_max);
        ret += rng(eff_type->scaling_mods.hurt_min, eff_type->base_mods.hurt_max) * intensity;
    } else {
        ret += rng(eff_type->base_mods.hurt_reduced_min, eff_type->base_mods.hurt_reduced_max);
        ret += rng(eff_type->scaling_mods.hurt_reduced_min,
                    eff_type->scaling_mods.hurt_reduced_max) * intensity;
    }
    return ret;
}
int effect::get_hurt_chance(bool reduced)
{
    int ret = 0;
    if (!reduced) {
        ret += eff_type->base_mods.hurt_chance;
        ret += eff_type->scaling_mods.hurt_chance * intensity;
    } else {
        ret += eff_type->base_mods.hurt_chance_reduced;
        ret += eff_type->scaling_mods.hurt_chance_reduced * intensity;
    }
    return ret;
}
bool effect::get_hurt_sizing()
{
    return (eff_type->hurt_sizing);
}

int effect::get_cough_chance(bool reduced)
{
    int ret = 0;
    if (!reduced) {
        ret += eff_type->base_mods.cough_chance;
        ret += eff_type->scaling_mods.cough_chance * intensity;
    } else {
        ret += eff_type->base_mods.cough_chance_reduced;
        ret += eff_type->scaling_mods.cough_chance_reduced * intensity;
    }
    return ret;
}
bool effect::get_harmful_cough()
{
    return (eff_type->harmful_cough);
}
int effect::get_vomit_chance(bool reduced)
{
    int ret = 0;
    if (!reduced) {
        ret += eff_type->base_mods.vomit_chance;
        ret += eff_type->scaling_mods.vomit_chance * intensity;
    } else {
        ret += eff_type->base_mods.vomit_chance_reduced;
        ret += eff_type->scaling_mods.vomit_chance_reduced * intensity;
    }
    return ret;
}
int effect::get_pkill_amount(bool reduced)
{
    int ret = 0;
    if (!reduced) {
        ret += eff_type->base_mods.pkill_amount;
        ret += eff_type->scaling_mods.pkill_amount * intensity;
    } else {
        ret += eff_type->base_mods.pkill_amount_reduced;
        ret += eff_type->scaling_mods.pkill_amount_reduced * intensity;
    }
    return ret;
}
int effect::get_pkill_increment(bool reduced)
{
    int ret = 0;
    if (!reduced) {
        ret += eff_type->base_mods.pkill_increment;
        ret += eff_type->scaling_mods.pkill_increment * intensity;
    } else {
        ret += eff_type->base_mods.pkill_increment_reduced;
        ret += eff_type->scaling_mods.pkill_increment_reduced * intensity;
    }
    return ret;
}
int effect::get_pkill_max(bool reduced)
{
    int ret = 0;
    if (!reduced) {
        ret += eff_type->base_mods.pkill_max;
        ret += eff_type->scaling_mods.pkill_max * intensity;
    } else {
        ret += eff_type->base_mods.pkill_max_reduced;
        ret += eff_type->scaling_mods.pkill_max_reduced * intensity;
    }
    return ret;
}
bool effect::get_pkill_addict_reduces()
{
    return (eff_type->pkill_addict_reduces);
}

std::string effect::get_resist_trait()
{
    return eff_type->resist_trait;
}

efftype_id effect::get_morph_id()
{
    return eff_type->get_morph_id();
}
body_part effect::get_morph_bp()
{
    if (eff_type->get_morph_with_parts()) {
        return bp;
    } else {
        return num_bp;
    }
}
int effect::get_morph_side()
{
    if (eff_type->get_morph_with_parts()) {
        return side;
    } else {
        return -1;
    }
}
int effect::get_morph_duration()
{
    return eff_type->get_morph_duration();
}
bool effect::get_morph_perm()
{
    return eff_type->get_morph_perm();
}
int effect::get_morph_intensity()
{
    if (eff_type->get_morph_with_intensity()) {
        return intensity;
    } else {
        return eff_type->get_morph_intensity();
    }
}
std::string effect::get_cancel_trait()
{
    return eff_type->get_cancel_trait();
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

    new_etype.apply_message = jo.get_string("apply_message", "");
    new_etype.remove_message = jo.get_string("remove_message", "");
    new_etype.apply_memorial_log = jo.get_string("apply_memorial_log", "");
    new_etype.remove_memorial_log = jo.get_string("remove_memorial_log", "");

    new_etype.max_intensity = jo.get_int("max_intensity", 1);
    new_etype.decay_rate = jo.get_int("decay_rate", 0);
    new_etype.additive = jo.get_int("additive", 1);
    new_etype.main_parts_only = jo.get_bool("main_parts", false);

    new_etype.health_affects = jo.get_bool("health_affects", false);
    new_etype.resist_trait = jo.get_string("resist_trait", "");
    
    new_etype.pain_sizing = jo.get_bool("pain_sizing", false);
    new_etype.hurt_sizing = jo.get_bool("hurt_sizing", false);
    new_etype.harmful_cough = jo.get_bool("harmful_cough", false);
    new_etype.pkill_addict_reduces = jo.get_bool("pkill_addict_reduces", false);

    new_etype.base_mods.load(jo, "base_mods");
    new_etype.scaling_mods.load(jo, "scaling_mods");
    
    new_etype.morph.load(jo, "morph");

    effect_types[new_etype.id] = new_etype;
}

void reset_effect_types()
{
    effect_types.clear();
}
