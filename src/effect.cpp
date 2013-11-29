#include "effect.h"
#include "rng.h"
#include <map>

std::map<std::string, effect_type> effect_types;

effect_type::effect_type() {}
effect_type::effect_type(const effect_type & rhs) {}

effect::effect() {}
effect::effect(effect_type* peff_type, int dur) {
    eff_type = peff_type;
    duration = dur;
}
effect::effect(const effect & rhs) {}

void effect::do_effects(game* g, creature& t) {
    return;
}

effect_type* effect::get_effect_type() {
    return eff_type;
}

void load_effect_type(JsonObject& jo) {
    effect_type new_etype;
    new_etype.id = jo.get_string("id");

    new_etype.apply_message = jo.get_string("apply_message","");
    new_etype.remove_message = jo.get_string("remove_message","");
    new_etype.apply_memorial_log = jo.get_string("apply_memorial_log","");
    new_etype.remove_memorial_log = jo.get_string("remove_memorial_log","");

    effect_types[new_etype.id] = new_etype;
}
