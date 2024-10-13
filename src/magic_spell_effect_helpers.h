#pragma once
#ifndef CATA_SRC_MAGIC_SPELL_EFFECT_HELPERS_H
#define CATA_SRC_MAGIC_SPELL_EFFECT_HELPERS_H

#include <functional>
#include <set>

#include "coords_fwd.h"

class Creature;
class spell;

// spells do not reduce in damage the further away from the epicenter the targets are
// rather they do their full damage in the entire area of effect
std::set<tripoint_bub_ms> calculate_spell_effect_area( const spell &sp,
        const tripoint_bub_ms &target, const Creature &caster );

#endif // CATA_SRC_MAGIC_SPELL_EFFECT_HELPERS_H
