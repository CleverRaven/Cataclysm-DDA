#include "wound.h"
#include "player.h"
#include "rng.h"
#include "bodypart.h"

std::map<std::string, wound_eff_type> wound_eff_types;

bool wound_effect_info::load(JsonObject &jsobj, std::string member) {
    if( jsobj.has_object(member) ) {
        JsonObject j = jsobj.get_object(member);
        effect_duration = j.get_int("effect_duration", 1);
        effect_intensity = j.get_int("effect_intensity", 1);
        heal_mod_100K = j.get_float("heal_mod_100K", 0);
        if(j.has_member("bleed")) {
            JsonArray jsarr = j.get_array("bleed");
                bleed_min = jsarr.get_float(0);
            if (jsarr.size() == 2) {
                bleed_max = jsarr.get_float(1);
            } else {
                bleed_max = bleed_min;
            }
        }
        if(j.has_member("pain")) {
            JsonArray jsarr = j.get_array("pain");
                pain_min = jsarr.get_float(0);
            if (jsarr.size() == 2) {
                pain_max = jsarr.get_float(1);
            } else {
                pain_max = pain_min;
            }
        }
        if(j.has_member("hurt")) {
            JsonArray jsarr = j.get_array("hurt");
                hurt_min = jsarr.get_float(0);
            if (jsarr.size() == 2) {
                hurt_max = jsarr.get_float(1);
            } else {
                hurt_max = hurt_min;
            }
        }
        str_mod = j.get_float("str_mod", 0);
        dex_mod = j.get_float("dex_mod", 0);
        per_mod = j.get_float("per_mod", 0);
        int_mod = j.get_float("int_mod", 0);
        speed_mod = j.get_int("speed_mod", 0);

        return true;
    } else {
        return false;
    }
}

bool wound_trigger_info::load(JsonObject &jsobj, std::string member) {
    if( jsobj.has_object(member) ) {
        JsonObject j = jsobj.get_object(member);
        trig_delay = j.get_int("trig_delay", 0);
        trig_chance_100K = j.get_int("trig_chance_100K", 0);
        health_mult = j.get_float("health_mult", 0);
        chance_min = j.get_int("chance_min", 0);
        chance_max = j.get_int("chance_max", 100000);
        rem_effect = j.get_bool("rem_effect", false);
        heal_wound = j.get_bool("heal_wound", false);
        trig_message = j.get_string("trig_message", "");
        weff_added = j.get_string("weff_added", "");

        flat_mods.load(j, "flat_mods");
        sev_mods.load(j, "sev_mods");

        return true;
    } else {
        return false;
    }
}

wound_eff_type::wound_eff_type() :
    id(""),
    name_mod(""),
    desc(""),
    
    effect_placed(""),
    targeted_effect(false),
    effect_perm(false),
    targeted_harm(false),

    base_mods(),
    sev_scale_mods(),
    
    trigger_info()
{ }

