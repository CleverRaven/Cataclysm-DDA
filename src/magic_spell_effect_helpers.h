#pragma once
#ifndef CATA_SRC_MAGIC_SPELL_EFFECT_HELPERS_H
#define CATA_SRC_MAGIC_SPELL_EFFECT_HELPERS_H

#include <functional>
#include <set>

class Creature;
class spell;
struct tripoint;

// spells do not reduce in damage the further away from the epicenter the targets are
// rather they do their full damage in the entire area of effect
std::set<tripoint> calculate_spell_effect_area( const spell &sp, const tripoint &target,
        const std::function<std::set<tripoint>( const spell &, const tripoint &, const tripoint &, int, bool )>
        &aoe_func, const Creature &caster, bool ignore_walls = false );

#endif // CATA_SRC_MAGIC_SPELL_EFFECT_HELPERS_H
