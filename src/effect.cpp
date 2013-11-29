#include "effect.h"

effect_type::effect_type() {}
effect_type::effect_type(const effect_type & rhs) {}

effect::effect() {}
effect::effect(effect_type* eff_type, int dur) {}
effect::effect(const effect & rhs) {}

void effect::do_effects(game* g, creature& t) {
    return;
}

effect_type* effect::get_effect_type() {
    return NULL;
}