int wound_eff_type::get_trig_str_mod(int sev)
{
    float ret = 0;
    ret += trigger_info.flat_mods.str_mod;
    ret += trigger_info.sev_mods.str_mod * sev;
    return int(ret);
}
int wound_eff_type::get_trig_dex_mod(int sev)
{
    float ret = 0;
    ret += trigger_info.flat_mods.dex_mod;
    ret += trigger_info.sev_mods.dex_mod * sev;
    return int(ret);
}
int wound_eff_type::get_trig_per_mod(int sev)
{
    float ret = 0;
    ret += trigger_info.flat_mods.per_mod;
    ret += trigger_info.sev_mods.per_mod * sev;
    return int(ret);
}
int wound_eff_type::get_trig_int_mod(int sev)
{
    float ret = 0;
    ret += trigger_info.flat_mods.int_mod;
    ret += trigger_info.sev_mods.int_mod * sev;
    return int(ret);
}
int wound_eff_type::get_trig_speed_mod(int sev)
{
    float ret = 0;
    ret += trigger_info.flat_mods.speed_mod;
    ret += trigger_info.sev_mods.speed_mod * sev;
    return int(ret);
}
int wound_eff_type::get_trig_bleed(int sev)
{
    float ret = 0;
    ret += rng(trigger_info.flat_mods.bleed_min, trigger_info.flat_mods.bleed_max);
    ret += rng(trigger_info.sev_mods.bleed_min, trigger_info.sev_mods.bleed_max) * sev;
    return int(ret);
}
int wound_eff_type::get_trig_pain(int sev)
{
    float ret = 0;
    ret += rng(trigger_info.flat_mods.pain_min, trigger_info.flat_mods.pain_max);
    ret += rng(trigger_info.sev_mods.pain_min, trigger_info.sev_mods.pain_max) * sev;
    return int(ret);
}
int wound_eff_type::get_trig_hurt(int sev)
{
    float ret = 0;
    ret += rng(trigger_info.flat_mods.hurt_min, trigger_info.flat_mods.hurt_max);
    ret += rng(trigger_info.sev_mods.hurt_min, trigger_info.sev_mods.hurt_max) * sev;
    return int(ret);
}
int wound_eff_type::get_trig_heal_mod(int sev)
{
    float ret = 0;
    ret += trigger_info.flat_mods.heal_mod_100K;
    ret += trigger_info.sev_mods.heal_mod_100K * sev;
    return int(ret);
}
int wound_eff_type::get_trig_effect_dur(int sev)
{
    float ret = 0;
    ret += trigger_info.flat_mods.effect_duration;
    ret += trigger_info.sev_mods.effect_duration * sev;
    return int(ret);
}
int wound_eff_type::get_trig_effect_int(int sev)
{
    float ret = 0;
    ret += trigger_info.flat_mods.effect_intensity;
    ret += trigger_info.sev_mods.effect_intensity * sev;
    return int(ret);
}
int wound_eff_type::get_effect_dur(int sev)
{
    float ret = 0;
    ret += trigger_info.flat_mods.effect_duration;
    ret += trigger_info.sev_mods.effect_duration * sev;
    return int(ret);
}
int wound_eff_type::get_effect_int(int sev)
{
    float ret = 0;
    ret += trigger_info.flat_mods.effect_intensity;
    ret += trigger_info.sev_mods.effect_intensity * sev;
    return int(ret);
}
wefftype_id wound_eff_type::get_effect_placed()
{
    return effect_placed;
}
bool wound_eff_type::get_targeted_effect()
{
    return targeted_effect;
}
bool wound_eff_type::get_effect_perm()
{
    return effect_perm;
}
bool wound_eff_type::get_targeted_harm()
{
    return targeted_harm;
}
bool wound_eff_type::get_heal_wound()
{
    return trigger_info.heal_wound;
}
std::string wound_eff_type::get_weff_added()
{
    return trigger_info.weff_added;
}
bool wound_eff_type::get_rem_effect()
{
    return trigger_info.rem_effect;
}
int wound_eff_type::get_trig_delay()
{
    return trigger_info.trig_delay;
}
int wound_eff_type::get_trig_chance(int health)
{
    int ret = trigger_info.trig_chance_100K;
    ret += health * trigger_info.health_mult;
    //Check caps
    ret = std::min(ret, trigger_info.chance_max);
    ret = std::max(ret, trigger_info.chance_min);
    
    return ret;
}

wound::wound() :
    wound_effects(),
    bp(num_bp),
    side(-1),
    severity(0),
    base_heal_mod(0),
    base_str_mod(0),
    base_dex_mod(0),
    base_per_mod(0),
    base_int_mod(0),
    base_speed_mod(0),
    age(0)
{ }

wound::wound(body_part target, int nside, int sev, std::vector<wefftype_id> wound_effs) :
    wound_effects(wound_effs),
    bp(target),
    side(nside),
    severity(sev),
    base_heal_mod(0),
    base_str_mod(0),
    base_dex_mod(0),
    base_per_mod(0),
    base_int_mod(0),
    base_speed_mod(0),
    age(0)
{ }

wound::wound(const wound &rhs) : JsonSerializer(), JsonDeserializer(),
    wound_effects(rhs.wound_effects),
    bp(rhs.bp),
    side(rhs.side),
    severity(rhs.severity),
    base_heal_mod(rhs.base_heal_mod),
    base_str_mod(rhs.base_str_mod),
    base_dex_mod(rhs.base_dex_mod),
    base_per_mod(rhs.base_per_mod),
    base_int_mod(rhs.base_int_mod),
    base_speed_mod(rhs.base_speed_mod),
    age(rhs.age)
{ }

wound &wound::operator=(const wound &rhs)
{
    if (this == &rhs) {
        return *this;    // No self-assignment
    }

    bp = rhs.bp;
    side = rhs.side;
    severity = rhs.severity;
    wound_effects = rhs.wound_effects;
    base_heal_mod = rhs.base_heal_mod;
    base_str_mod = rhs.base_str_mod;
    base_dex_mod = rhs.base_dex_mod;
    base_per_mod = rhs.base_per_mod;
    base_int_mod = rhs.base_int_mod;
    base_speed_mod = rhs.base_speed_mod;
    age = rhs.age;

    return *this;
}

