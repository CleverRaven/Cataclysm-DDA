#include "mondefense.h"
#include "monster.h"
#include "creature.h"
#include "damage.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "bodypart.h"
#include "messages.h"

void mdefense::none(monster *, Creature *, const projectile *)
{
}

void mdefense::zapback(monster *const m, Creature *const source, projectile const* const proj)
{
    if ((proj) ||                              // Not a melee attack
        (rng(0, 100) > m->def_chance) ||       // Attacker lucked out
        (rl_dist(m->pos(), source->pos()) > 1) // Out of range
    ) {
        return;
    }

    auto const is_eligible_player = [](player const *const foe) {
        return foe && (foe->weapon.conductive() || foe->unarmed_attack()) && // Eligible for zap
            !foe->has_active_bionic("bio_faraday") &&
            !foe->worn_with_flag("ELECTRIC_IMMUNE") &&
            !foe->has_artifact_with(AEP_RESIST_ELECTRICITY);
    };

    auto const is_eligible_monster = [](monster const *const othermon) {
        return othermon && (othermon->type->sp_defense != &mdefense::zapback) &&
            !othermon->has_flag(MF_ELECTRIC);
    };

    if (!is_eligible_player(dynamic_cast<player*>(source)) &&
        !is_eligible_monster(dynamic_cast<monster*>(source)))
    {
        return; // Nothing eligible
    }

    damage_instance const shock {DT_ELECTRIC, static_cast<float>(rng(1, 5))};
    source->deal_damage(m, bp_arm_l, shock);
    source->deal_damage(m, bp_arm_r, shock);

    if (g->u.sees(source->pos())) {
        auto const msg_type = (source == &g->u) ? m_bad : m_info;
        add_msg(msg_type, _("Striking the %s shocks %s!"),
            m->name().c_str(), source->disp_name().c_str());
    }
}
