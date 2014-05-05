#include "wound.h"
#include "rng.h"
#include "bodypart.h"

std::map<std::string, wound_effect_type> wound_effect_types;

bool effect_morph_info::load(JsonObject &jsobj, std::string member) {
    if( jsobj.has_object(member) ) {
        JsonObject j = jsobj.get_object(member);
        effect_placed = j.get_string("effect_placed", "");
        targeted_effect = j.get_bool("targeted_effect", true);
        effect_duration = j.get_int("effect_duration", 0);
        effect_perm = j.get_bool("effect_perm", false);
        effect_intensity = j.get_int("effect_intensity", 0);
        bleed = j.get_float("bleed", 0);

        return true;
    } else {
        return false;
    }
}

wound::wound() :
    body_part bp(num_bp),
    side(-1),
    duration(0),
    severity(0),
    heal_mod(0)
{ }

wound::wound(body_part target, int nside, int dur, int sev, int nheal_mod) :
    body_part bp(target),
    side(nside),
    duration(dur),
    severity(sev),
    heal_mod(nheal_mod)
{ }

wound::wound(const wound &rhs) : JsonSerializer(), JsonDeserializer(),
    body_part bp(rhs.bp),
    side(rhs.side),
    duration(rhs.duration),
    severity(rhs.severity),
    heal_mod(rhs.heal_mod)
{ }

wound &wound::operator=(const wound &rhs)
{
    if (this == &rhs) {
        return *this;    // No self-assignment
    }

    bp = rhs.bp;
    side = rhs.side;
    duration = rhs.duration;
    severity = rhs.severity;
    heal_mod = rhs.heal_mod;

    return *this;
}

int get_duration()
{
    return duration;
}
void mod_duration(int dur)
{
    duration += dur;
}
void set_duration(int dur)
{
    duration = dur;
}
int get_heal_mod()
{
    return heal_mod;
}
void mod_heal_mod(int mod)
{
    heal_mod += mod;
}
void set_heal_mod(int mod)
{
    heal_mod = mod;
}
int get_severity()
{
    return severity;
}
void mod_severity(int sev);
{
    severity += sev;
}
void set_severity(int sev)
{
    severity = sev;
}
body_part get_bp()
{
    return bp;
}
int get_side()
{
    return side;
}

void load_wound_effect_type(JsonObject &jo)
{
    wound_effect_type new_wetype;
    new_wetype.id = jo.get_string("id");
    new_wetype.name_mod = jo.get_string("name_mod", "");
    new_wetype.name_mod_trig = jo.get_string("name_mod_trig", new_wetype.name_mod);
    new_wetype.desc = jo.get_string("desc", "");
    new_wetype.desc_trig = jo.get_string("desc_trig", new_wetype.desc);
    new_wetype.trig_delay = jo.get_int("trig_delay", 0);
    new_wetype.base_trig_chance = jo.get_int("trig_chance_10K", 0);
    new_wetype.trig_max = jo.get_int("trig_max", 0);
    new_wetype.heal_delay = jo.get_int("heal_delay", 0);
    new_wetype.base_heal_chance = jo.get_int("heal_chance_10K", 0);
    new_wetype.heal_mod = jo.get_int("heal_mod", 0);
    
    new_wetype.base_mods.load(jo, "base_mods");
    new_wetype.sev_scale_mods.load(jo, "sev_scale_mods");

    wound_effect_types[new_wetype.id] = new_wetype;
}

void reset_wound_effect_types()
{
    wound_effect_types.clear();
}