bool wound::add_wound_effect(wefftype_id eff)
{
    if (!has_wound_effect(eff)) {
        wound_effects.push_back(eff);
        return true;
    }
    return false;
}
bool wound::remove_wound_effect(wefftype_id eff)
{
    for (std::vector<wefftype_id>::iterator it = wound_effects.begin(); it != wound_effects.end();
          ++it) {
        if (eff == *it) {
            wound_effects.erase(it);
            return true;
        }
    }
    return false;
}

bool wound::has_wound_effect(wefftype_id eff)
{
    for (std::vector<wefftype_id>::iterator it = wound_effects.begin(); it != wound_effects.end();
          ++it) {
        if (eff == *it) {
            return true;
        }
    }
    return false;
}

int wound::get_age()
{
    return age;
}
void wound::mod_age(int nage)
{
    age += nage;
}
void wound::set_age(int nage)
{
    age = nage;
}
int wound::get_base_heal_mod()
{
    return base_heal_mod;
}
int wound::get_base_str_mod()
{
    return base_str_mod;
}
int wound::get_base_dex_mod()
{
    return base_dex_mod;
}
int wound::get_base_per_mod()
{
    return base_per_mod;
}
int wound::get_base_int_mod()
{
    return base_int_mod;
}
int wound::get_base_speed_mod()
{
    return base_speed_mod;
}

void wound::mod_base_heal_mod(int mod)
{
    base_heal_mod += mod;
}
void wound::mod_base_str_mod(int mod)
{
    base_str_mod += mod;
}
void wound::mod_base_dex_mod(int mod)
{
    base_dex_mod += mod;
}
void wound::mod_base_per_mod(int mod)
{
    base_per_mod += mod;
}
void wound::mod_base_int_mod(int mod)
{
    base_int_mod += mod;
}
void wound::mod_base_speed_mod(int mod)
{
    base_speed_mod += mod;
}

void wound::set_base_heal_mod(int mod)
{
    base_heal_mod = mod;
}
void wound::set_base_str_mod(int mod)
{
    base_str_mod = mod;
}
void wound::set_base_dex_mod(int mod)
{
    base_dex_mod = mod;
}
void wound::set_base_per_mod(int mod)
{
    base_per_mod = mod;
}
void wound::set_base_int_mod(int mod)
{
    base_int_mod = mod;
}
void wound::set_base_speed_mod(int mod)
{
    base_speed_mod = mod;
}

int wound::get_severity()
{
    return severity;
}
void wound::mod_severity(int sev)
{
    severity += sev;
}
void wound::set_severity(int sev)
{
    severity = sev;
}
body_part wound::get_bp()
{
    return bp;
}
int wound::get_side()
{
    return side;
}
int wound::get_str_mod()
{
    float ret = get_base_str_mod();
    for (std::vector<wefftype_id>::iterator it = wound_effects.begin(); it != wound_effects.end();
          ++it) {
        ret += wound_eff_types[*it].base_mods.str_mod;
        ret += wound_eff_types[*it].sev_scale_mods.str_mod * severity;
    }
    return int(ret);
}
int wound::get_dex_mod()
{
    float ret = get_base_dex_mod();
    for (std::vector<wefftype_id>::iterator it = wound_effects.begin(); it != wound_effects.end();
          ++it) {
        ret += wound_eff_types[*it].base_mods.dex_mod;
        ret += wound_eff_types[*it].sev_scale_mods.dex_mod * severity;
    }
    return int(ret);
}
int wound::get_per_mod()
{
    float ret = get_base_per_mod();
    for (std::vector<wefftype_id>::iterator it = wound_effects.begin(); it != wound_effects.end();
          ++it) {
        ret += wound_eff_types[*it].base_mods.per_mod;
        ret += wound_eff_types[*it].sev_scale_mods.per_mod * severity;
    }
    return int(ret);
}
int wound::get_int_mod()
{
    float ret = 0;
    for (std::vector<wefftype_id>::iterator it = wound_effects.begin(); it != wound_effects.end();
          ++it) {
        ret += wound_eff_types[*it].base_mods.int_mod;
        ret += wound_eff_types[*it].sev_scale_mods.int_mod * severity;
    }
    return int(ret);
}
int wound::get_speed_mod()
{
    float ret = get_base_speed_mod();
    for (std::vector<wefftype_id>::iterator it = wound_effects.begin(); it != wound_effects.end();
          ++it) {
        ret += wound_eff_types[*it].base_mods.speed_mod;
        ret += wound_eff_types[*it].sev_scale_mods.speed_mod * severity;
    }
    return int(ret);
}

int wound::get_bleed()
{
    float ret = 0;
    for (std::vector<wefftype_id>::iterator it = wound_effects.begin(); it != wound_effects.end();
          ++it) {
        ret += rng(wound_eff_types[*it].base_mods.bleed_min, wound_eff_types[*it].base_mods.bleed_max);
        ret += rng(wound_eff_types[*it].sev_scale_mods.bleed_min, wound_eff_types[*it].sev_scale_mods.bleed_max) * severity;
    }
    return int(ret);
}
int wound::get_pain()
{
    float ret = 0;
    for (std::vector<wefftype_id>::iterator it = wound_effects.begin(); it != wound_effects.end();
          ++it) {
        ret += rng(wound_eff_types[*it].base_mods.pain_min, wound_eff_types[*it].base_mods.pain_max);
        ret += rng(wound_eff_types[*it].sev_scale_mods.pain_min, wound_eff_types[*it].sev_scale_mods.pain_max) * severity;
    }
    return int(ret);
}
int wound::get_hurt()
{
    float ret = 0;
    for (std::vector<wefftype_id>::iterator it = wound_effects.begin(); it != wound_effects.end();
          ++it) {
        ret += rng(wound_eff_types[*it].base_mods.hurt_min, wound_eff_types[*it].base_mods.hurt_max);
        ret += rng(wound_eff_types[*it].sev_scale_mods.hurt_min, wound_eff_types[*it].sev_scale_mods.hurt_max) * severity;
    }
    return int(ret);
}

int wound::get_heal_chance_100K()
{
    float ret = float(base_heal_mod);
    for (std::vector<wefftype_id>::iterator it = wound_effects.begin(); it != wound_effects.end();
          ++it) {
        ret += wound_eff_types[*it].base_mods.heal_mod_100K;
        ret += wound_eff_types[*it].sev_scale_mods.heal_mod_100K * severity;
    }
    return int(ret);
}

bool wound::roll_trigs(player &p)
{
    bool ret = false;
    int added_count = 0;
    for (size_t i = 0; i < wound_effects.size() - added_count; ++i) {
        wound_eff_type &weff = wound_eff_types[wound_effects[i]];
        if (age >= weff.get_trig_delay()) {
            if (x_in_y(weff.get_trig_chance(p.get_healthy()), 100000)) {
                //Instant effects
                if (!p.has_trait("NOPAIN")) {
                    p.mod_pain(weff.get_trig_pain(severity));
                }
                
                //Not implemented yet
                //p.bleed(weff.get_trig_bleed(severity));
                
                if (get_bp() == num_bp) {
                    p.hurt(bp_torso, -1, weff.get_trig_hurt(severity));
                } else {
                    p.hurt(get_bp(), get_side(), weff.get_trig_hurt(severity));
                }
                
                // Stacking effects
                mod_base_heal_mod(weff.get_trig_heal_mod(severity));
                mod_base_str_mod(weff.get_trig_str_mod(severity));
                mod_base_dex_mod(weff.get_trig_dex_mod(severity));
                mod_base_per_mod(weff.get_trig_per_mod(severity));
                mod_base_int_mod(weff.get_trig_int_mod(severity));
                mod_base_speed_mod(weff.get_trig_speed_mod(severity));
                
                //Add effects
                if (weff.effect_placed != "") {
                    if (weff.get_targeted_effect()) {
                        p.add_effect(weff.effect_placed, weff.get_trig_effect_dur(severity),
                                    weff.get_effect_perm(), weff.get_trig_effect_int(severity), bp, side);
                    } else {
                        p.add_effect(weff.effect_placed, weff.get_trig_effect_dur(severity),
                                    weff.get_effect_perm(), weff.get_trig_effect_int(severity));
                    }
                }
                
                if (weff.get_heal_wound() == true) {
                    ret = true;
                }
                if (weff.get_weff_added() != "") {
                    add_wound_effect(weff.get_weff_added());
                    //New effects aren't processed
                    added_count++;
                }
                if (weff.get_rem_effect() == true) {
                    remove_wound_effect(weff.id);
                    i--;
                }
            }
        }
    }
    return ret;
}

void load_wound_eff_type(JsonObject &jo)
{
    wound_eff_type new_wetype;
    new_wetype.id = jo.get_string("id");
    new_wetype.name_mod = jo.get_string("name_mod", "");
    new_wetype.desc = jo.get_string("desc", "");
    
    new_wetype.effect_placed = jo.get_string("effect_placed", "");
    new_wetype.targeted_effect = jo.get_bool("targeted_effect", false);
    new_wetype.effect_perm = jo.get_bool("effect_perm", false);
    new_wetype.targeted_harm = jo.get_bool("targeted_harm", false);
    
    new_wetype.base_mods.load(jo, "base_mods");
    new_wetype.sev_scale_mods.load(jo, "sev_scale_mods");
    
    new_wetype.trigger_info.load(jo, "trigger_info");

    wound_eff_types[new_wetype.id] = new_wetype;
}

void reset_wound_eff_types()
{
    wound_eff_types.clear();
}
